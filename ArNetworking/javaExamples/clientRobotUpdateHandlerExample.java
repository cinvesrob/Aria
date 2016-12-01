

/* This example shows how to receive updates from an ArNetworking server (e.g.
 * arnlServer) providing robot data (position estimate, velocity), server mode string and
 * status string.
 */

import com.mobilerobots.Aria.*;
import com.mobilerobots.ArNetworking.*;


public class clientRobotUpdateHandlerExample {


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

    if (!client.blockingConnect("localhost", 7272)) // change hostname or ip address here to connect to a remote host
    {
      System.err.println("Error: Could not connect to server on localhost, exiting.");
      System.exit(1);
    }    

    
    client.runAsync();
    
    ArClientHandlerRobotUpdate updates =  new ArClientHandlerRobotUpdate(client);
    updates.requestUpdates();

    // Example server request:
    // client.requestOnce("wander");

    while(client.isConnected())
    {
      updates.lock();
      System.out.println(
        "Mode: " + updates.getMode() + " " +
        "Status: " + updates.getStatus() + " " +
        "Pos: ("+
        updates.getX()+", "+
        updates.getY()+", "+
        updates.getTh()+")"
      );
      updates.unlock();
      ArUtil.sleep(2000);
    }

    Aria.shutdown();
  }
}
