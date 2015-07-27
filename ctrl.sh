#!/bin/sh
# Script for testing sending control data to the fifo in /tmp.
# run with 'stop', 'start' or an unsigned number as an
# argument to stop, start, and change the speed of the drive,
# respectively.

fifodir=/tmp
fifoname=mbdfifo
start_cmd='start'
stop_cmd='stop'

num=$(ls $fifodir/$fifoname* | wc -l)

if [ $# -ne 1 ]
then
	echo "Usage: $0 [ start | stop | <speed> ]"
	exit 1
fi

if [ $num -eq 0 ]
then
	echo "fifo not found. exiting."
	exit 1
elif [ $num -ne 1 ]
then
	echo "more than one fifo file found. exiting."
	exit 1
fi

fifo=$(ls $fifodir/$fifoname*)

echo -n "$1" > "$fifo" &
kill -USR1 $(echo -n "$fifo" | awk -F '.' '{print $2}')


