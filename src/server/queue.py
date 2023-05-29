
from flask import request, Blueprint
import json
import os
import yt_dlp

queue = Blueprint("queue", __name__)


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

@queue.route('/queue', methods=["POST"])
def add_to_queue():
	data = request.get_json()
	url = data["url"]
	print(data)

	radio_context = os.path.join(os.getcwd(), "radio/context.json")

	with yt_dlp.YoutubeDL(ydl_opts) as ydl:
		yt_info = ydl.extract_info(url, download=False)
		with open("temp.json", "w") as write_file:
			json.dump(ydl.sanitize_info(yt_info), write_file, indent="\t")

	if yt_info["_type"] == "playlist":
		for entry in yt_info["entries"]:
			entry_url = entry["webpage_url"]
			with open(radio_context, "r+") as file:
				context = json.load(file)
				context["queue"].append(entry_url)
				file.seek(0)
				json.dump(context, file, indent="\t")
		return "Added playlist to Queue"

	if yt_info["_type"] == "video":
		with open(radio_context, "r+") as file:
			context = json.load(file)
			context["queue"].append(url)
			file.seek(0)
			json.dump(context, file, indent="\t")
		return "Added Video to Queue"

	return "Error Adding to queue"

@queue.route('/queue', methods=["GET"])
def get_queue():
	radio_context = os.path.join(os.getcwd(), "radio/context.json")

	with open(radio_context, "r") as read_file:
		context = json.load(read_file)
	return context["queue"]
	

	



