
import argparse
import asyncio
from http import server
from threading import Thread
from generator.radio import radio
from server.index import server
import asyncio
from multiprocessing import Process
import time

def parse_args():
	parser = argparse.ArgumentParser()

	parser.add_argument("--url", default=None)
	parser.add_argument("--path", default=None, nargs="*")
	parser.add_argument("--gap", default=15)
	parser.add_argument("--quality", default=99.5)
	parser.add_argument("--padding", default=5000, type=int)
	parser.add_argument("--algo", default="mse", choices=["mse","cossim"])
	parser.add_argument("--quantity", default=None, type=int)
	parser.add_argument("--output", default=False, action=argparse.BooleanOptionalAction)
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
	# loop = asyncio.get_event_loop()
	# args.basePath = os.path.join(os.getcwd(), args.path or os.path.join("data", args.pair, args.market, args.type, args.interval))
	radio_thread = Thread(target=asyncio.run, args=(radio(args),))
	server_thread = Thread(target=lambda: server.run(debug=False, use_reloader=False))

	server_thread.daemon = True
	radio_thread.daemon = True
	
	server_thread.start()
	radio_thread.start()

	try:
		while True:
			time.sleep(2)

	except KeyboardInterrupt:
		print("Keyboard Interrupt")
		# radio_process.terminate()
	# server_thread.join()
	# print("Server Thread Join")

	# loop.run_until_complete(radio(args))