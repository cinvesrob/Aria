#include "Aria.h"
#include "ArExport.h"
#include "ArClientData.h"
#include "ArNetPacket.h"

AREXPORT ArClientData::ArClientData(
	const char *name, const char *description, unsigned int command,
	ArFunctor1<ArNetPacket *> *functor) :
    myDataFlagsBuilder(512, '|')
{ 
  myDataMutex.setLogName("ArClientData::myDataMutex");
  myMutex.setLogName("ArClientData::myMutex");
  myName = name; 
  myDescription = description; 
  myCommand = command; 
  if (functor != NULL)
    addFunctor(functor);
}

AREXPORT ArClientData::~ArClientData()
{

}

AREXPORT bool ArClientData::hasDataFlag(const char *dataFlag)
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



/// Sets the data flags
AREXPORT void ArClientData::addDataFlags(const char *dataFlags) 
{
  if (dataFlags != NULL)
  {
    myDataMutex.lock();
    myDataFlagsBuilder.add(dataFlags);
    myDataMutex.unlock();
  }
}
