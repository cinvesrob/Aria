#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeDrive.h"

AREXPORT ArServerModeDrive::ArServerModeDrive(ArServerBase *server, 
					      ArRobot *robot,
					      bool takeControlOnJoystick) : 
  ArServerMode(robot, server, "drive"),
  myDriveGroup(robot),
  myServerDriveJoystickCB(this, &ArServerModeDrive::serverDriveJoystick),
  myJoyUserTaskCB(this, &ArServerModeDrive::joyUserTask),
  myServerSafeDrivingEnableCB(this, 
			      &ArServerModeDrive::serverSafeDrivingEnable),
  myServerSafeDrivingDisableCB(this, 
			      &ArServerModeDrive::serverSafeDrivingDisable)
{
  myHandlerCommands = NULL;
  myTakeControlOnJoystick = takeControlOnJoystick;
  myDriveSafely = true;
  myJoydriveAction.setStopIfNoButtonPressed(false);
  myDriveGroup.addAction(&myJoydriveAction, 75);
  myDriveGroup.addAction(&myStopAction, 60);
  myInputAction = myDriveGroup.getActionInput();
  setThrottleParams(200, 2000);
  myExtraUnsafeAction = NULL;
  if (myServer != NULL)
  {
    addModeData("driveJoystick", 
		"drives the robot as with a joystick",
		&myServerDriveJoystickCB,
		"byte2: vel, byte2: rotVel",
		"none", "Movement", "RETURN_NONE");    
  }
  if ((myJoyHandler = Aria::getJoyHandler()) == NULL)
  {
    myJoyHandler = new ArJoyHandler;
    myJoyHandler->init();
    Aria::setJoyHandler(myJoyHandler);
  }
  myRobot->addUserTask("driveJoyUserTask", 75, &myJoyUserTaskCB);
}

AREXPORT ArServerModeDrive::~ArServerModeDrive()
{

}

AREXPORT void ArServerModeDrive::activate(void)
{
  driveJoystick(0, 0, true);
}

AREXPORT void ArServerModeDrive::deactivate(void)
{
  myDriveGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeDrive::setSafeDriving(bool safe)
{
  // if this is a change then print it
  if (safe != myDriveSafely)
  {
    if (safe)
    {
      ArLog::log(ArLog::Normal, "Driving safely again");
    }
    else
    {
      ArLog::log(ArLog::Normal, "Driving UNSAFELY");
    }
    myNewDriveSafely = true;
  }
  myDriveSafely = safe;
  if (isActive())
    driveJoystick(myVel, myRotVel);
}

AREXPORT bool ArServerModeDrive::getSafeDriving(void)
{
  return myDriveSafely;
}

AREXPORT void ArServerModeDrive::serverSafeDrivingEnable(void)
{
  myRobot->lock();
  setSafeDriving(true);
  myRobot->unlock();
}

AREXPORT void ArServerModeDrive::serverSafeDrivingDisable(void)
{
  myRobot->lock();
  setSafeDriving(false);
  myRobot->unlock();
}

AREXPORT void ArServerModeDrive::addControlCommands(ArServerHandlerCommands *handlerCommands)
{
  myHandlerCommands = handlerCommands;
  myHandlerCommands->addCommand(
	  "safeDrivingEnable",
	  "Enables safe driving, which will attempt to prevent collisions (default)",
	  &myServerSafeDrivingEnableCB, "UnsafeMovement");
  myHandlerCommands->addCommand(
	  "safeDrivingDisable",
	  "Disables safe driving, this is UNSAFE and will let you drive your robot into things or down stairs, use at your own risk",
	  &myServerSafeDrivingDisableCB, "UnsafeMovement");
}

AREXPORT void ArServerModeDrive::setThrottleParams(int lowSpeed, int highSpeed)
{
  myJoydriveAction.setThrottleParams(lowSpeed, highSpeed);
}
 

/**
 * @param vel Forward velocity
 * @param rotVel Rotational velocity
 * @param isActivating a bool set to true only if this method is called from the activate()
 * method, otherwise false
**/
AREXPORT void ArServerModeDrive::driveJoystick(double vel, double rotVel, bool isActivating)
{
  bool wasActive;
  wasActive = isActive();
 
  myVel = vel;
  myRotVel = rotVel;
 
  // KMC: Changed the following test to include isActivating.
  // if (!wasActive && !baseActivate())
  //  return;
 
  // The baseActivate() method should only be called in the context of the activate()
  // method.
  if (isActivating && !wasActive) {
    if (!baseActivate()) {
      return;
    }
  } // end if activating and wasn't previously active


  // This is to handle the case where ratioDrive is called outside the
  // activate() method, and the activation was not successful.
  if (!isActive()) {
     return;
  }
  
  if (!wasActive || myNewDriveSafely)
  {
    myRobot->clearDirectMotion();
    if (myDriveSafely)
    {
      myDriveGroup.activateExclusive();
      myMode = "Drive";
      ArLog::log(ArLog::Verbose, "Driving safely");
    }
    else
    {
      myRobot->deactivateActions();
      myJoydriveAction.activate();
      myInputAction->activate();
      if (myExtraUnsafeAction != NULL)
	      myExtraUnsafeAction->activate();
      myMode = "UNSAFE Drive";
      ArLog::log(ArLog::Verbose, "Driving unsafely");
    }
  }
  myNewDriveSafely = false;
  
  myVel = vel;
  myRotVel = rotVel;
  
  myDriveGroup.setVel(vel);
  myDriveGroup.setRotVel(rotVel);
  
  setActivityTimeToNow();
}

AREXPORT void ArServerModeDrive::serverDriveJoystick(ArServerClient *client, 
						     ArNetPacket *packet)
{
  double vel;
  double rotVel;

  vel = packet->bufToByte2();
  rotVel = packet->bufToByte2();
  myRobot->lock();
  
  // Activate if necessary.  Note that this is done before the ratioDrive
  // call because we want the new ratio values to be applied after the
  // default ones.
  if (!isActive()) {
    activate();
  }
  driveJoystick(vel, rotVel);
  myRobot->unlock();
}

AREXPORT void ArServerModeDrive::joyUserTask(void)
{
  // if we're not active but we should be
  if (myTakeControlOnJoystick && !isActive() && 
      myJoyHandler->haveJoystick() && myJoyHandler->getButton(1))
  {
    if (ArServerMode::getActiveMode() != NULL)
      ArLog::log(ArLog::Normal, 
		 "ArServerModeDrive: Activating instead of %s because of local joystick", ArServerMode::getActiveMode()->getName());
    else
      ArLog::log(ArLog::Normal, 
		 "ArServerModeDrive: Activating because of local joystick");
    activate();
  }
}

AREXPORT void ArServerModeDrive::userTask(void)
{
  // Sets the robot so that we always thing we're trying to move in
  // this mode
  myRobot->forceTryingToMove();

  if (myJoyHandler->haveJoystick() && myJoyHandler->getButton(1))
    setActivityTimeToNow();
  if (!myStatusSetThisCycle)
  {
    if (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
      myStatus = "Stalled";
    // this works because if the motors stalled above caught it, if
    // not and more values it means a stall
    else if (myRobot->getStallValue())
      myStatus = "Bumped";
    /// MPL 2014_04_17 centralizing all the places stopped is calculated
    //else if (ArMath::fabs(myRobot->getVel()) < 2 && 
    //ArMath::fabs(myRobot->getRotVel()) < 2)
    else if (myRobot->isStopped())
      myStatus = "Stopped";
    else
      myStatus = "Driving";
  }
  myStatusSetThisCycle = false;
}




