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
#include "Aria.h"
#include "ArNetworking.h"


int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  // robot
  ArRobot robot;
  /// our server
  ArServerBase server;

  // set up our parser
  ArArgumentParser parser(&argc, argv);
  // set up our simple connector
  ArSimpleConnector simpleConnector(&parser);

  // set up a gyro
  ArAnalogGyro gyro(&robot);

  // load the default arguments 
  parser.loadDefaultArguments();

  // parse the command line... fail and print the help if the parsing fails
  // or if the help was requested
  if (!simpleConnector.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    simpleConnector.logOptions();
    exit(1);
  }

  if (!server.loadUserInfo("userServerTest.userInfo"))
  {
    printf("Could not load user info, exiting\n");
    exit(1);
  }

  server.logUsers();

  // first open the server up
  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  // sonar, must be added to the robot
  ArSonarDevice sonarDev;
  // add the sonar to the robot
  robot.addRangeDevice(&sonarDev);

  ArIRs irs;
  robot.addRangeDevice(&irs);

  ArBumpers bumpers;
  robot.addRangeDevice(&bumpers);

  // a laser in case one is used
  ArSick sick(361, 180);
  // add the laser to the robot
  robot.addRangeDevice(&sick);  


  

  // attach stuff to the server
  ArServerInfoRobot serverInfoRobot(&server, &robot);
  ArServerInfoSensor serverInfoSensor(&server, &robot);
  ArServerInfoDrawings drawings(&server);
  drawings.addRobotsRangeDevices(&robot);

  // ways of moving the robot
  ArServerModeStop modeStop(&server, &robot);
  ArServerModeDrive modeDrive(&server, &robot);
  ArServerModeRatioDrive modeRatioDrive(&server, &robot);
  ArServerModeWander modeWander(&server, &robot);
  modeStop.addAsDefaultMode();
  modeStop.activate();

  // set up the simple commands
  ArServerHandlerCommands commands(&server);
  // add the commands for the microcontroller
  ArServerSimpleComUC uCCommands(&commands, &robot);
  // add the commands for logging
  ArServerSimpleComMovementLogging loggingCommands(&commands, &robot);
  // add the commands for the gyro
  ArServerSimpleComGyro gyroCommands(&commands, &robot, &gyro);

  // add the commands to enable and disable safe driving to the simple commands
  modeDrive.addControlCommands(&commands);

  // Forward any video if we have some to forward.. this will forward
  // from SAV or ACTS, you can find both on our website
  // http://robots.activmedia.com, ACTS is for color tracking and is
  // charged for but SAV just does software A/V transmitting and is
  // free to all our customers... just run ACTS or SAV before you
  // start this program and this class here will forward video from it
  // to MobileEyes
  ArHybridForwarderVideo videoForwarder(&server, "localhost", 7070);
  
  // make a camera to use in case we have video
  ArPTZ *camera = NULL;
  ArServerHandlerCamera *handlerCamera = NULL;
  // if we have video then set up a camera 
  if (videoForwarder.isForwardingVideo())
  {
    bool invertedCamera = false;
    camera = new ArVCC4(&robot,	invertedCamera, 
			ArVCC4::COMM_UNKNOWN, true, true);
    camera->init();
    handlerCamera = new ArServerHandlerCamera(&server, &robot, camera);
  }

  server.logCommandGroups();
  server.logCommandGroupsToFile("userServerTest.commandGroups");

  // now let it spin off in its own thread
  server.runAsync();

  // set up the robot for connecting
  if (!simpleConnector.connectRobot(&robot))
  {
    printf("Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }

  // set up the laser before handing it to the laser mode
  simpleConnector.setupLaser(&sick);

  robot.enableMotors();
  // start the robot running, true so that if we lose connection the run stops
  robot.runAsync(true);

  sick.runAsync();

  // connect the laser if it was requested
  if (!simpleConnector.connectLaser(&sick))
  {
    printf("Could not connect to laser... exiting\n");
    Aria::shutdown();
    return 1;
  }

  robot.waitForRunExit();
  // now exit
  Aria::shutdown();
  exit(0);  
}


