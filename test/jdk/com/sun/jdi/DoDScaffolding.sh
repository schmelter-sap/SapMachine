#!/bin/bash

source "$(dirname $0)"/ShellScaffold.sh

mydojdbCmds()
{
   # Wait for jdb to start before we start sending cmds
   if [ $(echo $debuggeeJdwpOpts | grep -c suspend=y) = 1 ]; then 
       waitForJdbMsg ']' 1
   else
       waitForJdbMsg '(> )|(VM Started: )' 1
   fi

   # Send commands from the test
   dojdbCmds
   # Finish jdb with quit command
   dofinish "quit"
}

dod_setup() {
    setup
    address=
    jcmdOutputFile="$tmpFileDir/jcmdOutput.txt"
    rm -f "$jcmdOutputFile"
}

dod_runjcmd() {
    echo "running jcmd: jcmd $realDebuggeePid $*"

    echo "running $jdk/bin/jcmd $realDebuggeePid $*" >> "$jcmdOutputFile"
    sh -c "$jdk/bin/jcmd $realDebuggeePid $* 2>&1 | tee -a $jcmdOutputFile"
}

dod_startJdb() {
    startJdb
}

dod_startDebuggee() {
    startDebuggee
	sleep 2
}

dod_startJcmd() {
    debuggeeCmd=`$jdk/bin/jps -v | $grep $debuggeeKeyword`
    realDebuggeePid=`echo "$debuggeeCmd" | sed -e 's@ .*@@'`

    if [ ! -z "$realDebuggeePid" ] ; then
        dojcmdCmds
	fi
}
