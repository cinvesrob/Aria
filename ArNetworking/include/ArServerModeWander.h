#ifndef ARNETMODEWANDER_H
#define ARNETMODEWANDER_H

#include "ariaTypedefs.h"
#include "Aria.h"
#include "ArNetworking.h"
#include "ArServerMode.h"

class ArServerModeWander : public ArServerMode
{
public:
  AREXPORT ArServerModeWander(ArServerBase *server, ArRobot *robot);
  AREXPORT virtual ~ArServerModeWander();
  AREXPORT virtual void activate(void);
  AREXPORT virtual void deactivate(void);
  AREXPORT void wander(void);
  AREXPORT void netWander(ArServerClient *client, ArNetPacket *packet);
  AREXPORT virtual void userTask(void);
  AREXPORT virtual void checkDefault(void) { activate(); }
  AREXPORT virtual ArActionGroup *getActionGroup(void) {return &myWanderGroup;}
protected:
  ArActionGroupWander myWanderGroup;
  ArFunctor2C<ArServerModeWander, ArServerClient *, ArNetPacket *> myNetWanderCB;
};

#endif // ARNETMODEWANDER_H
