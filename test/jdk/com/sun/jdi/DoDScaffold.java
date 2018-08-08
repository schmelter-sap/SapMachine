class DebuggerConf {
    public final String connector;
    public final String address;
    public final boolean isListener;

    private DebuggerConf(String connector, String address, boolean isListener) {
        this.connector = connector;
        this.address = address;
        this.isListener = isListener;
    }

    public static DebuggerConf createSocketListener(String address) {
        return new DebuggerConf("com.sun.jdi.SocketListen", address, true);
    }
}

public class DoDScaffold extends TestScaffold {

    public DoDScaffold(String[] args) {
        super(args);
    }

    @Override
    protected void runTests() throws Exception {
        // TODO Auto-generated method stub
    }

    public void setDebuggerConf(DebuggerConf conf) {
        // TODO: Sets system props accodring to spec.
    }
 
    protected VMConnection getCurrentConnection() {
        return null;
    }

    protected Process getCurrentDebuggee() {
        return null;
    }
}
