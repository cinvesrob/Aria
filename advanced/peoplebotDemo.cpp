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

#define CAMERA_X_CORRECTION 0
//-.085
#define BACKUP_DIST -600
#define BACKUP_TIME 3000

class ArActionTableSensorLimiter : public ArAction
{
public:
  ArActionTableSensorLimiter(void);
  virtual ~ArActionTableSensorLimiter(void) {}
  virtual ArActionDesired *fire(ArActionDesired currentDesired);
protected:
  ArActionDesired myDesired;
};

ArActionTableSensorLimiter::ArActionTableSensorLimiter(void) :
  ArAction("Table sensor limiter")
{

}

ArActionDesired *ArActionTableSensorLimiter::fire(
	ArActionDesired currentDesired)
{
  myDesired.reset();

// Note the following commented out section will not work with IR's that
// are sent through byte 4 of the IO packet.
// Reference the NewTableSensingIR parameter
  /*
  printf("%d ", myRobot->getDigIn());
  if (!(myRobot->getDigIn() & ArUtil::BIT0))
    printf("leftTable ");
  if (!(myRobot->getDigIn() & ArUtil::BIT1))
    printf("rightTable ");
  if (!(myRobot->getDigIn() & ArUtil::BIT3))
    printf("leftBREAK ");
  if (!(myRobot->getDigIn() & ArUtil::BIT2))
    printf("rightBREAK ");
  printf("\n");
  */

  if (myRobot->isLeftTableSensingIRTriggered() ||
      myRobot->isRightTableSensingIRTriggered())
  {
    myDesired.setMaxVel(0);
    return &myDesired;
  }
  return NULL;  
}

class DriveTo : public ArAction
{
public:
  enum State {
    STATE_START_LOOKING,
    STATE_LOOKING,
    STATE_FAILED,
    STATE_SUCCEEDED
  };
  DriveTo(ArACTS_1_2 *acts, ArGripper *gripper, ArPTZ *amptu);
  ~DriveTo(void);
  virtual ArActionDesired *fire(ArActionDesired currentDesired);
  bool setChannel(int channel);
  State getState(void) { return myState; }
  enum {
    WIDTH = 160,
    HEIGHT = 120
  };
protected:
  ArActionDesired myDesired;
  ArACTS_1_2 *myActs;
  ArGripper *myGripper;
  ArPTZ *myAMPTU;
  ArPose myLastPose;
  ArTime myLastMoved;
  int myChannel;
  bool myPickup;
  State myState;
  ArTime myLastSeen;
};

DriveTo::DriveTo(ArACTS_1_2 *acts, ArGripper *gripper, ArPTZ *amptu) :
    ArAction("DriveTo", "Drives to something.")
{
  myActs = acts;
  myGripper = gripper;
  myAMPTU = amptu;
  myChannel = 0;
  myState = STATE_FAILED;
  //setChannel(1);
}

DriveTo::~DriveTo(void)
{

}

ArActionDesired *DriveTo::fire(ArActionDesired currentDesired)
{
  ArACTSBlob blob;
  double xRel, yRel;
  ArPose pose;
  double dist;

  if (myState == STATE_SUCCEEDED || myState == STATE_FAILED)
  {
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }

  if (myState == STATE_START_LOOKING)
  {
    myState = STATE_LOOKING;
    myLastSeen.setToNow();
    myLastPose = myRobot->getPose();
    myLastMoved.setToNow();
  }

  pose = myRobot->getPose();
  dist = myLastPose.findDistanceTo(pose);
  if (dist < 5 && myLastMoved.mSecSince() > 1500)
  {
    printf("DriveTo: Failed, no movement in the last 1500 msec.\n");
    myState = STATE_FAILED;
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  else if (dist > 5)
  {
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  }

  if (myActs->getNumBlobs(myChannel) == 0 || 
      !myActs->getBlob(myChannel, 1, &blob))
  {
    if (myLastSeen.mSecSince() > 1000)
    {
      printf("DriveTo:  Lost the blob, failed.\n");
      myState = STATE_FAILED;
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      return &myDesired;
    }
  }
  else
  {
    myLastSeen.setToNow();
  }

  xRel = (double)(blob.getXCG() - WIDTH/2.0) / (double)WIDTH + CAMERA_X_CORRECTION;
  yRel = (double)(blob.getYCG() - HEIGHT/2.0) / (double)HEIGHT;
  
  //printf("xRel %.3f yRel %.3f\n", xRel, yRel);

  myDesired.reset();
  // this if the stuff we want to do if we're not going to just drive forward
  // and home in on the color, ie the pickup-specific stuff
  if (currentDesired.getMaxVelStrength() > 0 &&
      currentDesired.getMaxVel() < 125)
  {
    printf("DriveTo:  Close to a wall of some sort, succeeded.\n");
    myState = STATE_SUCCEEDED;
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  
  if (ArMath::fabs(xRel) < .10)
  {
    //printf("Going straight ahead\n");
    myDesired.setDeltaHeading(0);
  }
  else
  {
    //printf("Turning %.2f\n", -xRel * 20);
    if (-xRel > 1.0)
      myDesired.setDeltaHeading(20);
    else if (-xRel < -1.0)
      myDesired.setDeltaHeading(-20);
    else
      myDesired.setDeltaHeading(-xRel * 20);
  }
  myDesired.setVel(150);
  return &myDesired;
}

bool DriveTo::setChannel(int channel)
{
  if (channel >= 1 && channel <= ArACTS_1_2::NUM_CHANNELS)
  {
    myChannel = channel;
    myState = STATE_START_LOOKING;
    return true;
  }
  else
    return false;
}


class PickUp : public ArAction
{
public:
  enum State {
    STATE_START_LOOKING,
    STATE_LOOKING,
    STATE_FAILED,
    STATE_SUCCEEDED
  };
  PickUp(ArACTS_1_2 *acts, ArGripper *gripper, ArPTZ *amptu);
  ~PickUp(void);
  virtual ArActionDesired *fire(ArActionDesired currentDesired);
  bool setChannel(int channel);
  State getState(void) { return myState; }
  enum {
    WIDTH = 160,
    HEIGHT = 120
  };
protected:
  ArActionDesired myDesired;
  ArACTS_1_2 *myActs;
  ArGripper *myGripper;
  ArPTZ *myAMPTU;
  int myChannel;
  bool myPickup;
  bool myTried;
  bool myPointedDown;
  State myState;
  ArPose myLastPose;
  ArTime myLastMoved;
  ArTime myLastSeen;
  ArTime mySentGripper;
  ArTime myTriedStart;
  bool myWaitingOnGripper;
  bool myWaitingOnLower;
  bool myWaitingOnRaised;
  bool myStartRaised;
  bool myLowered;
};

PickUp::PickUp(ArACTS_1_2 *acts, ArGripper *gripper, ArPTZ *amptu) :
    ArAction("PickUp", "Picks up something.")
{
  myActs = acts;
  myGripper = gripper;
  myAMPTU = amptu;
  myChannel = 0;
  myState = STATE_FAILED;
  //setChannel(2);
}

PickUp::~PickUp(void)
{

}

ArActionDesired *PickUp::fire(ArActionDesired currentDesired)
{
  ArPose pose;
  ArACTSBlob blob;
  bool blobSeen = false;
  double xRel, yRel;
  double dist;


  myDesired.reset();
  if (myState == STATE_SUCCEEDED)
  {
    //printf("PickUp: Succeeded\n");
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  else if (myState == STATE_FAILED)
  {
    //printf("PickUp: Failed\n");
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }

  if (myActs->getNumBlobs(myChannel) > 0 &&
      (blobSeen = myActs->getBlob(myChannel, 1, &blob)))
  {
    myLastSeen.setToNow();
  }

  // this if the stuff we want to do if we're not going to just drive forward
  // and home in on the color, ie the pickup-specific stuff
  if (myState == STATE_START_LOOKING)
  {
    myGripper->gripOpen();
    mySentGripper.setToNow();
    myPointedDown = false;
    myState = STATE_LOOKING;
    myLastSeen.setToNow();
    myTried = false;
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
    myWaitingOnGripper = true;
    myWaitingOnLower = false;
    myLowered = false;
    myWaitingOnRaised = false;
    myStartRaised = false;
    printf("@@@@@ Pickup: Starting\n");
  }

  // we want to sit still until the lift is down or for a second and a half

  if (myWaitingOnGripper) 
  {
    if (mySentGripper.mSecSince() < 500 || 
	(myGripper->getGripState() != 1
	 && mySentGripper.mSecSince() < 2000))
    {
      myGripper->gripOpen();
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      myLastMoved.setToNow();
      myLastPose = myRobot->getPose();
      return &myDesired;
    }
    else
    {
      myWaitingOnGripper = false;
    }
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  } 

  //printf("sensors %d %d stall %d %d\n",!(myRobot->getDigIn() & ArUtil::BIT2),
  //!(myRobot->getDigIn() & ArUtil::BIT3),
  //myRobot->isLeftMotorStalled(), myRobot->isRightMotorStalled());
  if ((myRobot->isLeftBreakBeamTriggered() &&
       myRobot->isRightBreakBeamTriggered()) ||
       myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
  {
    if (!myWaitingOnLower && !myLowered && !myWaitingOnRaised)
    {
      myWaitingOnLower = true;
      printf("PickUp: Lowering gripper\n");
      mySentGripper.setToNow();
    }
  }
  if (myWaitingOnLower)
  {
    /// TODO 
    if (mySentGripper.mSecSince() < 600 || 
	(!myGripper->isLiftMaxed() 
	 && mySentGripper.mSecSince() < 20000))
    {
      myGripper->liftDown();
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      myLastMoved.setToNow();
      myLastPose = myRobot->getPose();
      return &myDesired;
    }
    else
    {
      myWaitingOnLower = false;
      myWaitingOnRaised = true;
      myStartRaised = true;
    }
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  }

  if (myWaitingOnRaised)
  {
    if (myStartRaised)
    {
      printf("PickUp: Raising gripper a little bit\n");
      myGripper->liftCarry(15);
      mySentGripper.setToNow();
      myStartRaised = false;
    }
    if (mySentGripper.mSecSince() > 1000)
    {
      printf("PickUp: Raised the gripper a little bit\n");
      myWaitingOnRaised = false;
      myLowered = true;
    }
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
    return &myDesired;
  }

  if (myLowered && myGripper->getBreakBeamState() != 0)
  {
    if (!myTried)
    {
      printf("PickUp: Trying to pick up.\n");
      myTriedStart.setToNow();
    }
    myTried = true;
    myGripper->gripClose();
    if (myGripper->getGripState() == 2 || myTriedStart.mSecSince() > 5000)
    {
      printf("PickUp: Succeeded, have block.\n");
      myGripper->liftUp();
      myState = STATE_SUCCEEDED;
    }
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  // this means that the grippers are closed, but we don't have anything in 
  // them, ie that we failed to get the block
  else if (myTried && (myGripper->getGripState() == 2 || 
		       myTriedStart.mSecSince() > 5000))
  {
    myState = STATE_FAILED;
    myGripper->gripOpen();
    ArUtil::sleep(3);
    myGripper->liftUp();
    printf("PickUp: Grippers closed, didn't get a block, failed.\n");
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }


  pose = myRobot->getPose();
  dist = myLastPose.findDistanceTo(pose);
  if (dist < 5 && myLastMoved.mSecSince() > 2500)
  {
    printf("PickUp: Failed, no movement in the last 2500 msec.\n");
    myState = STATE_FAILED;
    myGripper->gripOpen();
    myGripper->liftUp();
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  else if (dist > 5)
  {
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  }

  if (!blobSeen)
  {
    if (((!myPointedDown && myLastSeen.mSecSince() > 1500) ||
	 (myPointedDown && myLastSeen.mSecSince() > 4000)) &&
	myGripper->getBreakBeamState() == 0)
    {
      printf("PickUp:  Lost the blob, failed, last saw it %ld msec ago.\n",
	     myLastSeen.mSecSince());
      myState = STATE_FAILED;
      myGripper->gripOpen();
      ArUtil::sleep(3);
      myGripper->liftUp();
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      return &myDesired;
    }
  } 
  else
    myLastSeen.setToNow();

  if (blobSeen)
  {
    xRel = (double)(blob.getXCG() - WIDTH/2.0) / (double)WIDTH + CAMERA_X_CORRECTION;
    yRel = -(double)(blob.getYCG() - HEIGHT/2.0) / (double)HEIGHT;
    //printf("xRel %.3f yRel %.3f:  ", xRel, yRel);
  } 
  else
  {
    //printf("No blob: ");
  }

  if (blobSeen && yRel < -.2  && !myPointedDown)
  {
    printf("PickUp: Pointing the camera down!!!\n");
    myAMPTU->panTilt(0, -75);
    myPointedDown = true;
  }
  
  
  if (!blobSeen || ArMath::fabs(xRel) < .001)
  {
    //printf("Going straight ahead\n");
    myDesired.setDeltaHeading(0);
  }
  else
  {
    //printf("Turning %.2f\n", -xRel * 30);
    if (-xRel > 1.0)
      myDesired.setDeltaHeading(30);
    else if (-xRel < -1.0)
      myDesired.setDeltaHeading(-30);
    else
      myDesired.setDeltaHeading(-xRel * 30);
  }
  if (myRobot->isLeftTableSensingIRTriggered() ||
      myRobot->isRightTableSensingIRTriggered())
    myDesired.setVel(50);
  else
    myDesired.setVel(100);
  return &myDesired;
}

bool PickUp::setChannel(int channel)
{
  if (channel >= 1 && channel <= ArACTS_1_2::NUM_CHANNELS)
  {
    myChannel = channel;
    myState = STATE_START_LOOKING;
    return true;
  }
  else
    return false;
}

class DropOff : public ArAction
{
public:
  enum State {
    STATE_START_LOOKING,
    STATE_LOOKING,
    STATE_FAILED,
    STATE_SUCCEEDED
  };
  DropOff(ArACTS_1_2 *acts, ArGripper *gripper, ArPTZ *amptu);
  ~DropOff(void);
  virtual ArActionDesired *fire(ArActionDesired currentDesired);
  bool setChannel(int channel);
  State getState(void) { return myState; }
  enum {
    WIDTH = 160,
    HEIGHT = 120
  };
protected:
  ArActionDesired myDesired;
  ArACTS_1_2 *myActs;
  ArGripper *myGripper;
  ArPTZ *myAMPTU;
  int myChannel;
  bool myPickup;
  bool myTried;
  bool myPointedDown;
  State myState;
  ArPose myLastPose;
  ArTime myLastMoved;
  ArTime myLastSeen;
  ArTime mySentGripper;
  ArTime myTriedStart;
  bool myWaitingOnGripper;
  bool myWaitingOnLower;
  bool myWaitingOnRaised;
  bool myStartRaised;
  bool myLowered;
};

DropOff::DropOff(ArACTS_1_2 *acts, ArGripper *gripper, ArPTZ *amptu) :
    ArAction("DropOff", "Picks up something.")
{
  myActs = acts;
  myGripper = gripper;
  myAMPTU = amptu;
  myChannel = 0;
  myState = STATE_FAILED;
  //setChannel(2);
}

DropOff::~DropOff(void)
{

}

ArActionDesired *DropOff::fire(ArActionDesired currentDesired)
{
  ArPose pose;
  ArACTSBlob blob;
  bool blobSeen = false;
  double xRel, yRel;
  double dist;


  myDesired.reset();
  if (myState == STATE_SUCCEEDED)
  {
    printf("DropOff: Succeeded\n");
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  else if (myState == STATE_FAILED)
  {
    printf("DropOff: Failed\n");
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }

  if (myActs->getNumBlobs(myChannel) > 0 &&
      (blobSeen = myActs->getBlob(myChannel, 1, &blob)))
  {
    myLastSeen.setToNow();
  }

  // this if the stuff we want to do if we're not going to just drive forward
  // and home in on the color, ie the pickup-specific stuff
  if (myState == STATE_START_LOOKING)
  {
    mySentGripper.setToNow();
    myPointedDown = false;
    myState = STATE_LOOKING;
    myLastSeen.setToNow();
    myTried = false;
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
    //myWaitingOnGripper = true;
    myWaitingOnLower = false;
    myLowered = false;
    myWaitingOnRaised = false;
    myStartRaised = false;
    printf("@@@@@ DropOff: Starting\n");
  }

  // we want to sit still until the lift is down or for a second and a half
  /*
  if (myWaitingOnGripper) 
  {
    if (mySentGripper.mSecSince() < 500 || 
	(myGripper->getGripState() != 1
	 && mySentGripper.mSecSince() < 4000))
    {
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      myLastMoved.setToNow();
      myLastPose = myRobot->getPose();
      return &myDesired;
    }
    else
    {
      myWaitingOnGripper = false;
    }
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  } 
  */
  //printf("sensors %d %d stall %d %d\n",!(myRobot->getDigIn() & ArUtil::BIT2),
  //!(myRobot->getDigIn() & ArUtil::BIT3),
  //myRobot->isLeftMotorStalled(), myRobot->isRightMotorStalled());
  if ((myRobot->isLeftBreakBeamTriggered() &&
       myRobot->isRightBreakBeamTriggered()) ||
       myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
  {
    if (!myWaitingOnLower && !myLowered && !myWaitingOnRaised)
    {
      myWaitingOnLower = true;
      printf("DropOff: Lowering gripper\n");
      mySentGripper.setToNow();
    }
  }
  if (myWaitingOnLower)
  {
    /// TODO
    if (mySentGripper.mSecSince() < 600 || 
	(!myGripper->isLiftMaxed() && mySentGripper.mSecSince() < 20000))
    {
      myGripper->liftDown();
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      myLastMoved.setToNow();
      myLastPose = myRobot->getPose();
      return &myDesired;
    }
    else
    {
      printf("DropOff: Lowered!\n");
      myWaitingOnLower = false;
      myWaitingOnRaised = true;
      myStartRaised = true;
    }
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  }

  if (myWaitingOnRaised)
  {
    if (myStartRaised)
    {
      printf("DropOff: Raising gripper a little bit\n");
      myGripper->liftCarry(15);
      mySentGripper.setToNow();
      myStartRaised = false;
    }
    if (mySentGripper.mSecSince() > 1000)
    {
      printf("DropOff: Raised the gripper a little bit\n");
      myWaitingOnRaised = false;
      myLowered = true;
    }
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
    return &myDesired;
  }

  if (myLowered)
  {
    if (!myTried)
    {
      printf("DropOff: Trying to let go of the block.\n");
      myTriedStart.setToNow();
      myTried = true;
    }
    myGripper->gripOpen();
    if (myGripper->getGripState() == 1 || myTriedStart.mSecSince() > 3000)
    {
      printf("DropOff: Succeeded, dropped off the block.\n");
      myGripper->liftUp();
      myState = STATE_SUCCEEDED;
    }
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }

  pose = myRobot->getPose();
  dist = myLastPose.findDistanceTo(pose);
  if (dist < 5 && myLastMoved.mSecSince() > 2500)
  {
    printf("DropOff: Failed, no movement in the last 2500 msec.\n");
    myState = STATE_FAILED;
    myGripper->gripOpen();
    myGripper->liftUp();
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
  else if (dist > 5)
  {
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
  }

  if (!blobSeen)
  {
    if (((!myPointedDown && myLastSeen.mSecSince() > 1500) ||
	 (myPointedDown && myLastSeen.mSecSince() > 4000)) &&
	myGripper->getBreakBeamState() == 0)
    {
      printf("DropOff:  Lost the blob, failed, last saw it %ld msec ago.\n",
	     myLastSeen.mSecSince());
      myState = STATE_FAILED;
      myGripper->gripOpen();
      ArUtil::sleep(3);
      myGripper->liftUp();
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      return &myDesired;
    }
  } 
  else
    myLastSeen.setToNow();

  if (blobSeen)
  {
    xRel = (double)(blob.getXCG() - WIDTH/2.0) / (double)WIDTH + CAMERA_X_CORRECTION;
    yRel = -(double)(blob.getYCG() - HEIGHT/2.0) / (double)HEIGHT;
    //printf("xRel %.3f yRel %.3f:  ", xRel, yRel);
  } 
  else
  {
    //printf("No blob: ");
  }

  if (blobSeen && yRel < -.2  && !myPointedDown)
  {
    printf("DropOff: Pointing the camera down!!!\n");
    myAMPTU->panTilt(0, -75);
    myPointedDown = true;
  }
  
  
  if (!blobSeen || ArMath::fabs(xRel) < .001)
  {
    //printf("Going straight ahead\n");
    myDesired.setDeltaHeading(0);
  }
  else
  {
    //printf("Turning %.2f\n", -xRel * 30);
    if (-xRel > 1.0)
      myDesired.setDeltaHeading(30);
    else if (-xRel < -1.0)
      myDesired.setDeltaHeading(-30);
    else
      myDesired.setDeltaHeading(-xRel * 30);
  }
  if (myRobot->isLeftTableSensingIRTriggered() ||
      myRobot->isRightTableSensingIRTriggered())
    myDesired.setVel(50);
  else
    myDesired.setVel(100);
  return &myDesired;
}

bool DropOff::setChannel(int channel)
{
  if (channel >= 1 && channel <= ArACTS_1_2::NUM_CHANNELS)
  {
    myChannel = channel;
    myState = STATE_START_LOOKING;
    return true;
  }
  else
    return false;
}

class Acquire : public ArAction
{
public:
  enum State {
    STATE_START_LOOKING,
    STATE_LOOKING,
    STATE_FAILED,
    STATE_SUCCEEDED
  };
  Acquire(ArACTS_1_2 *acts, ArGripper *gripper);
  virtual ~Acquire(void);
  virtual ArActionDesired *fire(ArActionDesired currentDesired);
  bool setChannel(int channel);
  State getState(void) { return myState; }
protected:
  State myState;
  int myChannel;
  ArSectors myFirstTurn;
  ArSectors mySecondTurn;
  ArActionDesired myDesired;
  ArACTS_1_2 *myActs;
  ArGripper *myGripper;
  ArTime myStartUp;
  bool myTryingGripper;
};

Acquire::Acquire(ArACTS_1_2 *acts, ArGripper *gripper) :
  ArAction("Acquire", "Turns until it can find the given channel, gives up after 1 revolution")
{
  myActs = acts;
  myGripper = gripper;
  myState = STATE_FAILED;
}

Acquire::~Acquire(void)
{
}

ArActionDesired *Acquire::fire(ArActionDesired currentDesired)
{
  myDesired.reset();
  myDesired.setVel(0);
  //printf("%d %d %d\n", myActs->getNumBlobs(1),myActs->getNumBlobs(2),
  //myActs->getNumBlobs(3));
  switch (myState) {
  case STATE_START_LOOKING:
    myFirstTurn.clear();
    mySecondTurn.clear();
    myState = STATE_LOOKING;
    myGripper->liftUp();
    myGripper->gripClose();
    printf("Acquire: Raising lift\n");
    myStartUp.setToNow();
    myTryingGripper = true;
  case STATE_LOOKING:
    /// TODO
    if (myTryingGripper && (myStartUp.mSecSince() < 600 ||
			    ((!myGripper->isLiftMaxed() ||
			      myGripper->getGripState() != 2) && 
			     myStartUp.mSecSince() < 20000)))
    {
      myGripper->liftUp();
      myGripper->gripClose();
      myDesired.setVel(0);
      myDesired.setDeltaHeading(0);
      return &myDesired;
    } 
    else if (myTryingGripper)
    {
      printf("Acquire: Done raising lift %ld after started.\n", 
	     myStartUp.mSecSince());
      myTryingGripper = false;
    }
    if (myActs->getNumBlobs(myChannel) > 0)
    {
      myDesired.setDeltaHeading(0);
      myState = STATE_SUCCEEDED;
      printf("Acquire: Succeeded!\n");
    }
    else if (myFirstTurn.didAll() && mySecondTurn.didAll())
    {
      myDesired.setDeltaHeading(0);
      myState = STATE_FAILED;
      printf("Acquire: Did two revolutions, didn't see the blob, Failed!\n");
    } 
    else
    {
      myFirstTurn.update(myRobot->getTh());
      if (myFirstTurn.didAll())
	mySecondTurn.update(myRobot->getTh());
      myDesired.setDeltaHeading(8);
    }
    return &myDesired;
  default:
    myDesired.setVel(0);
    myDesired.setDeltaHeading(0);
    return &myDesired;
  }
}

bool Acquire::setChannel(int channel)
{
  if (channel >= 1 && channel <= ArACTS_1_2::NUM_CHANNELS)
  {
    myChannel = channel;
    myState = STATE_START_LOOKING;
    return true;
  }
  else
    return false;
}

class TakeBlockToWall
{
public:
  enum State { 
    STATE_START,
    STATE_ACQUIRE_BLOCK,
    STATE_PICKUP_BLOCK,
    STATE_BACKUP,
    STATE_FORWARD,
    STATE_ACQUIRE_BLOCK2,
    STATE_PICKUP_BLOCK2,
    STATE_PICKUP_BACKUP,
    STATE_ACQUIRE_DROP_WALL,
    STATE_DRIVETO_DROP_WALL,
    STATE_DROP_BACKUP,
    STATE_ACQUIRE_LAP_WALL,
    STATE_DRIVETO_LAP_WALL,
    STATE_BACKUP_LAP_WALL,
    STATE_FORWARD_LAP_WALL,
    STATE_SWITCH,
    STATE_FAILED
  };
  TakeBlockToWall(ArRobot *robot, ArGripper *gripper, ArPTZ *amptu,
		  Acquire *acquire, DriveTo *driveTo, PickUp *pickUp,
		  DropOff *dropOff,
		  ArActionTableSensorLimiter *tableLimiter);
  ~TakeBlockToWall(void);
  void handler(void);
  void setState(State state);
  enum Color {
    COLOR_FIRST_WALL = 1,
    COLOR_SECOND_WALL = 3,
    COLOR_BLOCK = 2
  };
    
protected:
  ArTime myStateStartTime;
  ArPose myStateStartPos;
  ArRobot *myRobot;
  ArFunctorC<TakeBlockToWall> myHandlerCB;
  ArGripper *myGripper;
  ArPTZ *myAMPTU;
  Acquire *myAcquire;
  DriveTo *myDriveTo;
  DropOff *myDropOff;
  PickUp *myPickUp;
  ArActionTableSensorLimiter *myTableLimiter;
  State myState;
  Color myDropWall;
  Color myLapWall;
  bool myNewState;
  bool myGripOpenSent;
};

TakeBlockToWall::TakeBlockToWall(ArRobot *robot, ArGripper *gripper, 
				 ArPTZ *amptu, Acquire *acquire, 
				 DriveTo *driveTo, PickUp *pickUp,
				 DropOff *dropOff,
				 ArActionTableSensorLimiter *tableLimiter) :
  myHandlerCB(this, &TakeBlockToWall::handler)
{
  myRobot = robot;
  myRobot->addUserTask("TakeBlockToWall", 75, &myHandlerCB);
  myGripper = gripper;
  myAcquire = acquire;
  myDriveTo = driveTo;
  myDropOff = dropOff;
  myTableLimiter = tableLimiter;
  myPickUp = pickUp;
  myAMPTU = amptu;
  myState = STATE_START;
  myNewState = true;
}

TakeBlockToWall::~TakeBlockToWall(void)
{
}

void TakeBlockToWall::setState(State state)
{
  myState = state;
  myNewState = true; 
  myStateStartTime.setToNow();
  myStateStartPos = myRobot->getPose();
}


void TakeBlockToWall::handler(void)
{
  Color tempColor;

  switch (myState) 
  {
  case STATE_START:
    setState(STATE_ACQUIRE_BLOCK);
    myDropWall = COLOR_FIRST_WALL;
    myLapWall = COLOR_SECOND_WALL;
    printf("!! Started state handling!\n");
    //handler();
    return;
    break;
  case STATE_ACQUIRE_BLOCK:
    if (myNewState)
    {
      printf("!! Acquire block\n");
      myNewState = false;
      myAMPTU->panTilt(0, -40);
      myAcquire->activate();
      myAcquire->setChannel(COLOR_BLOCK);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myGripper->getGripState() == 2 &&
	myGripper->getBreakBeamState() != 0)
    {
      printf("###### AcquireBlock: Successful (have cube?)\n");
      setState(STATE_PICKUP_BACKUP);
      //handler();
      return;
    } 
    else if (myGripper->getBreakBeamState() != 0)
    {
      printf("###### AcquireBlock: Successful (cube in gripper?)\n");
      setState(STATE_PICKUP_BLOCK);
      //handler();
      return;
    }
    if (myAcquire->getState() == Acquire::STATE_FAILED || 
	myStateStartTime.mSecSince() > 35000)
    {
      printf("###### AcqiureBlock: failed\n");
      setState(STATE_BACKUP);
      //handler();
      return;
    }
    else if (myAcquire->getState() == Acquire::STATE_SUCCEEDED)
    {
      printf("###### AcquireBlock: successful\n");
      setState(STATE_PICKUP_BLOCK);
      //handler();
      return;
    }
    break;
  case STATE_PICKUP_BLOCK:
    if (myNewState)
    {
      printf("!! Pickup block\n");
      myNewState = false;
      myAMPTU->panTilt(0, -35);
      myAcquire->deactivate();
      myPickUp->activate();
      myPickUp->setChannel(COLOR_BLOCK);
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myPickUp->getState() == PickUp::STATE_FAILED)
    {
      printf("###### PickUpBlock: failed\n");
      setState(STATE_BACKUP);
      //handler();
      return;
    }
    else if (myPickUp->getState() == PickUp::STATE_SUCCEEDED)
    {
      printf("###### PickUpBlock: successful\n");
      setState(STATE_PICKUP_BACKUP);
      //handler();
      return;
    }
    break;
  case STATE_BACKUP:
    if (myNewState)
    {
      myNewState = false;
      myRobot->move(BACKUP_DIST * .75);
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
    {
      printf("###### Backup: Failed, going forwards\n");
      myRobot->clearDirectMotion();
      setState(STATE_FORWARD);      
    }
    if (myStateStartTime.mSecSince() > BACKUP_TIME || 
	myStateStartPos.findDistanceTo(myRobot->getPose()) > BACKUP_DIST * .95 * .75)
    {
      printf("###### Backup: Succeeded\n");
      myRobot->clearDirectMotion();
      setState(STATE_ACQUIRE_BLOCK2);
      //handler();
      return;
    }
    break;
  case STATE_FORWARD:
    if (myNewState)
    {
      myNewState = false;
      myRobot->move(-BACKUP_DIST * .75);
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
    {
      printf("###### Forward: Failed\n");
      myRobot->clearDirectMotion();
      setState(STATE_FAILED);      
    }
    if (myStateStartTime.mSecSince() > BACKUP_TIME || 
	myStateStartPos.findDistanceTo(myRobot->getPose()) > 
	ArMath::fabs(BACKUP_DIST * .95 * .75))
    {
      printf("###### Forward: Succeeded\n");
      myRobot->clearDirectMotion();
      setState(STATE_ACQUIRE_BLOCK2);
      //handler();
      return;
    }
    break;
  case STATE_ACQUIRE_BLOCK2:
    if (myNewState)
    {
      printf("!! Acquire block 2\n");
      myNewState = false;
      myAMPTU->panTilt(0, -40);
      myAcquire->activate();
      myAcquire->setChannel(COLOR_BLOCK);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myGripper->getGripState() == 2 &&
	myGripper->getBreakBeamState() != 0)
    {
      printf("###### AcquireBlock2: Successful (have cube?)\n");
      setState(STATE_PICKUP_BACKUP);
      //handler();
      return;
    }
    else if (myGripper->getBreakBeamState() != 0)
    {
      printf("###### AcquireBlock2: Successful (cube in gripper?)\n");
      setState(STATE_PICKUP_BLOCK2);
      //handler();
      return;
    }
    if (myAcquire->getState() == Acquire::STATE_FAILED ||
	myStateStartTime.mSecSince() > 35000)
    {
      printf("###### AcqiureBlock2: failed\n");
      setState(STATE_FAILED);
      //handler();
      return;
    }
    else if (myAcquire->getState() == Acquire::STATE_SUCCEEDED)
    {
      printf("###### AcquireBlock2: successful\n");
      setState(STATE_PICKUP_BLOCK2);
      //handler();
      return;
    }
    break;
  case STATE_PICKUP_BLOCK2:
    if (myNewState)
    {
      printf("!! Pickup block 2\n");
      myNewState = false;
      myAcquire->deactivate();
      myPickUp->activate();
      myAMPTU->panTilt(0, -55);
      myPickUp->setChannel(COLOR_BLOCK);
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myPickUp->getState() == PickUp::STATE_FAILED)
    {
      printf("###### PickUpBlock2: failed\n");
      setState(STATE_FAILED);
      //handler();
      return;
    }
    else if (myPickUp->getState() == PickUp::STATE_SUCCEEDED)
    {
      printf("###### PickUpBlock2: successful\n");
      setState(STATE_PICKUP_BACKUP);
      //handler();
      return;
    }
    break;
  case STATE_PICKUP_BACKUP:
    if (myNewState)
    {
      myNewState = false;
      myRobot->move(BACKUP_DIST);
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myStateStartTime.mSecSince() > BACKUP_TIME || 
	myStateStartPos.findDistanceTo(myRobot->getPose()) > 
	ArMath::fabs(BACKUP_DIST * .95))
    {
      printf("###### PickUp_BackUp: done\n");
      myRobot->clearDirectMotion();
      setState(STATE_ACQUIRE_DROP_WALL);
      //handler();
      return;
    }
    break;
  case STATE_ACQUIRE_DROP_WALL:
    if (myNewState)
    {
      printf("!! Acquire Drop wall, channel %d\n", myDropWall);
      myNewState = false;
      myAMPTU->panTilt(0, -30);
      myAcquire->activate();
      myAcquire->setChannel(myDropWall);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myGripper->getGripState() != 2 || 
	myGripper->getBreakBeamState() == 0)
    {
      printf("###### AcquireDropWall:: failed (lost cube %d %d)\n",
	     myGripper->getGripState(), myGripper->getBreakBeamState());
      setState(STATE_BACKUP);	       
      //handler();
      return;
    }
    if (myAcquire->getState() == Acquire::STATE_FAILED ||
	myStateStartTime.mSecSince() > 35000)
    {
      printf("###### AcquireDropWall:: failed\n");
      setState(STATE_FAILED);
      //handler();
      return;
    }
    else if (myAcquire->getState() == Acquire::STATE_SUCCEEDED)
    {
      printf("###### AcquireDropWall: successful\n");
      setState(STATE_DRIVETO_DROP_WALL);
      //handler();
      return;
    }
    break;
  case STATE_DRIVETO_DROP_WALL:
    if (myNewState)
    {
      printf("!! DropOff Drop wall, channel %d\n", myDropWall);
      myNewState = false;
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->activate();
      myDropOff->setChannel(myDropWall);
      myTableLimiter->deactivate();
    }
    if (myDropOff->getState() == DropOff::STATE_FAILED)
    {
      printf("###### DropOffDropWall: failed\n");
      setState(STATE_FAILED);
      //handler();
      return;
    }
    else if (myDropOff->getState() == DropOff::STATE_SUCCEEDED)
    {
      printf("###### DropOffDropWall: succesful\n");
      setState(STATE_DROP_BACKUP);
      //handler();
      return;
    }
    break;
  case STATE_DROP_BACKUP:
    if (myNewState)
    {
      myNewState = false;
      myRobot->move(BACKUP_DIST);
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myStateStartTime.mSecSince() > BACKUP_TIME || 
	myStateStartPos.findDistanceTo(myRobot->getPose()) > 
	ArMath::fabs(BACKUP_DIST * .95))
    {
      printf("###### Drop_Backup: done\n");
      myRobot->clearDirectMotion();
      setState(STATE_ACQUIRE_LAP_WALL);
      //handler();
      return;
    }
    break;
  case STATE_ACQUIRE_LAP_WALL:
    if (myNewState)
    {
      printf("!! Acquire Lap wall, channel %d\n", myLapWall);
      myNewState = false;
      myAMPTU->panTilt(0, -30);
      myAcquire->activate();
      myAcquire->setChannel(myLapWall);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->activate();
    }
    if (myAcquire->getState() == Acquire::STATE_FAILED ||
	myStateStartTime.mSecSince() > 35000)
    {
      printf("###### AcquireLapWall:: failed\n");
      setState(STATE_SWITCH);
      //handler();
      return;
    }
    else if (myAcquire->getState() == Acquire::STATE_SUCCEEDED)
    {
      printf("###### AcquireLapWall: successful\n");
      setState(STATE_DRIVETO_LAP_WALL);
      //handler();
      return;
    }
    break;
  case STATE_DRIVETO_LAP_WALL:
    if (myNewState)
    {
      printf("!! Driveto Lap wall, channel %d\n", myLapWall);
      myNewState = false;
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->activate();
      myDriveTo->setChannel(myLapWall);
      myDropOff->deactivate();
      myTableLimiter->activate();
    }
    if (myDriveTo->getState() == DriveTo::STATE_FAILED)
    {
      printf("###### DriveToLapWall: failed\n");
      setState(STATE_BACKUP_LAP_WALL);
      //handler();
      return;
    }
    else if (myDriveTo->getState() == DriveTo::STATE_SUCCEEDED)
    {
      printf("###### DriveToLapWall: succesful\n");
      setState(STATE_BACKUP_LAP_WALL);
      //handler();
      return;
    }
    break;
  case STATE_BACKUP_LAP_WALL:
    if (myNewState)
    {
      myNewState = false;
      myRobot->move(BACKUP_DIST * .75);
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
    {
      printf("###### BackupLapWall: Failed, going forwards\n");
      myRobot->clearDirectMotion();
      setState(STATE_FORWARD_LAP_WALL);      
    }
    if (myStateStartTime.mSecSince() > BACKUP_TIME || 
	myStateStartPos.findDistanceTo(myRobot->getPose()) > 
	ArMath::fabs(BACKUP_DIST * .95 * .75))
    {
      printf("###### BackupLapWall: Succeeded\n");
      myRobot->clearDirectMotion();
      setState(STATE_SWITCH);
      //handler();
      return;
    }
    break;
  case STATE_FORWARD_LAP_WALL:
    if (myNewState)
    {
      myNewState = false;
      myRobot->move(-BACKUP_DIST * .75);
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myDropOff->deactivate();
      myTableLimiter->deactivate();
    }
    if (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
    {
      printf("###### ForwardLapWall: Failed\n");
      myRobot->clearDirectMotion();
      setState(STATE_FAILED);      
    }
    if (myStateStartTime.mSecSince() > BACKUP_TIME || 
	myStateStartPos.findDistanceTo(myRobot->getPose()) > 
	ArMath::fabs(BACKUP_DIST * .95 * .75))
    {
      printf("###### ForwardLapWall: Succeeded\n");
      myRobot->clearDirectMotion();
      setState(STATE_SWITCH);
      //handler();
      return;
    }
    break;

  case STATE_SWITCH:
    printf("!! Switching walls around.\n");
    tempColor = myDropWall;
    myDropWall = myLapWall;
    myLapWall = tempColor;
    setState(STATE_ACQUIRE_BLOCK);
    //handler();
    return;
  case STATE_FAILED:
    printf("@@@@@ Failed to complete the task!\n");
    myRobot->comInt(ArCommands::SONAR, 0);
    ArUtil::sleep(50);
    myRobot->comStr(ArCommands::SAY, "\52\77\37\62\42\70");
    ArUtil::sleep(500);
    Aria::shutdown();
    myRobot->disconnect();
    myRobot->stopRunning();
    return;
  }

}

int main(void)
{
  ArSerialConnection con;
  ArRobot robot;
  int ret;
  std::string str;
  ArActionLimiterForwards limiter("speed limiter near", 225, 600, 250);
  ArActionLimiterForwards limiterFar("speed limiter far", 225, 1100, 400);
  ArActionTableSensorLimiter tableLimiter;
  ArActionLimiterBackwards backwardsLimiter;
  ArActionConstantVelocity stop("stop", 0);
  ArSonarDevice sonar;
  ArACTS_1_2 acts;
  ArPTZ *ptz;
  ptz = new ArVCC4(&robot, true);
  ArGripper gripper(&robot);
  
  Acquire acq(&acts, &gripper);
  DriveTo driveTo(&acts, &gripper, ptz);
  DropOff dropOff(&acts, &gripper, ptz);
  PickUp pickUp(&acts, &gripper, ptz);
  

  TakeBlockToWall takeBlock(&robot, &gripper, ptz, &acq, &driveTo, &pickUp,
			    &dropOff, &tableLimiter);

  if (!acts.openPort(&robot))
  {
    printf("Could not connect to acts, exiting\n");
    exit(0);    
  }
  Aria::init();
  
  robot.addRangeDevice(&sonar);
  //con.setBaud(38400);
  if ((ret = con.open()) != 0)
  {
    str = con.getOpenMessage(ret);
    printf("Open failed: %s\n", str.c_str());
    Aria::shutdown();
    return 1;
  }

  robot.setDeviceConnection(&con);
  if (!robot.blockingConnect())
  {
    printf("Could not connect to robot... exiting\n");
    Aria::shutdown();
    return 1;
  }

  ptz->init();
  ArUtil::sleep(8000);
  printf("### 2222\n");
  ptz->panTilt(0, -40);
  printf("### whee\n");
  ArUtil::sleep(8000);
  robot.setAbsoluteMaxTransVel(400);

  robot.setStateReflectionRefreshTime(250);
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::SOUNDTOG, 0);

  ArUtil::sleep(200);
  robot.addAction(&tableLimiter, 100);
  robot.addAction(&limiter, 99);
  robot.addAction(&limiterFar, 98);
  robot.addAction(&backwardsLimiter, 97);
  robot.addAction(&acq, 77);
  robot.addAction(&driveTo, 76);
  robot.addAction(&pickUp, 75);
  robot.addAction(&dropOff, 74);
  robot.addAction(&stop, 30);

  robot.run(true);
  
  Aria::shutdown();
  return 0;
}
