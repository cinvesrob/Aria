#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeIdle.h"

AREXPORT ArServerModeIdle::ArServerModeIdle(ArServerBase *server, 
					    ArRobot *robot) :
  ArServerMode(robot, server, "idle"),
  myStopGroup(robot)
{
  myStatus = "Idle processing";
  myMode = "Idle";

  myUseLocationDependentDevices = true;

  myModeInterrupted = NULL;

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

AREXPORT ArServerModeIdle::~ArServerModeIdle()
{

}

AREXPORT void ArServerModeIdle::activate(void)
{
  if (isActive())
    return;

  if (!baseActivate())
  {
    /*
    ArLog::log(ArLog::Normal, 
	       "IDLE: Clearing mode interrupted since could not activate...");
    */
    myModeInterrupted = NULL;
    return;
  }

  //ArLog::log(ArLog::Normal, "IDLE... um... %p", myModeInterrupted);

  myRobot->stop();
  myRobot->clearDirectMotion();
  myStopGroup.activateExclusive();
  setActivityTimeToNow();
  ArLog::log(ArLog::Normal, "Idle processing mode activated");
}

AREXPORT void ArServerModeIdle::deactivate(void)
{
  ArLog::log(ArLog::Normal, "Idle processing mode deactivating");
  myStopGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeIdle::userTask(void)
{
  /// MPL 2014_04_17 centralizing all the places stopped is calculated
  //if (fabs(myRobot->getVel()) > 2 || fabs(myRobot->getRotVel()) > 2 || 
  //(myRobot->hasLatVel() && fabs(myRobot->getLatVel()) > 2))
  if (!myRobot->isStopped())
  {
    myStatus = "Stopping";
    setActivityTimeToNow();
  }
  else
  {
    myStatus = "Idle processing";
  }
    
  //ArLog::log(ArLog::Normal, "Idle mode called");
  if (!myServer->idleProcessingPending())
  {
    //ArLog::log(ArLog::Normal, "Idle mode done");
    deactivate();
  }
}

AREXPORT void ArServerModeIdle::addToConfig(ArConfig *config, 
					      const char *section)
{
  myLimiterForward->addToConfig(config, section, "Forward");
  myLimiterBackward->addToConfig(config, section, "Backward");
  if (myLimiterLateralLeft != NULL)
    myLimiterLateralLeft->addToConfig(config, section, "Lateral");
  if (myLimiterLateralRight != NULL)
    myLimiterLateralRight->addToConfig(config, section, "Lateral");
}

AREXPORT void ArServerModeIdle::setUseLocationDependentDevices(
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

AREXPORT bool ArServerModeIdle::getUseLocationDependentDevices(void)
{
  return myUseLocationDependentDevices;
}


AREXPORT void ArServerModeIdle::setModeInterrupted(
	ArServerMode *modeInterrupted)
{
  /*
  if (modeInterrupted != NULL)
    ArLog::log(ArLog::Normal, "IDLE: Setting mode interrupted to %s", 
	       modeInterrupted->getName());
  else
    ArLog::log(ArLog::Normal, "IDLE: Setting mode interrupted to NULL");
  */
  myModeInterrupted = modeInterrupted;
}  

AREXPORT ArServerMode *ArServerModeIdle::getModeInterrupted(void)
{
  return myModeInterrupted;
}

