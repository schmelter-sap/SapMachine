public class InfLoop {

    public static void main(String[] args) throws InterruptedException {
        long l = 0;

        while (true) {
            System.out.println(l + "...");
            Thread.sleep(1000);
            l += 1;
        }

    }
}
