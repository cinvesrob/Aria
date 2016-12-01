
import com.mobilerobots.Aria.*;
import com.mobilerobots.ArNetworking.*;

class RequestCallback extends ArFunctor_ServerData
{
  public void invoke(ArServerClient client, ArNetPacket packet)
  {
    ArNetPacket sending = new ArNetPacket();
    System.out.println("customServerExample: RequestCallback: responding to a request packet with ID " + packet.getCommand());
    client.sendPacketTcp(sending); // just send back an empty packet.
  }
}

public class customServerExample {


  /* This loads all the ArNetworking classes (they will be in the global
   * namespace) when this class is loaded: */
  static {
    try {
        System.loadLibrary("AriaJava");
        System.loadLibrary("ArNetworkingJava");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code libraries (AriaJava and ArNetworkingJava .so or .DLL) failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }


  public static void main(String[] argv)
  {
    Aria.init();
    ArServerBase server = new ArServerBase();
    ArNetPacket packet = new ArNetPacket(); // empty packet to test broadcasting

    RequestCallback testCB = new RequestCallback();

    server.addData("test", "some wierd test", testCB, "none", "none");
    server.addData("test2", "another wierd test", testCB, "none", "none");
    server.addData("test3", "yet another wierd test", testCB, "none", "none");
    if (!server.open(7273))
    {
      System.err.println("customServerExample: Could not open server port 7273");
      System.exit(1);
    }
    server.runAsync();
    System.out.println("customServerExample: ready for customClientExamples to connect on port 7273; will broadcast \"test3\" packet every 4 seconds as well.");
    while (server.getRunningWithLock())
    {
      ArUtil.sleep(4000);
      server.broadcastPacketTcp(packet, "test3");
    }
    Aria.shutdown();
  }
}

