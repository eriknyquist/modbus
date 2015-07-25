#!/usr/bin/python2.7
#
# mbd_mqtt_client.py
#
# this script allows control of a running instance of mbd through MQTT.
# It subscribes to the topic TOPIC on the broker at HOST, listens
# for control messages, and if one is received it will write to the file
# /tmp/mbdfifo.* and send SIGUSR1 to the running instance of mbd.
#
# Needs Paho MQTT client- get from https://pypi.python.org/pypi/paho-mqtt
# or using pip, i.e. "pip install paho-mqtt"
#
# Erik Nyquist <eknyquist@gmail.com>

import glob
import sys
import os
import paho.mqtt.client as mqtt
from subprocess import Popen, PIPE

TOPIC =         'test'
HOST =          'test.mosquitto.org'
PORT =          1883
KEEPALIVE =     60

MINSPEED =      0
MAXSPEED =      10000

FIFODIR =       '/tmp'
FIFONAME =      'mbdfifo'

START_CMD =     'start'
STOP_CMD =      'stop'
KILL_CMD =      'kill -USR1'

def die(msg):
	print("%s: %s" % (sys.argv[0], msg))
	sys.exit(-1)

def convert_speed(strspeed):
	try:
		speed = int(strspeed)
	except ValueError:
		print("Error interpreting speed command")
		speed = 0

	return speed

def get_fifo_name():
	files = glob.glob(FIFODIR + '/' + FIFONAME + '.*')

	if len(files) == 0:
		die("fifo '%s/%s.*' not found" % (FIFODIR, FIFONAME))
	elif len(files) > 1:
		die("more than one fifo found\n%s" % (files))

	return files[0]

def send_ctrl_msg(msg):
	fifo = get_fifo_name()
	pid = os.path.basename(fifo).split('.')[1]
	echocmd = 'echo -n \'' + msg + '\' > ' + fifo
	sigcmd = KILL_CMD + ' ' + pid

	Popen(echocmd, shell=True)
	Popen(sigcmd, shell=True)

def on_connect(client, userdata, rc):
	print("Connected to %s" % HOST)
	client.subscribe(TOPIC)

def on_message(client, userdata, msg):
	received = str(msg.payload)

	if received == START_CMD:
		send_ctrl_msg(START_CMD)
	elif received == STOP_CMD:
		send_ctrl_msg(STOP_CMD)
	else:
		speed = convert_speed(received)

		if speed >= MINSPEED and speed <= MAXSPEED:
			send_ctrl_msg(str(speed))
		else:
			print("Can't set speed of %d- must be between "
			      "%d and %d" % (speed, MINSPEED,
			      MAXSPEED))

def start_listening():
	client = mqtt.Client()
	client.on_connect = on_connect
	client.on_message = on_message
	client.connect(HOST, PORT, KEEPALIVE)
	client.loop_forever()

def main():
	start_listening()

main()
