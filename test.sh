#!/bin/sh

dname=bin/mbd
this="$0"
testconf=conf/test
pconf="$testconf"/shouldrun
nconf="$testconf"/shouldnotrun
RUNTIME=2
SHOULDRUN=1
NUMTESTS=0
FAILED=0
NRESULTS=""
PRESULTS=""

if [ ! -f "$dname" ]
then
	echo "Can't find executable file $dname. Try running 'make' first?"
	exit 1
fi

pcheck() {
	file="$1"
	bfile=$(basename $file)

	./"$dname" -c "$file"
	sleep $RUNTIME

	num=$(ps aux | grep [m]bd | wc -l )
	if [ $num -eq 1 ]
	then
		kill $(ps aux | grep [m]bd | awk '{print $2}')
		PRESULTS="$PRESULTS#File '$bfile' passed."
	else
		PRESULTS="$PRESULTS#File '$bfile' failed: $num instances of $dname after execution."
		FAILED=$((FAILED + 1))
		dump_results 0
		exit 1
	fi
}

ncheck() {
	file="$1"
	bfile=$(basename $file)

	./"$dname" -f -c "$file"
	sleep 1

	num=$(ps aux | grep [m]bd | wc -l)
	if [ $num -gt 0 ]
	then
		NRESULTS="$NRESULTS#File '$bfile' failed: should have terminated due to faulty conf, but ran anyway."

		if [ $num -gt 1 ]
		then
			echo "Multiple instances of daemon after negative test- terminating."
			dump_results 0
			exit 1
		fi

		kill $(ps aux | grep [m]bd | awk '{print $2}')	
		FAILED=$((FAILED + 1))
	else
		NRESULTS="$NRESULTS#File '$bfile' passed."
	fi
}

runtests () {
	dir="$1"
	running=$(ps aux | grep [m]bd)
	num=$(ps aux | grep [m]bd | wc -l)

	if [ $num -gt 0 ]
	then
		echo "$dname already has $num instance(s) running."
		echo "$running"
		echo "Exiting..."
		exit 1
	fi

	for file in "$dir"/*
	do
		NUMTESTS=$((NUMTESTS + 1))
		if [ $SHOULDRUN -eq 1 ]
		then
			pcheck "$file"	
		else
			ncheck "$file"
		fi
	done
}

dump_results() {
	finished="$1"
	PASSED=$((NUMTESTS - FAILED))

	echo
	echo

	if [ $finished -eq 1 ]
	then
		echo "-------------------------TEST RUN FINISHED-----------------------"
	else
		echo "--------------------------TEST RUN FAILED------------------------"
	fi
		
	echo
	echo
	echo "positive tests (daemon should run using these configurations)"
	echo "-----------------------------------------------------------------"
	echo "$PRESULTS" | tr '#' '\n'
	echo
	echo
	echo "negative tests (daemon should refuse to run and terminate)"
	echo "-----------------------------------------------------------------"
	echo "$NRESULTS" | tr '#' '\n'
	echo
	echo
	echo "Ran $NUMTESTS test(s)"
	echo
	echo "$FAILED failed"
	echo "$PASSED passed"
	echo
}

runtests "$pconf"
SHOULDRUN=0
runtests "$nconf"

dump_results 1
