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
    jdbConnectionOptions="$1"
    startJdb
}

dod_startDebuggee() {
    debuggeeJdwpOpts="$1"
    startDebuggee
}

dod_startJcmd() {
    debuggeeCmd=`$jdk/bin/jps -v | $grep $debuggeeKeyword`
    realDebuggeePid=`echo "$debuggeeCmd" | sed -e 's@ .*@@'`

    if [ ! -z "$realDebuggeePid" ] ; then
        dojcmdCmds
	fi
}

dod_getOnDemandPort() {
    cat "$jcmdOutputFile"
    cat "$jcmdOutputFile"
	cat "$jcmdOutputFile" | grep "The address is" | tail -n 1 | cut -d ':' -f 2 | tr -d '\r\n'
	address=$(cat "$jcmdOutputFile" | grep 'The address is' | tail -n 1 | cut -d ':' -f 2 | tr -d '\r\n' )
}
