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
    }

    @Override
    protected abstract void runTests() throws Exception;
 
    protected VMConnection getCurrentConnection() {
        return null;
    }

    protected Process getCurrentDebuggee() {
        return null;
    }
}
