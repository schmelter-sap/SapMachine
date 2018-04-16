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

createJavaFile()
{
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

dojdbCmds()
{
   setBkpts @1
   runToBkpt @1
   cmd set w = true
   cmd allowExit cont
}

compileOptions=-g
dod_setup
docompile

jdbConnectionOptions="-connect com.sun.jdi.SocketListen:port=0,timeout=100000"
startJdb

debuggeeJdwpOpts=-agentlib:jdwp=ondemand=n,transport=dt_socket,address=localhost:$address,server=n,suspend=n
startDebuggee

wait $debuggeepid

# dod_runjcmd $debuggeepid DoD.start address=$address timeout=100000 is_server=true
# waitForFinish

pass

