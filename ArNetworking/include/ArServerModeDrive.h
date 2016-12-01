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
#ifndef ARSERVERMODEDRIVE_H
#define ARSERVERMODEDRIVE_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArNetworking.h"
#include "ArServerMode.h"

class ArServerModeDrive : public ArServerMode
{
public:
  AREXPORT ArServerModeDrive(ArServerBase *server, ArRobot *robot,
			     bool takeControlOnJoystick = false);
  AREXPORT virtual ~ArServerModeDrive();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  /// This adds commands that'll let you disable and enable safe driving
  AREXPORT void addControlCommands(ArServerHandlerCommands *handlerCommands);
  AREXPORT void driveJoystick(double vel, 
                              double rotVel, 
                              bool isActivating = true);
  AREXPORT void serverDriveJoystick(ArServerClient *client,
				    ArNetPacket *packet);
  AREXPORT virtual void userTask(void);
  AREXPORT void setThrottleParams(int lowSpeed, int highSpeed);
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myDriveGroup;}
  AREXPORT void setSafeDriving(bool safe);
  AREXPORT bool getSafeDriving(void);
  /// this action will be activated in unsafe mode
  AREXPORT void setExtraUnsafeAction(ArAction *action) 
    { myExtraUnsafeAction = action; }
protected:
  AREXPORT void serverSafeDrivingEnable(void);
  AREXPORT void serverSafeDrivingDisable(void);
  AREXPORT void joyUserTask(void);
  ArAction *myExtraUnsafeAction;
  ArJoyHandler *myJoyHandler;
  ArActionJoydrive myJoydriveAction;
  ArActionInput *myInputAction;
  ArActionStop myStopAction;
  ArActionGroupInput myDriveGroup;
  ArFunctor2C<ArServerModeDrive, ArServerClient *, ArNetPacket *> myServerDriveJoystickCB;
  ArFunctorC<ArServerModeDrive> myJoyUserTaskCB;
  bool myDriveSafely;
  bool myNewDriveSafely;
  double myVel;
  double myRotVel;
  bool myTakeControlOnJoystick;
  // for the simple commands
  ArServerHandlerCommands *myHandlerCommands;
  ArFunctorC<ArServerModeDrive> myServerSafeDrivingEnableCB;
  ArFunctorC<ArServerModeDrive> myServerSafeDrivingDisableCB;
};


#endif // ARNETMODEDRIVE_H
