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

#ifndef ARSERVERMODEROBOTMOTION_H
#define ARSERVERMODEROBOTMOTION_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArServerMode.h"

class ArServerHandlerCommands;

/** Mode that allows clients to request simple discrete motions of the robot.
    Use the "moveDistance" request for forward/backward motion by the given amount (mm).
    Use the "turnByAngle" request to rotate by the given angle (deg).
    Use the "turnToHeading" request to rotate to a given absolute heading angle
(deg). 
    This mode will become active if any of the above requests are received.  any
active robot motion will be stopped once the mode becomes active, and also upon
deactivation before switching to a new mode.
    Each moveDistance request interrupts any previous moveDistance request if
still active.  Each turnByAngle or turnToHeading interrupts any previous turnByAngle
or turnToHeading if still active.
    For continuous velocity control, see ArServerModeRatioDrive instead.
    Motion will be limited based on obstacles sensed. Clearences may be
    configured in configuration (ArConfig) if addToConfig() is called to
    associate with an ArConfig object.  (E.g.  <code>motionMode.addToConfig(Aria::getConfig())</code>).
*/

class ArServerModeJogPosition : public ArServerMode
{
public:
  AREXPORT ArServerModeJogPosition(ArServerBase *server, ArRobot *robot, const char *name = "jogPositionMode", ArServerHandlerCommands *customCommands = NULL);
  AREXPORT virtual ~ArServerModeJogPosition();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);


  /// Adds to a config in a section
  AREXPORT void addToConfig(ArConfig *config, const char *section = "Jog Robot Position");
 // AREXPORT virtual void userTask(void);
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myActionGroup;}
  ArActionInput* getTurnAction() { return myTurnAction; }
  ArActionDriveDistance* getDriveAction() { return myDriveAction; }

  /// Adds a callback when trying to back up
//  void addDrivingBackwardsCallback(ArFunctor *functor, int position = 50)
//    { myDrivingBackwardsCallbacks.addCallback(functor, position); }
  /// Removes a callback for trying to back up
//  void remDrivingBackwardsCallback(ArFunctor *functor)
 //   { myDrivingBackwardsCallbacks.remCallback(functor); }

  /// Request a turn. Mode must be active.
  AREXPORT void turn(double angle);
  /// Request a movement. Mode must be active.
  AREXPORT void move(double distance);
  /// Request a movement. Mode must be active.
  AREXPORT void heading(double angle);
      
protected:
  void serverMove(ArServerClient *client, ArNetPacket *packet);
  void serverTurn(ArServerClient *client, ArNetPacket *packet);
  void serverHeading(ArServerClient *client, ArNetPacket *packet);
  bool myPrinting;

  ArActionDeceleratingLimiter *myLimiterForward;
  ArActionDeceleratingLimiter *myLimiterBackward;
  ArActionDeceleratingLimiter *myLimiterLateralLeft;
  ArActionDeceleratingLimiter *myLimiterLateralRight;
  ArActionLimiterRot *myLimiterRot;
  ArActionInput *myTurnAction;
  ArActionDriveDistance *myDriveAction;
  ArActionMovementParameters *myMovementParameters;
  ArActionGroup myActionGroup;
  ArTime myLastCommand;
  ArFunctor2C<ArServerModeJogPosition, ArServerClient *, ArNetPacket *> myServerMoveCB;
  ArFunctor2C<ArServerModeJogPosition, ArServerClient *, ArNetPacket *> myServerTurnCB;
  ArFunctor2C<ArServerModeJogPosition, ArServerClient *, ArNetPacket *> myServerHeadingCB;

  ArServerHandlerCommands *myCustomCommandServer;
  ArFunctor1C<ArServerModeJogPosition, ArArgumentBuilder*> myStringCommandMoveCB;
  ArFunctor1C<ArServerModeJogPosition, ArArgumentBuilder*> myStringCommandTurnCB;
  ArFunctor1C<ArServerModeJogPosition, ArArgumentBuilder*> myStringCommandHeadingCB;
  void stringCmdMove(ArArgumentBuilder* args);
  void stringCmdTurn(ArArgumentBuilder* args);
  void stringCmdHeading(ArArgumentBuilder* args);
};


#endif // ARNETMODEROBOTMOTION_H
