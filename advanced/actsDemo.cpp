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
class DriveTo : public ArAction
{
public:
  enum State {
    STATE_START_LOOKING,
    STATE_LOOKING,
    STATE_FAILED,
    STATE_SUCCEEDED
  };
  DriveTo(ArACTS_1_2 *acts, ArGripper *gripper, ArSonyPTZ *sony);
  ~DriveTo(void);
  ArActionDesired *fire(ArActionDesired currentDesired);
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
  ArSonyPTZ *mySony;
  int myChannel;
  bool myPickup;
  State myState;
  ArTime myLastSeen;
};

DriveTo::DriveTo(ArACTS_1_2 *acts, ArGripper *gripper, ArSonyPTZ *sony) :
    ArAction("DriveTo", "Drives to something.")
{
  myActs = acts;
  myGripper = gripper;
  mySony = sony;
  myChannel = 0;
  myState = STATE_FAILED;
  setChannel(1);
}

DriveTo::~DriveTo(void)
{

}

ArActionDesired *DriveTo::fire(ArActionDesired currentDesired)
{
  ArACTSBlob blob;
  double xRel, yRel;

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

  xRel = (double)(blob.getXCG() - WIDTH/2.0) / (double)WIDTH;
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
    //printf("Turning %.2f\n", -xRel * 10);
    myDesired.setDeltaHeading(-xRel * 10);
  }
  myDesired.setVel(300);
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
  PickUp(ArACTS_1_2 *acts, ArGripper *gripper, ArSonyPTZ *sony);
  ~PickUp(void);
  ArActionDesired *fire(ArActionDesired currentDesired);
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
  ArSonyPTZ *mySony;
  int myChannel;
  bool myPickup;
  bool myTried;
  bool myPointedDown;
  State myState;
  ArPose myLastPose;
  ArTime myLastMoved;
  ArTime myLastSeen;
  ArTime mySentLiftDown;
  ArTime myTriedStart;
  bool myWaitingOnGripper;
};

PickUp::PickUp(ArACTS_1_2 *acts, ArGripper *gripper, ArSonyPTZ *sony) :
    ArAction("PickUp", "Picks up something.")
{
  myActs = acts;
  myGripper = gripper;
  mySony = sony;
  myChannel = 0;
  myState = STATE_FAILED;
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

  // this if the stuff we want to do if we're not going to just drive forward
  // and home in on the color, ie the pickup-specific stuff
  if (myState == STATE_START_LOOKING)
  {
    myGripper->gripOpen();
    ArUtil::sleep(3);
    myGripper->liftDown();
    mySentLiftDown.setToNow();
    myPointedDown = false;
    myState = STATE_LOOKING;
    myLastSeen.setToNow();
    myTried = false;
    myLastMoved.setToNow();
    myLastPose = myRobot->getPose();
    myWaitingOnGripper = true;
    printf("@@@@@ Pickup: Lowering lift\n");
  }

  // we want to sit still until the lift is down or for a second and a half
  if (myWaitingOnGripper) 
  {
    if (mySentLiftDown.mSecSince() < 500 || 
	((!myGripper->isLiftMaxed() || myGripper->getGripState() != 2)
	 && mySentLiftDown.mSecSince() < 5000))
    {
      myGripper->liftDown();
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

  if (myGripper->getBreakBeamState() != 0)
  {
    if (myGripper->getGripState() == 2)
    {
      printf("PickUp: Succeeded, have block.\n");
      myGripper->liftUp();
      myState = STATE_SUCCEEDED;
    }
    else if (!myTried)
    {
      myGripper->gripClose();
      printf("PickUp: Trying to pick up.\n");
      myTried = true;
      myTriedStart.setToNow();
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
  if (dist < 5 && myLastMoved.mSecSince() > 1500)
  {
    printf("PickUp: Failed, no movement in the last 1500 msec.\n");
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

  if (myActs->getNumBlobs(myChannel) == 0 || 
      !(blobSeen = myActs->getBlob(myChannel, 1, &blob)))
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
    xRel = (double)(blob.getXCG() - WIDTH/2.0) / (double)WIDTH;
    yRel = (double)(blob.getYCG() - HEIGHT/2.0) / (double)HEIGHT;
    //printf("xRel %.3f yRel %.3f:\n", xRel, yRel);
  } 
  else
  {
    //printf("No blob: ");
  }

  if (blobSeen && yRel < 0.2 && !myPointedDown)
  {
    printf("PickUp: Pointing the camera down!!!\n");
    mySony->panTilt(0, -ArSonyPTZ::MAX_TILT);
    myPointedDown = true;
  }
  
  
  if (!blobSeen || ArMath::fabs(xRel) < .10)
  {
    //printf("Going straight ahead\n");
    myDesired.setDeltaHeading(0);
  }
  else
  {
    //printf("Turning %.2f\n", -xRel * 10);
    if (ArMath::fabs(-xRel * 10) <= 10)
      myDesired.setDeltaHeading(-xRel * 10);
    else if (-xRel > 0)
      myDesired.setDeltaHeading(10);
    else
      myDesired.setDeltaHeading(-10);
  }
  myDesired.setVel(150);
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
  ArActionDesired *fire(ArActionDesired currentDesired);
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
    if (myTryingGripper && (myStartUp.mSecSince() < 600 ||
			    ((!myGripper->isLiftMaxed() ||
			      myGripper->getGripState() != 2) && 
			     myStartUp.mSecSince() < 5000)))
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
    STATE_ACQUIRE_BLOCK2,
    STATE_PICKUP_BLOCK2,
    STATE_ACQUIRE_DROP_WALL,
    STATE_DRIVETO_DROP_WALL,
    STATE_DROP,
    STATE_DROP_BACKUP,
    STATE_ACQUIRE_LAP_WALL,
    STATE_DRIVETO_LAP_WALL,
    STATE_SWITCH,
    STATE_FAILED
  };
  TakeBlockToWall(ArRobot *robot, ArGripper *gripper, ArSonyPTZ *sony,
		  Acquire *acquire, DriveTo *driveTo, PickUp *pickup,
		  ArActionConstantVelocity *backup);
  ~TakeBlockToWall(void);
  void handler(void);
  void setState(State state) 
    { myState = state; myNewState = true; myStateStart.setToNow(); }
  enum Color {
    COLOR_FIRST_WALL = 1,
    COLOR_SECOND_WALL = 3,
    COLOR_BLOCK = 2
  };
    
protected:
  ArTime myStateStart;
  ArRobot *myRobot;
  ArFunctorC<TakeBlockToWall> myHandlerCB;
  ArGripper *myGripper;
  ArSonyPTZ *mySony;
  Acquire *myAcquire;
  DriveTo *myDriveTo;
  PickUp *myPickUp;
  ArActionConstantVelocity *myBackup;
  State myState;
  Color myDropWall;
  Color myLapWall;
  bool myNewState;
  bool myGripOpenSent;
};

TakeBlockToWall::TakeBlockToWall(ArRobot *robot, ArGripper *gripper, 
				 ArSonyPTZ *sony, Acquire *acquire, 
				 DriveTo *driveTo, PickUp *pickUp,
				 ArActionConstantVelocity *backup) : 
  myHandlerCB(this, &TakeBlockToWall::handler)
{
  myRobot = robot;
  myRobot->addUserTask("TakeBlockToWall", 75, &myHandlerCB);
  myGripper = gripper;
  myAcquire = acquire;
  myDriveTo = driveTo;
  myPickUp = pickUp;
  mySony = sony;
  myBackup = backup;
  myState = STATE_START;
  myNewState = true;
}

TakeBlockToWall::~TakeBlockToWall(void)
{
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
      mySony->panTilt(0, -10);
      myAcquire->activate();
      myAcquire->setChannel(COLOR_BLOCK);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->deactivate();
    }
    if (myGripper->getGripState() == 2 &&
	myGripper->getBreakBeamState() != 0)
    {
      printf("###### AcquireBlock: Successful (have cube?)\n");
      setState(STATE_ACQUIRE_DROP_WALL);
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
	myStateStart.mSecSince() > 35000)
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
      mySony->panTilt(0, -15);
      myAcquire->deactivate();
      myPickUp->activate();
      myPickUp->setChannel(COLOR_BLOCK);
      myDriveTo->deactivate();
      myBackup->deactivate();
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
      setState(STATE_ACQUIRE_DROP_WALL);
      //handler();
      return;
    }
    break;
  case STATE_BACKUP:
    if (myNewState)
    {
      printf("!! Backup\n");
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->activate();
      myNewState = false;
    }
    if (myStateStart.mSecSince() > 2000)
    {
      printf("###### Backup: done\n");
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
      mySony->panTilt(0, -25);
      myAcquire->activate();
      myAcquire->setChannel(COLOR_BLOCK);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->deactivate();
    }
    if (myGripper->getGripState() == 2 &&
	myGripper->getBreakBeamState() != 0)
    {
      printf("###### AcquireBlock2: Successful (have cube?)\n");
      setState(STATE_ACQUIRE_DROP_WALL);
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
	myStateStart.mSecSince() > 35000)
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
      mySony->panTilt(0, -25);
      myPickUp->setChannel(COLOR_BLOCK);
      myDriveTo->deactivate();
      myBackup->deactivate();
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
      mySony->panTilt(0, -5);
      myAcquire->activate();
      myAcquire->setChannel(myDropWall);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->deactivate();
    }
    if (myGripper->getGripState() != 2 || 
	myGripper->getBreakBeamState() == 0)
    {
      printf("###### AcquireDropWall:: failed (lost cube)\n");
      setState(STATE_BACKUP);	       
      //handler();
      return;
    }
    if (myAcquire->getState() == Acquire::STATE_FAILED ||
	myStateStart.mSecSince() > 35000)
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
      printf("!! Driveto Drop wall, channel %d\n", myDropWall);
      myNewState = false;
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->activate();
      myDriveTo->setChannel(myDropWall);
      myBackup->deactivate();
    }
    if (myGripper->getGripState() != 2 || 
	myGripper->getBreakBeamState() == 0)
    {
      printf("###### DriveToDropWall:: failed (lost cube)\n");
      setState(STATE_BACKUP);
      //handler();
      return;
    }
    if (myDriveTo->getState() == DriveTo::STATE_FAILED)
    {
      printf("###### DriveToDropWall: failed\n");
      setState(STATE_FAILED);
      //handler();
      return;
    }
    else if (myDriveTo->getState() == DriveTo::STATE_SUCCEEDED)
    {
      printf("###### DriveToDropWall: succesful\n");
      setState(STATE_DROP);
      //handler();
      return;
    }
    break;
  case STATE_DROP:
    if (myNewState)
    {
      printf("!! Drop\n");
      printf("@@@@@ Drop lowering lift\n");
      myGripper->liftDown(); 
      myNewState = false;
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->deactivate();
      myGripOpenSent = false;
    }
    
    myGripper->liftDown();
    if ((myStateStart.mSecSince() > 500 && 
	 myGripper->isLiftMaxed() && myStateStart.mSecSince() < 5000) ||
	myStateStart.mSecSince() > 5000)
    {
      myGripper->gripOpen();
      /*if (!myGripOpenSent)
      {
	ArUtil::sleep(3);
	myGripper->gripOpen();
      }
      myGripOpenSent = true;
      */
    }
    if (myGripper->getGripState() == 1 || myStateStart.mSecSince() > 6000)
    {
      printf("###### Drop: success\n");
      setState(STATE_DROP_BACKUP);
      //handler();
      return;
    }
    break;
  case STATE_DROP_BACKUP:
    if (myNewState)
    {
      printf("!! Drop backup\n");
      myAcquire->deactivate();
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->activate();
      myNewState = false;
    }
    if (myStateStart.mSecSince() > 2000)
    {
      printf("###### Drop_Backup: done\n");
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
      mySony->panTilt(0, -5);
      myAcquire->activate();
      myAcquire->setChannel(myLapWall);
      myPickUp->deactivate();
      myDriveTo->deactivate();
      myBackup->deactivate();
    }
    if (myAcquire->getState() == Acquire::STATE_FAILED ||
	myStateStart.mSecSince() > 35000)
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
      myBackup->deactivate();
    }
    if (myDriveTo->getState() == DriveTo::STATE_FAILED)
    {
      printf("###### DriveToLapWall: failed\n");
      setState(STATE_SWITCH);
      //handler();
      return;
    }
    else if (myDriveTo->getState() == DriveTo::STATE_SUCCEEDED)
    {
      printf("###### DriveToLapWall: succesful\n");
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
  ArActionLimiterForwards limiter("speed limiter near", 300, 600, 250);
  ArActionLimiterForwards limiterFar("speed limiter far", 300, 1100, 400);
  ArActionLimiterBackwards backwardsLimiter;
  ArActionConstantVelocity stop("stop", 0);
  ArActionConstantVelocity backup("backup", -200);
  ArSonarDevice sonar;
  ArACTS_1_2 acts;
  ArSonyPTZ sony(&robot);
  ArGripper gripper(&robot, ArGripper::GENIO);
  
  Acquire acq(&acts, &gripper);
  DriveTo driveTo(&acts, &gripper, &sony);
  PickUp pickUp(&acts, &gripper, &sony);

  TakeBlockToWall takeBlock(&robot, &gripper, &sony, &acq, &driveTo, &pickUp,
			    &backup);

  Aria::init();

   if (!acts.openPort(&robot))
   {
     printf("Could not connect to acts\n");
     exit(1);
   }
  
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

  sony.init();
  ArUtil::sleep(1000);
  //robot.setAbsoluteMaxTransVel(400);

  robot.setStateReflectionRefreshTime(250);
  robot.comInt(ArCommands::ENABLE, 1);
  robot.comInt(ArCommands::SOUNDTOG, 0);

  ArUtil::sleep(200);
  robot.addAction(&limiter, 100);
  robot.addAction(&limiterFar, 99);
  robot.addAction(&backwardsLimiter, 98);
  robot.addAction(&acq, 77);
  robot.addAction(&driveTo, 76);
  robot.addAction(&pickUp, 75);
  robot.addAction(&backup, 50);
  robot.addAction(&stop, 30);

  robot.run(true);
  
  Aria::shutdown();
  return 0;
}
