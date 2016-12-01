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

ArRobot *robot;

void printer(void)
{
  printf("vel %5.0f rot vel %5.0f\n", robot->getVel(), robot->getRotVel());
}


int main(int argc, char **argv)
{
  double speed = 1000;
  double squareSide = 2000;

  // whether to use the sim for the laser or not, if you use the sim
  // for hte laser, you have to use the sim for the robot too
  // robot
  robot = new ArRobot;
  // the laser
  ArSick sick;
  // set up our simpleConnector
  ArSimpleConnector simpleConnector(&argc, argv);

  // set up a key handler so escape exits and attach to the robot
  ArKeyHandler keyHandler;
  robot->attachKeyHandler(&keyHandler);

  // parse its arguments
  if (simpleConnector.parseArgs())
  {
    simpleConnector.logOptions();
    keyHandler.restore();
    exit(1);
  }

  // if there are more arguments left then it means we didn't
  // understand an option
  /*
  if (argc > 1)
  {    
    simpleConnector.logOptions();
    keyHandler.restore();
    exit(1);
  }
  */

  // set a default filename
  //std::string filename = "c:\\log\\1scans.2d";
  std::string filename = "1scans.2d";
  // see if we want to use a different filename
  //if (argc > 1)
  //Lfilename = argv[1];
  printf("Logging to file %s\n", filename.c_str());
  // start the logger with good values
  
  //sick.configureShort(useSim, ArSick::BAUD38400, 	 ArSick::DEGREES180, ArSick::INCREMENT_HALF);
  ArSickLogger logger(robot, &sick, 300, 25, filename.c_str());
  
  // mandatory init
  Aria::init();

  // add it to the robot
  robot->addRangeDevice(&sick);

  //ArAnalogGyro gyro(robot);


  // set up the robot for connecting
  if (!simpleConnector.connectRobot(robot))
  {
    printf("Could not connect to robot->.. exiting\n");
    Aria::shutdown();
    return 1;
  }

  robot->setRotVelMax(300);
  robot->setRotAccel(300);
  robot->setRotDecel(300);

  robot->setAbsoluteMaxTransVel(2000);
  robot->setTransVelMax(2000);
  robot->setTransAccel(500);
  robot->setTransDecel(500);
  /*
  robot->comInt(82, 30); // rotkp
  robot->comInt(83, 200); // rotkv
  robot->comInt(84, 0); // rotki
  robot->comInt(85, 15); // transkp
  robot->comInt(86, 450); // transkv
  robot->comInt(87, 4); // transki
  */
  robot->comInt(82, 30); // rotkp
  robot->comInt(83, 200); // rotkv
  robot->comInt(84, 0); // rotki
  robot->comInt(85, 30); // transkp
  robot->comInt(86, 450); // transkv
  robot->comInt(87, 4); // transki
  // run the robot, true here so that the run will exit if connection lost
  robot->runAsync(true);



  // set up the laser before handing it to the laser mode
  simpleConnector.setupLaser(&sick);

  // now that we're connected to the robot, connect to the laser
  sick.runAsync();

 if (!sick.blockingConnect())
  {
    printf("Could not connect to SICK laser... exiting\n");
    robot->disconnect();
    Aria::shutdown();
    return 1;
  }

  robot->lock();
  robot->addUserTask("printer", 50, new ArGlobalFunctor(&printer));
  robot->unlock();

#ifdef WIN32
  // wait until someone pushes the motor button to go
  while (1)
  {
    robot->lock();
    if (!robot->isRunning())
      exit(0);
    if (robot->areMotorsEnabled())
    {
      robot->unlock();
      break;
    }
    robot->unlock();
    ArUtil::sleep(100);
  }
#endif

  // basically from here on down the robot just cruises around a bit

  printf("Starting moving\n");
  robot->lock();
  // enable the motors, disable amigobot sounds
  robot->comInt(ArCommands::ENABLE, 1);
  robot->setHeading(0);
  robot->setVel(1000);
  robot->unlock();

  ArUtil::sleep(speed / 500.0 * 1000.0);
  printf("Should be up to speed, moving on first side\n");
  ArUtil::sleep(squareSide / speed * 1000);
  printf("Turning to second side\n");
  robot->lock();
  robot->setHeading(90);
  robot->setVel(speed);
  robot->unlock();
  ArUtil::sleep(squareSide / speed * 1000);
  printf("Turning to third side\n");
  robot->lock();
  robot->setHeading(180);
  robot->setVel(speed);
  robot->unlock();
  ArUtil::sleep(squareSide / speed * 1000);
  printf("Turning to last side\n");
  robot->lock();
  robot->setHeading(-90);
  robot->setVel(speed);
  robot->unlock();
  ArUtil::sleep(squareSide / speed * 1000);
  printf("Pointing back original direction and stopping\n");
  robot->lock();
  robot->setHeading(0);
  robot->setVel(0);
  robot->unlock();
  ArUtil::sleep(300);

  printf("Stopped\n");
  sick.lockDevice();
  sick.disconnect();
  sick.unlockDevice();
  robot->lock();
  robot->disconnect();
  robot->unlock();

  // now exit
  Aria::shutdown();
  return 0;
}

