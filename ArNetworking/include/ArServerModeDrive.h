#ifndef ARSERVERMODEDRIVE_H
#define ARSERVERMODEDRIVE_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArNetworking.h"
#include "ArServerMode.h"

class ArServerModeDrive : public ArServerMode
{
public:
  AREXPORT ArServerModeDrive(ArServerBase *server, ArRobot *robot,
			     bool takeControlOnJoystick = false);
  AREXPORT virtual ~ArServerModeDrive();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  /// This adds commands that'll let you disable and enable safe driving
  AREXPORT void addControlCommands(ArServerHandlerCommands *handlerCommands);
  AREXPORT void driveJoystick(double vel, 
                              double rotVel, 
                              bool isActivating = true);
  AREXPORT void serverDriveJoystick(ArServerClient *client,
				    ArNetPacket *packet);
  AREXPORT virtual void userTask(void);
  AREXPORT void setThrottleParams(int lowSpeed, int highSpeed);
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myDriveGroup;}
  AREXPORT void setSafeDriving(bool safe);
  AREXPORT bool getSafeDriving(void);
  /// this action will be activated in unsafe mode
  AREXPORT void setExtraUnsafeAction(ArAction *action) 
    { myExtraUnsafeAction = action; }
protected:
  AREXPORT void serverSafeDrivingEnable(void);
  AREXPORT void serverSafeDrivingDisable(void);
  AREXPORT void joyUserTask(void);
  ArAction *myExtraUnsafeAction;
  ArJoyHandler *myJoyHandler;
  ArActionJoydrive myJoydriveAction;
  ArActionInput *myInputAction;
  ArActionStop myStopAction;
  ArActionGroupInput myDriveGroup;
  ArFunctor2C<ArServerModeDrive, ArServerClient *, ArNetPacket *> myServerDriveJoystickCB;
  ArFunctorC<ArServerModeDrive> myJoyUserTaskCB;
  bool myDriveSafely;
  bool myNewDriveSafely;
  double myVel;
  double myRotVel;
  bool myTakeControlOnJoystick;
  // for the simple commands
  ArServerHandlerCommands *myHandlerCommands;
  ArFunctorC<ArServerModeDrive> myServerSafeDrivingEnableCB;
  ArFunctorC<ArServerModeDrive> myServerSafeDrivingDisableCB;
};


#endif // ARNETMODEDRIVE_H
