#include "Aria.h"
#include "ArExport.h"
#include "ArServerModeWander.h"

AREXPORT ArServerModeWander::ArServerModeWander(ArServerBase *server, ArRobot *robot) : 
  ArServerMode(robot, server, "wander"),
  myWanderGroup(robot),
  myNetWanderCB(this, &ArServerModeWander::netWander)
{
  myMode = "Wander";
  if (myServer != NULL)
  {
    addModeData("wander", "makes the robot wander", &myNetWanderCB,
		"none", "none", "Movement", "RETURN_NONE");
  }
}

AREXPORT ArServerModeWander::~ArServerModeWander()
{

}

AREXPORT void ArServerModeWander::activate(void)
{
  if (!baseActivate())
    return;

  setActivityTimeToNow();
  myRobot->clearDirectMotion();
  myWanderGroup.activateExclusive();
  myStatus = "Wandering";
}

AREXPORT void ArServerModeWander::deactivate(void)
{
  myWanderGroup.deactivate();
  baseDeactivate();
}

AREXPORT void ArServerModeWander::wander(void)
{
  activate();
}

AREXPORT void ArServerModeWander::netWander(ArServerClient *client, 
				     ArNetPacket *packet)
{
  myRobot->lock();
  ArLog::log(ArLog::Verbose, "Wandering");
  wander();
  myRobot->unlock();
  setActivityTimeToNow();
}

AREXPORT void ArServerModeWander::userTask(void)
{
  setActivityTimeToNow();

  // Sets the robot so that we always thing we're trying to move in
  // this mode
  myRobot->forceTryingToMove();

  //myStatus = "Wandering";
}
