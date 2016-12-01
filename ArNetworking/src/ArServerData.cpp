#include "Aria.h"
#include "ArExport.h"
#include "ArServerData.h"
#include "ArServerClient.h"
#include "ArNetPacket.h"

AREXPORT ArServerData::ArServerData(
	const char *name, const char *description, unsigned int command,
	ArFunctor2<ArServerClient *, ArNetPacket *> *functor,
	const char *argumentDescription, const char *returnDescription,
	const char *commandGroup, const char *dataFlags,
	ArRetFunctor1<long, unsigned int> *getFrequencyFunctor,
	ArFunctor2<long, unsigned int> *requestChangedFunctor,
	ArRetFunctor2<bool, ArServerClient *, ArNetPacket *> 
	*requestOnceFunctor) : 
  myDataFlagsBuilder(512, '|')
{ 
  myDataMutex.setLogName("ArServerData::myDataMutex");
  myName = name; 
  myDescription = description; 
  myReturnDescription = returnDescription;
  myArgumentDescription = argumentDescription;
  myCommand = command;
  myFunctor = functor; 
  if (commandGroup != NULL)
    myCommandGroup = commandGroup;
  else
    myCommandGroup = "";
  myGetFrequencyFunctor = getFrequencyFunctor;
  myRequestChangedFunctor = requestChangedFunctor;
  myRequestOnceFunctor = requestOnceFunctor;
  if (dataFlags != NULL)
  {
    myDataFlagsBuilder.add(dataFlags);
  }

  mySlowPacket = hasDataFlag("SLOW_PACKET");
  myIdlePacket = hasDataFlag("IDLE_PACKET");
}

AREXPORT ArServerData::~ArServerData()
{

}

AREXPORT bool ArServerData::hasDataFlag(const char *dataFlag)
{
  myDataMutex.lock();
  size_t i;
  for (i = 0; i < myDataFlagsBuilder.getArgc(); i++)
  {
    if (strcmp(myDataFlagsBuilder.getArg(i), dataFlag) == 0)
    {
      myDataMutex.unlock();
      return true;
    }
  }

  myDataMutex.unlock();
  return false;
}

AREXPORT bool ArServerData::addDataFlags(const char *dataFlags)
{
  myDataMutex.lock();
  myDataFlagsBuilder.add(dataFlags);
  myDataMutex.unlock();
  return true;
}

AREXPORT bool ArServerData::remDataFlag(const char *dataFlag)
{
  myDataMutex.lock();
  size_t i;
  for (i = myDataFlagsBuilder.getArgc(); i < myDataFlagsBuilder.getArgc(); i++)
  {
    if (strcmp(myDataFlagsBuilder.getArg(i), dataFlag) == 0)
    {
      myDataFlagsBuilder.removeArg(i);
      myDataMutex.unlock();
      return true;
    }
  }

  myDataMutex.unlock();
  return false;
}

AREXPORT void ArServerData::callRequestChangedFunctor(void)
{ 
  if (myGetFrequencyFunctor == NULL || myRequestChangedFunctor == NULL)
    return;
  myRequestChangedFunctor->invoke(
	  myGetFrequencyFunctor->invokeR(myCommand), myCommand);
}
