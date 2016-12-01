#ifndef ARSERVERMODE_H
#define ARSERVERMODE_H

#include "Aria.h"
#include "ArServerBase.h"

class ArServerModeIdle;
class ArServerClient;

/// A mode for controlling the robot (only one active)

/**
   This is a lot like ArMode but for net control instead...

   Only one mode can control the robot at once, this is made to be
   subclassed.  Each subclass needs to implement activate and
   deactivate, activate MUST call baseActivate and if baseActivate
   returns false then the mode is NOT active and shouldn't do
   anything.  Further each class should set myMode and myStatus to the
   mode its in and the things its doing... You can lock your mode in
   with lockMode and unlock it with unlockMode but you should redefine
   the inherited requestUnlock and unlock yourself and deactivate
   gracefully.  

   If you want to implement a server mode as a default mode then you also
   need to implement checkDefault which will be called when nothing
   else is active

   Note that there's a new mechanism in place where if you use
   addModeData instead of addData on the ArServerBase the command will
   be placed into a map of available modes with commands that can be
   gotten by the client.  The active mode and whether its locked and
   will unlock and such can now be gotten too... this is all so that
   different commands can be disabled in client GUIs when they aren't
   available.
**/
class ArServerMode
{
public:
  /// Constructor
  AREXPORT ArServerMode(ArRobot *robot, ArServerBase *server, const char *name);
  /// Destructor
  AREXPORT virtual ~ArServerMode();
  /// This function should return the action group this mode uses
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return NULL; }
  /** The function called when the mode is activated. Subclasses must implement
    * this, but also call baseActivate().-
    */
  AREXPORT virtual void activate(void) = 0;
  /** The function called when the mode is deactivated. Subclasses must
   * implement this, and also call baseDeactivate().
   */
  AREXPORT virtual void deactivate(void) = 0;
  /// The function called if the mode is not activated because another mode superceded it.
  AREXPORT virtual void activationDenied(void) {}

  /// The ArMode's user task, don't need one, subclass must provide if needed
  AREXPORT virtual void userTask(void) {}
  /// This will be called if another mode wants the lock broken (can ignore)
  AREXPORT virtual void requestUnlock(void) { }
  /// This will be called if another mode has to break the lock
  AREXPORT virtual void forceUnlock(void);
  /// Locks this mode in until its unlocked (mode must be the active mode)
  AREXPORT void lockMode(bool willUnlockIfRequested = false);
  /// Unlocks the mode so other modes can activate
  AREXPORT void unlockMode(void);      
  /// Gets a string representing the mode we're in
  AREXPORT const char *getMode(void) const { return myMode.c_str(); }
  /// Gets a string representing the status of the mode we're in
  AREXPORT const char *getStatus(void) const { return myStatus.c_str(); }
  /// Gets a string representing the extended status of the mode we're
  /// in
  AREXPORT const char *getExtendedStatus(void) const { return myExtendedStatus.c_str(); }
  /// Gets the active mode string
  AREXPORT static const char *getActiveModeModeString(void);
  /// Gets the active status string
  AREXPORT static const char *getActiveModeStatusString(void);
  /// Gets the active extended status string
  AREXPORT static const char *getActiveModeExtendedStatusString(void);
  /// Gets the name of the mode we're in
  AREXPORT const char *getName(void) const { return myName.c_str(); }
  /// Gets if this mode is active or not
  AREXPORT bool isActive(void) const { return myIsActive; }
  /// Gets if the active mode is locked or not 
  AREXPORT static bool isLocked(void);
  /// Gets whether we'll unlock if requested or not
  AREXPORT static bool willUnlockIfRequested(void);
  /// Gets the active mode
  AREXPORT static ArServerMode *getActiveMode(void);
  /// Gets if we've set our activity time
  AREXPORT bool hasSetActivityTime(void) { return myHasSetActivityTime; }
  /// Gets the time of our last activity 
  AREXPORT ArTime getActivityTime(void);
  /// Sets that we're active right now
  AREXPORT void setActivityTimeToNow(void);
  /// Gets the seconds since the activity of our active mode
  AREXPORT static int getActiveModeActivityTimeSecSince(void);
  /// Gets if the active mode set the activity time this cycle or not 
  AREXPORT static bool getActiveModeSetActivityThisCycle(void);
  /// Sets this mode to default (so if a mode deactivates it activates this)
  AREXPORT void addAsDefaultMode(ArListPos::Pos pos = ArListPos::LAST);
  /// Sees if this wants to become the default mode
  /**
     This will be called when there would be no other modes, if this
     mode wants to take over it should activate itself, if it doesn't
     want to take over it shouldn't activate
   **/
  AREXPORT virtual void checkDefault(void) {}
  /// This should only be used by careful people and probably not then
  AREXPORT void setMode(const char *str) { myMode = str; }
  /// This should only be used by careful people and probably not then
  /**
     This changes the status of the mode and sets the
     myStatusSetThisCycle to true.  If the mode inheriting is setting
     the status in cycles then it should use this status instead and
     then clear the myStatusSetThisFlag to false.
   **/
  AREXPORT void setStatus(const char *str) 
    { myStatus = str; myStatusSetThisCycle = true; }
  /// Adds data to the list of this mode's commands
  
 
  /// Returns whether the mode should be automatically resumed after it was interrupted
  /**
   * This method primarily applies when the mode has been interrupted by a 
   * forced docking situation.  In that case, when the docking mode is complete,
   * it will resume the interrupted mode -- if this method returns true.  This
   * would primarily be applicable to very long, complex modes.
   *
   * The default return value is false.
  **/
  AREXPORT virtual bool isAutoResumeAfterInterrupt();
 

  AREXPORT bool addModeData(
	  const char *name, const char *description,
	  ArFunctor2<ArServerClient *, ArNetPacket *> *functor,
	  const char *argumentDescription, const char *returnDescription,
	  const char *commandGroup = NULL, const char *dataFlags = NULL);
  /// Gets the list of data for each mode
  AREXPORT static void getModeDataList(ArServerClient *client, 
				       ArNetPacket *packet);
  /// Gets the info about which mode is active and such
  AREXPORT static void getModeInfo(ArServerClient *client, 
				   ArNetPacket *packet);
  /// Handles the packet request for the mode busy state.
  AREXPORT static void getModeBusy(ArServerClient *client,
                                   ArNetPacket *packet);
  
  /// Adds a callback for when this class is activated
  void addActivateCallback(
	  ArFunctor *functor, int position = 50) 
    { myActivateCallbacks.addCallback(functor, position); }
  /// Removes a callback for when this class is activated
  void remActivateCallback(ArFunctor *functor)
    { myActivateCallbacks.remCallback(functor); }
  /// Adds a callback for when this class is deactivated
  void addDeactivateCallback(
	  ArFunctor *functor, int position = 50) 
    { myDeactivateCallbacks.addCallback(functor, position); }
  /// Removes a callback for when this class is deactivated
  void remDeactivateCallback(ArFunctor *functor)
    { myDeactivateCallbacks.remCallback(functor); }

  /// Adds a single shot callback for when this class is deactivated
  void addSingleShotDeactivateCallback(
	  ArFunctor *functor, int position = 50) 
    { mySingleShotDeactivateCallbacks.addCallback(functor, position); }
  /// Removes a single shot callback for when this class is deactivated
  void remSingleShotDeactivateCallback(ArFunctor *functor)
    { mySingleShotDeactivateCallbacks.remCallback(functor); }


  /// Adds a single shot callback for just after the deactivate happens (when the next mode has activated)
  void addSingleShotPostDeactivateCallback(
	  ArFunctor *functor, int position = 50) 
    { mySingleShotPostDeactivateCallbacks.addCallback(functor, position); }
  /// Removes a single shot callback for just after the deactivate happens (when the next mode has activated)
  void remSingleShotPostDeactivateCallback(ArFunctor *functor)
    { mySingleShotPostDeactivateCallbacks.remCallback(functor); }

  /// Call that gets our idle mode
  AREXPORT static ArServerModeIdle *getIdleMode(void);

#ifndef SWIG
  /// Internal call to set the activity time (this is dangerous and
  /// shouldn't be used)
  /// @internal
  void internalSetActivityTime(ArTime time);
#endif

protected:
  AREXPORT static void modeUserTask(void);
  AREXPORT static void buildModeInfoPacket(ArNetPacket *packet);
  AREXPORT static std::list<ArServerMode *> *getRequestedActivateModes(void);
  AREXPORT void checkBroadcastModeInfoPacket(void);
  /** Activates this mode if it can (returns true if it can, false otherwise).
      If we have an ArRobot instance (myRobot), then robot motion is stopped.
      Activity time is reset, and activate callbacks are called.
  */
  AREXPORT bool baseActivate(bool canSelfActivateIfLocked = false);
  /** Deactivates this mode. Deactivation callbacks are called. The next mode to
      be activated is activated, if any. (E.g. the global default mode)
  */
  AREXPORT void baseDeactivate(void);

  ArCallbackList myActivateCallbacks;
  ArCallbackList myDeactivateCallbacks;
  ArCallbackList mySingleShotDeactivateCallbacks;
  ArCallbackList mySingleShotPostDeactivateCallbacks;

  bool myIsActive;
  ArRobot *myRobot;
  ArServerBase *myServer;
  std::string myName;
  // inheritors will want to play with these two strings as they are
  // what is shown to the user
  std::string myMode;
  std::string myStatus;
  bool myStatusSetThisCycle;
  std::string myExtendedStatus;

  // variables for activity time (so we can do things if idle) this
  // isn't static so that each mode can have its own
  // 'myHasSetActivityTime' so that if a mode doesn't use this scheme
  // it doesn't get tromped on
  bool myHasSetActivityTime;
  bool mySetActivityThisCycle;
  ArTime myActivityTime;
  ArMutex myActivityTimeMutex;
  
  AREXPORT static bool ourActiveModeLocked;
  AREXPORT static bool ourActiveModeWillUnlockIfRequested;
  AREXPORT static ArServerMode *ourActiveMode;
  AREXPORT static ArServerMode *ourNextActiveMode;
  AREXPORT static ArServerMode *ourLastActiveMode;
  AREXPORT static bool ourActiveModeSetActivityThisCycle;

  AREXPORT ArServerMode *getLastActiveMode();

  AREXPORT static bool ourIsBusy;
  AREXPORT static ArServerBase *ourServerBase;

  AREXPORT static std::list<ArServerMode *> ourModes;
  AREXPORT static std::list<ArServerMode *> ourDefaultModes;
  AREXPORT static std::list<ArServerMode *> ourRequestedActivateModes;
  AREXPORT static std::multimap<std::string, std::string> ourModeDataMap;
  AREXPORT static ArMutex ourModeDataMapMutex;
  
  AREXPORT static ArGlobalFunctor2<ArServerClient *, 
				   ArNetPacket *> ourGetModeDataListCB;
  AREXPORT static ArGlobalFunctor2<ArServerClient *, 
				   ArNetPacket *> ourGetModeInfoCB;
  AREXPORT static ArGlobalFunctor2<ArServerClient *, 
				   ArNetPacket *> ourGetModeBusyCB;

  AREXPORT static bool ourBroadcastActiveModeLocked;
  AREXPORT static ArServerMode *ourBroadcastActiveMode;
  AREXPORT static bool ourBroadcastActiveModeWillUnlockIfRequested;
  AREXPORT static ArGlobalFunctor ourUserTaskCB;    
  AREXPORT static bool ourUserTaskAdded;  
  // so we can easily just get the verbose out of this one
  ArLog::LogLevel myVerboseLogLevel;

  AREXPORT static ArServerModeIdle *ourIdleMode;
  AREXPORT static ArMutex ourIdleModeMutex;
  AREXPORT static bool ourIdleModeCreated;
};

#endif // ARSERVERMODE_H
