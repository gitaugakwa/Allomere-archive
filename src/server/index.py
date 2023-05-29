from __future__ import absolute_import

from flask import Flask
from flask_cors import CORS
from queue import queue
from hls import radio

server = Flask(__name__)
CORS(server)

server.register_blueprint(queue)
server.register_blueprint(radio)

if __name__ == '__main__':
	server.run(debug=True)