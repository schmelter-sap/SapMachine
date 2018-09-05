import java.io.IOException;

import jdk.test.lib.JDKToolFinder;
import jdk.test.lib.process.ProcessTools;

class DebuggerConf {
    public final String connector;
    public final String address;
    public final boolean isListener;

    private DebuggerConf(String connector, String address, boolean isListener) {
        this.connector = connector;
        this.address = address;
        this.isListener = isListener;
    }

    public static DebuggerConf createSocketListener(int port) {
        String con = "com.sun.jdi.SocketListen:port=" + port;
        return new DebuggerConf(con, "localhost:" + port, true);
    }

    public static DebuggerConf createSocketListener(String host) {
        String con = "com.sun.jdi.SocketListen:localAddress=" + host;
        return new DebuggerConf(con, "localhost", true);
    }

    public static DebuggerConf createSocketListener(String host, int port) {
        String con = "com.sun.jdi.SocketListen:localAddress=" + host + 
                     ",port=" + port;
        return new DebuggerConf(con, host + ":" + port, true);
    }
}

public abstract class DoDScaffold extends TestScaffold {

    public DoDScaffold(DebuggerConf conf) {
        super(new String[] {"-connect", conf.connector});
        delayedStart = true;
    }

    @Override
    protected abstract void runTests() throws Exception;
 
    protected VMConnection getCurrentConnection() {
        return null;
    }

    protected Process getCurrentDebuggee() {
        return null;
    }

    public static void startDebuggingViaJcmd(long pid, String address, 
            boolean server, int timeout) throws IOException {
        ProcessBuilder pb = new ProcessBuilder(
                JDKToolFinder.getJDKTool("jcmd"),
                Long.toString(pid),
                "DoD.start",
                "address=" + address,
                "is_server=" + server,
                "timeout=" + timeout).inheritIO();
        System.out.println("Starting jcmd via " + pb.command());

        try {
            pb.start().waitFor();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static void getDoDInfo(long pid) throws IOException {
        ProcessBuilder pb = new ProcessBuilder(
                JDKToolFinder.getJDKTool("jcmd"),
                Long.toString(pid),
                "DoD.info").inheritIO();
        System.out.println("Starting jcmd via " + pb.command());

        try {
            pb.start().waitFor();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
