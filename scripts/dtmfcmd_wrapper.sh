#!/bin/sh

#didn't want to depend on the dpkg util start-stop-daemon

PID_FILE="/var/lock/dtmfcmd.pid"

if [ $# -ne 1 ]
then
        echo "Too few arguments"
        echo "Usage: $0 (start|stop)"
	exit 1
fi

case $1
in
	start) 
		/usr/share/dtmfcmd/set_recording.sh
		arecord -D hw:0,0 -f S16_LE -c 2 -t raw | dtmfcmd -r 8000 -c 2 -f s16_le &
		PID=$!
		echo ${PID} > ${PID_FILE};;
	stop)
		if [ -r ${PID_FILE} ]; then
			while read line; do
				echo "killing pid: $line"
				PID=$line
				kill ${PID}
			done < ${PID_FILE}
			rm ${PID_FILE}

			/usr/share/dtmfcmd/restore_state.sh
		else
			echo "No pid file"
		fi;;
	*) 
		echo "Sorry but '$1' is not a proper argument"
		exit 1;;
esac
