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
#include "ArServerModeStop.h"

AREXPORT ArServerModeStop::ArServerModeStop(ArServerBase *server, 
					    ArRobot *robot,
					    bool defunct) : 
  ArServerMode(robot, server, "stop"),
  myStopGroup(robot),
  myNetStopCB(this, &ArServerModeStop::netStop)
{
  myMode = "Stop";
  if (myServer != NULL)
  {
    addModeData("stop", "stops the robot", &myNetStopCB,
		"none", "none", "Stop", "RETURN_NONE");
  }

  myUseLocationDependentDevices = true;

  myLimiterForward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterForward");
  myStopGroup.addAction(myLimiterForward, 150);

  myLimiterBackward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterBackward", 
	  ArActionDeceleratingLimiter::BACKWARDS);
  myStopGroup.addAction(myLimiterBackward, 149);

  myLimiterLateralLeft = NULL;
  if (myRobot->hasLatVel())
  {
    myLimiterLateralLeft = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralLeft", 
	    ArActionDeceleratingLimiter::LATERAL_LEFT);
    myStopGroup.addAction(myLimiterLateralLeft, 148);
  }

  myLimiterLateralRight = NULL;
  if (myRobot->hasLatVel())
  {
    myLimiterLateralRight = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralRight", 
	    ArActionDeceleratingLimiter::LATERAL_RIGHT);
    myStopGroup.addAction(myLimiterLateralRight, 147);
  }

}

AREXPORT ArServerModeStop::~ArServerModeStop()
{

}

AREXPORT void ArServerModeStop::activate(void)
{
  if (isActive() || !baseActivate())
    return;
  setActivityTimeToNow();
  myRobot->stop();
  myRobot->clearDirectMotion();
  myStopGroup.activateExclusive();
  myStatus = "Stopping";
}

AREXPORT void ArServerModeStop::deactivate(void)
{
  myStopGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeStop::stop(void)
{
  activate();
}

AREXPORT void ArServerModeStop::netStop(ArServerClient *client, 
				     ArNetPacket *packet)
{
  setActivityTimeToNow();
  myRobot->lock();
  ArLog::log(ArLog::Verbose, "Stopping");
  stop();
  myRobot->unlock();
}

AREXPORT void ArServerModeStop::userTask(void)
{
  /// MPL 2014_04_17 centralizing all the places stopped is calculated
  //if (myRobot->getVel() < 2 && myRobot->getRotVel() < 2)
  if (myRobot->isStopped())
  {
    myStatus = "Stopped";
  }
  else
  {
    setActivityTimeToNow();
    myStatus = "Stopping";
  }
}

AREXPORT void ArServerModeStop::addToConfig(ArConfig *config, 
					      const char *section)
{
  myLimiterForward->addToConfig(config, section, "Forward");
  myLimiterBackward->addToConfig(config, section, "Backward");
  if (myLimiterLateralLeft != NULL)
    myLimiterLateralLeft->addToConfig(config, section, "Lateral");
  if (myLimiterLateralRight != NULL)
    myLimiterLateralRight->addToConfig(config, section, "Lateral");
}

AREXPORT void ArServerModeStop::setUseLocationDependentDevices(
	bool useLocationDependentDevices, bool internal)
{
  if (!internal)
    myRobot->lock();
  // if this is a change then print it
  if (useLocationDependentDevices != myUseLocationDependentDevices)
  {
    myUseLocationDependentDevices = useLocationDependentDevices;
    myLimiterForward->setUseLocationDependentDevices(
	    myUseLocationDependentDevices);
    myLimiterBackward->setUseLocationDependentDevices(
	    myUseLocationDependentDevices);
    if (myLimiterLateralLeft != NULL)
      myLimiterLateralLeft->setUseLocationDependentDevices(
	      myUseLocationDependentDevices);
    if (myLimiterLateralRight != NULL)
      myLimiterLateralRight->setUseLocationDependentDevices(
	      myUseLocationDependentDevices);
  }
  if (!internal)
    myRobot->unlock();
}

AREXPORT bool ArServerModeStop::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}

