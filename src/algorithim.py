from __future__ import unicode_literals
# from tensorflow_io.python.api.audio import AudioIOTensor
import tensorflow as tf
import tensorflow_io as tfio
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
import pyaudio
from pyaudio import Stream
from webserver import Flask, Response,render_template
app = Flask(__name__)

audio = pyaudio.PyAudio()

from copy import deepcopy

FORMAT = pyaudio.paInt16
DURATION = 3 * 60 # 3 minute songs
SAMPLE_RATE = 44100 # my FLAC sample rate
CHANNELS = 2
CHUNK = 1024

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
	audio, sr = librosa.core.load(file_path, sr=SAMPLE_RATE, mono=False )
	# tensor = AudioIOTensor(file_path)
	# audio = (tf.cast(tensor.to_tensor(),dtype=tf.float32)/32768.0).numpy().T
	# sr = tensor.rate.numpy()
	return audio, sr


async def get_mel_spectrogram(waveform, pad: bool = True, input_samples= DURATION * SAMPLE_RATE, frame_step = 128, frame_length=255 ):
  # Zero-padding for an audio waveform with less than 16,000 samples.
  channels = waveform.shape[1]
  input_len = input_samples
  waveform = waveform[:input_len]
  
  # Cast the waveform tensors' dtype to float32.
  waveform = tf.cast(waveform, dtype=tf.float32)
  # Concatenate the waveform with `zero_padding`, which ensures all audio
  # clips are of the same length.
  tfio.audio.melscale()
  spectrogram = []
  for channel in range(channels):
    data = waveform[:-1,channel]
    if pad:
        zero_padding = tf.zeros(
            input_samples - waveform.shape[0],
            dtype=tf.float32)
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

async def get_spectrogram(waveform, pad: bool = True, input_samples= DURATION * SAMPLE_RATE, frame_step = 128, frame_length=255 ):
  # Zero-padding for an audio waveform with less than 16,000 samples.
  channels = waveform.shape[1]
  input_len = input_samples
  waveform = waveform[:input_len]
  
  # Cast the waveform tensors' dtype to float32.
  waveform = tf.cast(waveform, dtype=tf.float32)
  # Concatenate the waveform with `zero_padding`, which ensures all audio
  # clips are of the same length.
  spectrogram = []
  for channel in range(channels):
    data = waveform[:-1,channel]
    if pad:
        zero_padding = tf.zeros(
            input_samples - waveform.shape[0],
            dtype=tf.float32)
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

async def get_beat_track(waveforms, sr):
	tempos = np.array([np.sum(np.squeeze(librosa.beat.tempo(y=waveform)),axis=-1)/2 for waveform in waveforms])
	# l_tempo, r_tempo = np.squeeze(librosa.beat.tempo(y=waveforms))
	# print(l_tempo, r_tempo)
	# tempo = math.ceil((l_tempo+r_tempo)/2)

	beat_tracks = np.array([librosa.beat.beat_track(y=librosa.to_mono(waveform), sr=sr, tightness=1, hop_length=256, trim=False, units="samples")[1] for waveform in waveforms])
	# print(beat_tracks.shape)
	# print(tempos.shape)

	# time_stamps =librosa.samples_to_time(beat_track, sr=sr)

	# print(np.array(beat_track).reshape((-1,1)).shape)
	# print(beat_track)
	# print(time_stamps)
	# print(tempo)
	return [beat_tracks, tempos]
async def get_cross_similarity_mat(waveform, sr):
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
	csm_mat = np.zeros((beats, beats))
	for a, (a_sample_start, a_sample_end, _) in enumerate(previous_and_next(beat_track)):
		print(a)
		for b, (b_sample_start, b_sample_end, _) in enumerate(previous_and_next(beat_track[a+1:])):
			csm_mat[a, b+1] = np.mean(librosa.segment.cross_similarity(waveform[:,a_sample_start:a_sample_end], waveform[:,b_sample_start:b_sample_end], k=2))
	# beat_samples = np.array([np.mean(librosa.segment.cross_similarity(waveform[:,p_sample:sample], waveform[:,sample:n_sample]), axis=0) for p_sample, sample, n_sample in previous_and_next(beat_track)])

	# mse_arrays = [np.concatenate([np.zeros((i,1)),beat_samples[i,:,1],beat_samples[i:,:,1]]) for i in range(beats)]
	return csm_mat.T, beat_track, tempo


async def get_cosine_sim_mat_improved(waveform, sr, padding=5000):
	get_beat_track_r = get_beat_track(waveform, sr)

	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)
	
	def cosine_sim(A,B):
		similarity = (1- (np.dot(A, B)/(np.linalg.norm(A)*np.linalg.norm(B)))).mean()
		# mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return similarity
	vcosine_sim = np.vectorize(cosine_sim, signature='(m),(m)->()')
	def matcosine_sim(A,B):
		return vcosine_sim(A,B).reshape(-1,1)
	vmatcosine_sim = np.vectorize(matcosine_sim, signature='(m),(n,m)->(n,1)')


	beat_track, tempo = await get_beat_track_r
	beats = beat_track.shape[0]
	samples = waveform.shape[1]
	beat_samples = np.array([np.mean(await get_spectrogram(waveform[:,max(sample-padding, 0):min(sample+padding, samples)].T, pad=False), axis=0) for _, sample, _ in previous_and_next(beat_track)])
	# print(beat_samples.shape)

	cosine_sim_arrays = [np.concatenate([np.zeros((i,1)),vmatcosine_sim(beat_samples[i,:,1],beat_samples[i:,:,1])]) for i in range(beats)]

	cosine_sim_mat = np.column_stack(cosine_sim_arrays)
	cosine_sim_mat += cosine_sim_mat.T
	# print(cosine_sim_mat.shape)
	return cosine_sim_mat,beat_track, tempo


async def get_mse_mat_improved(waveforms, sr, padding=5000):
	get_beat_track_r = get_beat_track(waveforms, sr)

	def previous_and_next(some_iterable):
		prevs, items, nexts = tee(some_iterable, 3)
		prevs = chain([None], prevs)
		nexts = chain(islice(nexts, 1, None), [None])
		return zip(prevs, items, nexts)

	async def get_spectrograms(waveform, beat_track, samples):
		return np.array([np.mean(await get_spectrogram(waveform[:,max(sample-padding, 0):min(sample+padding, samples)].T, pad=False), axis=0) for _, sample, _ in previous_and_next(beat_track)])
	
	def mse(A,B):
		mean = (np.squeeze(A - B)**2).mean()
		# print(mean)
		return mean
	vmse = np.vectorize(mse, signature='(m),(m)->()')
	def matmse(A,B):
		return vmse(A,B).reshape(-1,1)
	vmatmse = np.vectorize(matmse, signature='(m),(n,m)->(n,1)')


	beat_tracks, tempo = await get_beat_track_r
	beats = [beat_track.shape[0] for beat_track in beat_tracks]
	# print(beats)
	# print(waveforms.shape)
	samples = [waveform.shape[1] for waveform in waveforms]
	beat_spectrograms = np.array([await get_spectrograms(waveform,beat_track, samples) for waveform,beat_track, samples in zip(waveforms, beat_tracks, samples)])
	# print(beat_spectrograms[0].shape)
	# print(beat_spectrograms[1].shape)
	spectrograms = np.concatenate([spectrogram[:,:,1] for spectrogram in beat_spectrograms])

	if(waveforms.shape[0] == 2):
		mse_arrays = [np.concatenate([np.zeros((i,1)),vmatmse(spectrograms[i,:], spectrograms[i:,:])]) for i in range(np.sum(beats))]
	else:
		mse_arrays = [np.concatenate([np.zeros((i,1)),vmatmse(beat_spectrograms[0][i,:,1],beat_spectrograms[0][i:,:,1])]) for i in range(np.sum(beats))]

	mse_mat = np.column_stack(mse_arrays)
	mse_mat += mse_mat.T
	# print(mse_mat.shape)
	return mse_mat,beat_tracks, tempo


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
song = 0

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

def audioGenerator(waveforms, beat_tracks, branches, output):
	global pos
	global song
	global playback
	global branch
	global branches_prob
	global best_match_map_filtered_by_beat
	play_buffer = [waveform.T for waveform in deepcopy(waveforms)]
	best_match_map_filtered_by_beat = branches[branches[:, 0].argsort()]
	print(best_match_map_filtered_by_beat.shape)

	def get_beat(beat): 
		beat = int(beat)
		if(beat < beat_tracks[0].shape[0]):
			return [0, beat]
		else:
			return [1, beat-beat_tracks[0].shape[0]]

	branch = np.zeros((branches.shape[1], 1))
	branches_prob = np.random.randn(branches.shape[0], 1)
	on_branch = Thread(target=lambda : print("Branched"))
	generateBranchProbability(branches, best_match_map_filtered_by_beat.shape[0])
	branch_prob_thread = None
	data = np.zeros((1,2))

	while output:
		[from_beat,_, to_beat] = branch
		from_song, from_beat = get_beat(from_beat)
		to_song, to_beat = get_beat(to_beat)

		if(pos % 50000 == 0):
			print(f"[{pos}] Next Branch: ({from_song}){from_beat}[{beat_tracks[from_song][from_beat]}] -> ({to_song}){to_beat}[{beat_tracks[to_song][to_beat]}]")
		if song == from_song and pos == beat_tracks[from_song][from_beat]:
			data = play_buffer[to_song][beat_tracks[to_song][to_beat],:]
			pos = beat_tracks[to_song][to_beat] + 1
			song = to_song

			on_branch = Thread(target=lambda : print(f"[{pos}] Branched: ({from_song}){from_beat}[{beat_tracks[from_song][from_beat]}] -> ({to_song}){to_beat}[{pos-1}]"))
			on_branch.start()
			on_branch.join()
			if not branch_prob_thread == None:
				branch_prob_thread.join()
			branch_prob_thread = Thread(target= generateBranchProbability, args=(branches, best_match_map_filtered_by_beat.shape[0],))
			branch_prob_thread.start()
		else:
			data = play_buffer[song][pos,:]
			pos += 1

		if pos >= play_buffer[song].shape[0]:
			if song == len(play_buffer) - 1:
				print(f"[{pos}] Ended")
				output = False
				data.fill(0)
				yield data, pos
			pos = 0
			song += 1

		# if(song == 1 and pos >= play_buffer[1].shape[0]):
			
		
		if playback["mute"]:
			data.fill(0)
		yield data, pos

	while not output:
		yield data, pos

def generate_audio_stream(waveforms, beat_tracks, branches, output):
	def callback(_, frame_count, time_info, status_flags):
		global pos
		global song
		global playback
		global branch
		global branches_prob
		global best_match_map_filtered_by_beat
		play_buffer = [waveform.T for waveform in deepcopy(waveforms)]
		best_match_map_filtered_by_beat = branches[branches[:, 0].argsort()]
		print(best_match_map_filtered_by_beat.shape)

		def get_beat(beat): 
			beat = int(beat)
			if(beat < beat_tracks[0].shape[0]):
				return [0, beat]
			else:
				return [1, beat-beat_tracks[0].shape[0]]

		branch = np.zeros((branches.shape[1], 1))
		branches_prob = np.random.randn(branches.shape[0], 1)
		on_branch = Thread(target=lambda : print("Branched"))
		generateBranchProbability(branches, best_match_map_filtered_by_beat.shape[0])
		branch_prob_thread = None
		data = np.zeros((1,2))
		while output:
			[from_beat,_, to_beat] = branch
			from_song, from_beat = get_beat(from_beat)
			to_song, to_beat = get_beat(to_beat)

			if(pos % 50000 == 0):
				print(f"[{pos}] Next Branch: ({from_song}){from_beat}[{beat_tracks[from_song][from_beat]}] -> ({to_song}){to_beat}[{beat_tracks[to_song][to_beat]}]")
			if song == from_song and pos == beat_tracks[from_song][from_beat]:
				data_start=beat_tracks[to_song][to_beat]
				data = play_buffer[to_song][data_start:data_start+frame_count,:]
				pos = beat_tracks[to_song][to_beat] +frame_count
				song = to_song

				on_branch = Thread(target=lambda : print(f"[{pos}] Branched: ({from_song}){from_beat}[{beat_tracks[from_song][from_beat]}] -> ({to_song}){to_beat}[{pos-1}]"))
				on_branch.start()
				on_branch.join()
				if not branch_prob_thread == None:
					branch_prob_thread.join()
				branch_prob_thread = Thread(target= generateBranchProbability, args=(branches, best_match_map_filtered_by_beat.shape[0],))
				branch_prob_thread.start()
			else:
				data = play_buffer[song][pos: pos+frame_count,:]
				pos += frame_count

			if pos >= play_buffer[song].shape[0]:
				if song == len(play_buffer) - 1:
					print(f"[{pos}] Ended")
					output = False
					data.fill(0)
					yield (data, pyaudio.paContinue)
				pos = 0
				song += 1

			# if(song == 1 and pos >= play_buffer[1].shape[0]):
				
			
			if playback["mute"]:
				data.fill(0)
			yield (data, pyaudio.paComplete)

		while not output:
			yield (data, pyaudio.paComplete)
		
	stream = Stream(
		PA_manager=audio,
		rate=SAMPLE_RATE,
		channels=CHANNELS,
		format=FORMAT,
		frames_per_buffer=CHUNK,
		stream_callback=callback)
	
	return stream

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
		global song
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
				new_pos = pos - sr * 5
				if song != 0 and new_pos <0:
					song -= 1
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

async def get_beats_map(waveforms, sr, algo, padding, quality, quantity, gap ):
	# mse_mat, beat_track,tempo=await get_cross_similarity_mat(waveform,sr)
	# mse_mat, beat_track,tempo=await get_mse_mat_improved(waveform,sr)
	# mse_mat, beat_track,tempo=await get_mse_mat_improved(waveform,sr, padding)

	if(algo == "mse"):
		dif_mat, beat_tracks,tempo=await get_mse_mat_improved(waveforms,sr, padding)
	if(algo == "cossim"):
		dif_mat, beat_tracks,tempo=await get_cosine_sim_mat_improved(waveforms,sr, padding)

	# print(dif_mat)
	beats = [beat_track.shape[0] for beat_track in beat_tracks]
	# beats = beat_track.shape[0]

	source_beat = np.tile(np.arange(np.sum(beats)), (np.sum(beats), 1)).T
	end_beat = source_beat.T
	print(dif_mat.shape)
	print(source_beat.shape)
	print(end_beat.shape)

	beat_to_beat_mse = np.stack([source_beat, dif_mat, end_beat], axis=2)
	# print(beat_to_beat_mse.shape)
	beat_to_beat_map = beat_to_beat_mse[beat_to_beat_mse[:,:, 1] > 0]
	beat_to_beat_map_sorted = beat_to_beat_map[beat_to_beat_map[:, 1].argsort()]

	# best_match_map_long = beat_to_beat_map_sorted[(beat_to_beat_map_sorted[:, 0] - beat_to_beat_map_sorted[:, 2]) > gap]
	best_match_map_filtered = beat_to_beat_map_sorted[abs(beat_to_beat_map_sorted[:, 0] -beat_to_beat_map_sorted[:, 2]) > gap]

	# best_match_map_filtered = best_match_map_filtered[((best_match_map_filtered[:, 1] /best_match_map_filtered[-1, 1]) * 100) <= (100-quality)]
	best_match_map_filtered = best_match_map_filtered[:int(best_match_map_filtered.shape[0] * (min(max(100-quality, 1e-4), 99.9999)/100)), :]
	if(quantity):
		# print(best_match_map_filtered.shape)
		best_match_map_filtered = best_match_map_filtered[:quantity, :]

	forward_map = best_match_map_filtered[(best_match_map_filtered[:, 0] - best_match_map_filtered[:, 2]) < 0]
	backward_map = best_match_map_filtered[(best_match_map_filtered[:, 0] - best_match_map_filtered[:, 2]) > 0]
	
	return beat_tracks,forward_map,backward_map, best_match_map_filtered


async def play_song(args):
	gap = args.gap
	quality = args.quality
	quantity = args.quantity
	padding = args.padding
	algo = args.algo

	songs = np.array([get_waveform(path) for path in args.path])
	_,sr = songs[0]

	if((songs[:, 1] != songs[0,1]).all()):
		raise Exception("The sample rates between the songs are different")
	
	beat_tracks, forward_map, backward_map, all_map = await get_beats_map(songs[:, 0], sr, algo, padding, quality, quantity, gap)

	print(f"Samples:\t{[waveform.shape[1] for waveform,_ in songs]}")
	print(f"SampleRate:\t{sr}")
	print(f"Beats:\t\t{[beat_track.shape[0] for beat_track in beat_tracks]}")
	print(f"Algorithm:\t{algo}")
	print(f"Padding:\t{padding}")
	print(f"Quality:\t{quality}")
	print(f"Quantity:\t{quantity}")
	print(f"Gap:\t\t{gap}")

	# beats = beat_track.shape[0]

	print(all_map)
	# print(beat_map.shape)
	output = True
	
	gen = audioGenerator([waveform for waveform,_ in songs], beat_tracks, all_map, output)
	# audio_thread = Thread(target=audio_loop, args=(gen,sr,))
	# audio_thread.start()
	stream = audio_loop(gen,sr, all_map)
	# audio_thread.join
	# print(stream)
	print(pos)


def parse_args():
	parser = argparse.ArgumentParser()

	parser.add_argument("--url", default=None)
	parser.add_argument("--path", default=None, nargs="*")
	parser.add_argument("--gap", default=15)
	parser.add_argument("--quality", default=99.5)
	parser.add_argument("--padding", default=5000, type=int)
	parser.add_argument("--algo", default="mse", choices=["mse","cossim"])
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

@app.route('/audio')
def audio(): 
	def genHeader(sampleRate, bitsPerSample, channels):
		datasize = 2000*10**6
		o = bytes("RIFF",'ascii')                                               # (4byte) Marks file as RIFF
		o += (datasize + 36).to_bytes(4,'little')                               # (4byte) File size in bytes excluding this and RIFF marker
		o += bytes("WAVE",'ascii')                                              # (4byte) File type
		o += bytes("fmt ",'ascii')                                              # (4byte) Format Chunk Marker
		o += (16).to_bytes(4,'little')                                          # (4byte) Length of above format data
		o += (1).to_bytes(2,'little')                                           # (2byte) Format type (1 - PCM)
		o += (channels).to_bytes(2,'little')                                    # (2byte)
		o += (sampleRate).to_bytes(4,'little')                                  # (4byte)
		o += (sampleRate * channels * bitsPerSample // 8).to_bytes(4,'little')  # (4byte)
		o += (channels * bitsPerSample // 8).to_bytes(2,'little')               # (2byte)
		o += (bitsPerSample).to_bytes(2,'little')                               # (2byte)
		o += bytes("data",'ascii')                                              # (4byte) Data Chunk Marker
		o += (datasize).to_bytes(4,'little')                                    # (4byte) Data size in bytes
		return o
	
	def sound():
		CHUNK = 1024
		sampleRate = 44100
		bitsPerSample = 16
		channels = 2
		wav_header = genHeader(sampleRate, bitsPerSample, channels)		
		stream = generate_audio_stream()
		first_run = True
		while True:
			if first_run:
				data = wav_header + stream.read(CHUNK)
				first_run = False
			else:
				data = stream.read(CHUNK)
			yield(data)

	return Response(sound())


if __name__ == "__main__":
	args = parse_args()
	loop = asyncio.get_event_loop()
	if(args.url):
		args.path = os.path.join(os.getcwd(), get_from_url(args.url))
	# args.basePath = os.path.join(os.getcwd(), args.path or os.path.join("data", args.pair, args.market, args.type, args.interval))
	loop.run_until_complete(play_song(args))