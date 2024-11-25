/**
 * @test
 * @summary Runs the test for the jcm agent integration.
 *
 * @run main/othervm -Dcp=agent-1.0.1-SNAPSHOT-sap-tests.jar TestJmcAgentIntegration
 */

import java.lang.reflect.Method;
import java.io.File;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;

public class TestJmcAgentIntegration {

    public static void main(String[] args) throws Exception {
        File testSrc = new File(System.getProperty("test.src"));
        ArrayList<Path> jarFiles = new ArrayList<>();

        try (DirectoryStream<Path> dirStream = Files.newDirectoryStream(
            testSrc.toPath(), "agent-*-sap-tests.jar")) {
            dirStream.forEach(path -> jarFiles.add(path));
        }

        if (jarFiles.size() != 1) {
            throw new RuntimeException("Found " + jarFiles.size() + " test jare files");
        }

        File file = jarFiles.get(0).toFile();
        URL url = file.toURI().toURL();
        String classPath = System.getProperty("java.class.path", ".");

        System.setProperty("java.class.path", classPath + System.getProperty("path.separator") + file.toString());
        System.setProperty("useJmcAgentOption", "true");
        System.setProperty("traceExecs", "true");

        URLClassLoader cl = new URLClassLoader(new URL[] {url}, TestJmcAgentIntegration.class.getClassLoader());
        Class<?> testClass = Class.forName("org.openjdk.jmc.agent.sap.test.TestRunner", true, cl);
        Method mainMethod = testClass.getDeclaredMethod("main", String[].class);
        mainMethod.invoke(null, new Object[] {new String[0]});
    }
}
