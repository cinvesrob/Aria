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
#include "ArMTXIO.h"

/** @example mtxPowerControl.cpp  Shows how to turn power outputs on and off on
 * MTX.
 * The 'mtx' Linux kernel module must be loaded, and the /dev/mtx character
 * device must have been created (see mtxPowerControlDriver documentation).  You must
 * have read/write access to /dev/mtx.
 */

void testPower(ArMTXIO& io, const char* name, int bank, int bit, const char *conndesc)
{
  io.lock();
  ArLog::log(ArLog::Terse, "mtxPowerControl: Turning on %s (bank %d bit %d) for 3 seconds. This is at connector %s", name, bank, bit, conndesc);
  io.setPowerOutput(bank, bit, true);
  io.unlock();
  ArUtil::sleep(3000);
  io.lock();
  ArLog::log(ArLog::Terse, "mtxPowerControl: Turning off %s.", name);
  io.setPowerOutput(bank, bit, false);
  io.unlock();
  ArUtil::sleep(3000);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);
  parser.loadDefaultArguments();
  ArRobot robot;

  ArRobotConnector robotConnector(&parser, &robot);
  if(!robotConnector.connectRobot())
  {
    ArLog::log(ArLog::Terse, "mtxPowerControl: Could not connect to the robot.");
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
  ArLog::log(ArLog::Normal, "mtxPowerControl: Connected.");
  robot.comInt(ArCommands::JOYINFO, 0);

  ArMTXIO io;

  if(!io.isEnabled())
  {
    ArLog::log(ArLog::Terse, "mtxPowerControl: Error opening MTX IO device interface!");
    Aria::exit(4);
  }

  robot.runAsync(true);

  // Note that bank and bits are 0-indexed, so 1 is subtracted here to effect
  // that.
  testPower(io, "Aux_5V_Out", 3-1, 1-1, "AUX POWER pin #4");
  testPower(io, "Aux_12V_Out", 3-1, 2-1, "AUX POWER pin #5");
  testPower(io, "Aux_20V_Out", 3-1, 3-1, "AUX POWER pin #6");
  testPower(io, "Aux_24V_Raw_1", 2-1, 5-1, "USER POWER pin #7");
  testPower(io, "Aux_24V_Raw_2", 2-1, 6-1, "USER POWER pin #8");
  testPower(io, "Aux_24V_Raw_3_4", 2-1, 7-1, "USER POWER pins 9, 10 (no estop), 11, 12 with estop)");
  

  // You can control power to robot components as well. See manual for more
  // info.
  // LRF power is bank 1 bit 3
  // Wheel lights is bank 1 bit 6
  // Sonar 1 power is bank 1 bit 7
  // Sonar 2 power is bank 1 bit 8
  // Audio amp. is bank 3 bit 5
  // USB 1+2 is Bank 2 bit 1 (USB12_5V_SW on AUX SENSORS pin 9, and on USB connectors 1 and 2)
  // USB 3 is Bank 2 bit 2 (USB3_5V_SW on AUX SENSORS pin 14, and on USB connector 3)
  // Aux laser 1 power is bank 2 bit 3 (vert) (Vertical_Laser_Power, AUX SENSORS pins 5 and 10)
  // Aux laser 2 power is bank 2 bit 4 (foot) (Foot_Laser_Power, AUX SENSORS pin 15



  ArLog::log(ArLog::Terse, "mtxPowerControl: Exit");
  Aria::exit(0);

}
