import java.io.IOException;
import java.util.Arrays;
import java.util.List;

import jdk.test.lib.JDKToolFinder;
import jdk.test.lib.process.OutputBuffer;
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

    public static DebuggerConf createSocketAttacher(String host) {
        String con = "com.sun.jdi.SocketAttach:hostname=" + host;
        return new DebuggerConf(con, host + ":0", true);
    }

    public static DebuggerConf createSocketAttacher(int port) {
        String con = "com.sun.jdi.SocketAttach:port=" + port;
        return new DebuggerConf(con, "localhost:" + port, true);
    }

    public static DebuggerConf createSocketAttacher(String host, int port) {
        String con = "com.sun.jdi.SocketAttach:hostname=" + host +
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

    public Process startedListening(String address) throws IOException {
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

    public static DoDInfo getDoDInfo(long pid) throws IOException {
        DoDInfo info = new DoDInfo();

        try {
            ProcessBuilder pb = new ProcessBuilder(
                    JDKToolFinder.getJDKTool("jcmd"),
                    Long.toString(pid),
                    "DoD.info");
            System.out.println("Starting jcmd via " + pb.command());

            List<String> lines = ProcessTools.executeCommand(pb).asLines();
            String addressLine = "The address is ";

            for (String line: lines) {
                if (line.startsWith(addressLine)) {
                    info.currentAddress = line.substring(addressLine.length());
                }
            }
        }
        catch (Exception e) {
            throw new IOException("getting DoDInfo failed", e);
        }

        return info;
    }

    public static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static class DoDInfo {
        public String currentAddress;

        public String getHost() {
            if (currentAddress == null) {
                return "localhost";
            }

            int i = currentAddress.indexOf(':');

            if (i < 0) {
                return currentAddress;
            }

            return currentAddress.substring(0, i);
        }

        public int getPort() {
            if (currentAddress == null) {
                return 0;
            }

            int i = currentAddress.indexOf(':');

            if (i < 0) {
                return 0;
            }

            return Integer.parseInt(currentAddress.substring(i + 1));
        }
    }
}
