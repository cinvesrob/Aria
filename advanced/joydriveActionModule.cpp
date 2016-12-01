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


void printStatus(ArModuleLoader::Status status)
{
  if (status == ArModuleLoader::STATUS_ALREADY_LOADED)
    printf("Module already loaded\n");
  else if (status == ArModuleLoader::STATUS_FAILED_OPEN)
    printf("Failed to find or open the simpleMod module\n");
  else if (status == ArModuleLoader::STATUS_INVALID)
    printf("Invalid file\n");
  else if (status == ArModuleLoader::STATUS_INIT_FAILED)
    printf("Module Init failed\n");
  else if (status == ArModuleLoader::STATUS_SUCCESS)
    printf("Module succedded\n");
  else if (status == ArModuleLoader::STATUS_EXIT_FAILED)
    printf("Module exit sequence failed\n");
}

int main()
{
  ArModuleLoader::Status status;
  ArSerialConnection con;
  ArRobot robot;
  int ret;
  std::string str;

  Aria::init();

  status=ArModuleLoader::load("./joydriveActionMod", &robot);
  printStatus(status);

  if (status == ArModuleLoader::STATUS_INIT_FAILED)
    return(1);

  if ((ret = con.open()) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    Aria::shutdown();
    return 1;
  }
  
  robot.setDeviceConnection(&con);
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }

  robot.comInt(ArCommands::SONAR, 0);
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::SOUNDTOG, 0);

  robot.run(true);
  
  status=ArModuleLoader::close("./joydriveActionMod");
  printStatus(status);

  Aria::shutdown();
  return 0;
}
