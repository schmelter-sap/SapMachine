#!/bin/sh

#  @test
#  @bug 4422141 4695338
#  @summary Simple Test
#  @author Ralf Schmelter
#
#  @key intermittent
#  @run shell DoDSimpleServerTest.sh

#set -x
source "$(dirname $0)"/DoDScaffolding.sh

classname=DoDSimpleServerTest

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
    dod_runjcmd DoD.start address=localhost:0 is_server=true timeout=100000
	dod_runjcmd DoD.info
    dod_getOnDemandPort
}

compileOptions=-g
dod_setup
docompile

dod_startDebuggee "-agentlib:jdwp=ondemand=y,transport=dt_socket,suspend=n"
sleep 2
dod_startJcmd
dod_startJdb "-connect com.sun.jdi.SocketAttach:port=$address,timeout=100000"

wait $debuggeepid

pass
