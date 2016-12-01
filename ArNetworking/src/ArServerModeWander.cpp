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
#include "ArServerModeWander.h"

AREXPORT ArServerModeWander::ArServerModeWander(ArServerBase *server, ArRobot *robot) : 
  ArServerMode(robot, server, "wander"),
  myWanderGroup(robot),
  myNetWanderCB(this, &ArServerModeWander::netWander)
{
  myMode = "Wander";
  if (myServer != NULL)
  {
    addModeData("wander", "makes the robot wander", &myNetWanderCB,
		"none", "none", "Movement", "RETURN_NONE");
  }
}

AREXPORT ArServerModeWander::~ArServerModeWander()
{

}

AREXPORT void ArServerModeWander::activate(void)
{
  if (!baseActivate())
    return;

  setActivityTimeToNow();
  myRobot->clearDirectMotion();
  myWanderGroup.activateExclusive();
  myStatus = "Wandering";
}

AREXPORT void ArServerModeWander::deactivate(void)
{
  myWanderGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeWander::wander(void)
{
  activate();
}

AREXPORT void ArServerModeWander::netWander(ArServerClient *client, 
				     ArNetPacket *packet)
{
  myRobot->lock();
  ArLog::log(ArLog::Verbose, "Wandering");
  wander();
  myRobot->unlock();
  setActivityTimeToNow();
}

AREXPORT void ArServerModeWander::userTask(void)
{
  setActivityTimeToNow();

  // Sets the robot so that we always thing we're trying to move in
  // this mode
  myRobot->forceTryingToMove();

  //myStatus = "Wandering";
}
