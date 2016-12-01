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
#include "ArServerModeJogPosition.h"
#include "ArServerHandlerCommands.h"
#include "ArActionLimiterRot.h"
#include <assert.h>
#include <sstream>

AREXPORT ArServerModeJogPosition::ArServerModeJogPosition(ArServerBase *server, ArRobot *robot, const char *name, ArServerHandlerCommands *customCommands) : 
  ArServerMode(robot, server, name),
  myPrinting(true),
  myActionGroup(robot),
  myServerMoveCB(this, &ArServerModeJogPosition::serverMove),
  myServerTurnCB(this, &ArServerModeJogPosition::serverTurn),
  myServerHeadingCB(this, &ArServerModeJogPosition::serverHeading),
  myCustomCommandServer(customCommands),
  myStringCommandMoveCB(this, &ArServerModeJogPosition::stringCmdMove),
  myStringCommandTurnCB(this, &ArServerModeJogPosition::stringCmdTurn),
  myStringCommandHeadingCB(this, &ArServerModeJogPosition::stringCmdHeading)
{
//  myGotServerCommand = true;
  
  myTurnAction = new ArActionInput;
  myActionGroup.addAction(myTurnAction, 50);
  myDriveAction = new ArActionDriveDistance;
  myActionGroup.addAction(myDriveAction, 51);

  myLimiterRot = new ArActionLimiterRot("LimiterRot");
  myActionGroup.addAction(myLimiterRot, 45);

  myLimiterForward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterForward", ArActionDeceleratingLimiter::FORWARDS);
  myActionGroup.addAction(myLimiterForward, 40);

  myLimiterBackward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterBackward", 
	  ArActionDeceleratingLimiter::BACKWARDS);
  myActionGroup.addAction(myLimiterBackward, 39);


  myLimiterLateralLeft = NULL;
  myLimiterLateralRight = NULL;
  if (myRobot->hasLatVel())
  {
    myLimiterLateralLeft = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralLeft", 
	    ArActionDeceleratingLimiter::LATERAL_LEFT);
    myActionGroup.addAction(myLimiterLateralLeft, 38);
    myLimiterLateralRight = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralRight", 
	    ArActionDeceleratingLimiter::LATERAL_RIGHT);
    myActionGroup.addAction(myLimiterLateralRight, 37);
  }

  myMovementParameters = new ArActionMovementParameters("JogPositionParameters", false);
  myMovementParameters->enable();
  myActionGroup.addAction(myMovementParameters, 1);

  myLimiterForward->setUseLocationDependentDevices(true);
  myLimiterBackward->setUseLocationDependentDevices(true);
  if (myLimiterLateralLeft != NULL) myLimiterLateralLeft->setUseLocationDependentDevices(true);
  if (myLimiterLateralRight != NULL) myLimiterLateralRight->setUseLocationDependentDevices(true);

  assert(myServer);
  
  addModeData("moveDist", "drives the robot forward/backwards the given distance (mm)", &myServerMoveCB, "double: distanceToMove", "none", "Movement", "RETURN_NONE");
  addModeData("turnByAngle", "rotate the robot by the given angle (deg)", &myServerTurnCB, "double: angleToTurn", "none", "Movement", "RETURN_NONE");
  addModeData("turnToHeading", "rotate the robot to the given absolute heading (deg)", &myServerHeadingCB, "double: heading", "none", "Movement", "RETURN_NONE");


  // TODO add a task that monitors actions and sets status to "done" when done
  // moving/turning. Or use callbacks from those actions (may need mutex to
  // protect status which could be set from network request thread).

  if(customCommands)
  {
    customCommands->addStringCommand("JogPosition:MoveDist", "Move forward/back by given distance (mm)", &myStringCommandMoveCB);
    customCommands->addStringCommand("JogPosition:TurnByAngle", "Turn by given angle (deg) relative to current position", &myStringCommandTurnCB);
    customCommands->addStringCommand("JogPosition:TurnToHeading", "Turn to given heading (deg)", &myStringCommandHeadingCB);
  }
}

AREXPORT ArServerModeJogPosition::~ArServerModeJogPosition()
{
  ArLog::log(ArLog::Verbose, "ArServerModeJogPosition::destructor");
  // TODO cleanup:
  //   remModeData("moveDist");
  //   remModeData("turnAngle");
  //   remove drive, turn, limiterForward, limiterBacward, movementeParameters actions from config
  

}

AREXPORT void ArServerModeJogPosition::addToConfig(ArConfig *config, 
						  const char *section)
{
  //myDriveAction->addToConfig(config, section);
  //myTurnAction->addToConfig(config, section);
  myLimiterForward->addToConfig(config, section, "Forward");
  myLimiterBackward->addToConfig(config, section, "Backward");
  if (myLimiterLateralLeft != NULL)
    myLimiterLateralLeft->addToConfig(config, section, "Lateral");
  if (myLimiterLateralRight != NULL)
    myLimiterLateralRight->addToConfig(config, section, "Lateral");
  myMovementParameters->addToConfig(config, section, "JogPosition");
}

AREXPORT void ArServerModeJogPosition::activate(void)
{
  bool wasActive = isActive();

  // The baseActivate() method should only be called in the context of the activate()
  // method.
  if (!isActive()) {
    if (!baseActivate()) {
      return;
    }
  }


  if (!wasActive)
  {
    myRobot->clearDirectMotion();
    myRobot->stop();
    myActionGroup.activateExclusive();
    myMode = "Jog";
    ArLog::log(ArLog::Normal, "%s: Activated", myName.c_str());
  }
}

AREXPORT void ArServerModeJogPosition::deactivate(void)
{
  myActionGroup.deactivate();
  myRobot->stop();
  myRobot->clearDirectMotion();
  baseDeactivate();
}

AREXPORT void ArServerModeJogPosition::move(double distance)
{
  if (!isActive()) {
     activate();
     if(!isActive()) return; // activation failed
  }
  setActivityTimeToNow();
  myLastCommand.setToNow();
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: drive %.0f", getName(), distance);
  myRobot->clearDirectMotion();
  myDriveAction->cancelDistance();
  myDriveAction->setDistance(distance);
  std::stringstream ss;
  ss << "Moving " << distance << " mm";
  myStatus = ss.str();
  // when c++11 support is more common: myStatus = std::string("Moving ") + std::to_string(distance) + " mm";
}

AREXPORT void ArServerModeJogPosition::turn(double angle)
{
  if (!isActive()) {
     activate();
     if(!isActive()) return; // activation failed
  }
  setActivityTimeToNow();
  myLastCommand.setToNow();
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: turn %.0f", getName(), angle);
  myTurnAction->clear();
  myRobot->clearDirectMotion();
  ArRobot *robot = myTurnAction->getRobot();
  if(!robot) return;
  robot->lock();
  double heading = ArMath::fixAngle(robot->getTh() + angle);
  robot->unlock();
  if(myPrinting)
    ArLog::log(ArLog::Normal, "%s: calculated absolute angle %f from robot theta %f + relative turn angle %f", getName(), heading, robot->getTh(), angle);
  myTurnAction->setHeading(heading);
  std::stringstream ss;
  ss << "Turning by " << angle << " deg";
  myStatus = ss.str();
  // when c++11 support is more common: myStatus = std::string("Turning ") + std::to_string(angle) + " deg";
}

AREXPORT void ArServerModeJogPosition::heading(double angle)
{
  if (!isActive()) {
     activate();
     if(!isActive()) return; // activation failed
  }
  setActivityTimeToNow();
  myLastCommand.setToNow();
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: heading %.0f", getName(), angle);
  myTurnAction->clear();
  myRobot->clearDirectMotion();
  myTurnAction->setHeading(angle); 
  std::stringstream ss;
  ss << "Turning to heading " << angle << " deg";
  myStatus = ss.str();
  // when c++11 support is more common: myStatus = std::string("Turning ") + std::to_string(angle) + " deg";
}

void ArServerModeJogPosition::serverMove(ArServerClient *client, ArNetPacket *packet)
{
  double distance = packet->bufToDouble();
//  myGotServerCommand = true;
//  if (!isActive()) {
//    activate();
//  }
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: serverCmd (%s) moveDist %.0f", getName(), client->getIPString(), distance);
  move(distance);
}



void ArServerModeJogPosition::serverTurn(ArServerClient *client, ArNetPacket *packet)
{
  double angle = packet->bufToDouble();
//  myGotServerCommand = true;
//  if (!isActive()) {
//    activate();
//  }
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: serverCmd (%s) turnByAngle %.0f", getName(), client->getIPString(), angle);
  turn(angle);
}




void ArServerModeJogPosition::serverHeading(ArServerClient *client, ArNetPacket *packet)
{
  double angle = packet->bufToDouble();
//  myGotServerCommand = true;
//  if (!isActive()) {
//    activate();
//  }
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: serverCmd (%s) turnToHeading %.0f", getName(), client->getIPString(), angle);
  heading(angle);
}


void ArServerModeJogPosition::stringCmdMove(ArArgumentBuilder* args)
{
  bool ok = false;
  double dist = args->getArgDouble(0, &ok);
  if(!ok) {
    ArLog::log(ArLog::Terse, "%s: Move string command: error parsing argument", getName());
    return;
  }
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: Move string command: %.0f mm", getName(), dist);
  move(dist);
}


void ArServerModeJogPosition::stringCmdTurn(ArArgumentBuilder* args)
{
  bool ok = false;
  double angle = args->getArgDouble(0, &ok);
  if(!ok) {
    ArLog::log(ArLog::Terse, "%s: TurnByAngle string command: error parsing argument", getName());
    return;
  }
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: TurnByAngle string command: %.0f deg", getName(), angle);
  turn(angle);
}


void ArServerModeJogPosition::stringCmdHeading(ArArgumentBuilder* args)
{
  bool ok = false;
  double angle = args->getArgDouble(0, &ok);
  if(!ok) {
    ArLog::log(ArLog::Terse, "%s: TurnToHeading string command: error parsing argument", getName());
    return;
  }
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: TurnToHeading string command: %.0f deg", getName(), angle);
  heading(angle);
}




