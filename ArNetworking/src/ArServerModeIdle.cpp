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
#include "ArServerModeIdle.h"

AREXPORT ArServerModeIdle::ArServerModeIdle(ArServerBase *server, 
					    ArRobot *robot) :
  ArServerMode(robot, server, "idle"),
  myStopGroup(robot)
{
  myStatus = "Idle processing";
  myMode = "Idle";

  myUseLocationDependentDevices = true;

  myModeInterrupted = NULL;

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

AREXPORT ArServerModeIdle::~ArServerModeIdle()
{

}

AREXPORT void ArServerModeIdle::activate(void)
{
  if (isActive())
    return;

  if (!baseActivate())
  {
    /*
    ArLog::log(ArLog::Normal, 
	       "IDLE: Clearing mode interrupted since could not activate...");
    */
    myModeInterrupted = NULL;
    return;
  }

  //ArLog::log(ArLog::Normal, "IDLE... um... %p", myModeInterrupted);

  myRobot->stop();
  myRobot->clearDirectMotion();
  myStopGroup.activateExclusive();
  setActivityTimeToNow();
  ArLog::log(ArLog::Normal, "Idle processing mode activated");
}

AREXPORT void ArServerModeIdle::deactivate(void)
{
  ArLog::log(ArLog::Normal, "Idle processing mode deactivating");
  myStopGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeIdle::userTask(void)
{
  /// MPL 2014_04_17 centralizing all the places stopped is calculated
  //if (fabs(myRobot->getVel()) > 2 || fabs(myRobot->getRotVel()) > 2 || 
  //(myRobot->hasLatVel() && fabs(myRobot->getLatVel()) > 2))
  if (!myRobot->isStopped())
  {
    myStatus = "Stopping";
    setActivityTimeToNow();
  }
  else
  {
    myStatus = "Idle processing";
  }
    
  //ArLog::log(ArLog::Normal, "Idle mode called");
  if (!myServer->idleProcessingPending())
  {
    //ArLog::log(ArLog::Normal, "Idle mode done");
    deactivate();
  }
}

AREXPORT void ArServerModeIdle::addToConfig(ArConfig *config, 
					      const char *section)
{
  myLimiterForward->addToConfig(config, section, "Forward");
  myLimiterBackward->addToConfig(config, section, "Backward");
  if (myLimiterLateralLeft != NULL)
    myLimiterLateralLeft->addToConfig(config, section, "Lateral");
  if (myLimiterLateralRight != NULL)
    myLimiterLateralRight->addToConfig(config, section, "Lateral");
}

AREXPORT void ArServerModeIdle::setUseLocationDependentDevices(
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

AREXPORT bool ArServerModeIdle::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}


AREXPORT void ArServerModeIdle::setModeInterrupted(
	ArServerMode *modeInterrupted)
{
  /*
  if (modeInterrupted != NULL)
    ArLog::log(ArLog::Normal, "IDLE: Setting mode interrupted to %s", 
	       modeInterrupted->getName());
  else
    ArLog::log(ArLog::Normal, "IDLE: Setting mode interrupted to NULL");
  */
  myModeInterrupted = modeInterrupted;
}  

AREXPORT ArServerMode *ArServerModeIdle::getModeInterrupted(void)
{
  return myModeInterrupted;
}

