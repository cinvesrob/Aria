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
#include "ArExport.h"
#include "ArServerInfoRobot.h"
#include "ArServerMode.h"


AREXPORT ArServerInfoRobot::ArServerInfoRobot(ArServerBase *server,
					      ArRobot *robot) : 
  myUpdateCB(this, &ArServerInfoRobot::update),
  myUpdateNumbersCB(this, &ArServerInfoRobot::updateNumbers),
  myUpdateStringsCB(this, &ArServerInfoRobot::updateStrings),
  myBatteryInfoCB(this, &ArServerInfoRobot::batteryInfo),
  myPhysicalInfoCB(this, &ArServerInfoRobot::physicalInfo),
  myActivityTimeInfoCB(this, &ArServerInfoRobot::activityTimeInfo),
  myUserTaskCB(this, &ArServerInfoRobot::userTask)
{
  myServer = server;
  myRobot = robot;

  myServer->addData("update", 
		    "gets an update about the important robot status (you should request this at an interval)... for bandwidth savings this is deprecated in favor of updateNumbers and updateStrings",
		    &myUpdateCB, "none",
		    "string: status; string: mode; byte2: 10 * battery; byte4: x; byte4: y; byte2: th; byte2: transVel; byte2: rotVel, byte2: latVel, byte: temperature (deg c, -128 means unknown)", "RobotInfo",
		    "RETURN_SINGLE");

  myServer->addData("updateNumbers", 
		    "gets an update about the important robot status (you should request this at an interval)",
		    &myUpdateNumbersCB, "none",
		    "byte2: 10 * battery; byte4: x; byte4: y; byte2: th; byte2: transVel; byte2: rotVel, byte2: latVel, byte: temperature (deg c, -128 means unknown)", "RobotInfo",
		    "RETURN_SINGLE");

  myServer->addData("updateStrings", 
		    "gets an update about the important robot status (you should ask for this at -1 interval since it is broadcast when the strings change)",
		    &myUpdateStringsCB, "none",
		    "string: status; string: mode; string: extended status (this has newlines)", "RobotInfo",
		    "RETURN_SINGLE");


  myServer->addData("batteryInfo", 
		    "gets the low battery voltage and shutdown voltage (you only need to request this once)",
		    &myBatteryInfoCB, "none",
		    "double: low battery voltage, double: shutdown battery voltage: ubyte: voltageIsStateOfCharge, if 0 voltage (including update voltage) is really voltage, if 1 voltage (including update voltage) is state of charge", 
				"RobotInfo", 
				"RETURN_SINGLE");

  myServer->addData("physicalInfo", 
		    "gets the information about the physical robot (you only need to request this once)",
		    &myPhysicalInfoCB, "none",
		    "string: robotType; string: robotSubType; byte2: robotWidth; byte2: robotLengthFront; byte2: robotLengthRear; byte: 0 means no lateral control, 1 means lateral control", 
		    "RobotInfo", "RETURN_SINGLE");

  myServer->addData("activityTimeInfo", 
		    "Returns information about the active server mode's last activity time",
		    &myActivityTimeInfoCB, "none",
		    "byte4: seconds since",
		    "RobotInfo", "RETURN_SINGLE");
  myUserTaskCB.setName("ArServerInfoRobot");
  myRobot->addUserTask("ArServerInfoRobot", 50, &myUserTaskCB);

}

AREXPORT ArServerInfoRobot::~ArServerInfoRobot()
{
}

AREXPORT void ArServerInfoRobot::update(ArServerClient *client, 
					ArNetPacket *packet)
{
  ArNetPacket sending;

  myRobot->lock();

  ArServerMode *netMode;
  if ((netMode = ArServerMode::getActiveMode()) != NULL)
  {
    sending.strToBuf(netMode->getStatus());
    sending.strToBuf(netMode->getMode());
  }
  else 
  {
    sending.strToBuf("Unknown status");
    sending.strToBuf("Unknown mode");
  }


	//ArLog::log(ArLog::Normal,
  //                     "ArServerInfoRobot::update() havestateofcharge = %d, soc = %f, real = %f, reg = %f",
	//											myRobot->haveStateOfCharge(),
	//											myRobot->getStateOfCharge(),
	//											myRobot->getRealBatteryVoltage(),
	//											myRobot->getBatteryVoltage());

  if (myRobot->haveStateOfCharge())
    sending.byte2ToBuf(ArMath::roundInt(myRobot->getStateOfCharge() * 10));
  else if (myRobot->getRealBatteryVoltage() > 0)
    sending.byte2ToBuf(ArMath::roundInt(
	    myRobot->getRealBatteryVoltage() * 10));
  else
    sending.byte2ToBuf(ArMath::roundInt(
	    myRobot->getBatteryVoltage() * 10));

  int th;
  th = ArMath::roundInt(ArMath::fixAngle(myRobot->getTh()));
  if (th == -180)
    th = 180;

  sending.byte4ToBuf((int)myRobot->getX());
  sending.byte4ToBuf((int)myRobot->getY());
  sending.byte2ToBuf(th);
  sending.byte2ToBuf((int)myRobot->getVel());
  sending.byte2ToBuf((int)myRobot->getRotVel());
  sending.byte2ToBuf((int)myRobot->getLatVel());
  sending.byteToBuf((char)myRobot->getTemperature());
	//sending.byte2ToBuf((int)myRobot->getPayloadNumSlots());
  myRobot->unlock();

  client->sendPacketUdp(&sending);
}

AREXPORT void ArServerInfoRobot::updateNumbers(ArServerClient *client, 
					       ArNetPacket *packet)
{
  ArNetPacket sending;

  myRobot->lock();

  int th;
  th = ArMath::roundInt(ArMath::fixAngle(myRobot->getTh()));
  if (th == -180)
    th = 180;

  if (myRobot->haveStateOfCharge())
    sending.byte2ToBuf(ArMath::roundInt(myRobot->getStateOfCharge() * 10));
  else if (myRobot->getRealBatteryVoltage() > 0)
    sending.byte2ToBuf(ArMath::roundInt(
	    myRobot->getRealBatteryVoltage() * 10));
  else
    sending.byte2ToBuf(ArMath::roundInt(
	    myRobot->getBatteryVoltage() * 10));
  sending.byte4ToBuf((int)myRobot->getX());
  sending.byte4ToBuf((int)myRobot->getY());
  sending.byte2ToBuf(th);
  sending.byte2ToBuf((int)myRobot->getVel());
  sending.byte2ToBuf((int)myRobot->getRotVel());
  sending.byte2ToBuf((int)myRobot->getLatVel());
  sending.byteToBuf((char)myRobot->getTemperature());
	//sending.byte2ToBuf((int)myRobot->getPayloadNumSlots());
  myRobot->unlock();

  client->sendPacketUdp(&sending);
}

AREXPORT void ArServerInfoRobot::updateStrings(ArServerClient *client, 
					       ArNetPacket *packet)
{
  ArNetPacket sending;

  myRobot->lock();
  sending.strToBuf(myStatus.c_str());
  sending.strToBuf(myMode.c_str());
  sending.strToBuf(myExtendedStatus.c_str());
  myRobot->unlock();

  client->sendPacketTcp(&sending);
}


AREXPORT void ArServerInfoRobot::batteryInfo(ArServerClient *client, 
					     ArNetPacket *packet)
{
  ArNetPacket sending;

  myRobot->lock();
  if (myRobot->haveStateOfCharge())
  {
    sending.doubleToBuf(myRobot->getStateOfChargeLow());
    sending.doubleToBuf(myRobot->getStateOfChargeShutdown());
    // voltage is really state of charge
    sending.uByteToBuf(1);
  }
  else
  {
    const ArRobotConfigPacketReader *reader;
    reader = myRobot->getOrigRobotConfig();
    if (reader != NULL && reader->hasPacketArrived())
    {
      if (reader->getLowBattery() != 0)
	sending.doubleToBuf(reader->getLowBattery() * .1);
      else
	sending.doubleToBuf(11.5);
      if (reader->getShutdownVoltage() != 0)
	sending.doubleToBuf(reader->getShutdownVoltage() * .1);
      else
	sending.doubleToBuf(11);
    }
    else
    {
      sending.doubleToBuf(11.5);
      sending.doubleToBuf(11);
    }
    // voltage is voltage, not state of charge
    sending.uByteToBuf(0);
  }
  
  myRobot->unlock();
  client->sendPacketTcp(&sending);

}


AREXPORT void ArServerInfoRobot::physicalInfo(ArServerClient *client, 
					     ArNetPacket *packet)

{
  ArNetPacket sending;

  myRobot->lock();
  sending.strToBuf(myRobot->getRobotType());
  sending.strToBuf(myRobot->getRobotSubType());
  sending.byte2ToBuf((int)myRobot->getRobotWidth());
  sending.byte2ToBuf((int)myRobot->getRobotLengthFront());
  sending.byte2ToBuf((int)myRobot->getRobotLengthRear());
  if (!myRobot->hasLatVel())
    sending.byteToBuf(0);
  else
    sending.byteToBuf(1);
  myRobot->unlock();
  
  client->sendPacketTcp(&sending);
}


AREXPORT void ArServerInfoRobot::activityTimeInfo(
	ArServerClient *client, 
	ArNetPacket *packet)
{
  ArNetPacket sending;

  // TODO Not entirely sure whether the robot needs to be locked here, but
  // it seems like it shouldn't hurt.
  //
  myRobot->lock();
  sending.byte4ToBuf(ArServerMode::getActiveModeActivityTimeSecSince());
  myRobot->unlock();
  
  client->sendPacketTcp(&sending);

} // end method activityTimeInfo

void ArServerInfoRobot::userTask(void)
{
  ArServerMode *netMode;
  ArNetPacket sending;
  
  if ((netMode = ArServerMode::getActiveMode()) != NULL)
  {
    myStatus = netMode->getStatus();
    myExtendedStatus = netMode->getExtendedStatus();
    if (myExtendedStatus.empty())
      myExtendedStatus = myStatus;
    myMode = netMode->getMode();
  }
  else 
  {
    myStatus = "Unknown status";
    myExtendedStatus = "Unknown extended status";
    myMode = "Unknown mode";
  }
  if (myStatus != myOldStatus || myMode != myOldMode || 
      myExtendedStatus != myOldExtendedStatus)
  {
    sending.strToBuf(myStatus.c_str());
    sending.strToBuf(myMode.c_str());
    sending.strToBuf(myExtendedStatus.c_str());
    myServer->broadcastPacketTcp(&sending, "updateStrings");
  }
  myOldStatus = myStatus;
  myOldMode = myMode;
  myOldExtendedStatus = myExtendedStatus;
}
