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

/*
  This is basically just a demo of how to use the AMPTU, but made fancy so
  you can drive around the robot and look at stuff with the camera.  Press
  button 1 to drive the robot and button two to move the camera.
*/

/*
  This class is the core of this demo, it adds itself to the robot given
  as a user task, then drives the robot and camera from the joystick
*/
class Joydrive
{
public:
  // constructor
  Joydrive(ArRobot *robot);
  // destructor, its just empty
  ~Joydrive(void) {}
  
  // the callback function
  void drive(void);

protected:
  // the rotational max for the robot
  int myRotValRobot;
  // translational max for the robot
  int myTransValRobot;
  // pan max for the camera
  int myPanValCamera;
  // tilt max for the camera
  int myTiltValCamera;
  // the joystick handler
  ArJoyHandler myJoyHandler;
  // the camera
  ArAMPTU myCam;
  // whether the camera has been inited or not
  bool myCamInited;
  // pointer to the robot
  ArRobot *myRobot;
  // callback for the drive function
  ArFunctorC<Joydrive> myDriveCB;
};

/*
  Constructor, sets the robot pointer, and some initial values, also note the
  use of constructor chaining on myCam and myDriveCB.
*/
Joydrive::Joydrive(ArRobot *robot) :
  myCam(robot),
  myDriveCB(this, &Joydrive::drive)
{
  // set the robot pointer and add the joydrive as a user task
  myRobot = robot;
  myRobot->addUserTask("joydrive", 50, &myDriveCB);

  // initialize some variables
  myRotValRobot = 100;
  myTransValRobot = 700;
  myPanValCamera = 8;
  myTiltValCamera = 6;
  myCamInited = false;

  // initialize the joystick
  myJoyHandler.init();
  // see if we have a joystick, and let user know the results
  if (myJoyHandler.haveJoystick())
  {
    printf("Have a joystick\n\n");
  }
  // we don't have a joystick, so get out 
  else
  {
    printf("Do not have a joystick, set up the joystick then rerun the program\n\n");
    Aria::shutdown();
    exit(0);    
  }
}

// the important function
void Joydrive::drive(void)
{
  int trans, rot;
  int pan, tilt;

  // if the camera isn't initialized, initialize it here... it has to be 
  // done here instead of above because it needs to be done when the 
  // robot is connected
  if (!myCamInited && myRobot->isConnected())
  {
    myCam.init();
    myCamInited = true;
  }

  // if joystick button one is pushed, then drive the robot
  if (myJoyHandler.haveJoystick() && myJoyHandler.getButton(1))
  {
    // set the speeds on the joyhandler so we get the values out we want
    myJoyHandler.setSpeeds(myRotValRobot, myTransValRobot);
    // get the values from the joyhandler
    myJoyHandler.getAdjusted(&rot, &trans);
    // set the velocities
    myRobot->setVel(trans);
    myRobot->setRotVel(-rot);
  }
  // if the joystick button one isn't pushed, stop the robot
  else
  {
    myRobot->setVel(0);
    myRobot->setRotVel(0);
  }
  
  // if button two is pressed then move the camera
  if (myJoyHandler.haveJoystick() && myJoyHandler.getButton(2))
  {
    // set the speeds so we get out the values we want
    myJoyHandler.setSpeeds(myPanValCamera, myTiltValCamera);
    // get the values
    myJoyHandler.getAdjusted(&pan, &tilt);
    // move the camera
    myCam.panTilt(myCam.getPan() + pan, myCam.getTilt() + tilt);
  } 

}

int main(int argc, char **argv) 
{
  // just some stuff for returns
  std::string str;
  int ret;
  
  // robots connection
  ArSerialConnection con;
  // the robot, this turns state reflection off
  ArRobot robot(NULL, false);
  // the joydrive as defined above, this also adds itself as a user task
  Joydrive joyd(&robot);

  // mandatory init
  Aria::init();

  // open the connection, if it fails, exit
  if ((ret = con.open()) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    Aria::shutdown();
    return 1;
  }

  // set the connection o nthe robot
  robot.setDeviceConnection(&con);
  // connect, if we fail, exit
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }

  // turn off the sonar, enable the motors, turn off amigobot sounds
  robot.comInt(ArCommands::SONAR, 0);
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::SOUNDTOG, 0);

  // run, if we lose connection to the robot, exit
  robot.run(true);
  
  // shutdown and go away
  Aria::shutdown();
  return 0;
}

