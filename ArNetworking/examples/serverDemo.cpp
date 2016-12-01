#include "Aria.h"
#include "ArNetworking.h"

/** @example serverDemo.cpp Example ArNetworking server providing teleoperation,
 * sonar data, control the camera, etc.
 *
 * This is a basic ArNetworking server. It connects to a robot or simulator,
 * including, if available, IRs, gyro, and bumpers.  Give the option 
 * "-connectLaser" on the command line to enable the laser rangefinder,
 * if available.
 *
 * Run "./serverDemo -help" for a full list of command line options.
 *
 * Once running, connect to this server with a a client such as 
 * MobileEyes.
 *
 * This server provides the following services:
 *  - User login (optional)
 *  - Basic robot telemetry information 
 *  - Range sensor data values (not used by MobileEyes)
 *  - Graphics representing range sensor reading positions
 *  - Teleoperation modes (including safe/unsafe drive modes)
 *  - Wander mode
 *  - Various advanced "custom" commands to control logging, debugging, etc.
 *  - If an ACTS or SAV server is running, forward the video stream 
 *  - Camera control (pan/tilt/zoom) if cameras are available
 *
 * Note that this program requires a terminal to run -- i.e. you can't run
 * it in the background in Linux.  To modify it to allow that, remove the key
 * handler code in main().
 */

int main(int argc, char **argv)
{
  // mandatory init
  Aria::init();

  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  // set up our parser
  ArArgumentParser parser(&argc, argv);

  // load the default arguments 
  parser.loadDefaultArguments();

  // robot
  ArRobot robot;
  // set up our simple connector
  ArRobotConnector robotConnector(&parser, &robot);


  // add a gyro, it'll see if it should attach to the robot or not
  ArAnalogGyro gyro(&robot);


  // set up the robot for connecting
  if (!robotConnector.connectRobot())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::exit(1);
  }

  ArDataLogger dataLogger(&robot, "dataLog.txt");
  dataLogger.addToConfig(Aria::getConfig());
  
  // our base server object
  ArServerBase server;

  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);
  ArServerSimpleOpener simpleOpener(&parser);


  ArClientSwitchManager clientSwitchManager(&server, &parser);

  // parse the command line... fail and print the help if the parsing fails
  // or if the help was requested
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    Aria::logOptions();
    Aria::exit(1);
  }

  // Set up where we'll look for files such as user/password 
  char fileDir[1024];
  ArUtil::addDirectories(fileDir, sizeof(fileDir), Aria::getDirectory(), 
			 "ArNetworking/examples");

  // first open the server up
  if (!simpleOpener.open(&server, fileDir, 240))
  {
    if (simpleOpener.wasUserFileBad())
      printf("Bad user/password/permissions file\n");
    else
      printf("Could not open server port\n");
    exit(1);
  }

  // Range devices:
 
 
  ArSonarDevice sonarDev;
  robot.addRangeDevice(&sonarDev);

  ArIRs irs;
  robot.addRangeDevice(&irs);

  ArBumpers bumpers;
  robot.addRangeDevice(&bumpers);

  // attach services to the server
  ArServerInfoRobot serverInfoRobot(&server, &robot);
  ArServerInfoSensor serverInfoSensor(&server, &robot);
  ArServerInfoDrawings drawings(&server);

  // modes for controlling robot movement
  ArServerModeStop modeStop(&server, &robot);
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);  
  ArServerModeWander modeWander(&server, &robot);
  modeStop.addAsDefaultMode();
  modeStop.activate();

  // set up the simple commands
  ArServerHandlerCommands commands(&server);
  ArServerSimpleComUC uCCommands(&commands, &robot);  // send commands directly to microcontroller
  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot); // control debug logging
  ArServerSimpleComGyro gyroCommands(&commands, &robot, &gyro); // configure gyro
  ArServerSimpleComLogRobotConfig configCommands(&commands, &robot); // control more debug logging
  ArServerSimpleServerCommands serverCommands(&commands, &server); // control ArNetworking debug logging
  ArServerSimpleLogRobotDebugPackets logRobotDebugPackets(&commands, &robot, ".");  // debugging tool

  // ArServerModeDrive is an older drive mode. ArServerModeRatioDrive is newer and generally performs better,
  // but you can use this for old clients if neccesary.
  //ArServerModeDrive modeDrive(&server, &robot);
  //modeDrive.addControlCommands(&commands); // configure the drive modes (e.g. enable/disable safe drive)

  ArServerHandlerConfig serverHandlerConfig(&server, Aria::getConfig()); // make a config handler
  ArLog::addToConfig(Aria::getConfig()); // let people configure logging

  modeRatioDrive.addToConfig(Aria::getConfig(), "Teleop settings"); // able to configure teleop settings
  modeRatioDrive.addControlCommands(&commands);

  // Forward video if either ACTS or SAV server are running.
  // You can find out more about SAV and ACTS on our website
  // http://robots.activmedia.com. ACTS is for color tracking and is
  // a separate product. SAV just does software A/V transmitting and is
  // free to all our customers. Just run ACTS or SAV server before you
  // start this program and this class here will forward video from the
  // server to the client.
  ArHybridForwarderVideo videoForwarder(&server, "localhost", 7070);
  
  // Control a pan/tilt/zoom camera, if one is installed, and the video
  // forwarder was enabled above.
  ArPTZ *camera = NULL;
  ArServerHandlerCamera *handlerCamera = NULL;
  ArCameraCollection *cameraCollection = NULL;
  if (videoForwarder.isForwardingVideo())
  {
    bool invertedCamera = false;
    camera = new ArVCC4(&robot,	invertedCamera, 
			ArVCC4::COMM_UNKNOWN, true, true);
    camera->init();

    cameraCollection = new ArCameraCollection();
    cameraCollection->addCamera("Cam1", "VCC4", "Camera", "VCC4");
    handlerCamera = new ArServerHandlerCamera("Cam1", 
		                              &server, 
					      &robot,
					      camera, 
					      cameraCollection);
  }

  // You can use this class to send a set of arbitrary strings 
  // for MobileEyes to display, this is just a small example
  ArServerInfoStrings stringInfo(&server);
  Aria::getInfoGroup()->addAddStringCallback(stringInfo.getAddStringFunctor());
  Aria::getInfoGroup()->addStringInt(
	  "Motor Packet Count", 10, 
	  new ArConstRetFunctorC<int, ArRobot>(&robot, 
					       &ArRobot::getMotorPacCount));
  /*
  Aria::getInfoGroup()->addStringInt(
	  "Laser Packet Count", 10, 
	  new ArRetFunctorC<int, ArSick>(&sick, 
					 &ArSick::getSickPacCount));
  */
  
  // start the robot running, true means that if we lose connection the run thread stops
  robot.runAsync(true);


  // connect the laser(s) if it was requested
  if (!laserConnector.connectLasers())
  {
    printf("Could not connect to lasers... exiting\n");
    Aria::exit(2);
  }
  

  drawings.addRobotsRangeDevices(&robot);

  // log whatever we wanted to before the runAsync
  simpleOpener.checkAndLog();
  // now let it spin off in its own thread
  server.runAsync();

  printf("Server is now running...\n");

  // Add a key handler so that you can exit by pressing
  // escape. Note that a key handler prevents you from running
  // a program in the background on Linux, since it expects an 
  // active terminal to read keys from; remove this if you want
  // to run it in the background.
  ArKeyHandler *keyHandler;
  if ((keyHandler = Aria::getKeyHandler()) == NULL)
  {
    keyHandler = new ArKeyHandler;
    Aria::setKeyHandler(keyHandler);
    robot.lock();
    robot.attachKeyHandler(keyHandler);
    robot.unlock();
    printf("To exit, press escape.\n");
  }

  // Read in parameter files.
  std::string configFile = "serverDemoConfig.txt";
  Aria::getConfig()->setBaseDirectory("./");
  if (Aria::getConfig()->parseFile(configFile.c_str(), true, true))
  {
    ArLog::log(ArLog::Normal, "Loaded config file %s", configFile.c_str());
  }
  else
  {
    if (ArUtil::findFile(configFile.c_str()))
    {
      ArLog::log(ArLog::Normal, 
		 "Trouble loading configuration file %s, continuing",
		 configFile.c_str());
    }
    else
    {
      ArLog::log(ArLog::Normal, 
		 "No configuration file %s, will try to create if config used",
		 configFile.c_str());
    }
  }

  clientSwitchManager.runAsync();

  robot.lock();
  robot.enableMotors();
  robot.unlock();

  robot.waitForRunExit();
  Aria::exit(0);
}


