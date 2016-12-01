#ifndef ARNETMODEIDLE_H
#define ARNETMODEIDLE_H

#include "Aria.h"
#include "ArServerMode.h"

/// Mode that the ArServerMode infrastructure will activate if someone
/// tries to switch from one mode to another and there's idle
/// processing to be done...
class ArServerModeIdle : public ArServerMode
{
public:
  /// Constructor
  AREXPORT ArServerModeIdle(ArServerBase *server, ArRobot *robot);
  /// Destructor
  AREXPORT virtual ~ArServerModeIdle();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT virtual void userTask(void);
  /// Gets the mode that this mode interrupted (or NULL if none)
  AREXPORT void setModeInterrupted(ArServerMode *modeInterrupted);
  /// Gets the mode that this mode interrupted (or NULL if it didn't interrupt anything)
  AREXPORT ArServerMode *getModeInterrupted(void);
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
  ArServerMode *myModeInterrupted;
  ArFunctor2C<ArServerModeIdle, ArServerClient *, ArNetPacket *> myNetIdleCB;
};

#endif // ARNETMODEIDLE_H
