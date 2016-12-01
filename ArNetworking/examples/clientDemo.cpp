/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2015 Adept Technology, Inc.
Copyright (C) 2016 Omron Adept Technologies, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
/* This is the ArNetworking example client. 
 * It connects to an ArNetworking server, and provides continuous 
 * information about the robot state (position, speed, server mode, etc.),
 * and, if the server supports the commands, lets you drive the robot with
 * the keyboard.  
 *
 * To see the example client in action, first run a server on the robot's 
 * onboard computer, for example, serverDemo, testServer, guiServer (from ARNL),
 * or ARAM. Make sure the robot computer is on a network, and run this
 * clientDemo with the hostname of the robot computer as an argument:
 *
 *    ./clientDemo -host myrobot
 *
 */

#include "Aria.h"
#include "ArNetworking.h"
#include "ArClientRatioDrive.h"


#define VEL_AMOUNT 50  // percentage of server's configured maximum speed

/* This class receives key events via callbacks from ArKeyHandler, and in 
   response sets values for an ArClientRatioDrive object to send. 
 */
class InputHandler
{
public: 
  /** 
   * @param client  Our client networking object 
   * @param keyHandler  Key handler to register command callbacks with
   */
  InputHandler(ArClientBase *client, ArKeyHandler *keyHandler);
  virtual ~InputHandler(void);

protected:

  /// Up arrow key handler: drive the robot forward
  void up(void);

  /// Down arrow key handler: drive the robot backward
  void down(void);

  /// Left arrow key handler: turn the robot left 
  void left(void);

  /// Right arrow key handler: turn the robot right
  void right(void);

  /// Move the robot laterally right  (q key)
  void lateralLeft(void);

  /// Move the robot laterally right  (e key)
  void lateralRight(void);

  /// Send drive request to the server with stored values
  void sendInput(void);

public:
  /// Send a request to enable "safe drive" mode on the server
  void safeDrive();

protected:
  /// Send a request to disable "safe drive" mode on the server
  void unsafeDrive();

  /// Request stop
  void space();

  void listData();


  void logTrackingTerse();
  void logTrackingVerbose();
  void resetTracking();
  void toggleDebug();

  ArClientBase *myClient;
  ArKeyHandler *myKeyHandler;

  /// Set this to true in the constructor to print out debugging information
  bool myPrinting;

  /// Object that continuously sends driving requests in the background. 
  ArClientRatioDrive myDriveClient;

  /** Functor objects, given to the key handler, which then call our handler
   * methods above */
  ///@{
  ArFunctorC<InputHandler> myUpCB;
  ArFunctorC<InputHandler> myDownCB;
  ArFunctorC<InputHandler> myLeftCB;
  ArFunctorC<InputHandler> myRightCB;
  ArFunctorC<InputHandler> myLateralLeftCB;
  ArFunctorC<InputHandler> myLateralRightCB;
  ArFunctorC<InputHandler> mySafeDriveCB;
  ArFunctorC<InputHandler> myUnsafeDriveCB;
  ArFunctorC<InputHandler> myListDataCB;
  ArFunctorC<InputHandler> myLogTrackingTerseCB;
  ArFunctorC<InputHandler> myLogTrackingVerboseCB;
  ArFunctorC<InputHandler> myResetTrackingCB;
  ArFunctorC<InputHandler> mySpaceCB;
  ArFunctorC<InputHandler> myToggleDebugCB;
  ///@}
};

InputHandler::InputHandler(ArClientBase *client, ArKeyHandler *keyHandler) : 
  myClient(client), myKeyHandler(keyHandler),
  myPrinting(false), myDriveClient(client),

  /* Initialize functor objects with pointers to our handler methods: */
  myUpCB(this, &InputHandler::up),
  myDownCB(this, &InputHandler::down),
  myLeftCB(this, &InputHandler::left),
  myRightCB(this, &InputHandler::right),
  myLateralLeftCB(this, &InputHandler::lateralLeft),
  myLateralRightCB(this, &InputHandler::lateralRight),
  mySafeDriveCB(this, &InputHandler::safeDrive),
  myUnsafeDriveCB(this, &InputHandler::unsafeDrive),
  myListDataCB(this, &InputHandler::listData),
  myLogTrackingTerseCB(this, &InputHandler::logTrackingTerse),
  myLogTrackingVerboseCB(this, &InputHandler::logTrackingVerbose),
  myResetTrackingCB(this, &InputHandler::resetTracking),
  mySpaceCB(this, &InputHandler::space),
  myToggleDebugCB(this, &InputHandler::toggleDebug)
{

  /* Add our functor objects to the key handler, associated with the appropriate
   * keys: */
  myKeyHandler->addKeyHandler(ArKeyHandler::UP, &myUpCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::DOWN, &myDownCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::LEFT, &myLeftCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::RIGHT, &myRightCB);
  myKeyHandler->addKeyHandler('q', &myLateralLeftCB);
  myKeyHandler->addKeyHandler('e', &myLateralRightCB);
  myKeyHandler->addKeyHandler('s', &mySafeDriveCB);
  myKeyHandler->addKeyHandler('u', &myUnsafeDriveCB);
  myKeyHandler->addKeyHandler('l', &myListDataCB);
  myKeyHandler->addKeyHandler('t', &myLogTrackingTerseCB);
  myKeyHandler->addKeyHandler('v', &myLogTrackingVerboseCB);
  myKeyHandler->addKeyHandler('r', &myResetTrackingCB);
  myKeyHandler->addKeyHandler(ArKeyHandler::SPACE, &mySpaceCB);
  myKeyHandler->addKeyHandler('d', &myToggleDebugCB);
}

InputHandler::~InputHandler(void)
{

}

void InputHandler::space(void)
{
  if(myPrinting)
    puts("Stop");
  myDriveClient.stop();
}

void InputHandler::up(void)
{
  if (myPrinting)
    printf("Forwards\n");
  myDriveClient.setTransVelRatio(VEL_AMOUNT);
}

void InputHandler::down(void)
{
  if (myPrinting)
    printf("Backwards\n");
  myDriveClient.setTransVelRatio(-VEL_AMOUNT);
}

void InputHandler::left(void)
{
  if (myPrinting)
    printf("Left\n");
  myDriveClient.setRotVelRatio(VEL_AMOUNT);
}

void InputHandler::right(void)
{
  if (myPrinting)
    printf("Right\n");
  myDriveClient.setRotVelRatio(-VEL_AMOUNT);
}

void InputHandler::lateralLeft(void)
{
  if (myPrinting)
    printf("Lateral left\n");
  myDriveClient.setLatVelRatio(VEL_AMOUNT);
}

void InputHandler::lateralRight(void)
{
  if (myPrinting)
    printf("Lateral right\n");
  myDriveClient.setLatVelRatio(-VEL_AMOUNT);
}

void InputHandler::safeDrive()
{
  if(myPrinting)
    printf("Sending setSafeDrive 1.\n");
  myDriveClient.safeDrive();
}

void InputHandler::unsafeDrive()
{
  if(myPrinting)
    printf("Sending setSafeDrive 0.\n");
  myDriveClient.unsafeDrive();
}

void InputHandler::toggleDebug()
{
  myPrinting = !myPrinting;
  myDriveClient.setDebugPrint(myPrinting);
  printf("clientDemo debug printing is now %s\n", myPrinting?"on":"off");
}

void InputHandler::listData()
{
  myClient->logDataList();
}

void InputHandler::logTrackingTerse()
{
  myClient->logTracking(true);
}

void InputHandler::logTrackingVerbose()
{
  myClient->logTracking(false);
}

void InputHandler::resetTracking()
{
  myClient->resetTracking();
}


/** This class requests continual data updates from the server and prints them
 * out.
 */
class OutputHandler
{
public:
  OutputHandler(ArClientBase *client);
  virtual ~OutputHandler(void);
  
  /// This callback is called when an update on general robot state arrives
  void handleOutput(ArNetPacket *packet);
  /// This callback is called when an update on general robot state arrives
  void handleOutputNumbers(ArNetPacket *packet);
  /// This callback is called when an update on general robot state arrives
  void handleOutputStrings(ArNetPacket *packet);

  /// This callback is called when an update on the battery configuration changes
  void handleBatteryInfo(ArNetPacket *packet);
  /// This is called when the physical robot information comes back
  void handlePhysicalInfo(ArNetPacket *packet);
  /// This callback is called when an update on the temperature information changes
  void handleTemperatureInfo(ArNetPacket *packet);
  /// Called when the map on the server changes.
  void handleMapUpdated(ArNetPacket *packet);
protected:

  /// The results from the data update are stored in these variables
  //@{
  double myX;
  double myY;
  double myTh;
  double myVel;
  double myRotVel;
  double myLatVel;
  bool myVoltageIsStateOfCharge;
  char myTemperature;
  double myVoltage;
  char myStatus[256];
  char myMode[256];
  //@}
  ArClientBase *myClient;

  /** These functor objects are given to the client to receive updates when they
   * arrive from the server.
   */
  //@{
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleOutputCB;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleOutputNumbersCB;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleOutputStringsCB;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleBatteryInfoCB;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandlePhysicalInfoCB;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleTemperatureInfoCB;
  ArFunctor1C<OutputHandler, ArNetPacket *> myHandleMapUpdatedCB;
  //@}
  
  /// A header for the columns in the data printout is sometimes printed
  bool myNeedToPrintHeader;
  /// Don't print any information until we get the battery info
  bool myGotBatteryInfo;
};

OutputHandler::OutputHandler(ArClientBase *client) :
  myClient(client),
  myHandleOutputCB(this, &OutputHandler::handleOutput),
  myHandleOutputNumbersCB(this, &OutputHandler::handleOutputNumbers),
  myHandleOutputStringsCB(this, &OutputHandler::handleOutputStrings),
  myHandleBatteryInfoCB(this, &OutputHandler::handleBatteryInfo),
  myHandlePhysicalInfoCB(this, &OutputHandler::handlePhysicalInfo),
  myHandleTemperatureInfoCB(this, &OutputHandler::handleTemperatureInfo),
  myHandleMapUpdatedCB(this, &OutputHandler::handleMapUpdated),
  myNeedToPrintHeader(false),
  myGotBatteryInfo(false)
{
  /* Add a handler for battery info, and make a single request for it  */
  myClient->addHandler("physicalInfo", &myHandlePhysicalInfoCB);
  myClient->requestOnce("physicalInfo");


  /* Add a handler for battery info, and make a single request for it  */
  myClient->addHandler("batteryInfo", &myHandleBatteryInfoCB);
  myClient->requestOnce("batteryInfo");

  /* If it exists add a handler for temperature info, and make a
   * single request for it  */
  if (myClient->dataExists("temperatureInfo"))
  {
    myClient->addHandler("temperatureInfo", &myHandleTemperatureInfoCB);
    myClient->requestOnce("temperatureInfo");
  }

  // if we have the new way of broadcasting that only pushes strings
  // when they change then use that
  if (myClient->dataExists("updateNumbers") && 
      myClient->dataExists("updateStrings"))
  {
    printf("Using new updates\n");
    // get the numbers every 100 ms
    myClient->addHandler("updateNumbers", &myHandleOutputNumbersCB);
    myClient->request("updateNumbers", 100);
    // and the strings whenever they change (and are broadcast)
    myClient->addHandler("updateStrings", &myHandleOutputStringsCB);
    myClient->request("updateStrings", -1);
  }
  else
  {
    printf("Using old updates\n");
    // For the old way, just Add a handler for general info, and
    // request it to be called every 100 ms
    myClient->addHandler("update", &myHandleOutputCB);
    myClient->request("update", 100);
  }

  if(myClient->dataExists("mapUpdated"))
  {
    myClient->addHandler("mapUpdated", &myHandleMapUpdatedCB);
    myClient->request("mapUpdated", -1);
  }
}

OutputHandler::~OutputHandler(void)
{
  /* Halt the request for data updates */
  myClient->requestStop("update");
}

void OutputHandler::handleOutput(ArNetPacket *packet)
{
  /* Extract the data from the update packet. Its format is status and
   * mode (null-terminated strings), then 6 doubles for battery voltage, 
   * x position, y position and orientation (theta) (from odometry), current
   * translational velocity, and current rotational velocity. Translation is
   * always milimeters, rotation in degrees.
   */
  memset(myStatus, 0, sizeof(myStatus));
  memset(myMode, 0, sizeof(myMode));
  packet->bufToStr(myStatus, sizeof(myStatus));
  packet->bufToStr(myMode, sizeof(myMode));
  myVoltage = ( (double) packet->bufToByte2() )/10.0;
  myX = (double) packet->bufToByte4();
  myY = (double) packet->bufToByte4();
  myTh = (double) packet->bufToByte2();
  myVel = (double) packet->bufToByte2();
  myRotVel = (double) packet->bufToByte2();
  myLatVel = (double) packet->bufToByte2();
  myTemperature = (double) packet->bufToByte();

  if(myNeedToPrintHeader) 
  {
    printf("\n%6s|%6s|%6s|%4s|%6s|%6s|%4s|%6s|%10s|%20s|\n",
	   "x","y","theta", "vel", "rotVel", "latVel", "temp", myVoltageIsStateOfCharge ? "charge" : "volts", "mode","status");
    fflush(stdout);
    myNeedToPrintHeader = false;
  }
  if (myGotBatteryInfo)
    printf("%6.0f|%6.0f|%6.1f|%4.0f|%6.0f|%6.0f|%4.0d|%6.1f|%10s|%20s|\r",
	   myX, myY, myTh, myVel, myRotVel, myLatVel, myTemperature, myVoltage, myMode, myStatus);
  
  fflush(stdout);
}

void OutputHandler::handleOutputNumbers(ArNetPacket *packet)
{
  /* Extract the data from the updateNumbers packet. Its format is 6
   * doubles for battery voltage, x position, y position and
   * orientation (theta) (from odometry), current translational
   * velocity, and current rotational velocity. Translation is always
   * milimeters, rotation in degrees.
   */
  myVoltage = ( (double) packet->bufToByte2() )/10.0;
  myX = (double) packet->bufToByte4();
  myY = (double) packet->bufToByte4();
  myTh = (double) packet->bufToByte2();
  myVel = (double) packet->bufToByte2();
  myRotVel = (double) packet->bufToByte2();
  myLatVel = (double) packet->bufToByte2();
  myTemperature = (double) packet->bufToByte();

  if(myNeedToPrintHeader) 
  {
    printf("\n%6s|%6s|%6s|%4s|%6s|%6s|%4s|%6s|%10s|%20s|\n",
	   "x","y","theta", "vel", "rotVel", "latVel", "temp", myVoltageIsStateOfCharge ? "charge" : "volts", "mode","status");
    fflush(stdout);
    myNeedToPrintHeader = false;
  }
  if (myGotBatteryInfo)
    printf("%6.0f|%6.0f|%6.1f|%4.0f|%6.0f|%6.0f|%4.0d|%6.1f|%10s|%20s|\r",
	   myX, myY, myTh, myVel, myRotVel, myLatVel, myTemperature, myVoltage, myMode, myStatus);
  
  fflush(stdout);
}

void OutputHandler::handleOutputStrings(ArNetPacket *packet)
{
  /* Extract the data from the updateStrings packet. Its format is
   * status and mode (null-terminated strings).
   */
  memset(myStatus, 0, sizeof(myStatus));
  memset(myMode, 0, sizeof(myMode));
  packet->bufToStr(myStatus, sizeof(myStatus));
  packet->bufToStr(myMode, sizeof(myMode));
}

void OutputHandler::handleBatteryInfo(ArNetPacket *packet)
{
  /* Get battery configuration parameters: when the robot will begin beeping and 
   * warning about low battery, and when it will automatically disconnect and
   * shutdown. */
  double lowBattery = packet->bufToDouble();
  double shutdown = packet->bufToDouble();
  printf("Low battery voltage: %6g       Shutdown battery voltage: %6g\n", lowBattery, shutdown);
  fflush(stdout);
  myNeedToPrintHeader = true;
  myGotBatteryInfo = true;

  if (packet->getDataReadLength() == packet->getDataLength())
  {
    printf("Packet is too small so its an old server, though you could just get to the bufToUByte anyways, since it'd be 0 anyhow\n");
    myVoltageIsStateOfCharge = false;
  }
  else
    myVoltageIsStateOfCharge = (packet->bufToUByte() == 1);

}


void OutputHandler::handlePhysicalInfo(ArNetPacket *packet)
{
  /* Get phyiscal configuration parameters: */
  char robotType[512];
  char robotSubtype[512];
  int width;
  int lengthFront;
  int lengthRear;

  packet->bufToStr(robotType, sizeof(robotType));
  packet->bufToStr(robotSubtype, sizeof(robotSubtype));
  width = packet->bufToByte2();
  lengthFront = packet->bufToByte2();
  lengthRear = packet->bufToByte2();

  printf("Type: %s Subtype: %s Width %d: LengthFront: %d LengthRear: %d\n",
	 robotType, robotSubtype, width, lengthFront, lengthRear);
  fflush(stdout);
}

void OutputHandler::handleTemperatureInfo(ArNetPacket *packet)
{
  char warning = packet->bufToByte();
  char shutdown = packet->bufToByte();
  printf("High temperature warning: %4d       High temperature shutdown: %4d\n", warning, shutdown);
  fflush(stdout);
  myNeedToPrintHeader = true;
}

void OutputHandler::handleMapUpdated(ArNetPacket *packet)
{
  printf("\nMap changed.\n");
  fflush(stdout);
  myNeedToPrintHeader = true;
}


/* Key handler for the escape key: shutdown all of Aria. */
void escape(void)
{
  printf("esc pressed, shutting down aria\n");
  Aria::shutdown();
}

int main(int argc, char **argv)
{
  /* Aria initialization: */
  Aria::init();

  //ArLog::init(ArLog::StdErr, ArLog::Verbose);
 

  /* Create our client object. This is the object which connects with a remote
   * server over the network, and which manages all of our communication with it
   * once connected by sending data "requests".  Requests may be sent once, or
   * may be repeated at any frequency. Requests and replies to requsets contain 
   * payload "packets", into which various data types may be packed (when making a 
   * request), and from which they may also be extracted (when handling a reply). 
   * See the InputHandler and OutputHandler classes above for
   * examples of making requests and reading/writing the data in packets.
   */
  ArClientBase client;

  /* Aria components use this to get options off the command line: */
  ArArgumentParser parser(&argc, argv);

  /* This will be used to connect our client to the server, including
   * various bits of handshaking (e.g. sending a password, retrieving a list
   * of data requests and commands...)
   * It will get the hostname from the -host command line argument: */
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  
  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Connected to server.\n");

  client.setRobotName(client.getHost()); // include server name in log messages

  /* Create a key handler and also tell Aria about it */
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);

  /* Global escape-key handler to shut everythnig down */
  ArGlobalFunctor escapeCB(&escape);
  keyHandler.addKeyHandler(ArKeyHandler::ESCAPE, &escapeCB);

  /* Create the InputHandler object and request safe-drive mode */
  InputHandler inputHandler(&client, &keyHandler);
  inputHandler.safeDrive();

  /* Use ArClientBase::dataExists() to see if the "ratioDrive" request is available on the 
   * currently connected server.  */
  if(!client.dataExists("ratioDrive") )
      printf("Warning: server does not have ratioDrive command, can not use drive commands!\n");
  else
    printf("Keys are:\nUP: Forward\nDOWN: Backward\nLEFT: Turn Left\nRIGHT: Turn Right\n");
  printf("s: Enable safe drive mode (if supported).\nu: Disable safe drive mode (if supported).\nl: list all data requests on server\n\nDrive commands use 'ratioDrive'.\nt: logs the network tracking tersely\nv: logs the network tracking verbosely\nr: resets the network tracking\nd: turn on debugging of this example program\n\n");
  printf("ESC or CTRL-C: exit program\n\n");


  /* Create the OutputHandler object. It will begin printing out data from the
   * server. */
  OutputHandler outputHandler(&client);


  /* Now that we're connected, we can run the client in a background thread, 
   * sending requests and receiving replies. When a reply to a request arrives,
   * or the server makes a request of the client, a handler functor is invoked. 
   * The handlers for this program are registered with the client by the 
   * InputHandler and OutputHandler classes (in their constructors, above) */
  client.runAsync();

  /* While the client is still running (getRunningWithLock locks the "running"
   * flag until it returns), check keys on the key handler (which will call
   * our callbacks), then tell the input handler to send drive commands. 
   * Sleep a fraction of a second as well to avoid using
   * too much CPU time, and give other threads time to work.
   */
  while (client.getRunningWithLock())
  {
    keyHandler.checkKeys();
//    inputHandler.sendInput();
    ArUtil::sleep(100);
  }

  /* The client stopped running, due to disconnection from the server, general
   * Aria shutdown, or some other reason. */
  client.disconnect();
  Aria::exit(0);
}
