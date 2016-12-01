/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2014 Adept Technology

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


/** @example demo.cpp General purpose testing and demo program, using ArMode
 *    classes to provide keyboard control of various robot functions.
 *
 *  demo uses ArMode subclasses from ARIA. These modes
 *  provide keyboard control of various aspects and accessories of 
 *  the robot, and can be re-used in your programs if you wish.  
 *  The ArMode classes are defined in %ArModes.cpp.
 *
 *  "demo" is a useful program for testing out the operation of the robot
 *  for diagnostic or demonstration purposes.  Other example programs
 *  focus on individual areas.
 */

bool handleDebugMessage(ArRobotPacket *pkt)
{
  if(pkt->getID() != ArCommands::MARCDEBUG) return false;
  char msg[256];
  pkt->bufToStr(msg, sizeof(msg));
  msg[255] = 0;
  ArLog::log(ArLog::Terse, "Controller Firmware Debug: %s", msg);
  return true;
}

int main(int argc, char** argv)
{
  // Initialize some global data
  Aria::init();

  // If you want ArLog to print "Verbose" level messages uncomment this:
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  // This object parses program options from the command line
  ArArgumentParser parser(&argc, argv);

  // Load some default values for command line arguments from /etc/Aria.args
  // (Linux) or the ARIAARGS environment variable.
  parser.loadDefaultArguments();

  // Central object that is an interface to the robot and its integrated
  // devices, and which manages control of the robot by the rest of the program.
  ArRobot robot;

  // Object that connects to the robot or simulator using program options
  ArRobotConnector robotConnector(&parser, &robot);

  // If the robot has an Analog Gyro, this object will activate it, and 
  // if the robot does not automatically use the gyro to correct heading,
  // this object reads data from it and corrects the pose in ArRobot
  ArAnalogGyro gyro(&robot);

  robot.addPacketHandler(new ArGlobalRetFunctor1<bool, ArRobotPacket*>(&handleDebugMessage));

  // Connect to the robot, get some initial data from it such as type and name,
  // and then load parameter files for this robot.
  if (!robotConnector.connectRobot())
  {
    // Error connecting:
    // if the user gave the -help argumentp, then just print out what happened,
    // and continue so options can be displayed later.
    if (!parser.checkHelpAndWarnUnparsed())
    {
      ArLog::log(ArLog::Terse, "Could not connect to robot, will not have parameter file so options displayed later may not include everything");
    }
    // otherwise abort
    else
    {
      ArLog::log(ArLog::Terse, "Error, could not connect to robot.");
      Aria::logOptions();
      Aria::exit(1);
    }
  }

  if(!robot.isConnected())
  {
    ArLog::log(ArLog::Terse, "Internal error: robot connector succeeded but ArRobot::isConnected() is false!");
  }

  // Connector for laser rangefinders
  ArLaserConnector laserConnector(&parser, &robot, &robotConnector);

  // Connector for compasses
  ArCompassConnector compassConnector(&parser);

  // Parse the command line options. Fail and print the help message if the parsing fails
  // or if the help was requested with the -help option
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    Aria::logOptions();
    Aria::exit(1);
    return 1;
  }

  // Used to access and process sonar range data
  ArSonarDevice sonarDev;
  
  // Used to perform actions when keyboard keys are pressed
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);

  // ArRobot contains an exit action for the Escape key. It also 
  // stores a pointer to the keyhandler so that other parts of the program can
  // use the same keyhandler.
  robot.attachKeyHandler(&keyHandler);
  printf("You may press escape to exit\n");

  // Attach sonarDev to the robot so it gets data from it.
  robot.addRangeDevice(&sonarDev);

  
  // Start the robot task loop running in a new background thread. The 'true' argument means if it loses
  // connection the task loop stops and the thread exits.
  robot.runAsync(true);

  // Connect to the laser(s) if lasers were configured in this robot's parameter
  // file or on the command line, and run laser processing thread if applicable
  // for that laser class.  For the purposes of this demo, add all
  // possible lasers to ArRobot's list rather than just the ones that were
  // connected by this call so when you enter laser mode, you
  // can then interactively choose which laser to use from that list of all
  // lasers mentioned in robot parameters and on command line. Normally,
  // only connected lasers are put in ArRobot's list.
  if (!laserConnector.connectLasers(
        false,  // continue after connection failures
        false,  // add only connected lasers to ArRobot
        true    // add all lasers to ArRobot
  ))
  {
     printf("Warning: Could not connect to laser(s). Set LaserAutoConnect to false in this robot's individual parameter file to disable laser connection.\n");
  }

/* not needed, robot connector will do it by default
  if (!sonarConnector.connectSonars(
        false,  // continue after connection failures
        false,  // add only connected lasers to ArRobot
        true    // add all lasers to ArRobot
  ))
  {
    printf("Could not connect to sonars... exiting\n");
    Aria::exit(2);
  }
*/

  // Create and connect to the compass if the robot has one.
  ArTCM2 *compass = compassConnector.create(&robot);
  if(compass && !compass->blockingConnect()) {
    compass = NULL;
  }
  
  // Sleep for a second so some messages from the initial responses
  // from robots and cameras and such can catch up
  ArUtil::sleep(1000);

  // We need to lock the robot since we'll be setting up these modes
  // while the robot task loop thread is already running, and they 
  // need to access some shared data in ArRobot.
  robot.lock();

  // now add all the modes for this demo
  // these classes are defined in ArModes.cpp in ARIA's source code.
  
  if(robot.getOrigRobotConfig()->getHasGripper())
    new ArModeGripper(&robot, "gripper", 'g', 'G');
  else
    ArLog::log(ArLog::Normal, "Robot does not indicate that it has a gripper.");
  ArModeActs actsMode(&robot, "acts", 'a', 'A');
  ArModeTCM2 tcm2(&robot, "tcm2", 'm', 'M', compass);
  ArModeIO io(&robot, "io", 'i', 'I');
  ArModeRobotStatus stat(&robot, "detailed status/error flags", 'f', 'F');
  ArModeConfig cfg(&robot, "report robot config", 'o' , 'O');
  ArModeCommand command(&robot, "command", 'd', 'D');
  ArModeCamera camera(&robot, "camera", 'c', 'C');
  ArModePosition position(&robot, "position", 'p', 'P', &gyro);
  ArModeSonar sonar(&robot, "sonar", 's', 'S');
  ArModeBumps bumps(&robot, "bumps", 'b', 'B');
  ArModeLaser laser(&robot, "laser", 'l', 'L');
  ArModeWander wander(&robot, "wander", 'w', 'W');
  ArModeUnguardedTeleop unguardedTeleop(&robot, "unguarded teleop", 'u', 'U');
  ArModeTeleop teleop(&robot, "teleop", 't', 'T');


  // activate the default mode
  teleop.activate();

  // turn on the motors
  robot.comInt(ArCommands::ENABLE, 1);

  robot.unlock();
  
  // Block execution of the main thread here and wait for the robot's task loop
  // thread to exit (e.g. by robot disconnecting, escape key pressed, or OS
  // signal)
  robot.waitForRunExit();

  Aria::exit(0);
  return 0;

}

