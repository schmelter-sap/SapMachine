#!/bin/bash

source "$(dirname $0)"/ShellScaffold.sh

mydojdbCmds()
{
   # Wait for jdb to start before we start sending cmds
   if [ $(echo $debuggeeJdwpOpts | grep -c suspend=y) = 1 ]; then 
       waitForJdbMsg ']' 1
   else
#       waitForJdbMsg '>' 1
       waitForJdbMsg 'VM Started: ' 1
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
    echo "running jcmd: jcmd $*"

    echo "running $jdk/bin/jcmd $*" >> "jcmdOutputFile"
    sh -c "$jdk/bin/jcmd $* | tee $jcmdOutputFile"
}
