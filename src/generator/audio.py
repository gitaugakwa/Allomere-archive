from __future__ import unicode_literals
# from __future__ import absolute_import

# from tensorflow_io.python.api.audio import AudioIOTensor
# import tensorflow as tf
# import tensorflow_io as tfio
from datetime import datetime, timezone
from itertools import chain, islice, tee
from pathlib import Path
from tempfile import NamedTemporaryFile
# import sounddevice as sd
from threading import Thread

import numpy as np
# import youtube_dl
import pyaudio
from pyaudio import Stream
from random import randint 

# from pynput.keyboard import Key, Listener,GlobalHotKeys,KeyCode




# import pytz


audio_manager = pyaudio.PyAudio()


# Generate the audio file with the branched audio
# run ffmpeg on the file
# update list

FORMAT = pyaudio.paFloat32
DURATION = 3 * 60 # 3 minute songs
SAMPLE_RATE = 44100 # my FLAC sample rate
CHANNELS = 2
BUFFER_DURATION = 2 # Seconds
CHUNK = SAMPLE_RATE * BUFFER_DURATION

# songs deque of 3 -> prev, curr, next
async def get_audio_stream(songs, *args, **kwargs):
	passedCallback = kwargs.get("passedCallback", None)
	output = kwargs.get("output", None)
	# audio = np.array(librosa.core.load(path, sr=SAMPLE_RATE, mono=False )[0])
	song = songs[1]
	# curr_shape = None
	# curr_beat_map = None
	# curr_beat_track = None
	# branch = 0

	# def generate_new_branch():
	# 	nonlocal branch
	# 	nonlocal context
	# 	branch = randint(0, len(curr_beat_map)-1)
	# 	start_branch_beat, _ ,end_branch_beat = [int(val) for val in curr_beat_map[branch]]
	# 	start_branch_sample = curr_beat_track[start_branch_beat]
	# 	end_branch_sample = curr_beat_track[end_branch_beat]
	# 	context['start_branch'] = start_branch_sample
	# 	context['end_branch'] = end_branch_sample
	# 	print(f"[{song.pos}] Next Branched: (1){start_branch_beat}[{start_branch_sample}] -> (1){end_branch_beat}[{end_branch_sample-1}]")


	def init_song():
		# nonlocal curr_shape
		nonlocal song
		# nonlocal curr_beat_map
		# nonlocal curr_beat_track
		song = songs[1]
		# curr_shape = song.waveform.shape
		# curr_beat_map = song.beat_map
		# curr_beat_track = song.beat_track
		# generate_new_branch()
	
	init_song()

	def callback(_, frame_count, time_info, status_flags):
		nonlocal song
		# pos = 0
		# print(frame_count)
		# reduce volume by 2dB
		# This reduces the clipping in most songs
		db = -2

		# print(f"Pos:\t{pos}")
		new_pos = song.pos + frame_count
		# print(f"NewPos:\t{new_pos}")

		start_branch_sample, end_branch_sample = (song.start_branch_sample, song.end_branch_sample)
		start_branch_beat, end_branch_beat = (song.start_branch_beat, song.end_branch_beat)

		if(new_pos >= start_branch_sample and song.pos <= start_branch_sample):
			pos_delta = new_pos - start_branch_sample
			new_pos = end_branch_sample + pos_delta
			# print(f"[{song.pos}] Branched: (1){start_branch_beat}[{start_branch_sample}] -> (1){end_branch_beat}[{end_branch_sample-1}]")
			data = np.concatenate([
				song.waveform[song.pos : start_branch_sample],
				song.waveform[end_branch_sample : new_pos]
				])
			song.branched = True
			song.has_branched(new_pos)
			# pos = new_pos
		else:
			data = song.waveform[song.pos : new_pos]
		# data = song.waveform[pos : new_pos]
		print(f"[{song.pos}] Playing")

		# data has all the available samples from the current song
		if(new_pos >= song.waveform.shape[0]):
			songs.popleft()
			if(len(songs) >= 2):
				print(f"[{song.pos}] Next Song: (1)[{song.pos}] -> (2)[{new_pos - song.waveform.shape[0]}]")
				data = np.concatenate([
					data,
					songs[1].waveform[0 : new_pos - song.waveform.shape[0]]
					])
				new_pos = new_pos - song.waveform.shape[0]
				song.end()
				init_song()
				print(song.index)
				song.start()
			else:
				song.next()

		data = data * pow( 10.0, db * 0.05 )
		
		if(passedCallback):
			passedCallback(data,song.pos, frame_count, time_info, status_flags)

		if len(data) != frame_count:
			print(f"[{song.pos}] Ended: (1)")
			return data, pyaudio.paComplete

		song.pos = new_pos
		return data, pyaudio.paContinue

	# for i in range(audio_manager.get_device_count()):
	# 	print(audio_manager.get_device_info_by_index(i))

	stream = Stream(
		PA_manager=audio_manager,
		input=True,
		output=output,
		rate=SAMPLE_RATE,
		channels=CHANNELS,
		format=FORMAT,
		frames_per_buffer=CHUNK,
		stream_callback=callback)

	return stream
