
import com.mobilerobots.Aria.*;
import com.mobilerobots.ArNetworking.*;

public class simple {

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

  /* Main program: */
  public static void main(String argv[]) {
    System.out.println("Starting Java ArNetworking Test");

    /* Global Aria class inititalizaton */
    Aria.init(Aria.SigHandleMethod.SIGHANDLE_THREAD, true);

    /* Robot and device objects */
    ArRobot robot = new ArRobot("robot1", true, true, true);
    ArSonarDevice sonar = new ArSonarDevice();
    robot.addRangeDevice(sonar);

    /* Make connector and parse arguments from argv */
    ArSimpleConnector conn = new ArSimpleConnector(argv);
    if(!Aria.parseArgs())
    {
      Aria.logOptions();
      Aria.shutdown();
      System.exit(1); 
    }

    /* Connect to the robot */
    System.out.println("Connecting to robot...");
    if (!conn.connectRobot(robot))
    {
      System.err.println("Could not connect to robot, exiting.\n");
      System.exit(1);
    }

    /* Open the sever */
    ArServerBase server = new ArServerBase();
    if(!server.open(7272))
    {
      System.err.println("Could not open server on port 7272, exiting.");
      System.exit(1);
    }
    System.out.println("Opened server on port 7272.");

    /* Informational services: */
    ArServerInfoRobot servInfoRobot = new ArServerInfoRobot(server, robot);
    ArServerInfoSensor servInfoSensor = new ArServerInfoSensor(server, robot);
    ArServerInfoDrawings servInfoDraw = new ArServerInfoDrawings(server);
    servInfoDraw.addRobotsRangeDevices(robot);

    /* Control mode services: */
    ArServerModeStop servModeStop = new ArServerModeStop(server, robot);
    ArServerModeRatioDrive servModeDrive = new ArServerModeRatioDrive(server, robot);
    ArServerModeWander servModeWander = new ArServerModeWander(server, robot);
    servModeStop.addAsDefaultMode();
    servModeStop.activate();

    /* Simple text command  service ("custom commands" in MobileEyes): */
    ArServerHandlerCommands commands = new ArServerHandlerCommands(server);
    
    // Relay microcontroller commands directly:
    ArServerSimpleComUC cmdUC = new ArServerSimpleComUC(commands, robot);

    // Log information:
    ArServerSimpleComMovementLogging cmdLog = new ArServerSimpleComMovementLogging(commands, robot);
    ArServerSimpleComLogRobotConfig cmdConfigLog = new ArServerSimpleComLogRobotConfig(commands, robot);


    /* Run robot and server threads in the backrgound: */
    System.out.println("Running...");
    robot.enableMotors();
    robot.runAsync(true);
    server.runAsync();
    robot.waitForRunExit();
  }
}
