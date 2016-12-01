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

int main(int argc, char **argv)
{
  bool done;
  double distToTravel = 2300;

  // whether to use the sim for the laser or not, if you use the sim
  // for hte laser, you have to use the sim for the robot too
  bool useSim = false;
  // the laser
  ArSick sick;
  // connection
  ArDeviceConnection *con;
  // Laser connection
  ArSerialConnection laserCon;
  // robot
  ArRobot robot;

  // set a default filename
  //std::string filename = "c:\\log\\1scans.2d";
  std::string filename = "1scans.2d";
  // see if we want to use a different filename
  //if (argc > 1)
  //Lfilename = argv[1];
  printf("Logging to file %s\n", filename.c_str());
  // start the logger with good values
  sick.configureShort(useSim, ArSick::BAUD38400,
		 ArSick::DEGREES180, ArSick::INCREMENT_HALF);
  ArSickLogger logger(&robot, &sick, 300, 25, filename.c_str());
  
  // mandatory init
  Aria::init();

  // add it to the robot
  robot.addRangeDevice(&sick);

  //ArAnalogGyro gyro(&robot);


  // if we're not using the sim, make a serial connection and set it up
  if (!useSim)
  {
    ArSerialConnection *serCon;
    serCon = new ArSerialConnection;
    serCon->setPort();
    //serCon->setBaud(38400);
    con = serCon;
  }
  // if we are using the sim, set up a tcp connection
  else
  {
    ArTcpConnection *tcpCon;
    tcpCon = new ArTcpConnection;
    tcpCon->setPort();
    con = tcpCon;
  }

  // set the connection on the robot
  robot.setDeviceConnection(con);
  // try to connect, if we fail exit
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }


  // set up a key handler so escape exits and attach to the robot
  ArKeyHandler keyHandler;
  robot.attachKeyHandler(&keyHandler);

  // run the robot, true here so that the run will exit if connection lost
  robot.runAsync(true);



  // if we're not using the sim, set up the port for the laser
  if (!useSim)
  {
    laserCon.setPort(ArUtil::COM3);
    sick.setDeviceConnection(&laserCon);
  }


  // now that we're connected to the robot, connect to the laser
  sick.runAsync();


  if (!sick.blockingConnect())
  {
    printf("Could not connect to SICK laser... exiting\n");
    robot.disconnect();
    Aria::shutdown();
    return 1;
  }

#ifdef WIN32
  // wait until someone pushes the motor button to go
  while (1)
  {
    robot.lock();
    if (!robot.isRunning())
      exit(0);
    if (robot.areMotorsEnabled())
    {
      robot.unlock();
      break;
    }
    robot.unlock();
    ArUtil::sleep(100);
  }
#endif

  // basically from here on down the robot just cruises around a bit

  robot.lock();
  // enable the motors, disable amigobot sounds
  robot.comInt(ArCommands::ENABLE, 1);

  ArTime startTime;
  // move a couple meters
  robot.move(distToTravel);
  robot.unlock();
  startTime.setToNow();
  do {
    ArUtil::sleep(100);
    robot.lock();
    robot.setHeading(0);
    done = robot.isMoveDone(60);
    robot.unlock();
  } while (!done);

  /*
  // rotate a few times
  robot.lock();
  robot.setVel(0);
  robot.setRotVel(60);
  robot.unlock();
  ArUtil::sleep(12000);
  */

  robot.lock();
  robot.setHeading(180);
  robot.unlock();
  do {
    ArUtil::sleep(100);
    robot.lock();
    robot.setHeading(180);
    done = robot.isHeadingDone();
    robot.unlock();
  } while (!done);

  // move a couple meters
  robot.lock();
  robot.move(distToTravel);
  robot.unlock();
  startTime.setToNow();
  do {
    ArUtil::sleep(100);
    robot.lock();
    robot.setHeading(180);
    done = robot.isMoveDone(60);
    robot.unlock();
  } while (!done);

  robot.lock();
  robot.setHeading(0);
  robot.setVel(0);
  robot.unlock();
  startTime.setToNow();
  do {
    ArUtil::sleep(100);
    robot.lock();
    robot.setHeading(0);
    done = robot.isHeadingDone();
    robot.unlock();
  } while (!done);


  sick.lockDevice();
  sick.disconnect();
  sick.unlockDevice();
  robot.lock();
  robot.disconnect();
  robot.unlock();
  // now exit
  Aria::shutdown();
  return 0;
}

