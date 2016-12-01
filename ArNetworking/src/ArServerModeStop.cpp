#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeStop.h"

AREXPORT ArServerModeStop::ArServerModeStop(ArServerBase *server, 
					    ArRobot *robot,
					    bool defunct) : 
  ArServerMode(robot, server, "stop"),
  myStopGroup(robot),
  myNetStopCB(this, &ArServerModeStop::netStop)
{
  myMode = "Stop";
  if (myServer != NULL)
  {
    addModeData("stop", "stops the robot", &myNetStopCB,
		"none", "none", "Stop", "RETURN_NONE");
  }

  myUseLocationDependentDevices = true;

  myLimiterForward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterForward");
  myStopGroup.addAction(myLimiterForward, 150);

  myLimiterBackward = new ArActionDeceleratingLimiter(
	  "DeceleratingLimiterBackward", 
	  ArActionDeceleratingLimiter::BACKWARDS);
  myStopGroup.addAction(myLimiterBackward, 149);

  myLimiterLateralLeft = NULL;
  if (myRobot->hasLatVel())
  {
    myLimiterLateralLeft = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralLeft", 
	    ArActionDeceleratingLimiter::LATERAL_LEFT);
    myStopGroup.addAction(myLimiterLateralLeft, 148);
  }

  myLimiterLateralRight = NULL;
  if (myRobot->hasLatVel())
  {
    myLimiterLateralRight = new ArActionDeceleratingLimiter(
	    "DeceleratingLimiterLateralRight", 
	    ArActionDeceleratingLimiter::LATERAL_RIGHT);
    myStopGroup.addAction(myLimiterLateralRight, 147);
  }

}

AREXPORT ArServerModeStop::~ArServerModeStop()
{

}

AREXPORT void ArServerModeStop::activate(void)
{
  if (isActive() || !baseActivate())
    return;
  setActivityTimeToNow();
  myRobot->stop();
  myRobot->clearDirectMotion();
  myStopGroup.activateExclusive();
  myStatus = "Stopping";
}

AREXPORT void ArServerModeStop::deactivate(void)
{
  myStopGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeStop::stop(void)
{
  activate();
}

AREXPORT void ArServerModeStop::netStop(ArServerClient *client, 
				     ArNetPacket *packet)
{
  setActivityTimeToNow();
  myRobot->lock();
  ArLog::log(ArLog::Verbose, "Stopping");
  stop();
  myRobot->unlock();
}

AREXPORT void ArServerModeStop::userTask(void)
{
  /// MPL 2014_04_17 centralizing all the places stopped is calculated
  //if (myRobot->getVel() < 2 && myRobot->getRotVel() < 2)
  if (myRobot->isStopped())
  {
    myStatus = "Stopped";
  }
  else
  {
    setActivityTimeToNow();
    myStatus = "Stopping";
  }
}

AREXPORT void ArServerModeStop::addToConfig(ArConfig *config, 
					      const char *section)
{
  myLimiterForward->addToConfig(config, section, "Forward");
  myLimiterBackward->addToConfig(config, section, "Backward");
  if (myLimiterLateralLeft != NULL)
    myLimiterLateralLeft->addToConfig(config, section, "Lateral");
  if (myLimiterLateralRight != NULL)
    myLimiterLateralRight->addToConfig(config, section, "Lateral");
}

AREXPORT void ArServerModeStop::setUseLocationDependentDevices(
	bool useLocationDependentDevices, bool internal)
{
  if (!internal)
    myRobot->lock();
  // if this is a change then print it
  if (useLocationDependentDevices != myUseLocationDependentDevices)
  {
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

AREXPORT bool ArServerModeStop::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}

