import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class FakeJdb {

    public static void main(String[] args) throws Exception {
        if (args.length != 3) {
            System.out.println("Syntax: FakeJDB server|client <port> <action>");
            System.out.println("The action can be: no-handshake");
            System.out.println("                   wrong-hand");
            System.out.println("                   wrong-packet");
            System.exit(1);
        }

        Socket s;

        int port = Integer.parseInt(args[1]);
        byte[] handshake = "JDWP-Handshake".getBytes(StandardCharsets.UTF_8);

        if ("server".equals(args[0])) {
            ServerSocket ss = new ServerSocket(port);
            System.out.println("FakeJDB ist listening of local port " +
                    ss.getLocalPort());
            s = ss.accept();
            ss.close();
        } else {
            s = new Socket("localhost", port);
        }

        InputStream is = s.getInputStream();
        OutputStream os = s.getOutputStream();

        switch (args[2]) {
            case "no-handshake":
                break;

            case "wrong-hand":
                os.write(new byte[handshake.length]);
                break;

            case "wrong-packet":
                os.write(handshake.length);
                is.readNBytes(handshake, 0, handshake.length);
                os.write(new byte[13]);
        }

        while (s.isConnected()) {
            Thread.sleep(100);
        }
    }
}
