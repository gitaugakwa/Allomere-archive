from flask import  send_from_directory, Blueprint
import pathlib
import os

radio = Blueprint("radio", __name__)

@radio.route('/radio/<path:path>')
def send_report(path):
	
	# print(os.path.join(pathlib.Path(__file__).parent.resolve(), '..\..', 'radio'))
	# print(path)
	return send_from_directory(os.path.join(pathlib.Path(__file__).parent.resolve(), '..\..', 'radio'), path)

