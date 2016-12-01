#ifndef ARNETMODESTOP_H
#define ARNETMODESTOP_H

#include "Aria.h"
#include "ArServerMode.h"

class ArServerModeStop : public ArServerMode
{
public:
  AREXPORT ArServerModeStop(ArServerBase *server, ArRobot *robot, 
			    bool defunct = false);
  AREXPORT virtual ~ArServerModeStop();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void stop(void);
  AREXPORT void netStop(ArServerClient *client, ArNetPacket *packet);
  AREXPORT virtual void userTask(void);
  AREXPORT virtual void checkDefault(void) { activate(); }
  AREXPORT virtual ArActionGroup *getActionGroup(void) { return &myStopGroup; }
  /// Adds to the config
  AREXPORT void addToConfig(ArConfig *config, const char *section = "Teleop settings");
  /// Sets whether we're using the range devices that depend on location
  AREXPORT void setUseLocationDependentDevices(
	  bool useLocationDependentDevices, bool internal = false);
  /// Gets whether we're using the range devices that depend on location
  AREXPORT bool getUseLocationDependentDevices(void);
protected:
  ArActionDeceleratingLimiter *myLimiterForward;
  ArActionDeceleratingLimiter *myLimiterBackward;
  ArActionDeceleratingLimiter *myLimiterLateralLeft;
  ArActionDeceleratingLimiter *myLimiterLateralRight;
  ArActionGroupStop myStopGroup;
  bool myUseLocationDependentDevices;
  ArFunctor2C<ArServerModeStop, ArServerClient *, ArNetPacket *> myNetStopCB;
};

#endif // ARNETMODESTOP_H
