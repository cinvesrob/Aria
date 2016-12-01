#include "Aria.h"
#include "ArExport.h"
#include "ArServerMode.h"
#include "ArServerModeIdle.h"
#include <algorithm>

AREXPORT bool ArServerMode::ourActiveModeLocked = false;
AREXPORT bool ArServerMode::ourActiveModeWillUnlockIfRequested = false;
AREXPORT ArServerMode *ArServerMode::ourActiveMode = NULL;
AREXPORT ArServerMode *ArServerMode::ourNextActiveMode = NULL;
AREXPORT ArServerMode *ArServerMode::ourLastActiveMode = NULL;
AREXPORT std::list<ArServerMode *> ArServerMode::ourDefaultModes;
AREXPORT std::list<ArServerMode *> ArServerMode::ourRequestedActivateModes;
AREXPORT std::list<ArServerMode *> ArServerMode::ourModes;
AREXPORT bool ArServerMode::ourUserTaskAdded = false;
AREXPORT ArGlobalFunctor ArServerMode::ourUserTaskCB(
	ArServerMode::modeUserTask);
AREXPORT std::multimap<std::string, std::string> ArServerMode::ourModeDataMap;
AREXPORT ArMutex ArServerMode::ourModeDataMapMutex;
AREXPORT ArGlobalFunctor2<ArServerClient *, 
			  ArNetPacket *> ArServerMode::ourGetModeDataListCB(
				  ArServerMode::getModeDataList);
AREXPORT ArGlobalFunctor2<ArServerClient *, 
			  ArNetPacket *> ArServerMode::ourGetModeInfoCB(
				  ArServerMode::getModeInfo);
AREXPORT ArGlobalFunctor2<ArServerClient *, ArNetPacket *> 
          ArServerMode::ourGetModeBusyCB(ArServerMode::getModeBusy);

AREXPORT bool ArServerMode::ourBroadcastActiveModeLocked = false;
AREXPORT ArServerMode *ArServerMode::ourBroadcastActiveMode = NULL; 
AREXPORT bool ArServerMode::ourBroadcastActiveModeWillUnlockIfRequested = false;
AREXPORT bool ArServerMode::ourIsBusy = false;
AREXPORT ArServerBase *ArServerMode::ourServerBase = NULL;
AREXPORT ArServerModeIdle *ArServerMode::ourIdleMode = NULL;
AREXPORT ArMutex ArServerMode::ourIdleModeMutex;
AREXPORT bool ArServerMode::ourIdleModeCreated = false;
AREXPORT bool ArServerMode::ourActiveModeSetActivityThisCycle = false;

AREXPORT void ArServerMode::modeUserTask(void)
{
  if (ourActiveMode != NULL)
  {
    ourActiveMode->mySetActivityThisCycle = false;
    ourActiveMode->userTask();
    if (ourActiveMode != NULL)
      ourActiveModeSetActivityThisCycle = ourActiveMode->mySetActivityThisCycle;
    else
      ourActiveModeSetActivityThisCycle = true;
  }
  else
  {
    std::list<ArServerMode *>::iterator it;
    for (it = ourDefaultModes.begin(); 
	       it != ourDefaultModes.end() && ourActiveMode == NULL;
	       it++)
    {
      ArLog::log(ArLog::Terse, "Checking default on %s mode", (*it)->getName());
      (*it)->checkDefault();
      if (ourActiveMode != NULL)
      {
	      ArLog::log(ArLog::Normal, "Activated %s mode as default", (*it)->getName());
	      break;
      }
    } // end for each default mode
  } // end else no active mode

  bool wasBusy = ourIsBusy;
  // It seems to me that we wouldn't want to call the robot busy if there is no 
  // active mode.  (Perhaps getActiveModeActivityTimeSecSince() should return 
  // something besides zero in this case.)
  ourIsBusy = ((ourActiveMode != NULL) &&
               (ArServerMode::getActiveModeActivityTimeSecSince() == 0));

  if (wasBusy != ourIsBusy) {

    if (ourServerBase != NULL) {
      
      //ArLog::log(ArLog::Normal, "ArServerMode busy changed: %i", ourIsBusy);

      ArNetPacket packet;
      packet.byteToBuf(ourIsBusy);

      ourServerBase->broadcastPacketTcp(&packet, "modeBusyChanged");
    }
  } // end if busy changed

} // end method modeUserTask


AREXPORT ArServerMode::ArServerMode(ArRobot *robot, ArServerBase *server, 
				    const char *name)
{
  ourModeDataMapMutex.setLogName("ArServerMode::ourModeDataMapMutex");

  myName = name;
  std::replace(myName.begin(), myName.end(), ' ', '_');

  std::string activityMutexName;
  activityMutexName = "ArServerMode::";
  activityMutexName += myName;
  activityMutexName += "::myActivityTimeMutex";
  myActivityTimeMutex.setLogName(activityMutexName.c_str());
  myVerboseLogLevel = ArLog::Verbose;
  //myVerboseLogLevel = ArLog::Normal;
  myRobot = robot;
  myServer = server;
  myIsActive = false;
  myStatusSetThisCycle = false;
  ourModes.push_front(this);
  myHasSetActivityTime = false;
  mySetActivityThisCycle = false;

  std::string cbListName;
  cbListName = "ArServerMode::";
  cbListName += myName;
  cbListName += "::myActivateCallbacks";
  myActivateCallbacks.setName(cbListName.c_str());

  cbListName = "ArServerMode::";
  cbListName += myName;
  cbListName += "::myDeactivateCallbacks";
  myDeactivateCallbacks.setName(cbListName.c_str());

  cbListName = "ArServerMode::";
  cbListName += myName;
  cbListName += "::mySingleShotDeactivateCallbacks";
  mySingleShotDeactivateCallbacks.setName(cbListName.c_str());
  mySingleShotDeactivateCallbacks.setSingleShot(true);

  cbListName = "ArServerMode::";
  cbListName += myName;
  cbListName += "::mySingleShotPostDeactivateCallbacks";
  mySingleShotPostDeactivateCallbacks.setName(cbListName.c_str());
  mySingleShotPostDeactivateCallbacks.setSingleShot(true);

  if (!ourUserTaskAdded)
  {
    ourUserTaskAdded = true;
    myRobot->addUserTask("server mode", 50, &ourUserTaskCB);
    myServer->addData("getModeDataList", "Gets the list of commands associated with modes.", 
		      &ourGetModeDataListCB,
		      "none", 
		      "ubyte4: numData;  repeating numData times, string: mode; string: data", 
		      "RobotInfo", "RETURN_SINGLE");
    myServer->addData("getModeInfo", "Gets the current mode active and if its locked and will or won't unlock (for lists of which commands are with which modes use getModeDataList) commands associated with modes.", 
		      &ourGetModeInfoCB,
		      "none", 
		      "string: mode; uByte: locked (1 == locked, 0 == unlocked); uByte: willUnlockIfRequested (1 == will, 0 == won't)",
		      "RobotInfo", "RETURN_SINGLE");

    myServer->addData("modeBusyChanged",
                      "Broadcast when the busy state (non-zero idle time) of the active mode changes",
                      &ourGetModeBusyCB,
                      "None",
                      "byte: 1 if busy, 0 if idle",
                      "RobotInfo",
                      "RETURN_SINGLE");

  } // end if static tasks and handlers not yet added

  // Storing the first occurrence of the server base for use in the static 
  // modeUserTask method.  In general, there seems to be a single server base which 
  // isn't deleted during the program execution.  However, if this should change, 
  // then will need to add a way of resetting the static ourServerBase.
  if (ourServerBase == NULL) {
    ourServerBase = server;
  }

  ourIdleModeMutex.lock();
  if (ourIdleMode == NULL && !ourIdleModeCreated)
  {
    ourIdleModeCreated = true;
    ourIdleModeMutex.unlock();
    ourIdleMode = new ArServerModeIdle(server, robot);
  }
  else
  {
    ourIdleModeMutex.unlock();
  }
} // end ctor

AREXPORT ArServerMode::~ArServerMode()
{

}

AREXPORT void ArServerMode::lockMode(bool willUnlockIfRequested)
{
  if (!myIsActive || ourActiveMode != this)
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode::lockMode: mode %s could not lock because it is not active", getName());
    return;
  }
  ourActiveModeLocked = true;
  ourActiveModeWillUnlockIfRequested = willUnlockIfRequested;
  ArLog::log(myVerboseLogLevel, "ArServerMode: Locked into %s mode, will unlock %s", 
	     getName(), ArUtil::convertBool(willUnlockIfRequested));
  checkBroadcastModeInfoPacket();
}

AREXPORT void ArServerMode::unlockMode(void)
{
  if (!myIsActive || ourActiveMode != this)
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode::unlockMode: mode %s could not unlock because it is not locked", getName());
    return;
  }
  ourActiveModeLocked = false;
  ourActiveModeWillUnlockIfRequested = false;
  ArLog::log(myVerboseLogLevel, "ArServerMode: Unlocked from %s mode", getName());
  checkBroadcastModeInfoPacket();

  std::list<ArServerMode *>::iterator it;
  if (ourNextActiveMode == NULL)
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode: UnlockModeCheck(%s): No next active mode",
	       getName());

    // walk through until one of these is ready
    while ((it = ourRequestedActivateModes.begin()) != 
	   ourRequestedActivateModes.end())
    {
      std::string name;
      ArServerMode *serverMode = NULL;
      serverMode = (*it);
      name = serverMode->getName();
      ArLog::log(ArLog::Normal,"UnlockModeCheck(%s): Trying to activate %s", 
		 getName(), name.c_str());
      
      ourRequestedActivateModes.pop_front();

      serverMode->activate();

      if (ourActiveMode == ourIdleMode)
      {
	ArLog::log(ArLog::Normal, "UnlockModeCheck(%s): Idle mode activated instead, so leaving the requestedActivateModes alone", getName());
	return;
      }
      
      if (ourActiveMode != this && ourActiveMode != NULL)
      {
	ArLog::log(ArLog::Normal, "UnlockModeCheck(%s): Activated another mode  (%s) assuming it takes care of the rest", getName(), 
		   ourActiveMode->getName());
	return;
      }
    } // end while modes to activate
    ArLog::log(ArLog::Normal, 
	       "UnlockModeCheck(%s): No other mode activated",
	       getName());
  }
  else
  {
    ArLog::log(ArLog::Normal, "UnlockModeCheck: Our next active mode %s, removing it from requested activate modes", ourNextActiveMode->getName());
    ourRequestedActivateModes.remove(ourNextActiveMode);
    return;
  }
  ArLog::log(ArLog::Normal,
             "UnlockModeCheck: Nothing else wanted to activate after %s unlocked", getName());

}

/**
   This should only be called from places where a forced unlock _HAS_
   to happen, ie on a robot joystick, but modes should do whatever
   they have to in this to turn things off or what not.
**/
AREXPORT void ArServerMode::forceUnlock(void)
{
  ArLog::log(ArLog::Terse, "Mode %s being forcibly unlocked", getName()); 
  unlockMode();
}

/// Gets whether we'll unlock if requested or not
AREXPORT bool ArServerMode::willUnlockIfRequested(void) 
{
  if (ourActiveMode == NULL || !ourActiveModeLocked || 
      (ourActiveModeLocked && ourActiveModeWillUnlockIfRequested))
    return true;
  else
    return false;
}

/// Gets if the active mode is locked or not 
AREXPORT bool ArServerMode::isLocked(void) 
{
  if (ourActiveMode == NULL || !ourActiveModeLocked)
    return false;
  else
    return true;
}

AREXPORT bool ArServerMode::isAutoResumeAfterInterrupt()
{
  return false;
}


/**
   Makes this mode active if it can... If this returns false then the
   mode's activate() method should just return... Note that before calling
   this a mode should have already made sure it can activate... This also 
   calls the activate callbacks (only if the mode will be allowed to
   activate of course).

   IMPORTANT: This method must only be called within the context of the
   activate() method.
 **/

AREXPORT bool ArServerMode::baseActivate(bool canSelfActivateIfLocked)
{

  // if we're locked then return false so nothing else activates
  if (ourActiveMode != NULL && ourActiveModeLocked && 
      (ourActiveMode != this || !canSelfActivateIfLocked))
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode: Could not switch to %s mode because of lock (%s).", 
	       getName(), ArUtil::convertBool(canSelfActivateIfLocked));
 
    // request our active mode to unlock
    //
    // KMC: Moved the following line so that it is called after this mode is pushed
    // onto the ourRequestedActivatedModes queue.  Under certain conditions, 
    // the requestUnlock() call results in a mode being deactivated and the next
    // queued mode being activated. This mode will essentially be pushed onto the 
    // queue too late -- and will either be ignored or activated at an unexpected
    // time.  (An example scenario occurs when the robot is going to dock, and the
    // go-to-goal mode attempts to activate.)
    // ourActiveMode->requestUnlock();

    ArLog::log(myVerboseLogLevel, "ArServerMode: Removing this (%s) from requested activate modes",
               getName());

    ourRequestedActivateModes.remove(this);
    ArLog::log(myVerboseLogLevel, "ArServerMode: Mode %s wants to be activated, adding to requested activate modes.", getName());

    ourRequestedActivateModes.push_front(this);

    ourActiveMode->requestUnlock();
    
    return false;
  
  }

  if (this != ourIdleMode && myServer->idleProcessingPending())
  {
    if (ourActiveMode == ourIdleMode)
    {
      /* MPL this is the old behavior that caused by 13711 ... now we
       * add it toe the queue isntead

      ArLog::log(myVerboseLogLevel, 
		 "Since idle already active didn't set nextActiveMode when mode %s tried to activate, just returning", this->getName());      
      return false;
      */
      ArLog::log(myVerboseLogLevel, 
		 "ArServerMode: Since idle already active didn't set nextActiveMode when mode %s tried to activate, just adding to requested activate modes", this->getName());      
      ourRequestedActivateModes.remove(this);
      ourRequestedActivateModes.push_front(this);
      return false;
    }
    ourNextActiveMode = ourIdleMode;
    if (ourActiveMode != NULL)
      ArLog::log(myVerboseLogLevel, 
		 "ArServerMode: Setting nextActiveMode explicitly to idle mode from mode %s trying to activate while %s is active", this->getName(), ourActiveMode->getName());
    else
      ArLog::log(myVerboseLogLevel, 
		 "ArServerMode: Setting nextActiveMode explicitly to idle mode from mode %s trying to activate while no mode is active", this->getName());
		       
    if (ourActiveMode != NULL && ourActiveMode != ourIdleMode)
    {
      ourIdleMode->setModeInterrupted(ourActiveMode);
      ourActiveMode->deactivate();
    }
    ArLog::log(myVerboseLogLevel, "ArServerMode: Removing this (%s) from requested activate modes",
               getName());
    ourRequestedActivateModes.remove(this);
    ArLog::log(myVerboseLogLevel, "ArServerMode: Mode %s wants to be activated, denying for now because of idle, but adding to requested activate modes", getName());
    ourRequestedActivateModes.push_front(this);
    ourIdleMode->activate();
    return false;
  }

  ourNextActiveMode = this;
  if (ourActiveMode != NULL)
    ArLog::log(myVerboseLogLevel, "ArServerMode: Setting nextActiveMode to mode %s (ourActiveMode %s)",
	       ourNextActiveMode->getName(), ourActiveMode->getName());
  else
    ArLog::log(myVerboseLogLevel, "ArServerMode: Setting nextActiveMode to mode %s (ourActiveMode NULL)",
	       ourNextActiveMode->getName());
  if (ourActiveMode != NULL)
    ourActiveMode->deactivate();

  myIsActive = true;
  myActivityTimeMutex.lock();
  myActivityTime.setToNow(); 
  myActivityTimeMutex.unlock();

  /*
  if (myRobot != NULL)
  {
    myRobot->addUserTask(myName.c_str(), 50, &myUserTaskCB);
  }
  */
  ourLastActiveMode = ourActiveMode;
  ourActiveMode = this;
  ourNextActiveMode = NULL;

  ArLog::log(myVerboseLogLevel, "ArServerMode: Setting nextActiveMode to NULL");
  if (myRobot != NULL)
  {
    myRobot->stop();
    myRobot->clearDirectMotion();
  }
  /// Call our activate callbacks
  myActivateCallbacks.invoke();
  /// Set the activity time to now, but do NOT set the flag, since that flag is used to determine if a particular mode has used it or not
  ArLog::log(myVerboseLogLevel, "ArServerMode: Activated %s mode", getName());
  checkBroadcastModeInfoPacket();
  return true;
}

/**
   Whenever a mode uses this it should already have done all of its
   own deactivations (this also calls the deactive callbacks)
 **/
AREXPORT void ArServerMode::baseDeactivate(void)
{
  std::list<ArServerMode *>::iterator it;
  /*
  for (it = ourDefaultModes.begin(); 
       it != ourDefaultModes.end();
       it++)
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode: defaults are %s mode", (*it)->getName());
  }
  */
  // if we're the active mode, we're deactivating, and we're locked, then unlock
  if (ourActiveMode != NULL && ourActiveMode == this && ourActiveModeLocked)
    unlockMode();
  // if the unlock mode caused the deactivation of this mode to happen just return
  if (!myIsActive || ourActiveMode != this)
    return;
  /*
    if (myRobot != NULL)
    myRobot->remUserTask(&myUserTaskCB);
  */
  myIsActive = false;
  if (ourActiveMode == this)
  {
    ourLastActiveMode = ourActiveMode;
    ourActiveMode = NULL;
  }
  myDeactivateCallbacks.invoke();
  mySingleShotDeactivateCallbacks.invoke();
  
  ArLog::log(myVerboseLogLevel, "ArServerMode: Deactivated %s mode", getName());

  if (ourNextActiveMode == NULL)
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode: No next active mode %s", getName());

    // walk through until one of these is ready
    while ((it = ourRequestedActivateModes.begin()) != 
	   ourRequestedActivateModes.end())
    {
      std::string name;
      name = (*it)->getName();
      ArLog::log(myVerboseLogLevel, "ArServerMode: Trying to activate %s", name.c_str());
      
      (*it)->activate();

      if (ourActiveMode == ourIdleMode)
      {
	ArLog::log(myVerboseLogLevel, "ArServerMode: Idle mode activated instead, so leaving the requestedActivateModes alone");
	mySingleShotPostDeactivateCallbacks.invoke();
	return;
      }
      
      ArLog::log(myVerboseLogLevel, "ArServerMode: Popping front of requested activate modes (%s)", getName());
      
      ourRequestedActivateModes.pop_front();
      
      if (ourActiveMode != NULL)
      {
	ArLog::log(myVerboseLogLevel, "ArServerMode: and did, clearing requested activate modes (size = %i)",
                   ourRequestedActivateModes.size());
	// now clear out the old modes so that we don't wind up
	// stacking too much. First, notify them that they will not be activated.
        for (std::list<ArServerMode *>::iterator dIter = ourRequestedActivateModes.begin();
             dIter != ourRequestedActivateModes.end();
             dIter++) 
	{
          ArServerMode *deniedMode = *dIter;
          if (deniedMode != NULL) 
	  {
            deniedMode->activationDenied();
          }
        }
	
	ourRequestedActivateModes.clear();
	
        ArLog::log(myVerboseLogLevel,
                   "ArServerMode: Deactivate %s mode returns (1)", getName());
	mySingleShotPostDeactivateCallbacks.invoke();
	return;
      }
    } // end while modes to activate

    ArLog::log(myVerboseLogLevel, "ArServerMode: Deactivate did not activate any modes, clearing requested activate modes (size = %i)", ourRequestedActivateModes.size());
    
    // now clear out the old modes so that we don't wind up stacking
    // too much (should be empty anyways here, just make sure.  (Same logic
    // as above about notifying the mode that it won't be activated.)
    for (std::list<ArServerMode *>::iterator dIter = ourRequestedActivateModes.begin();
         dIter != ourRequestedActivateModes.end();
         dIter++) {
      ArServerMode *deniedMode = *dIter;
      if (deniedMode != NULL) {
        deniedMode->activationDenied();
      }
    }
    ourRequestedActivateModes.clear();
    
    for (it = ourDefaultModes.begin(); 
	 it != ourDefaultModes.end() && ourActiveMode == NULL;
	 it++)
    {
      ArLog::log(ArLog::Normal, "Checking default on %s mode", (*it)->getName());
      (*it)->checkDefault();
      if (ourActiveMode != NULL)
      {
	//printf("and did\n");
        ArLog::log(myVerboseLogLevel,
                   "ArServerMode: Deactivate %s mode returns (2)", getName());
	mySingleShotPostDeactivateCallbacks.invoke();
	return;
      }
    }
  }
  else
  {
    ArLog::log(myVerboseLogLevel, "ArServerMode: Our next active mode %s, removing it from requested activate modes", ourNextActiveMode->getName());
    ourRequestedActivateModes.remove(ourNextActiveMode);
  }
  ArLog::log(myVerboseLogLevel,
             "ArServerMode: Deactivate %s mode returns (3)", getName());
  mySingleShotPostDeactivateCallbacks.invoke();
  
}

/**
   The default mode is activated whenever a mode is deactivated, even
   if a mode is deactivated only for another mode to be activated
 **/
AREXPORT void ArServerMode::addAsDefaultMode(ArListPos::Pos pos)
{
  ArLog::log(ArLog::Normal, "Mode %s added as default mode", getName());
  if (pos == ArListPos::LAST)
    ourDefaultModes.push_back(this);
  else if (pos == ArListPos::FIRST)
    ourDefaultModes.push_front(this);
  else
  {
    ArLog::log(ArLog::Terse, "ArServerMode::addAsDefaultMode: bad list position.");
    ourDefaultModes.push_front(this);
  }
}

AREXPORT ArTime ArServerMode::getActivityTime(void) 
{ 
  ArTime ret;
  myActivityTimeMutex.lock();
  ret = myActivityTime;
  myActivityTimeMutex.unlock();
  return ret;
}

AREXPORT void ArServerMode::setActivityTimeToNow(void)     
{ 
  myActivityTimeMutex.lock();
  myHasSetActivityTime = true; 
  mySetActivityThisCycle = true;
  myActivityTime.setToNow(); 
  myActivityTimeMutex.unlock();
}

void ArServerMode::internalSetActivityTime(ArTime time)     
{ 
  myActivityTimeMutex.lock();
  myHasSetActivityTime = true; 
  mySetActivityThisCycle = false;
  myActivityTime = time;
  myActivityTimeMutex.unlock();
}

AREXPORT int ArServerMode::getActiveModeActivityTimeSecSince(void) 
{ 
  // chop this to an int so its easier to use, if you care about it
  // use the getActivityTime on the ourActiveMode
  if (ourActiveMode != NULL)
    return (int)ourActiveMode->getActivityTime().secSince();
  else 
    return -1;
}

AREXPORT bool ArServerMode::getActiveModeSetActivityThisCycle(void) 
{ 
  return ourActiveModeSetActivityThisCycle;
}

AREXPORT const char *ArServerMode::getActiveModeModeString(void) 
{ 
  if (ourActiveMode != NULL)
    return ourActiveMode->getMode();
  else 
    return NULL;
}


AREXPORT const char *ArServerMode::getActiveModeStatusString(void) 
{ 
  if (ourActiveMode != NULL)
    return ourActiveMode->getStatus();
  else 
    return NULL;
}

AREXPORT const char *ArServerMode::getActiveModeExtendedStatusString(void) 
{ 
  if (ourActiveMode != NULL)
    return ourActiveMode->getExtendedStatus();
  else 
    return NULL;
}

/**
   This basically just notes the commands associated with a mode so
   that clients can know what can and can't happen based on what mode
   is active/locked/etc.  You should call it only if the addData on
   the ArServerBase returns true (since otherwise it means that
   command wasn't added).
**/
AREXPORT bool ArServerMode::addModeData(
	const char *name, const char *description,
	ArFunctor2<ArServerClient *, ArNetPacket *> *functor,
	const char *argumentDescription, const char *returnDescription,
	const char *commandGroup, const char *dataFlags)
{
  if (myServer->addData(name, description, functor, argumentDescription,
			returnDescription, commandGroup, dataFlags))
  {
    ourModeDataMapMutex.lock();
    ourModeDataMap.insert(std::pair<std::string, std::string>(myName, name));
    ourModeDataMapMutex.unlock();
    return true;
  }
  else
  {
    ArLog::log(ArLog::Normal, "ArServerMode %s: Could not add mode data %s",
	       myName.c_str(), name);
    return false;
  }
      
}

/**
   This returns the list of which data is in which mode
**/
AREXPORT void ArServerMode::getModeDataList(ArServerClient *client, 
					    ArNetPacket *packet)

{
  ourModeDataMapMutex.lock();
  ArNetPacket sending;
  sending.uByte4ToBuf(ourModeDataMap.size());
  std::multimap<std::string, std::string>::iterator it;
  for (it = ourModeDataMap.begin(); it != ourModeDataMap.end(); it++)
  {
    sending.strToBuf((*it).first.c_str());
    sending.strToBuf((*it).second.c_str());
  }
  ourModeDataMapMutex.unlock();
  client->sendPacketTcp(&sending);
}

/**
   This will get the info about the current active mode
**/
AREXPORT void ArServerMode::getModeInfo(ArServerClient *client, 
					ArNetPacket *packet)
{
  ArNetPacket sending;
  buildModeInfoPacket(&sending);
  client->sendPacketTcp(&sending);
}

AREXPORT void ArServerMode::buildModeInfoPacket(ArNetPacket *sending)
{  
  if (ourActiveMode != NULL)
  {
    sending->strToBuf(ourActiveMode->getName());
    if (ourActiveModeLocked)
      sending->uByteToBuf(1);
    else
      sending->uByteToBuf(0);
    if (ourActiveModeWillUnlockIfRequested)
      sending->uByteToBuf(1);
    else
      sending->uByteToBuf(0);
  }
  else
  {
    sending->strToBuf("");
    sending->uByteToBuf(0);
    sending->uByteToBuf(0);
  }
  return;
}

AREXPORT void ArServerMode::checkBroadcastModeInfoPacket(void)
{
  if (ourActiveMode != ourBroadcastActiveMode || 
      ourActiveModeLocked != ourBroadcastActiveModeLocked ||
      ourActiveModeWillUnlockIfRequested != 
              ourBroadcastActiveModeWillUnlockIfRequested)
  {
    ArNetPacket sending;
    buildModeInfoPacket(&sending);
    myServer->broadcastPacketTcp(&sending, "getModeInfo");
  }
  
  ourBroadcastActiveMode = ourActiveMode;
  ourBroadcastActiveModeLocked = ourActiveModeLocked;
  ourBroadcastActiveModeWillUnlockIfRequested = ourActiveModeWillUnlockIfRequested;
}

AREXPORT ArServerMode* ArServerMode::getActiveMode(void) 
{ 
	return ourActiveMode; 
}
  
AREXPORT ArServerMode* ArServerMode::getLastActiveMode() 
{ 
	return ourLastActiveMode; 
}

AREXPORT ArServerModeIdle* ArServerMode::getIdleMode(void) { return ourIdleMode; }

AREXPORT std::list<ArServerMode *> *ArServerMode::getRequestedActivateModes(void)
{
  return &ourRequestedActivateModes;
}

AREXPORT void ArServerMode::getModeBusy(ArServerClient *client, 
					                              ArNetPacket *packet)
{
  ArNetPacket sending;
  sending.byteToBuf(ourIsBusy);
  client->sendPacketTcp(&sending);
}



