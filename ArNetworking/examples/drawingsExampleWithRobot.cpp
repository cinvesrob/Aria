#include "Aria.h"
#include "ArNetworking.h"
#include <math.h>

/** @example drawingsExampleWithRobot.cpp Example showing how to draw custom
 * graphics in clients such as MobileEyes, and enable drawing of sensor
 * readings as well.
 *
 * This is an example server that shows how to draw arbitrary figures in a
 * client (e.g. MobileEyes). It also uses the convenient built-in
 * support for visualization built in to ArRobot, ArSick, ArSonarDevice,
 * etc.  
 *
 * @sa drawingsExample.cpp which does not include robot and sensor drawings
 */

/* These are callbacks that respond to client requests for the drawings' 
 * geometry data. */
void exampleHomeDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);
void exampleDotsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);
void exampleXDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);
void exampleArrowsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt);

int main(int argc, char **argv)
{
  Aria::init();
  ArRobot robot;
  ArServerBase server;

  ArArgumentParser parser(&argc, argv);
  ArSimpleConnector simpleConnector(&parser);
  ArServerSimpleOpener simpleOpener(&parser);


  // parse the command line... fail and print the help if the parsing fails
  // or if help was requested
  parser.loadDefaultArguments();
  if (!simpleConnector.parseArgs() || !simpleOpener.parseArgs() || 
      !parser.checkHelpAndWarnUnparsed())
  {    
    simpleConnector.logOptions();
    simpleOpener.logOptions();
    exit(1);
  }

  // Set up where we'll look for files such as config file, user/password file,
  // etc.
  char fileDir[1024];
  ArUtil::addDirectories(fileDir, sizeof(fileDir), Aria::getDirectory(), 
			 "ArNetworking/examples");

  // first open the server up
  if (!simpleOpener.open(&server, fileDir, 240))
  {
    if (simpleOpener.wasUserFileBad())
      printf("Error: Bad user/password/permissions file.\n");
    else
      printf("Error: Could not open server port. Use -help to see options.\n");
    exit(1);
  }


  // Devices
  ArAnalogGyro gyro(&robot);

  ArSonarDevice sonarDev;
  robot.addRangeDevice(&sonarDev);

  ArIRs irs;
  robot.addRangeDevice(&irs);

  ArBumpers bumpers;
  robot.addRangeDevice(&bumpers);

  ArSick sick(361, 180);
  robot.addRangeDevice(&sick);  
  

  ArServerInfoRobot serverInfoRobot(&server, &robot);
  ArServerInfoSensor serverInfoSensor(&server, &robot);

  // This is the service that provides drawing data to the client.
  ArServerInfoDrawings drawings(&server);

  // Convenience function that sets up drawings for all the robot's current
  // range devices (using default shape and color info)
  drawings.addRobotsRangeDevices(&robot);

  // Add our custom drawings
  drawings.addDrawing(
      //                shape:      color:               size:   layer:
      new ArDrawingData("polyLine", ArColor(255, 0, 0),  2,      49),
      "exampleDrawing_Home", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleHomeDrawingNetCallback)
  );
  drawings.addDrawing(                                    
      new ArDrawingData("polyDots", ArColor(0, 255, 0), 250, 48),
      "exampleDrawing_Dots", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleDotsDrawingNetCallback)
  );
  drawings.addDrawing(
      new ArDrawingData("polySegments", ArColor(0, 0, 0), 4, 52),
      "exampleDrawing_XMarksTheSpot", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleXDrawingNetCallback)
  );
  drawings.addDrawing(
      new ArDrawingData("polyArrows", ArColor(255, 0, 255), 500, 100),
      "exampleDrawing_Arrows", 
      new ArGlobalFunctor2<ArServerClient*, ArNetPacket*>(&exampleArrowsDrawingNetCallback)
  );

  // modes for moving the robot
  ArServerModeStop modeStop(&server, &robot);
  ArServerModeDrive modeDrive(&server, &robot);
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);
  ArServerModeWander modeWander(&server, &robot);
  modeStop.addAsDefaultMode();
  modeStop.activate();

  

  // Connect to the robot.
  if (!simpleConnector.connectRobot(&robot))
  {
    printf("Error: Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }

  // set up the laser before handing it to the laser mode
  simpleConnector.setupLaser(&sick);

  robot.enableMotors();

  // start the robot cycle running in a background thread
  robot.runAsync(true);

  // start the laser processing cycle in a background thread
  sick.runAsync();

  // connect the laser if it was requested
  if (!simpleConnector.connectLaser(&sick))
  {
    printf("Error: Could not connect to laser... exiting\n");
    Aria::shutdown();
    return 1;
  }

  // log whatever we wanted to before the runAsync
  simpleOpener.checkAndLog();

  // run the server thread in the background
  server.runAsync();

  printf("Server is now running...\n");


  // Add a key handler mostly that windows can exit by pressing
  // escape, note that the key handler prevents you from running this program
  // in the background on Linux.
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

  robot.waitForRunExit();
 

  Aria::shutdown();
  exit(0);  
}




// Network callbacks for drawings' current geometry data:

void exampleHomeDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  ArNetPacket reply;

  // 7 Vertices
  reply.byte4ToBuf(7);

  // Centered on 0,0.
  // X:                    Y:
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(500);   // Vertex 1
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(-500);  // Vertex 2
  reply.byte4ToBuf(500);   reply.byte4ToBuf(-500);  // Vertex 3
  reply.byte4ToBuf(500);   reply.byte4ToBuf(500);   // Vertex 4
  reply.byte4ToBuf(0);     reply.byte4ToBuf(1000);  // Vertex 5
  reply.byte4ToBuf(-500);  reply.byte4ToBuf(500);   // Vertex 6
  reply.byte4ToBuf(500);   reply.byte4ToBuf(500);   // Vertex 7

  client->sendPacketUdp(&reply);
}

void exampleDotsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  ArNetPacket reply;

  unsigned int tik = ArUtil::getTime() % 200;
  double t = tik / 5.0;

  // Three dots
  reply.byte4ToBuf(3);

  // Dot 1:
  reply.byte4ToBuf(3000);  // X coordinate (mm)
  reply.byte4ToBuf((int) (sin(t) * 1000));// Y

  // Dot 2:
  reply.byte4ToBuf(3500);  // X
  reply.byte4ToBuf((int) (sin(t+500) * 1000));// Y

  // Dot 3:
  reply.byte4ToBuf(4000);  // X
  reply.byte4ToBuf((int) (sin(t+1000) * 1000));// Y

  client->sendPacketUdp(&reply);
}

void exampleXDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  ArNetPacket reply;

  // X marks the spot. 2 line segments, so 4 vertices:
  reply.byte4ToBuf(4);

  // Segment 1:
  reply.byte4ToBuf(-4250); // X1
  reply.byte4ToBuf(250);   // Y1
  reply.byte4ToBuf(-3750); // X2
  reply.byte4ToBuf(-250);  // Y2

  // Segment 2:
  reply.byte4ToBuf(-4250); // X1
  reply.byte4ToBuf(-250);  // Y1
  reply.byte4ToBuf(-3750); // X2
  reply.byte4ToBuf(250);   // Y2
  
  client->sendPacketUdp(&reply);
}

void exampleArrowsDrawingNetCallback(ArServerClient* client, ArNetPacket* requestPkt) {
  // 1 Arrow that points at the robot
  ArNetPacket reply;
  reply.byte4ToBuf(1);
  reply.byte4ToBuf(0);      // Pos. X
  reply.byte4ToBuf(700);   // Pos. Y
  client->sendPacketUdp(&reply);
}

