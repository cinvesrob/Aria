#include "Aria.h"
#include "ArExport.h"
#include "ArServerInfoDrawings.h"

AREXPORT ArServerInfoDrawings::ArServerInfoDrawings(ArServerBase *server) :
  myNetListDrawingsCB(this, &ArServerInfoDrawings::netListDrawings),
  myNetGetDrawingListCB(this, &ArServerInfoDrawings::netGetDrawingList)
{
  myServer = server;

  if (myServer != NULL)
  {
    myServer->addData("listDrawings", 
		      "Deprecated; use getDrawingList instead. Gets a list of things that can be drawn",
		      &myNetListDrawingsCB, 
		      "none", 
		      "byte4: numDrawings, <repeats numDrawings> string: name, string: shape, byte4: primaryColor(0RGB), byte4: size, byte4: layer, byte4: defaultRefreshTime(0 == don't request refresh), byte4: secondaryColor(0RGB)", "SensorInfo", "RETURN_SINGLE");
     myServer->addData("getDrawingList", 
		      "Gets a list of items to be drawn on the map. Each packet contains one drawing.  Terminates on empty packet.",
		      &myNetGetDrawingListCB, 
		      "none", 
          "For each drawing, a packet that contains: string: name, string: shape, byte4: primaryColor(0RGB), byte4: size, byte4: layer, byte4: defaultRefreshTime(0 == don't request refresh), byte4: secondaryColor(0RGB), string: visibility", 
          "SensorInfo", "RETURN_UNTIL_EMPTY");
 }

}

AREXPORT ArServerInfoDrawings::~ArServerInfoDrawings() 
{

}

/**
   @note 'name' must be unique.
   If the name is unique, the given drawing data is stored to be returned
   in response to the "listDrawings" client request, and a new data request
   is created with the given name and functor. This functor must send
   a reply packet to the client containing a 4-byte integer indicating the 
   number of coordinates or vertices in the figure, followed by a pair of 
   4-byte integers for each coordinate or vertex in global map space.
   For example, if drawingData describes a polyDots shape,
   then the functor must create a packet, add a 4-byte integer indicating the
   number of dots, then for each dot, add an integer for the X coordinate followed
   by an integer for the Y coordinate.
 */
AREXPORT bool ArServerInfoDrawings::addDrawing(ArDrawingData *drawingData, 
					   const char *name,
		         ArFunctor2<ArServerClient *, ArNetPacket *> *functor)
{
  if (myDrawingDatas.find(name) != myDrawingDatas.end() || 
      myDrawingCallbacks.find(name) != myDrawingCallbacks.end())
  {
    ArLog::log(ArLog::Normal, "ArServerInfoDrawings::addDrawing: Already a drawing of name '%s'", name);
    return false;
  }
  if (myServer == NULL || 
      !myServer->addData(name, "", functor, "none", 
			"See getDrawingList for the information on how this drawing should be drawn, and the documentation in ArDrawingData for that information means.", "SensorInfo",
			 "RETURN_SINGLE"))
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerInfoDrawings::addDrawing: Could not add data of name '%s' to server", 
	       name);
    return false;
  }
  myDrawingDatas[name] = drawingData;
  myDrawingCallbacks[name] = functor;
  return true;
}

AREXPORT void ArServerInfoDrawings::netListDrawings(ArServerClient *client, 
						    ArNetPacket *packet)
{
  ArNetPacket sendingPacket;
  std::map<std::string, ArDrawingData *, ArStrCaseCmpOp>::iterator it;
  
  sendingPacket.byte4ToBuf(myDrawingDatas.size());
  for (it = myDrawingDatas.begin(); it != myDrawingDatas.end(); it++)
  {
    sendingPacket.strToBuf((*it).first.c_str());
    sendingPacket.strToBuf((*it).second->getShape());
    sendingPacket.byte4ToBuf((*it).second->getPrimaryColor().colorToByte4());
    sendingPacket.byte4ToBuf((*it).second->getSize());
    sendingPacket.byte4ToBuf((*it).second->getLayer());
    sendingPacket.uByte4ToBuf((*it).second->getDefaultRefreshTime());
    sendingPacket.byte4ToBuf((*it).second->getSecondaryColor().colorToByte4());
  }
  client->sendPacketTcp(&sendingPacket);
}

AREXPORT void ArServerInfoDrawings::netGetDrawingList(ArServerClient *client, 
						    ArNetPacket *packet)
{
  ArNetPacket sendingPacket;
  
  // TODO: Any need to protect the map by a mutex?

  for (std::map<std::string, ArDrawingData *, ArStrCaseCmpOp>::iterator it = 
                myDrawingDatas.begin(); 
       it != myDrawingDatas.end(); 
       it++)
  {
    sendingPacket.empty();

    sendingPacket.strToBuf((*it).first.c_str());
    sendingPacket.strToBuf((*it).second->getShape());
    sendingPacket.byte4ToBuf((*it).second->getPrimaryColor().colorToByte4());
    sendingPacket.byte4ToBuf((*it).second->getSize());
    sendingPacket.byte4ToBuf((*it).second->getLayer());
    sendingPacket.uByte4ToBuf((*it).second->getDefaultRefreshTime());
    sendingPacket.byte4ToBuf((*it).second->getSecondaryColor().colorToByte4());
    sendingPacket.strToBuf((*it).second->getVisibility());

    client->sendPacketTcp(&sendingPacket);

  } // end for each drawing

  sendingPacket.empty();
  client->sendPacketTcp(&sendingPacket);
}

AREXPORT bool ArServerInfoDrawings::addRangeDevice(ArRangeDevice *rangeDevice)
{
  bool ret = true;
  char name[512];
  if (myServer == NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerInfoDrawings::addRangeDeviceForDrawing: srever is NULL");
    return false;
  }
  sprintf(name, "%sCurrent", rangeDevice->getName());
  // LEAK a little when called, shouldn't be called more than a few
  // times and isn't a leak unless its called more than once
  if (rangeDevice->getCurrentDrawingData() != NULL && 
      !addDrawing(rangeDevice->getCurrentDrawingData(), name, 
		  new ArFunctor3C<ArServerInfoDrawings, 
		  ArServerClient *, ArNetPacket *, 
		  ArRangeDevice *>(this, &ArServerInfoDrawings::netRangeDeviceCurrent, NULL, NULL, rangeDevice)))
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerInfoDrawings::addRangeDevice: Could not add data for range device '%s' to server ('%s')", rangeDevice->getName(), name);
    ret = false;
  }
  sprintf(name, "%sCumulative", rangeDevice->getName());
  if (rangeDevice->getCumulativeDrawingData() != NULL && 
      !addDrawing(rangeDevice->getCumulativeDrawingData(), name, 
		  new ArFunctor3C<ArServerInfoDrawings, 
		  ArServerClient *, ArNetPacket *, 
		  ArRangeDevice *>(this, &ArServerInfoDrawings::netRangeDeviceCumulative, NULL, NULL, rangeDevice)))
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerInfoDrawings::addRangeDevice: Could not add data for range device '%s' to server ('%s')", rangeDevice->getName(), name);
    ret = false;
  }
  return ret;
}

AREXPORT bool ArServerInfoDrawings::addRobotsRangeDevices(ArRobot *robot)
{
  std::list<ArRangeDevice *>::iterator it;
  bool ret = true;
  ArRangeDevice *device;
  //printf("0\n");
  if (robot == NULL || robot->getRangeDeviceList() == NULL)
  {
    ArLog::log(ArLog::Terse, "InfoDrawings::addRobotsRangeDevices: Robot or robot's range device list is NULL");
    return false;
  }
  //printf("1\n");
  for (it = robot->getRangeDeviceList()->begin();
       it != robot->getRangeDeviceList()->end();
       it++)
  {
    device = (*it);
    //printf("2 %s\n", device->getName());
    device->lockDevice();
    if (!addRangeDevice(device))
      ret = false;
    device->unlockDevice();
  }
  //printf("3\n");
  return ret;
}

AREXPORT void ArServerInfoDrawings::netRangeDeviceCurrent(
	ArServerClient *client, ArNetPacket *packet, ArRangeDevice *device)
{
  ArNetPacket sendPacket;
  std::list<ArPoseWithTime *> *readings;
  std::list<ArPoseWithTime *>::iterator it;

  device->lockDevice();
  readings = device->getCurrentBuffer();
  if (readings == NULL)
  {
    ArLog::log(ArLog::Verbose, "ArServerInfoDrawing::netRangeDeviceCurrent: No current buffer for %s", device->getName());
    device->unlockDevice();
    sendPacket.byte4ToBuf(0);
    client->sendPacketUdp(&sendPacket);
    return;
  } 
  
  sendPacket.byte4ToBuf(readings->size());
  for (it = readings->begin(); it != readings->end(); it++)
  {
    sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getX()));
    sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getY()));
  }
  device->unlockDevice();
  client->sendPacketUdp(&sendPacket);

}


AREXPORT void ArServerInfoDrawings::netRangeDeviceCumulative(
	ArServerClient *client, ArNetPacket *packet, ArRangeDevice *device)
{
  ArNetPacket sendPacket;
  std::list<ArPoseWithTime *> *readings;
  std::list<ArPoseWithTime *>::iterator it;

  device->lockDevice();
  readings = device->getCumulativeBuffer();
  if (readings == NULL)
  {
    ArLog::log(ArLog::Verbose, "ArServerInfoDrawing::netRangeDeviceCumulative: No cumulative buffer for %s", device->getName());
    device->unlockDevice();
    sendPacket.byte4ToBuf(0);
    client->sendPacketUdp(&sendPacket);
    return;
  } 
  
  sendPacket.byte4ToBuf(readings->size());
  for (it = readings->begin(); it != readings->end(); it++)
  {
    sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getX()));
    sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getY()));
  }
  device->unlockDevice();
  client->sendPacketUdp(&sendPacket);

}


AREXPORT ArDrawingData *ArServerInfoDrawings::internalGetDrawingData(
	const char *name)
{
  if (myDrawingDatas.find(name) == myDrawingDatas.end())
  {
    ArLog::log(ArLog::Normal, "ArServerInfoDrawings::internalGetDrawingData: No drawing of name '%s'", name);
    return NULL;
  }
  return myDrawingDatas[name];  
}

AREXPORT ArFunctor2<ArServerClient *, ArNetPacket *> *ArServerInfoDrawings::internalGetDrawingCallback(const char *name)
{
  if (myDrawingCallbacks.find(name) == myDrawingCallbacks.end())
  {
    ArLog::log(ArLog::Normal, "ArServerInfoDrawings::internalGetDrawingCallback: No drawing of name '%s'", name);
    return NULL;
  }
  return myDrawingCallbacks[name];  
}

