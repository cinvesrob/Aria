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

/* Tests SIM_SET_POSE command to move the robot in the simulator. */



#include "Aria.h"

void simSetPose(ArRobot *robot, const ArPose& pose)
{
  ArRobotPacket pkt;
  pkt.setID(ArCommands::SIM_SET_POSE);
  pkt.uByteToBuf(0);
  pkt.byte4ToBuf((ArTypes::Byte4)pose.getX());
  pkt.byte4ToBuf((ArTypes::Byte4)pose.getY());
  pkt.byte4ToBuf((ArTypes::Byte4)pose.getTh());
  pkt.finalizePacket();
  printf("-> SIM_SET_POSE %d %d %d\n", (ArTypes::Byte4)pose.getX(), (ArTypes::Byte4)pose.getY(), (ArTypes::Byte4)pose.getTh());
  robot->getDeviceConnection()->write(pkt.getBuf(), pkt.getLength());
  printf("Command sent.\n\n");
}

int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);
  ArSimpleConnector connector(&parser);
  ArRobot robot;

  if (!connector.parseArgs())
  {
    connector.logOptions();
    return 1;
  }

  if (!connector.connectRobot(&robot))
  {
    printf("Could not connect to robot... exiting\n");
    return 2;
  }
  printf("Connected to robot.\n");

  robot.runAsync(true);

  simSetPose(&robot, ArPose(1000, 1000, 90));
  ArUtil::sleep(500);

  simSetPose(&robot, ArPose(1000, -2000, -90));
  ArUtil::sleep(500);

  simSetPose(&robot, ArPose(0, -2000, 180));
  ArUtil::sleep(500);

  simSetPose(&robot, ArPose(8000, 1, 0));
  ArUtil::sleep(500);

  simSetPose(&robot, ArPose(-8000, -1000, 45));
  ArUtil::sleep(500);

  printf("** Done with tests. Sleeping for 3 seconds and then stopping ArRobot thread... **\n");
  ArUtil::sleep(3000);
  robot.stopRunning();
  printf("** Exiting. **\n");
  return 0;
}

