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
#ifndef ARNETMODEIDLE_H
#define ARNETMODEIDLE_H

#include "Aria.h"
#include "ArServerMode.h"

/// Mode that the ArServerMode infrastructure will activate if someone
/// tries to switch from one mode to another and there's idle
/// processing to be done...
class ArServerModeIdle : public ArServerMode
{
public:
  /// Constructor
  AREXPORT ArServerModeIdle(ArServerBase *server, ArRobot *robot);
  /// Destructor
  AREXPORT virtual ~ArServerModeIdle();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT virtual void userTask(void);
  /// Gets the mode that this mode interrupted (or NULL if none)
  AREXPORT void setModeInterrupted(ArServerMode *modeInterrupted);
  /// Gets the mode that this mode interrupted (or NULL if it didn't interrupt anything)
  AREXPORT ArServerMode *getModeInterrupted(void);
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myStopGroup; }
  /// Adds to the config
  AREXPORT void addToConfig(ArConfig *config, const char *section = "Teleop settings");
  /// Sets whether we're using the range devices that depend on location
  AREXPORT void setUseLocationDependentDevices(
	  bool useLocationDependentDevices, bool internal = false);
  /// Gets whether we're using the range devices that depend on location
  AREXPORT bool getUseLocationDependentDevices(void);
protected:
  ArActionDeceleratingLimiter *myLimiterForward;
  ArActionDeceleratingLimiter *myLimiterBackward;
  ArActionDeceleratingLimiter *myLimiterLateralLeft;
  ArActionDeceleratingLimiter *myLimiterLateralRight;
  ArActionGroupStop myStopGroup;
  bool myUseLocationDependentDevices;
  ArServerMode *myModeInterrupted;
  ArFunctor2C<ArServerModeIdle, ArServerClient *, ArNetPacket *> myNetIdleCB;
};

#endif // ARNETMODEIDLE_H
