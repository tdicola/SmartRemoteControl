import socket
import time

from flask import *

import config


# TCP port the Yun console listens for connections on.
CONSOLE_PORT = 6571

# Create flask application.
app = Flask(__name__)

# Get activity configuration.
activities = config.get_activities()


@app.route('/')
def root():
	return render_template('index.html', activities=activities)

@app.route('/activity/<int:index>', methods=['POST'])
def activity(index):
	# Connect to the console socket.
	console = socket.create_connection(('localhost', CONSOLE_PORT))
	# Send all the codes in order that are associated with the activity.
	for code in activities[index].get('codes', []):
		console.sendall(code + '\n')
		# Wait ~500 milliseconds between codes.
		time.sleep(0.5)
	console.close()
	return 'OK'


if __name__ == '__main__':
	# Create a server listening for external connections on the default
	# port 5000.  Enable debug mode for better error messages and live
	# reloading of the server on changes.  Also make the server threaded
	# so multiple connections can be processed at once (very important
	# for using server sent events).
	app.run(host='0.0.0.0', debug=True, threaded=True)
