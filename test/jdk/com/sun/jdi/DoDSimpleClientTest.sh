#!/bin/sh

#  @test
#  @bug 4422141 4695338
#  @summary Simple Client Test
#  @author Ralf Schmelter
#
#  @key intermittent
#  @run shell DoDSimpleClientTest.sh

#set -x
source "$(dirname $0)"/DoDScaffolding.sh

classname=DoDSimpleClientTest

createJavaFile() {
    cat <<EOF > $classname.java.1
class $classname {
    public static void main(String[] args) throws Exception {
        System.out.println("Hello Room!");
        boolean w = true;
        while(w) {
           Thread.sleep(1000); // @1 breakpoint
        }
        System.out.println("Hello World!");
    }
}
EOF
}

dojdbCmds() {
    setBkpts @1
    runToBkpt @1
    cmd set w = false
    cmd allowExit cont
}

dojcmdCmds() {
    dod_runjcmd DoD.start address=localhost:$address is_server=false timeout=10000
    dod_runjcmd DoD.info
}

compileOptions=-g
dod_setup
docompile

dod_startJdb "-connect com.sun.jdi.SocketListen:port=0,timeout=100000"
dod_startDebuggee "-agentlib:jdwp=ondemand=y,transport=dt_socket,suspend=n"
sleep 2
dod_startJcmd

wait $debuggeepid

# dod_runjcmd $debuggeepid DoD.start address=$address timeout=100000 is_server=true
# waitForFinish

pass
