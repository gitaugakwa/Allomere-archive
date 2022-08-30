from __future__ import unicode_literals
# from tensorflow_io.python.api.audio import AudioIOTensor
import tensorflow as tf
import argparse

from pynput.keyboard import Key, Listener,GlobalHotKeys,KeyCode

import math
import librosa

import asyncio

import numpy as np
from itertools import tee, islice, chain
import sounddevice as sd
from threading import Thread
import os

import youtube_dl

from copy import deepcopy

DURATION = 3 * 60 # 3 minute songs
SAMPLE_RATE = 44100 # my FLAC sample rate

def get_from_url(url):
	ydl_opts = {
		'format': 'bestaudio/best',
		# 'keepvideo': 'false',
		'forcefilename': 'true',
		'outtmpl': '%(id)s.%(ext)s',
		'postprocessors': [{
			'key': 'FFmpegExtractAudio',
			'preferredcodec': 'mp3',
			'preferredquality': '192',
		}],
	}
	with youtube_dl.YoutubeDL(ydl_opts) as ydl:
		song_info = ydl.extract_info(url)
		# print(song_info)
		# print(ydl.download([url]))
	return "{}.mp3".format(song_info['id'])

def get_waveform(file_path):
	audio, sr = librosa.core.load(file_path, sr=None, mono=False )
	# tensor = AudioIOTensor(file_path)
	# audio = (tf.cast(tensor.to_tensor(),dtype=tf.float32)/32768.0).numpy().T
	# sr = tensor.rate.numpy()
	return audio, sr


async def get_spectrogram(waveform, pad: bool = True, input_samples= DURATION * SAMPLE_RATE, frame_step = 128, frame_length=255 ):
  # Zero-padding for an audio waveform with less than 16,000 samples.
  channels = waveform.shape[1]
  input_len = input_samples
  waveform = waveform[:input_len]
  zero_padding = tf.zeros(
      input_samples - waveform.shape[0],
      dtype=tf.float32)
  # Cast the waveform tensors' dtype to float32.
  waveform = tf.cast(waveform, dtype=tf.float32)
  # Concatenate the waveform with `zero_padding`, which ensures all audio
  # clips are of the same length.
  spectrogram = []
  for channel in range(channels):
    data = waveform[:-1,channel]
    if pad:
        data = tf.concat([waveform[:,channel], zero_padding], 0)
    # Convert the waveform to a spectrogram via a STFT.
    spectrogram.append(tf.signal.stft(
        data, frame_length=frame_length, frame_step=frame_step))
    # Obtain the magnitude of the STFT.
    spectrogram[channel] = tf.abs(spectrogram[channel])
    spectrogram[channel] = spectrogram[channel][..., tf.newaxis]
    # Add a `channels` dimension, so that the spectrogram can be used
    # as image-like input data with convolution layers (which expect
    # shape (`batch_size`, `height`, `width`, `channels`).
    # spectrogram = spectrogram[..., tf.newaxis]
	# shape ('width', 'batch_size', 'height')
  return tf.concat(spectrogram, axis=2)

async def get_beat_track(waveform, sr):
	l_tempo, r_tempo = np.squeeze(librosa.beat.tempo(y=waveform))
	# print(l_tempo, r_tempo)
	tempo = math.ceil((l_tempo+r_tempo)/2)

	_, beat_track = librosa.beat.beat_track(y=librosa.to_mono(waveform), sr=sr, tightness=1, hop_length=256, trim=False, units="samples")
	# print(beat_track.shape)

	time_stamps =librosa.samples_to_time(beat_track, sr=sr)

	# print(np.array(beat_track).reshape((-1,1)).shape)
	# print(beat_track)
	# print(time_stamps)
	# print(tempo)
	return beat_track, tempo

async def get_mse_mat(waveform, sr):
	get_beat_track_r = get_beat_track(waveform, sr)

	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)
	
	def mse(A,B):
		mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return mean
	vmse = np.vectorize(mse, signature='(m),(m)->()')
	def matmse(A,B):
		return vmse(A,B).reshape(-1,1)
	vmatmse = np.vectorize(matmse, signature='(m),(n,m)->(n,1)')


	beat_track, tempo = await get_beat_track_r
	beats = beat_track.shape[0]
	beat_samples = np.array([np.mean(await get_spectrogram(waveform[:,p_sample:sample].T, pad=False), axis=0) for p_sample, sample, _ in previous_and_next(beat_track)])
	# print(beat_samples.shape)

	mse_arrays = [np.concatenate([np.zeros((i,1)),vmatmse(beat_samples[i,:,1],beat_samples[i:,:,1])]) for i in range(beats)]

	mse_mat = np.column_stack(mse_arrays)
	# print(mse_mat.shape)
	return mse_mat,beat_track, tempo

pos = 0

playback = {
		"pause": False,
		"mute": False,
		"stop": False
	}

branch = None # np.zeros((branches.shape[1], 1))
branches_prob = None # np.random.randn(branches.shape[0], 1)
best_match_map_filtered_by_beat = None

def generateBranchProbability(branches, size = 1):
	global branch
	global branches_prob
	# branches[:] = np.random.randn(size, 1)
	branch_index = np.squeeze(np.argmax(branches_prob))
	branch[:] = best_match_map_filtered_by_beat[branch_index].reshape((3,1))
	branches_prob[:] = np.random.randn(branches.shape[0], 1)

def audioGenerator(waveform, beat_track, branches, output):
	global pos
	global playback
	global branch
	global branches_prob
	global best_match_map_filtered_by_beat
	play_buffer = deepcopy(waveform).T
	best_match_map_filtered_by_beat = branches[branches[:, 0].argsort()]
	print(best_match_map_filtered_by_beat.shape)

	branch = np.zeros((branches.shape[1], 1))
	branches_prob = np.random.randn(branches.shape[0], 1)
	on_branch = Thread(target=lambda : print("Branched"))
	generateBranchProbability(branches, best_match_map_filtered_by_beat.shape[0])
	branch_prob_thread = None
	data = np.zeros((1,2))

	while output:
		[from_beat,_, to_beat] = branch

		if(pos % 50000 == 0):
			print("[{}] Next Branch: {}[{}] -> {}[{}]".format(pos,int(from_beat), beat_track[int(from_beat)], int(to_beat), beat_track[int(to_beat)]))
		if pos == beat_track[int(from_beat)]:
			data = play_buffer[beat_track[int(to_beat)],:]
			pos = beat_track[int(to_beat)] + 1

			on_branch = Thread(target=lambda : print("[{}] Branched: {}[{}] -> {}[{}]".format(pos,int(from_beat), beat_track[int(from_beat)], int(to_beat), pos-1)))
			on_branch.start()
			on_branch.join()
			if not branch_prob_thread == None:
				branch_prob_thread.join()
			branch_prob_thread = Thread(target= generateBranchProbability, args=(branches, best_match_map_filtered_by_beat.shape[0],))
			branch_prob_thread.start()
		else:
			data = play_buffer[pos,:]
			pos += 1

		if(pos >= play_buffer.shape[0]):
			print("[{}] Ended".format(pos))
			output = False
			data.fill(0)
			yield data, pos
		
		if playback["mute"]:
			data.fill(0)
		yield data, pos

	while not output:
		yield data, pos

def audio_loop(gen, sr, branches):
	global playback
	def audio_loop_thread(stream):
		while not stream.stopped or not stream.closed:
			True
			# Don't use closed
			# Most likely can be used in the parent thread

	def audio_callback(indata: np.ndarray, outdata: np.ndarray, frames: int, time, status: sd.CallbackFlags):
		if status:
			print(status)
	
		outdata[:] = np.array([next(gen)[0] for _ in range(frames)])

	def press_event():
		if(playback['pause'] and not stream.closed):
			stream.stop()
		elif not stream.closed:
			stream.start()
		if(playback["stop"] and not stream.closed):
			stream.close()
			return False
		return True
		


	def on_press(key):
		global pos
		try:
			if(key.char == "r"):
				generateBranchProbability(branches, best_match_map_filtered_by_beat.shape[0])
			if(key.char == "m"):
				playback["mute"] = not playback["mute"]
			if(key.char == "s"):
				playback["stop"] = not playback["stop"]
			return press_event()
			print('alphanumeric key {0} pressed'.format(
			key.char))
		except:
			if(key == Key.left):
				pos = max(pos - sr * 5, 0)
			if(key == Key.right):
				pos = pos + sr * 5
			if(key == Key.space):
				playback["pause"] = not playback["pause"]
			return press_event()
			print('special key {0} pressed'.format(
			key))

	def on_release(key):
		# print('{0} release'.format(
		# 	key))
		if key == Key.esc:
			# Stop listener
			return False

	stream = sd.Stream(samplerate=sr, channels=2, callback=audio_callback)
	stream.start()
	with Listener(
		on_press=on_press,
		on_release=on_release) as listener:
		listener.join()
	# sd.sleep(4*1000)
	# stream.stop()

	# Collect events until released

	audio_loop_thread(stream)
	# audio_thread.join()
	# return stream,audio_thread
	return stream

async def play_song(args):
	waveform, sr = get_waveform(args.path)
	mse_mat, beat_track,tempo=await get_mse_mat(waveform,sr)

	beats = beat_track.shape[0]
	gap = args.gap
	quality = args.quality
	quantity = args.quantity

	source_beat = np.tile(np.arange(beats), (beats, 1)).T
	end_beat = np.tile(np.arange(beats), (beats, 1))
	beat_to_beat_mse = np.stack([source_beat, mse_mat, end_beat], axis=2)
	beat_to_beat_map = beat_to_beat_mse[beat_to_beat_mse[:,:, 1] > 0]
	beat_to_beat_map_sorted = beat_to_beat_map[beat_to_beat_map[:, 1].argsort()]

	# best_match_map_long = beat_to_beat_map_sorted[(beat_to_beat_map_sorted[:, 0] - beat_to_beat_map_sorted[:, 2]) > gap]
	best_match_map_filtered = beat_to_beat_map_sorted[(beat_to_beat_map_sorted[:, 0] -beat_to_beat_map_sorted[:, 2]) > gap]

	best_match_map_filtered = best_match_map_filtered[:int(best_match_map_filtered.shape[0] * (min(max(100-quality, 1e-4), 99.9999)/100)), :]
	if(quantity):
		best_match_map_filtered = best_match_map_filtered[:quantity, :]

	print(best_match_map_filtered.shape)
	output = True
	
	gen = audioGenerator(waveform, beat_track, best_match_map_filtered, output)
	# audio_thread = Thread(target=audio_loop, args=(gen,sr,))
	# audio_thread.start()
	stream = audio_loop(gen,sr, best_match_map_filtered)
	# audio_thread.join
	# print(stream)
	print(waveform.shape)
	print(sr)
	print(pos)


def parse_args():
	parser = argparse.ArgumentParser()

	parser.add_argument("--url", default=None)
	parser.add_argument("--path", default=None)
	parser.add_argument("--gap", default=15)
	parser.add_argument("--quality", default=99.5)
	parser.add_argument("--quantity", default=None, type=int)
	# parser.add_argument("--zipPath", default=None)
	# parser.add_argument("--extractPath", default=None)
	# parser.add_argument("--extract", type=bool, default=True)
	# parser.add_argument("--market", default="spot", choices=MARKETS)
	# parser.add_argument("--type", default="klines", choices=TYPES)
	# parser.add_argument("--pair", default="BTCBUSD")
	# parser.add_argument("--interval", default="1d", choices=INTERVALS)
	# parser.add_argument("--date", default="2021-3-1")
	# parser.add_argument("--start-date", default=None)
	# parser.add_argument("--end-date", default=None)
	# parser.add_argument("--batch-size", type=int, default=32)
	# parser.add_argument("--epochs", type=int, default=1)
	# parser.add_argument("--learning-rate", type=float, default=1e-3)
	# parser.add_argument("--beta_1", type=float, default=0.9)
	# parser.add_argument("--beta_2", type=float, default=0.999)

	# Environment variables given by the training image
	# parser.add_argument("--model-dir", type=str, default=os.environ["SM_MODEL_DIR"])
	# parser.add_argument("--train", type=str, default=os.environ["SM_CHANNEL_TRAINING"])
	# parser.add_argument("--test", type=str, default=os.environ["SM_CHANNEL_TESTING"])

	# parser.add_argument("--current-host", type=str, default=os.environ["SM_CURRENT_HOST"])
	# parser.add_argument("--hosts", type=list, default=json.loads(os.environ["SM_HOSTS"]))

	return parser.parse_args()


if __name__ == "__main__":
	args = parse_args()
	loop = asyncio.get_event_loop()
	if(args.url):
		args.path = os.path.join(os.getcwd(), get_from_url(args.url))
	# args.basePath = os.path.join(os.getcwd(), args.path or os.path.join("data", args.pair, args.market, args.type, args.interval))
	loop.run_until_complete(play_song(args))