from __future__ import unicode_literals
# from __future__ import absolute_import

import argparse
import asyncio
import collections
import os
import re
import subprocess
from datetime import datetime, timezone, timedelta
from pathlib import Path
from pynput.keyboard import Key, Listener,GlobalHotKeys,KeyCode
# import sounddevice as sd
from threading import Thread
from tempfile import NamedTemporaryFile, TemporaryFile
import pathlib
import numpy as np
import librosa
import librosa.display
# import youtube_dl
import pyaudio
import soundfile
import yt_dlp
import pydub
import json
import shutil
import glob

import time
from random import randint 

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

from audio import get_audio_stream
from audio import audio_manager
from branch import get_beats_map



# import pytz


# Generate the audio file with the branched audio
# run ffmpeg on the file
# update list


FORMAT = pyaudio.paFloat32
DURATION = 3 * 60 # 3 minute songs
SAMPLE_RATE = 44100 # my FLAC sample rate
CHANNELS = 2
BUFFER_DURATION = 2 # Seconds
CHUNK = SAMPLE_RATE * BUFFER_DURATION

async def create_song(waveform, pos=0,state="Play",id=None, algo="mse", padding=10000, quality=0.05, quantity=None, gap=15):
	song = Song(waveform, pos, state,id)
	await song._init(algo, padding, quality, quantity, gap)
	song.generate_branch()
	return song

class Song:
	def __init__(self, waveform, pos=0, state="Play", id=None, index=0):
		self.waveform = waveform
		self.pos = pos
		self.id=id
		self.index=index
		self.branched = False
		self.branch_state=state
		self.prev_branch_state=state
		self.start_branch_beat=0
		self.end_branch_beat=0
		self.start_branch_sample=0
		self.end_branch_sample=0
		self.is_skipping_with_branches=False

	def set_state(self, state):
		self.prev_branch_state = self.branch_state
		self.branch_state = state
		# print(f"State Changed: {self.prev_branch_state} => {self.branch_state}")

	async def _init(self, algo="mse", padding=10000, quality=0.05, quantity=None, gap=15):
		# print(self.waveform.shape)
		tempDir = (os.path.join(os.getcwd(), "temp"))
		if os.path.exists(os.path.join(tempDir, f"{self.id}.json")):
			with open(os.path.join(tempDir, f"{self.id}.json"), "r") as read_file:
				context = json.load(read_file)
				beat_tracks = np.array(context["beat_tracks"])
				forward_map = np.array(context["forward_map"])
				backward_map = np.array(context["backward_map"])
				all_map = np.array(context["all_map"])
		else:
			start_time = time.time()
			beat_tracks, forward_map, backward_map, all_map = await get_beats_map(
				[self.waveform],
				SAMPLE_RATE,
				algo,
				padding, 
				quality,
				quantity,
				gap)
			end_time = time.time()
			print(f"get_beats_map: {end_time-start_time}s")
			with open(os.path.join(tempDir, f"{self.id}.json"), "w") as write_file:
				data = {
					"beat_tracks": beat_tracks.tolist(),
					"forward_map": forward_map.tolist(),
					"backward_map": backward_map.tolist(),
					"all_map": all_map.tolist(),
				}
				json.dump(data, write_file, indent=4)

		print(backward_map.shape)
		self.beat_track = beat_tracks[0]
		self.all_map = all_map
		self.forward_map = forward_map
		self.backward_map = backward_map

	def generate_branch(self,after=0, branch=None):
		start_branch_beat_raw = None
		branch_value_raw = None
		end_branch_beat_raw = None
		start_branch_beat = None
		end_branch_beat = None
		if self.branch_state == "Skip Branches":
			return
		elif not branch == None and branch < len(self.all_map):
			start_branch_beat_raw, branch_value_raw ,end_branch_beat_raw = self.all_map[branch]
			start_branch_beat, _ ,end_branch_beat = [int(val) for val in self.all_map[branch]]
		elif self.branch_state == "Endless":
			possible_branches = self.backward_map[self.backward_map[:, 0] > after]
			branch = randint(0, len(possible_branches)-1) if branch == None else branch
			start_branch_beat_raw, branch_value_raw ,end_branch_beat_raw = possible_branches[branch]
			start_branch_beat, _ ,end_branch_beat = [int(val) for val in possible_branches[branch]]
		elif self.branch_state == "Play":
			branch = randint(0, len(self.all_map)-1)
			start_branch_beat_raw, branch_value_raw ,end_branch_beat_raw = self.all_map[branch]
			start_branch_beat, _ ,end_branch_beat = [int(val) for val in self.all_map[branch]]
		elif self.branch_state == "Looping On Current Branch":
			if not branch == None:
				if branch <= len(self.backward_map) - 1:
					looping_branch = self.backward_map[branch]
					start_branch_beat_raw, branch_value_raw ,end_branch_beat_raw = looping_branch
					start_branch_beat, _ ,end_branch_beat = [int(val) for val in looping_branch]
			else:
				start_branch_beat_raw, branch_value_raw ,end_branch_beat_raw = [self.start_branch_beat_raw, self.branch_value_raw ,self.end_branch_beat_raw]
				start_branch_beat, end_branch_beat = [self.start_branch_beat, self.end_branch_beat]
		elif self.branch_state == "Skipping With Branches":
			possible_branches = self.forward_map[self.forward_map[:, 0] > after]
			if(len(possible_branches)):
				forward_branches = possible_branches[possible_branches[:, 0].argsort()]
				# longest_forward_branches = possible_branches[(possible_branches[:, 2]-possible_branches[:, 0]).argsort()[::-1]]
				start_branch_beat_raw, branch_value_raw ,end_branch_beat_raw = forward_branches[0] 
				start_branch_beat, _ ,end_branch_beat = [int(val) for val in forward_branches[0]]

		self.start_branch_beat_raw = start_branch_beat_raw if not start_branch_beat_raw == None else self.start_branch_beat_raw
		self.branch_value_raw = branch_value_raw if not branch_value_raw == None else self.branch_value_raw
		self.end_branch_beat_raw = end_branch_beat_raw if not end_branch_beat_raw == None else self.end_branch_beat_raw
		self.start_branch_beat = start_branch_beat if not start_branch_beat == None else self.start_branch_beat
		self.end_branch_beat = end_branch_beat if not end_branch_beat == None else self.end_branch_beat
		self.start_branch_sample = self.beat_track[self.start_branch_beat]
		self.end_branch_sample = self.beat_track[self.end_branch_beat]
		all_map_index = np.where(np.all(self.all_map == [self.start_branch_beat_raw, self.branch_value_raw ,self.end_branch_beat_raw], axis=1))[0]
		print(f"[{self.pos}] Next Branch: Index{all_map_index} {self.start_branch_beat}[{self.start_branch_sample}] -> {self.end_branch_beat}[{self.end_branch_sample-1}]")

	def has_branched(self, new_pos):
		all_map_index = np.where(np.all(self.all_map == [self.start_branch_beat_raw, self.branch_value_raw, self.end_branch_beat_raw], axis=1))[0]
		print(f"Branched: Index{all_map_index}  {self.start_branch_beat}[{self.start_branch_sample}] -> (1){self.end_branch_beat}[{self.end_branch_sample-1}]")
		if(self.branch_state == "Play"):
			self.generate_branch()
		elif(self.branch_state == "Endless"):
			new_beat = np.squeeze(np.where(self.beat_track>new_pos))[0]
			self.generate_branch(new_beat)
		elif(self.branch_state == "Skipping With Branches"):
			new_beat = np.squeeze(np.where(self.beat_track>new_pos))[0]
			self.generate_branch(new_beat)

	def start(self):
		radio_context = (os.path.join(os.getcwd(), "radio", "context.json"))
		with open(radio_context, "r+") as file:
			context = json.load(file)
			file.seek(0)
			context["index"] = self.index
			data = context
			json.dump(data, file, indent=4)

	def end(self):
		if(self.branch_state == "Skipping With Branches"):
			self.set_state("Play")

	def next(self):
		radio_context = (os.path.join(os.getcwd(), "radio", "context.json"))
		with open(radio_context, "r+") as file:
			context = json.load(file)
			file.seek(0)
			context["index"] = self.index + 1
			data = context
			json.dump(data, file, indent=4)

	def skip(self):
		self.pos = self.waveform.shape[0]
		print("Skipped")

	def restart(self):
		self.pos = 0
		print("Restarted")

	def skip_with_branch(self, after = None):
		after = self.pos if after == None else after

		self.set_state("Skipping With Branches")
		self.generate_branch(after=np.squeeze(np.where(self.beat_track>self.pos))[0])
		print(f"Skipping with branches from {self.pos}")
	
	def loop_on_branch(self, branch=None):
		looping_state = "Looping On Current Branch"
		if(self.branch_state == looping_state):
			self.set_state(self.prev_branch_state)
			return
		self.set_state(looping_state)
		self.generate_branch(branch=branch)
		print(f"Looping on branch: {self.start_branch_beat}[{self.start_branch_sample}] -> {self.end_branch_beat}[{self.end_branch_sample-1}]")



ydl_opts = {
		'format': 'bestaudio/best',
		# 'keepvideo': 'false',
		'forcefilename': 'true',
		'outtmpl': os.path.join(os.getcwd(),"temp",'%(id)s.%(ext)s'),
		'postprocessors': [{
			'key': 'FFmpegExtractAudio',
			'preferredcodec': 'mp3',
			'preferredquality': '320',
		}],
	}

def yt_client(video_url, download=False):
	with yt_dlp.YoutubeDL(ydl_opts) as ydl:
		return ydl.extract_info(video_url, download=download)

def write_m3u8_file(sections, count, directory, extension, clip = 0):
	timestamp = sections[0]["time"]
	iso_time = datetime.fromtimestamp(timestamp, tz=timezone.utc).isoformat()
	m3u8_file = open(os.path.join(os.getcwd(),f'radio\\{directory}\\live.m3u8'), "w")
	m3u8_file.write("#EXTM3U\n")
	m3u8_file.write("#EXT-X-VERSION:3\n")
	m3u8_file.write("#EXT-X-TARGETDURATION:3\n")
	# m3u8_file.write(f"#EXT-X-DISCONTINUITY-SEQUENCE:0\n")
	m3u8_file.write(f"#EXT-X-DISCONTINUITY-SEQUENCE:{max(count-1, 0)}\n")
	m3u8_file.write(f"#EXT-X-MEDIA-SEQUENCE:{count}\n")
	m3u8_file.write(f"#EXT-X-PROGRAM-DATE-TIME:{iso_time}Z\n")

	for index, section in enumerate(sections):
		if not index == 0:
			m3u8_file.write("#EXT-X-DISCONTINUITY\n")
		m3u8_file.write(f"#EXTINF:{(section['duration']-clip):.03f},live\n")
		m3u8_file.write(f"{section['name']}.{extension}\n")

	m3u8_file.close()

def write_radio_m3u8_file(directory):
	m3u8_file = open(os.path.join(os.getcwd(),f'radio\\{directory}\\radio.m3u8'), "w")
	m3u8_file.write("#EXTM3U\n")
	m3u8_file.write("#EXT-X-VERSION:3\n")
	m3u8_file.write("#EXT-X-STREAM-INF:\n")
	# m3u8_file.write(f"#EXT-X-DISCONTINUITY-SEQUENCE:{max(count-1, 0)}\n")
	m3u8_file.write(f"live.m3u8\n")

	m3u8_file.close()

def initialize(args):
	tempDir = (os.path.join(os.getcwd(), "temp"))
	radioDir = (os.path.join(os.getcwd(), "radio"))
	if not os.path.exists(tempDir):
		os.makedirs(tempDir)
	if not os.path.exists(radioDir):
		os.makedirs(radioDir)
	if not os.path.exists(os.path.join(radioDir, "mp3")):
		os.makedirs(os.path.join(radioDir, "mp3"))
	if not os.path.exists(os.path.join(radioDir, "hls")):
		os.makedirs(os.path.join(radioDir, "hls"))
	if not os.path.exists(os.path.join(radioDir, "flac")):
		os.makedirs(os.path.join(radioDir, "flac"))
	if not os.path.exists(os.path.join(radioDir, "aac")):
		os.makedirs(os.path.join(radioDir, "aac"))
	for sectionPath in Path(os.path.join(os.getcwd(), "radio")).glob("**/+(aac|flac|hls)/**/*"):
		if sectionPath.is_file():
			sectionPath.unlink()
	
	if not os.path.exists(os.path.join(radioDir, "context.json")):
		with open(os.path.join(radioDir, "context.json"), "w") as write_file:
			data = {
				"index": 0,
				"queue": [
					"https://www.youtube.com/watch?v=piwAsOjbSo4", # Seekae - Test and Recognize
					# "https://www.youtube.com/watch?v=q9p_1L2lyWg", # Flume - MUD
					"https://www.youtube.com/watch?v=fj9b4uxs2qs", # Kabza De Small - Abalele
					]
			}
			json.dump(data, write_file, indent=4)

	if not os.path.exists(os.path.join(tempDir, "buffers")):
		os.makedirs(os.path.join(tempDir, "buffers"))
	
	bufferDir = os.path.join(tempDir, "buffers")
	silence = pydub.AudioSegment(np.full(( SAMPLE_RATE * 2,CHANNELS), 0, dtype=np.int16 ), frame_rate=SAMPLE_RATE, sample_width=2, channels=CHANNELS)
	silence.export(os.path.join(bufferDir, "buffer0.wav"), format="wav",bitrate="1411k" )
	silence.export(os.path.join(bufferDir, "buffer1.wav"), format="wav",bitrate="1411k" )
	silence.export(os.path.join(bufferDir, "buffer2.wav"), format="wav",bitrate="1411k" )
	with open(os.path.join(bufferDir, "concat.txt"), "w") as write_file:
		write_file.write(f"file 'buffer0.wav'\n")
		write_file.write(f"file 'buffer1.wav'\n")
		write_file.write(f"file 'buffer2.wav'\n")


async def radio(args): 
	live = True
	algo = "mse" if args.algo is None else args.algo
	padding = 10000 if args.padding is None else args.padding
	quality = 0.05 if args.quality is None else args.quality # 99.5 # mse val <= 0.05
	quantity = 100 if args.quantity is None else args.quantity
	gap = 15 if args.gap is None else args.gap
	# print(f"Samples:\t{[waveform.shape[1] for waveform,_ in songs]}")
	print(f"SampleRate:\t{SAMPLE_RATE}")
	# print(f"Beats:\t\t{[beat_track.shape[0] for beat_track in beat_tracks]}")
	print(f"Algorithm:\t{algo}")
	print(f"Padding:\t{padding}")
	print(f"Quality:\t{quality}")
	print(f"Quantity:\t{quantity}")
	print(f"Gap:\t\t{gap}")
	print(f"Output:\t\t{args.output}")
	tempDir = (os.path.join(os.getcwd(), "temp"))
	bufferDir = os.path.join(tempDir, "buffers")

	initialize(args)

	radio_context = (os.path.join(os.getcwd(), "radio", "context.json"))

	with open(radio_context, "r+") as file:
		context = json.load(file)
		file.seek(0)
		if not args.index == None:
			context["index"] = args.index
			json.dump(context, file, indent="\t")

	m3u8_deque = collections.deque([], 10)
	m3u8_deque_overflow_count = 0
	count = 0
	startTime = datetime.now(timezone.utc).timestamp()
	buffer_deque = collections.deque([pydub.AudioSegment(np.full(( SAMPLE_RATE * 2,CHANNELS), 0, dtype=np.int16 ), frame_rate=SAMPLE_RATE, sample_width=2, channels=CHANNELS),pydub.AudioSegment(np.full(( SAMPLE_RATE * 2,CHANNELS), 0, dtype=np.int16 ), frame_rate=SAMPLE_RATE, sample_width=2, channels=CHANNELS),pydub.AudioSegment(np.full(( SAMPLE_RATE * 2,CHANNELS), 0, dtype=np.int16 ), frame_rate=SAMPLE_RATE, sample_width=2, channels=CHANNELS)], 3)

# [0, 1, 2]
	def radio_callback(input_samples,pos,  frame_count, time_info, status_flags):
		nonlocal m3u8_deque_overflow_count
		nonlocal count
		nonlocal startTime
		nonlocal songs
		suffix = f"{startTime:.2f}".replace(".", "_")
		section = os.path.join(os.getcwd(),f'radio\\mp3\\live_{suffix}.mp3')
		flac = os.path.join(os.getcwd(),f'radio\\flac\\live_{suffix}.flac')
		buffer = os.path.join(os.getcwd(),'temp\\buffers\\buffer.wav')
		# aac = os.path.join(os.getcwd(),f'radio\\aac\\live_{suffix}.m4a')
		# print(section)
		song = pydub.AudioSegment(np.int16(input_samples * 2 ** 15), frame_rate=SAMPLE_RATE, sample_width=2, channels=CHANNELS)

		buffer_deque.append(song)
		for index, buffer_segment in enumerate(buffer_deque):
			buffer_segment.export(os.path.join(bufferDir, f"buffer{index}.wav"), format="wav",bitrate="1411k")#,parameters=["-vol", "70"] )

		# song.export(section, format="mp3", bitrate="320k")

		# song.export(flac, format="flac", bitrate="911k")
		# song.export(aac, format="m4a", bitrate="128k")

		command = ["ffmpeg",
				'-y',
				"-hide_banner",
				# "-ac", "2",
				"-guess_layout_max","0",
				# '-channel_layout', 'stereo',
				"-y",
				"-f", "concat",
				'-i', "temp/buffers/concat.txt",
				# "-safe", "0",

				'-c', 'copy',
				"temp/buffers/buffer.wav"
				]
		result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
		# print("CONCAT BUFFERS")
		# print(result.stderr)
		command = ["ffmpeg",
				'-y',
				"-hide_banner",
				# "-ac", "2",
				"-guess_layout_max","0",
				'-channel_layout', 'stereo',
				'-i', buffer,
				# "-acodec", "libmp3lame",
				# '-ab', '320k',
				# "-c:a", "libtwolame",
				# "-b:a", "384k"
				"-b:a", "256k",

				# '-f', 'segment',
				# "-segment_time", "2",
				#  os.path.join(bufferDir, "hls%03d.ts"),
				'-f', 'hls',
				"-hls_time", "2",
				"-hls_segment_filename", os.path.join(bufferDir, "hls%03d.ts"),
				os.path.join(bufferDir, "stream.m3u8")
				]
		result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
		# print("SEGMENT TO HLS")
		# print(result.stderr)
		shutil.copyfile(os.path.join(bufferDir, "hls001.ts"),os.path.join(os.getcwd(), "radio", "hls", f"live_{suffix}.ts"))
		# command = ["ffmpeg",
		# 		'-y',
		# 		"-hide_banner",
		# 		# "-ac", "2",
		# 		"-guess_layout_max","0",
		# 		# '-channel_layout', 'stereo',
		# 		'-i', flac,

		# 		'-c:a', 'libfdk_aac',
		# 		"-profile:a", "aac_he_v2",
		# 		'-b:a', '128k',
		# 		 f"radio/aac/live_{suffix}.aac"
		# 		 ]
		# result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
		# command = ["ffmpeg",
		# 		'-y',
		# 		"-hide_banner",
		# 		# "-ac", "2",
		# 		"-guess_layout_max","0",
		# 		# '-channel_layout', 'stereo',
		# 		'-i', flac,

		# 		"-acodec", "libmp3lame",
		# 		'-ab', '320k',
		# 		"-map_metadata", "0",
		# 		'-id3v2_version', '3',
		# 		 f"radio/mp3/live_{suffix}.mp3"
		# 		 ]
		# result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


		# command = ["ffmpeg",
		# 		'-y',
		# 		"-hide_banner",
		# 		# "-ac", "2",
		# 		"-guess_layout_max","0",
		# 		# '-channel_layout', 'stereo',
		# 		'-i', flac,

		# 		# '-c:a', 'copy',
		# 		# '-q:a', '0',
		# 		"-acodec", "libmp3lame",
		# 		'-ab', '320k',
		# 		"-map_metadata", "0",
		# 		'-id3v2_version', '3',
		# 		 f"radio/hls/live_{suffix}.ts"
		# 		 ]
		# result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
		# print(result.stderr)
		# duration = re.findall(r"time=(\d+:\d+:\d+.\d+)+", result.stderr)[-1]
		# seconds = duration.split(":")[-1]
		section_dict = {}
		section_dict["time"] = startTime
		section_dict["duration"] = 2 # float(seconds)
		section_dict["name"] = f"live_{suffix}"

		if(m3u8_deque.maxlen == len(m3u8_deque)):
			m3u8_deque_overflow_count = m3u8_deque_overflow_count + 1
		m3u8_deque.append(section_dict)
		# print(m3u8_deque)
		outputs = [
			# ["mp3", "mp3", 0],
			["hls", "ts", 0],
			# ["flac", "flac", 0],
			# ["aac", "aac", 0]
			]
		for directory, extension, clip in outputs:
			write_m3u8_file(m3u8_deque, m3u8_deque_overflow_count, directory, extension, clip)
		# print(pos)
		startTime = startTime + 2 # float(seconds)
		count = count + 1

	songs = collections.deque([None], 3)

	def generate_branch(beat_map, beat_track):
		branch = randint(0, len(beat_map)-1)
		start_branch_beat, _ ,end_branch_beat = [int(val) for val in beat_map[branch]]
		start_branch_sample = beat_track[start_branch_beat]
		end_branch_sample = beat_track[end_branch_beat]
		return ((start_branch_beat, start_branch_sample), (end_branch_beat, end_branch_sample))

	# url = queue[queue_index]
	# song_details = yt_client(url, False)
	# song_id = song_details["id"]
	# song_path = os.path.join("temp", f"{song_id}.mp3")
	# if not os.path.exists(song_path):
	# 	yt_client(url, True)
	# audio = np.array(librosa.core.load(song_path, sr=SAMPLE_RATE, mono=False )[0])
	# song = await create_song(audio.T, 0,"Skip Branches",song_id, algo, padding, quality, quantity, gap)
	# songs.append(song)
	
	write_radio_m3u8_file("hls")

	# stream = await get_audio_stream(songs, passedCallback=radio_callback, output=args.output, )
	# stream.start_stream()

	# Create a garbage collection thread
	def cleanup():
		while(live):
			for filename in glob.glob(os.path.join(os.getcwd(), "radio", "hls", "live_*"))[:-15]:
				with open(filename, "w") as f:
					True
				# print(f"deleted {filename}")
				os.remove(filename) 
			time.sleep(2)


	# Queue thread
	async def queue_manager():
		stream = None
		while(live):
			initial = None
			# if(not stream == None):
			# 	print(stream.is_active())
			# 	print(stream.is_stopped())
			# 	print(len(songs))
			# 	time.sleep(2)
			if(len(songs) >= 2):
				if(stream == None or not stream.is_active()):
					stream = await get_audio_stream(songs, passedCallback=radio_callback if args.callback == True else None, output=args.output, )
					# stream = await get_audio_stream(songs, output=args.output, )
					stream.start_stream()
			if(len(songs) <= 2):
				# start_time = time.time()
				with open(radio_context, "r") as read_file:
					context = json.load(read_file)
				queue_index = context["index"]
				queue = context["queue"]
				# print(f"A: {time.time()-start_time}s")
				if(not songs[-1] == None):
					index = songs[-1].index + 1
				else:
					initial = args.initial
					index = queue_index + len(songs)-1
				if(index >= len(queue)):
					continue
				url = queue[index]
				song_details = yt_client(url, False)
				song_id = song_details["id"]
				song_path = os.path.join("temp", f"{song_id}.mp3")
				# print(f"B: {time.time()-start_time}s")
				print(f"Adding Song: [{index}] {song_id}")

				if not os.path.exists(song_path):
					yt_client(url, True)
				# print(f"C: {time.time()-start_time}s")
				audio = np.array(librosa.core.load(song_path, sr=SAMPLE_RATE, mono=False )[0])
				song = Song(audio.T, 0, "Endless",song_id, index)
				# print(f"D: {time.time()-start_time}s")
				songs.append(song)
				# print(f"E: {time.time()-start_time}s")
				# this seems to be garbage collected for some reason cause it is not being awaited
				await song._init(algo, padding, quality, quantity, gap)
				# print(f"F: {time.time()-start_time}s")
				song.generate_branch(branch=initial)
				# print(f"G: {time.time()-start_time}s")
				# song = await create_song(audio.T, 0,"Play", algo, padding, quality, quantity, gap)
				print(f"Added Song to Songs: [{index}] {song_id}")



	# Keyboard Listener
	keyboard_context = {}
	
	def on_press(key):
		try:
			# if(key.char == "r"):
			# 	generateBranchProbability(branches, best_match_map_filtered_by_beat.shape[0])
			# if(key.char == "m"):
			# 	playback["mute"] = not playback["mute"]
			# if(key.char == "s"):
			# 	playback["stop"] = not playback["stop"]
			# return press_event()
			key.char
			key_code = str(key).replace("'", '')
			if(key.char == "l" or key_code == "\\x0c"):
				if(keyboard_context.get("ctrl",False)):
					if(keyboard_context.get("shift", False)):
						# print("Looping current branch")
						songs[1].loop_on_branch()
						return True
			if(key.char == "r" or key_code == "\\x12"):
				if(keyboard_context.get("ctrl",False)):
					if(keyboard_context.get("shift", False)):
						# print("Generate new branch")
						songs[1].generate_branch()
						return True
			if(key.char == "p" or key_code == "\\x10"):
				if(keyboard_context.get("ctrl",False)):
					if(keyboard_context.get("shift", False)):
						print("Set song state to Play")
						songs[1].set_state("Play")
						return True
			# print('alphanumeric key {0} pressed'.format(
			# key.char))
			return True
		except:
			# if(key == Key.left):
			# 	songs[1].pos = max(songs[1].pos - SAMPLE_RATE * 5, 0)
			# 	print("-5 sec")
			# 	# new_pos = pos - sr * 5
			# 	# if song != 0 and new_pos <0:
			# 	# 	song -= 1
			# 	# pos = max(pos - sr * 5, 0)
			# if(key == Key.right):
			# 	songs[1].pos = songs[1].pos + SAMPLE_RATE * 5
			# 	print("+5 sec")
			if(key == Key.shift_l or key == Key.shift_r):
				keyboard_context["shift"] = True
				return True
			if(key == Key.ctrl_l or key == Key.ctrl_r):
				keyboard_context["ctrl"] = True
				return True
			if(key == Key.alt_l or key == Key.alt_r):
				keyboard_context["alt"] = True
				return True
			if(key == Key.media_next):
				songs[1].skip()
				return True
			if(key == Key.media_previous):
				songs[1].restart()
				return True
			if(key == Key.right):
				if(keyboard_context.get("ctrl",False)):
					if(keyboard_context.get("shift", False)):
						# print("Skipping song")
						songs[1].skip()
						return True
					# print("Skipping song with branches")
					songs[1].skip_with_branch()
					return True
			if(key == Key.left):
				if(keyboard_context.get("ctrl",False)):
					if(keyboard_context.get("shift", False)):
						# print("Skipping song")
						songs[1].restart()
						return True
					return True

			
			# if(key == Key.space):
			# 	playback["pause"] = not playback["pause"]
			# return press_event()
			# print('special key {0} pressed'.format(
			# key))
			return True

	def on_release(key):
		# print('{0} release'.format(
		# 	key))
		if(key == Key.shift_l or key == Key.shift_r ):
			keyboard_context["shift"] = False
			return True
		if(key == Key.ctrl_l or key == Key.ctrl_r):
			keyboard_context["ctrl"] = False
			return True
		if(key == Key.alt_l or key == Key.alt_r):
			keyboard_context["alt"] = False
			return True
		if key == Key.esc:
			# Stop listener
			return False

	# def skip_song():
	# 	print("Skip Song")
	# def skip_song_with_branches():
	# 	print("Skip Song")
	# def loop_on_branch():
	# 	print("Loop On Branch")
	# keyboard_thread = GlobalHotKeys({
	# 	"<ctrl>+<shift>+l": loop_on_branch,
	# })
	keyboard_thread = Listener(on_press, on_release)

	cleanup_thread = Thread(target=cleanup, args=())
	cleanup_thread.start()
	queue_thread = Thread(target=asyncio.run, args=(queue_manager(),))
	queue_thread.start()
	keyboard_thread.start()

	# fig = plt.figure(figsize=(20,4), dpi=100)
	# librosa.display.waveshow(songs[1].waveform[:,0], sr=SAMPLE_RATE)
	# fig.tight_layout()
	# fig.show()

	if(args.display):
		def animate_audio(i):
			plt.clf()
			plt.plot(np.linspace(0, songs[1].waveform.shape[0]/SAMPLE_RATE, num=songs[1].waveform.shape[0], endpoint=False), songs[1].waveform[:,0])
			plt.axvline(x=songs[1].pos/SAMPLE_RATE, color='r')
			plt.axvline(x=songs[1].start_branch_sample/SAMPLE_RATE, color='g')
			plt.axvline(x=songs[1].end_branch_sample/SAMPLE_RATE, color='y')
			plt.xlabel("Seconds")
			plt.ylabel("Audio")

		ani = FuncAnimation(plt.gcf(), animate_audio, interval=2000, save_count=0)

		plt.show()

	try:
		while live:
			# Display Audio
			# plt.clf()
			# plt.plot(np.linspace(0, songs[1].waveform.shape[0], songs[1].waveform.shape[0], endpoint=False), songs[1].waveform[:,0])
			# plt.axvline(x=context["pos"], color='r')
			# plt.axvline(x=context["start_branch"], color='g')
			# plt.axvline(x=context["end_branch"], color='y')
			# plt.xlabel("Samples")
			# plt.ylabel("Audio")
			# plt.pause(2)
			# plt.draw()
			# plt.show()
			True
	except KeyboardInterrupt:
		print("keyboard interupt")
		plt.close("all")
		keyboard_thread.stop()
		live= False
		True

	
	# stream.stop_stream()
	# stream.close()

	keyboard_thread.join()
	cleanup_thread.join()
	queue_thread.join()
	audio_manager.terminate()


def parse_args():
	parser = argparse.ArgumentParser()

	parser.add_argument("--url", default=None)
	parser.add_argument("--path", default=None, nargs="*")
	parser.add_argument("--gap", default=15)
	parser.add_argument("--quality", default=0.05)
	parser.add_argument("--padding", default=10000, type=int)
	parser.add_argument("--algo", default="mse", choices=["mse","cossim"])
	parser.add_argument("--quantity", default=None, type=int)
	parser.add_argument("--initial", default=None, type=int)
	parser.add_argument("--index", default=None, type=int)
	parser.add_argument("--callback", default=False, action=argparse.BooleanOptionalAction)
	parser.add_argument("--output", default=False, action=argparse.BooleanOptionalAction)
	parser.add_argument("--display", default=False, action=argparse.BooleanOptionalAction)
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

	# Display Initialization
	# plt.ion()
	# plt.show()


	# if(args.url):
	# 	args.path = os.path.join(os.getcwd(), get_from_url(args.url))
	# args.basePath = os.path.join(os.getcwd(), args.path or os.path.join("data", args.pair, args.market, args.type, args.interval))
	loop.run_until_complete(radio(args))
