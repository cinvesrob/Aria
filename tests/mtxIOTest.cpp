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
#include "ArMTXIO.h"


int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);
  parser.loadDefaultArguments();
  ArRobot robot;

  ArRobotConnector robotConnector(&parser, &robot);
  if(!robotConnector.connectRobot())
  {
    ArLog::log(ArLog::Terse, "mtxIOTest: Could not connect to the robot.");
    if(parser.checkHelpAndWarnUnparsed())
    {
        // -help not given
        Aria::logOptions();
        Aria::exit(1);
    }
  }

  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    Aria::exit(1);
  }
  ArLog::log(ArLog::Normal, "mtxIOTest: Connected.");
  robot.comInt(ArCommands::JOYINFO, 0);

  ArMTXIO mtxIO;

  if(!mtxIO.isEnabled())
  {
    ArLog::log(ArLog::Terse, "mtxIOTest: Error opening MTX IO device interface!");
    Aria::exit(4);
  }

  robot.runAsync(true);

  ArMTXIO mtxIO2;
  if(!mtxIO2.isEnabled())
  {
    ArLog::log(ArLog::Terse, "mtxIOTest: Error opening a second MTX IO interface");
    Aria::exit(5);
  }

  unsigned char out = 1;
  while(true)
  {
    mtxIO.lock();
    unsigned char cur;
    // get current state
    if(!mtxIO.getDigitalOutputControl1(&cur))
    {
      ArLog::log(ArLog::Terse, "mtxIOTest: Error getting current state of output control 1");
      mtxIO.unlock();
      Aria::exit(2);
    }

    // set new state
    cur = out;
    if(!mtxIO.setDigitalOutputControl1(&cur))
    {
      ArLog::log(ArLog::Terse, "mtxIOTest: Error setting state of output control 1");
      mtxIO.unlock();
      Aria::exit(3);
    }
  

    mtxIO2.lock();
    if(!mtxIO2.setDigitalOutputControl2(&cur))
    {
      ArLog::log(ArLog::Terse, "mtxIOTest: Error setting state of output control 2 through second interface object");
      mtxIO.unlock();
      Aria::exit(6);
    }
    mtxIO2.unlock();
      
    mtxIO.unlock();
    ArUtil::sleep(500);
 
    // shift 
    out = out << 1;
    if(out == 0) out = 1;
  }
    
    
  Aria::exit(0);

}
