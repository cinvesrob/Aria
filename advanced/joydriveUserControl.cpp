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
  std::string str;
  int ret;
  time_t lastTime;
  int trans, rot;
  
  ArJoyHandler joyHandler;
  ArSerialConnection con;
  ArRobot robot(NULL, false);

  joyHandler.init();
  joyHandler.setSpeeds(100, 700);

  if (joyHandler.haveJoystick())
  {
    printf("Have a joystick\n\n");
  }
  else
  {
    printf("Do not have a joystick, set up the joystick then rerun the program\n\n");
//    exit(0);    
  }

  if ((ret = con.open()) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    exit(0);
  }

  
  robot.setDeviceConnection(&con);
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    exit(0);
  }

  robot.comInt(ArCommands::SONAR, 0);
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::SOUNDTOG, 0);
  //robot.comInt(ArCommands::ENCODER, 1);

  //robot.comStrN(ArCommands::SAY, "\1\6\2\105", 4);
  
  lastTime = time(NULL);
  while (1)
  {
    if (!robot.isConnected())
    {
      printf("No longer connected to robot, exiting.\n");
      exit(0);
    }
    robot.loopOnce();
    if (lastTime != time(NULL))
    {
      printf("\rx %6.1f  y %6.1f  tth  %6.1f vel %7.1f mpacs %3d", robot.getX(),
	     robot.getY(), robot.getTh(), robot.getVel(), 
	     robot.getMotorPacCount());
      fflush(stdout);
      lastTime = time(NULL);
    }
    if (joyHandler.haveJoystick() && (joyHandler.getButton(1) ||
				      joyHandler.getButton(2)))
    {
      if (ArMath::fabs(robot.getVel()) < 10.0)
	robot.comInt(ArCommands::ENABLE, 1);
      joyHandler.getAdjusted(&rot, &trans);
      robot.comInt(ArCommands::VEL, trans);
      robot.comInt(ArCommands::RVEL, -rot);
    }
    else
    {
      robot.comInt(ArCommands::VEL, 0);
      robot.comInt(ArCommands::RVEL, 0);
    }
    ArUtil::sleep(100);
  }

  
}

