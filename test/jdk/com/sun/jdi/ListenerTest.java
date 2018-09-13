/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/**
 * @test
 * @bug 8205608
 * @summary Test that getting the stack trace for a very large stack does
 * not take too long
 *
 * @author Ralf Schmelter
 *
 * @library /test/lib
 * @run build TestScaffold VMConnection TargetListener TargetAdapter
 * @run compile -g ListenerTest.java
 * @run driver ListenerTest
 */

// "@run main/othervm/timeout=3600 -agentlib:jdwp=transport=dt_socket,address=localhost:8000,server=y,suspend=y ListenerTest"

import com.sun.jdi.*;
import com.sun.jdi.event.*;
import com.sun.jdi.request.*;

import jdk.test.lib.process.ProcessTools;

import java.io.IOException;
import java.util.*;
import java.util.stream.*;


    /********** target program **********/

class ListenerTestTArg {

    public static void main(String[] args) throws InterruptedException {
        for (int i = 0; i < 1000; ++i) {
            System.out.println("Step nr. " + i);
            sleepSome();
        }
    }

    public static void sleepSome() throws InterruptedException {
        Thread.sleep(100);
    }
}

    /********** test program **********/

public class ListenerTest extends DoDScaffold 
                          implements VMConnection.ListenerCallback {

    private VMConnection connection;

    public ListenerTest(String args[]) {
        super(DebuggerConf.createSocketListener("localhost", 0));
    }

    public static void main(String[] args) throws Exception {
        new ListenerTest(args).startTests();
    }

    @Override
    public Process startedListening(String address) throws IOException {
        System.out.println("Debugger listening at " + address);

//        ProcessBuilder pb = ProcessTools.createJavaProcessBuilder(
//                "-agentlib:jdwp=transport=dt_socket,ondemand=n," +
//                "server=n,address=" + address + 
//                ",onthrow=java.lang.OutOfMemoryError,launch=true," + 
//                "logflags=0xffff,logfile=C:\\priv\\jdwp.log",
//                ListenerTestTArg.class.getName()).inheritIO();
        ProcessBuilder pb = ProcessTools.createJavaProcessBuilder(
                "-agentlib:jdwp=transport=dt_socket,ondemand=y," + 
                "logflags=0xffff,logfile=jdwp.log",
                ListenerTestTArg.class.getName()).inheritIO();
        Process debuggee = pb.start();
        sleep(2000);
        startDebuggingViaJcmd(debuggee.pid(), address, false, 20000);
        sleep(2000);
        getDoDInfo(debuggee.pid());
        return debuggee;
    }

    @Override
    protected void vmConnectionCreated(VMConnection connection) {
        this.connection = connection;
        System.out.println("Setting callback for listener ...");
        connection.setListenerCallback(this);
    }

    /********** test core **********/

    protected void runTests() throws Exception {
        BreakpointEvent bpe = startTo(ListenerTestTArg.class.getName(), 
                                      "sleepSome", "()V");
        System.out.println("Current stack: ");
        System.out.println(bpe.thread().frames());
        System.out.println("Ending process ...");
        connection.clearProcess().destroy();

        listenUntilVMDisconnect();

        if (!testFailed) {
            println("ListenerTest: passed");
        } else {
            throw new Exception("ListenerTest: failed");
        }
    }
}
