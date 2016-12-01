#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeRatioDrive.h"
#include "ArServerHandlerCommands.h"

AREXPORT ArServerModeRatioDrive::ArServerModeRatioDrive(
	ArServerBase *server, ArRobot *robot, bool takeControlOnJoystick,
	bool useComputerJoystick, bool useRobotJoystick, 
	bool useServerCommands, const char *name,
	bool robotJoystickOverridesLocks) : 
  ArServerMode(robot, server, name),
  myRatioDriveGroup(robot),
  myJoyUserTaskCB(this, &ArServerModeRatioDrive::joyUserTask),
  myServerSetSafeDriveCB(this, 
			 &ArServerModeRatioDrive::serverSetSafeDrive),
  myServerGetSafeDriveCB(this, 
			 &ArServerModeRatioDrive::serverGetSafeDrive),
  myServerRatioDriveCB(this, &ArServerModeRatioDrive::serverRatioDrive),
  myRatioFireCB(this, &ArServerModeRatioDrive::ratioFireCallback),
  myServerSafeDrivingEnableCB(this, 
			     &ArServerModeRatioDrive::serverSafeDrivingEnable),
  myServerSafeDrivingDisableCB(this, 
			     &ArServerModeRatioDrive::serverSafeDrivingDisable)
{
  myHandlerCommands = NULL;
  myDriveSafely = true;
  myTakeControlOnJoystick = takeControlOnJoystick;
  myUseComputerJoystick = useComputerJoystick;
  myUseRobotJoystick = useRobotJoystick;
  myUseServerCommands = useServerCommands;
  myUseLocationDependentDevices = true;
  myRobotJoystickOverridesLock = robotJoystickOverridesLocks;
  myTimeout = 2;
  myGotServerCommand = true;

  myLastTimedOut = false;
  
  // SEEKUR
  mySentRecenter = false;

  // add the actions, put the ratio input on top, then have the
  // limiters since the ratio doesn't touch decel except lightly
  // whereas the limiter will touch it strongly

  myRatioAction = new ArActionRatioInput;
  myRatioDriveGroup.addAction(myRatioAction, 50);

  myLimiterForward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterForward", ArActionDeceleratingLimiter::FORWARDS);
  myRatioDriveGroup.addAction(myLimiterForward, 40);

  myLimiterBackward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterBackward", 
	  ArActionDeceleratingLimiter::BACKWARDS);
  myRatioDriveGroup.addAction(myLimiterBackward, 39);

  myLimiterLateralLeft = NULL;
  myLimiterLateralRight = NULL;
  if (myRobot->hasLatVel())
  {
    myLimiterLateralLeft = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralLeft", 
	    ArActionDeceleratingLimiter::LATERAL_LEFT);
    myRatioDriveGroup.addAction(myLimiterLateralLeft, 38);
    myLimiterLateralRight = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralRight", 
	    ArActionDeceleratingLimiter::LATERAL_RIGHT);
    myRatioDriveGroup.addAction(myLimiterLateralRight, 37);
  }

  myMovementParameters = new ArActionMovementParameters("TeleopMovementParameters", false);
  myRatioDriveGroup.addAction(myMovementParameters, 1);

  myRatioFireCB.setName("ArServerModeRatioDrive");
  myRatioAction->addFireCallback(30, &myRatioFireCB);
  
  myLastRobotSafeDrive = true;
  
  if (myServer != NULL && myUseServerCommands)
  {
    addModeData("ratioDrive", "drives the robot as with a joystick",
		&myServerRatioDriveCB,
		"double: transRatio; double: rotRatio; double: throttleRatio ",
		"none", "Movement", "RETURN_NONE");
    myServer->addData("setSafeDrive", 
		      "sets whether we drive the robot safely or not",
		      &myServerSetSafeDriveCB,
		      "byte: 1 == drive safely, 0 == drive unsafely",
		      "none", "UnsafeMovement", "RETURN_NONE");
    myServer->addData("getSafeDrive", 
		      "gets whether we drive the robot safely or not",
		      &myServerGetSafeDriveCB,
		      "none", 
		      "byte: 1 == driving safely, 0 == driving unsafely", 
		      "Movement", "RETURN_SINGLE");
  }

  if (myUseComputerJoystick)
  {
    myJoydrive = new ArRatioInputJoydrive(robot, myRatioAction);
    if ((myJoyHandler = Aria::getJoyHandler()) == NULL)
    {
      myJoyHandler = new ArJoyHandler;
      myJoyHandler->init();
      Aria::setJoyHandler(myJoyHandler);
    }
  }
  if (myUseRobotJoystick)
  {
    myRobotJoydrive = new ArRatioInputRobotJoydrive(robot, myRatioAction);
    if ((myRobotJoyHandler = Aria::getRobotJoyHandler()) == NULL)
    {
      myRobotJoyHandler = new ArRobotJoyHandler(robot);
      Aria::setRobotJoyHandler(myRobotJoyHandler);
    }
  }
  if (myUseRobotJoystick || myUseComputerJoystick)
  {
    std::string taskName = name;
    taskName += "::joyUserTask";
    myRobot->addUserTask(taskName.c_str(), 75, &myJoyUserTaskCB);
  }

  myPrinting = false;
}

AREXPORT ArServerModeRatioDrive::~ArServerModeRatioDrive()
{

}

AREXPORT void ArServerModeRatioDrive::addToConfig(ArConfig *config, 
						  const char *section)
{
  config->addParam(
	  ArConfigArg(
		  "Timeout", &myTimeout,
		  "If there are no commands for this period of time, then the robot will stop.  0 Disables.  This is a double so you can do like .1 seconds if you want.", 0),
	  section, ArPriority::ADVANCED);
  myRatioAction->addToConfig(config, section);
  myLimiterForward->addToConfig(config, section, "Forward");
  myLimiterBackward->addToConfig(config, section, "Backward");
  if (myLimiterLateralLeft != NULL)
    myLimiterLateralLeft->addToConfig(config, section, "Lateral");
  if (myLimiterLateralRight != NULL)
    myLimiterLateralRight->addToConfig(config, section, "Lateral");
  myMovementParameters->addToConfig(config, section, "Teleop");
  
}

AREXPORT void ArServerModeRatioDrive::activate(void)
{
  //if (!baseActivate()) {
  //   return;
  //}
  ratioDrive(0, 0, 100, true);
}

AREXPORT void ArServerModeRatioDrive::deactivate(void)
{
  myRatioDriveGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeRatioDrive::setSafeDriving(bool safe, bool internal)
{
  if (!internal)
    myRobot->lock();
  // if this is a change then print it
  if (safe != myDriveSafely)
  {
    if (safe)
    {
      ArLog::log(ArLog::Normal, "%s: Driving safely again", myName.c_str());
    }
    else
    {
      ArLog::log(ArLog::Normal, "%s: Driving UNSAFELY", myName.c_str());
    }
    myNewDriveSafely = true;
  }
  myDriveSafely = safe;
  // ratioDrive is only called if this mode is already active (and now just sends 0s)
  if (isActive())
    ratioDrive(0, 0, 0, false, 0);
  /*
    ratioDrive(myRatioAction->getTransRatio(), 
    myRatioAction->getRotRatio(),
    myRatioAction->getThrottleRatio());
  */
  if (!internal)
    myRobot->unlock();
}

AREXPORT bool ArServerModeRatioDrive::getSafeDriving(void)
{
  return myDriveSafely;
}

AREXPORT void ArServerModeRatioDrive::serverSafeDrivingEnable(void)
{
  setSafeDriving(true);
}

AREXPORT void ArServerModeRatioDrive::serverSafeDrivingDisable(void)
{
  setSafeDriving(false);
}

AREXPORT void ArServerModeRatioDrive::addControlCommands(ArServerHandlerCommands *handlerCommands)
{
  if (!myUseServerCommands)
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerModeRatioDrive::addControlCommands: Tried to add control commands to a ratio drive not using the server");
    return;
  }
  myHandlerCommands = handlerCommands;
  myHandlerCommands->addCommand(
	  "safeRatioDrivingEnable",
	  "Enables safe driving with ratioDrive, which will attempt to prevent collisions (default)",
	  &myServerSafeDrivingEnableCB, "UnsafeMovement");
  myHandlerCommands->addCommand(
	  "safeRatioDrivingDisable",
	  "Disables safe driving with ratioDrive, this is UNSAFE and will let you drive your robot into things or down stairs, use at your own risk",
	  &myServerSafeDrivingDisableCB, "UnsafeMovement");
}


/**
 * @param isActivating a bool set to true only if this method is called from the activate()
 * method, otherwise false
 * @param transRatio Amount of forward velocity to request
 * @param rotRatio Amount of rotational velocity to request
 * @param throttleRatio Amount of speed to request
 * @param latRatio amount of lateral velocity to request (if robot supports it)
**/
AREXPORT void ArServerModeRatioDrive::ratioDrive(
	double transRatio, double rotRatio, double throttleRatio,
	bool isActivating, double latRatio)
{
  bool wasActive;
  wasActive = isActive();

  myTransRatio = transRatio;
  myRotRatio = rotRatio;
  myThrottleRatio = throttleRatio;
  myLatRatio = latRatio;

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
      myRatioDriveGroup.activateExclusive();
      myMode = "Drive";
      ArLog::log(ArLog::Normal, "%s: Driving safely", myName.c_str());
    }
    else
    {
      myRobot->deactivateActions();
      myRatioAction->activate();
      myMode = "UNSAFE Drive";
      ArLog::log(ArLog::Normal, "%s: Driving unsafely", myName.c_str());
    }
    if (myDriveSafely)
      mySafeDrivingCallbacks.invoke();
    else
      myUnsafeDrivingCallbacks.invoke();
  }
  myNewDriveSafely = false;

  // MPL why is this here twice?
  myTransRatio = transRatio;
  myRotRatio = rotRatio;
  myThrottleRatio = throttleRatio;
  myLatRatio = latRatio;
  
  setActivityTimeToNow();
  myLastCommand.setToNow();
  
  
  // SEEKUR
  mySentRecenter = false;
  
  if (myPrinting)
    ArLog::log(ArLog::Normal, "%s: cmd %.0f %.0f %.0f %.0f", 
	       getName(), transRatio, rotRatio, throttleRatio, latRatio);
  if (myTransRatio < -0.1)
    myDrivingBackwardsCallbacks.invoke();
  //myRatioAction.setRatios(transRatio, rotRatio, throttleRatio);
}

AREXPORT void ArServerModeRatioDrive::serverRatioDrive(ArServerClient *client, 
						       ArNetPacket *packet)
{
  double transRatio = packet->bufToDouble();
  double rotRatio   = packet->bufToDouble();
  double throttleRatio = packet->bufToDouble();
  double lateralRatio = packet->bufToDouble();

  myGotServerCommand = true;

  if (!myDriveSafely && !client->hasGroupAccess("UnsafeMovement"))    
    serverSafeDrivingEnable();
  myRobot->lock();
 
  // Activate if necessary.  Note that this is done before the ratioDrive
  // call because we want the new ratio values to be applied after the
  // default ones.
  if (!isActive()) {
    activate();
  }
  
  if (myPrinting)
    ArLog::log(ArLog::Normal, 
	       "%s: serverCmd (%s) trans %.0f rot %.0f lat %.0f ratio %.0f",
	       
	       getName(), client->getIPString(),
	       transRatio, rotRatio, lateralRatio, throttleRatio);

  ratioDrive(transRatio, rotRatio, throttleRatio, false, lateralRatio);
  myRobot->unlock();
}

AREXPORT void ArServerModeRatioDrive::serverSetSafeDrive(
	ArServerClient *client, ArNetPacket *packet)
{
  if (packet->bufToUByte() == 0)
    setSafeDriving(false);
  else
    setSafeDriving(true);
}

AREXPORT void ArServerModeRatioDrive::serverGetSafeDrive(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  if (getSafeDriving())
    sendPacket.uByteToBuf(1);
  else
    sendPacket.uByteToBuf(0);
  
  client->sendPacketTcp(&sendPacket);
}


AREXPORT void ArServerModeRatioDrive::joyUserTask(void)
{
  // if we're not active but we should be
  if (myTakeControlOnJoystick && !isActive() && 
      ((myUseComputerJoystick && myJoyHandler->haveJoystick() && 
	myJoyHandler->getButton(1)) || 
       (myUseRobotJoystick && myRobotJoyHandler->gotData() && 
	myRobotJoyHandler->getButton1())))
  {
    if (ArServerMode::getActiveMode() != NULL)
      ArLog::log(ArLog::Normal, 
		 "%s: Activating instead of %s because of local joystick", 
		 myName.c_str(),
		 ArServerMode::getActiveMode()->getName());
    else
      ArLog::log(ArLog::Normal, 
		 "%s: Activating because of local joystick",
		 myName.c_str());
    // if we're locked and are overriding that lock for the robot
    // joystick and it was the robot joystick that caused it to happen
    if (myUseRobotJoystick && myRobotJoyHandler->gotData() && 
	myRobotJoyHandler->getButton1() && myRobotJoystickOverridesLock && 
	ArServerMode::ourActiveModeLocked)
    {
      ArLog::log(ArLog::Terse, "Robot joystick is overriding locked mode %s", 
		 ourActiveMode->getName());
      ourActiveMode->forceUnlock();
      myRobot->enableMotors();
    }
    activate();
  }
  bool unsafeRobotDrive;
  if (myUseRobotJoystick && myRobotJoyHandler->gotData() && 
      ((unsafeRobotDrive = 
	(bool)(myRobot->getFaultFlags() & ArUtil::BIT15)) !=
       !myLastRobotSafeDrive))
  {
    myLastRobotSafeDrive = !unsafeRobotDrive;
    setSafeDriving(myLastRobotSafeDrive, true);
  }
}

AREXPORT void ArServerModeRatioDrive::userTask(void)
{
  // Sets the robot so that we always think we're trying to move in
  // this mode
  myRobot->forceTryingToMove();

  // MPL 2014_04_17 centralizing all the places stopped is calculated
  //bool moving = (fabs(myRobot->getVel()) > 1 || 
  //		 fabs(myRobot->getRotVel()) > 1 || 
  //		 fabs(myRobot->getLatVel()) > 1);

  bool moving = !myRobot->isStopped();

  bool wantToMove;
  if ((fabs(myTransRatio) < .0000001 && fabs(myRotRatio) < .000001 && 
       fabs(myLatRatio) < .0000001) ||
      myThrottleRatio < .000001)
    wantToMove = false;
  else
    wantToMove = true;

  // if the joystick is pushed then set that we're active, server
  // commands'll go into ratioDrive and set it there too
  if ((myUseComputerJoystick && myJoyHandler->haveJoystick() && 
       myJoyHandler->getButton(1)) ||
      (myUseRobotJoystick && myRobotJoyHandler->gotData() && 
       myRobotJoyHandler->getButton1()) || 
      (myUseServerCommands && myGotServerCommand) || 
      moving)
  {
    setActivityTimeToNow();
  }

  myGotServerCommand = false;

  bool timedOut = false;
  // if we want to move, and there is a timeout, and the activity time is
  // greater than the timeout, then stop the robot
  
  if (wantToMove &&
      myTimeout > .0000001 && 
      myLastCommand.mSecSince()/1000.0 >= myTimeout)

  {
    if (!myLastTimedOut)
    {
      ArLog::log(ArLog::Normal, "Stopping the robot since teleop timed out");
      myRobot->stop();
      myRobot->clearDirectMotion();
      ArTime lastCommand = myLastCommand;
      ratioDrive(0, 0, 0, false, 0);
      myLastCommand = lastCommand;
    }
    timedOut = true;
  }

  myLastTimedOut = timedOut;

  // SEEKUR (needed for prototype versions)
  /*
  if (myRobot->hasLatVel() && !mySentRecenter && 
      getActivityTime().secSince() >= 10)
  {
    mySentRecenter = true;
    myRobot->com(120);
  }
  */

  if (!myStatusSetThisCycle)
  {
    if (myRobot->isLeftMotorStalled() || myRobot->isRightMotorStalled())
      myStatus = "Stalled";
    // this works because if the motors stalled above caught it, if
    // not and more values it means a stall
    else if (myRobot->getStallValue())
      myStatus = "Bumped";
    // MPL 2014_04_17 centralizing all the places stopped is calculated
    //else if (ArMath::fabs(myRobot->getVel()) < 2 && 
    //ArMath::fabs(myRobot->getRotVel()) < 1 && 
    //(!myRobot->hasLatVel() || ArMath::fabs(myRobot->getLatVel()) < 2))
    else if (myRobot->isStopped())
      myStatus = "Stopped";
    else
      myStatus = "Driving";
  }

  myStatusSetThisCycle = false;
} // end method userTask


AREXPORT void ArServerModeRatioDrive::ratioFireCallback(void)
{
  if (myPrinting)
	  ArLog::log(ArLog::Normal, "ArServerModeRatioDrive: TransRatio=%.0f RotRatio=%.0f ThrottleRatio=%.0f LatRatio=%.0f", 
	       myTransRatio, myRotRatio, myThrottleRatio, myLatRatio);
  myRatioAction->setRatios(myTransRatio, myRotRatio, myThrottleRatio, 
			   myLatRatio);
}

AREXPORT void ArServerModeRatioDrive::setUseLocationDependentDevices(
	bool useLocationDependentDevices, bool internal)
{
  if (!internal)
    myRobot->lock();
  // if this is a change then print it
  if (useLocationDependentDevices != myUseLocationDependentDevices)
  {
    if (useLocationDependentDevices)
    {
      ArLog::log(ArLog::Normal, "%s: Using location dependent range devices",
		 myName.c_str());
    }
    else
    {
      ArLog::log(ArLog::Normal, 
		 "%s: Not using location dependent range devices",
		 myName.c_str());
    }
    myUseLocationDependentDevices = useLocationDependentDevices;
    myLimiterForward->setUseLocationDependentDevices(
	    myUseLocationDependentDevices);
    myLimiterBackward->setUseLocationDependentDevices(
	    myUseLocationDependentDevices);
    if (myLimiterLateralLeft != NULL)
      myLimiterLateralLeft->setUseLocationDependentDevices(
	      myUseLocationDependentDevices);
    if (myLimiterLateralRight != NULL)
      myLimiterLateralRight->setUseLocationDependentDevices(
	      myUseLocationDependentDevices);

  }
  if (!internal)
    myRobot->unlock();
}

AREXPORT bool ArServerModeRatioDrive::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}




