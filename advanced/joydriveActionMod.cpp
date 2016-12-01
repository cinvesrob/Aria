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


// the action which'll drive the robot
class JoydriveAction : public ArAction
{
public:
  // constructor
  JoydriveAction(void);
  // empty destructor
  virtual ~JoydriveAction(void);
  //the fire which will actually tell the resolver what to do
  virtual ArActionDesired *fire(ArActionDesired currentDesired);
  // whether the joystick is initalized or not
  bool joystickInited(void);
protected:
  // action desired
  ArActionDesired myDesired;
  // joystick handler
  ArJoyHandler myJoyHandler;
};

class JoydriveMod : public ArModule
{
public:

  bool init(ArRobot *robot, void *argument = NULL);
  bool exit();
};

JoydriveMod aModule;
ARDEF_MODULE(aModule);
JoydriveAction aJDAct;

bool JoydriveMod::init(ArRobot *robot, void *argument)
{
  if (!aJDAct.joystickInited())
  {
    printf("Do not have a joystick, set up the joystick then rerun the program\n\n");
    return(false);
  }

  robot->addAction(&aJDAct, 100);

  return(true);
}

bool JoydriveMod::exit()
{
  return(true);
}


/*
  Note the use of constructor chaining with ArAction.
*/  
JoydriveAction::JoydriveAction(void) :
  ArAction("Joydrive Action", "This action reads the joystick and sets the translational and rotational velocity based on this.")
{
  // initialize the joystick
  myJoyHandler.init();
  // set up the speed parameters on the joystick
  myJoyHandler.setSpeeds(50, 700);
}

JoydriveAction::~JoydriveAction(void)
{
  // it just removes itself from the robots action list
  if (myRobot != NULL)
    myRobot->remAction(this);
}

// whether the joystick is there or not
bool JoydriveAction::joystickInited(void)
{
  return myJoyHandler.haveJoystick();
}

// the guts of the thing
ArActionDesired *JoydriveAction::fire(ArActionDesired currentDesired)
{
  int rot, trans;

  // print out some info about hte robot
  printf("\rx %6.1f  y %6.1f  tth  %6.1f vel %7.1f mpacs %3d", myRobot->getX(),
	 myRobot->getY(), myRobot->getTh(), myRobot->getVel(), 
	 myRobot->getMotorPacCount());
  fflush(stdout);

  // see if one of the buttons is pushed, if so drive
  if (myJoyHandler.haveJoystick() && (myJoyHandler.getButton(1) ||
				    myJoyHandler.getButton(2)))
  {
    // get the readings from the joystick
    myJoyHandler.getAdjusted(&rot, &trans);
    // set what we want to do
    myDesired.setVel(trans);
    myDesired.setDeltaHeading(-rot);
    // return the actionDesired
    return &myDesired;
  }
  else
  {
    // set what we want to do
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    // return the actionDesired
    return &myDesired;
  }
}

