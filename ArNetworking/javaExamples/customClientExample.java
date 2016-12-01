
import com.mobilerobots.Aria.*;
import com.mobilerobots.ArNetworking.*;

class ResponseCallback extends ArFunctor_NetPacket
{
  public void invoke(ArNetPacket packet)
  {
    System.out.println("customClientExample: ResponseCallback: Got a packet from the server with type " + packet.getCommand());
  }
}

public class customClientExample {


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

  public static void main(String argv[])
  {
    Aria.init();

    ArClientBase client = new ArClientBase();
    ResponseCallback testCB = new ResponseCallback();

    ArTime startTime = new ArTime();
    startTime.setToNow();
    System.out.println("customClientExample: trying to connect to a server running on the local host at port 7273...");
    if (!client.blockingConnect("localhost", 7273))
    {
      System.err.println("Error: Could not connect to server on localhost:7273, exiting.");
      System.exit(1);
    }    

    System.out.println("customClientExample: Connected after " + startTime.mSecSince() + " msec.");
    
    client.runAsync();
    
    System.out.println("\ncustomClientExample: Adding data handler callbacks...");
    client.addHandler("test", testCB);
    client.addHandler("test2", testCB);
    client.addHandler("test3", testCB);
    client.logDataList();
    
    System.out.println("\ncustomClientExample: Requesting \"test\" once...");
    client.requestOnce("test");

    System.out.println("\ncustomClientExample: Requesting \"test2\" with a frequency of 10ms...");
    client.request("test2", 100);

    System.out.println("\ncustomClientExample: Requesting \"test3\" with a frequency of -1 (updates may happen at any time the server sends them) and waiting 5 sec...");
    client.request("test3", -1);
    ArUtil.sleep(5000);

    System.out.println("\ncustomClientExample: Changing request frequency of \"test2\" to 300ms and waiting 5 sec");
    client.request("test2", 300);
    ArUtil.sleep(5000);

    System.out.println("\ncustomClientExample: Cancelling \"test2\" request.");
    client.requestStop("test2");
    
    System.out.println("\ncustomClientExample: waiting 10 seconds, then disconnecting...");
    ArUtil.sleep(10000);
    client.disconnect();
    ArUtil.sleep(50);
  }
}
