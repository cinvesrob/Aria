/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2015 Adept Technology, Inc.
Copyright (C) 2016 Omron Adept Technologies, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
#include "Aria.h"
#include "ArExport.h"
#include "ArServerHandlerMap.h"
#ifdef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "ArMap.h"

/**
   @param server the server to add our data too

   @param arMap If this points to a map file then this will simply
   serve up that map file and add in a map changed cb for that map,
   otherwise it'll operate via the Aria::getConfig.

   @param dataToSend Which data to send, just the lines, the points,
   or both
 **/
AREXPORT ArServerHandlerMap::ArServerHandlerMap(ArServerBase *server, 
						                                    ArMapInterface *arMap,
						                                    DataToSend dataToSend) :
  myGetMapIdCB(this, &ArServerHandlerMap::serverGetMapId),
  myGetMapNameCB(this, &ArServerHandlerMap::serverGetMapName),
  myGetMapCB(this, &ArServerHandlerMap::serverGetMap),
  myGetMapBinaryCB(this, &ArServerHandlerMap::serverGetMapBinary),
  myGetMapMultiScansCB(this, &ArServerHandlerMap::serverGetMapMultiScans),
  myGetMapMaxCategoryCB(this, &ArServerHandlerMap::serverGetMapWithMaxCategory),
  myGetGoalsCB(this, &ArServerHandlerMap::serverGetGoals),
  myCheckMapCB(this, &ArServerHandlerMap::handleCheckMap),
  myProcessFileCB(this, &ArServerHandlerMap::processFile),
  myMapChangedCB(this, &ArServerHandlerMap::mapChanged)
{
  myServer = server;
  myOwnMap = false;
  myMap = arMap;
  setDataToSend(dataToSend);
  myMapChangedCB.setName("ArServerHandlerMap");
  myProcessFileCB.setName("ArServerHandlerMap");
  if (myMap != NULL)
  {
    strcpy(myMapFileName, myMap->getFileName());
    myAlreadyLoaded = false;
    myOwnMap = false;
    myMap->addMapChangedCB(&myMapChangedCB, ArListPos::FIRST);
  }
  else
  {
    myMapFileName[0] = '\0';
    Aria::getConfig()->addParam(ArConfigArg("Map", myMapFileName, 
					    "map file to load", 
					    sizeof(myMapFileName)),
              "Files", 
              ArPriority::IMPORTANT);
    Aria::getConfig()->setSectionComment("Files", 
					 "The files where important data is stored");
    Aria::getConfig()->addProcessFileCB(&myProcessFileCB, 95);
  }


  if (myServer != NULL)
  {
    myServer->addData("getMapId", "Gets the ID of the map being used",
		                  &myGetMapIdCB, 
                      "none", 
                      "string: map name (empty string if no maps)",  // TODO! 
                      "Map", 
                      "RETURN_SINGLE");
    
    myServer->addData("getMapName", "gets the name of the map being used",
		      &myGetMapNameCB, "none", "string: map name (empty string if no maps)", "Map", "RETURN_SINGLE");

    // The "getMapBinary" request replaces the old text-based "getMap" request.
    myServer->addData("getMapBinary", "gets the map objects as ascii and the data points as binary", 
		      &myGetMapBinaryCB, 
		      "none", 
		      "packets of '<string>: line' for header, followed by packets of '<byte4>: numPtsInPacket, (<double>:x, <double>:y)*' until numPtsInPacket == 0",
		      "Map", "RETURN_UNTIL_EMPTY");

    // 
    myServer->addData("getMapMultiScans", 
                      "Deprecated; getMapWithMaxCategory is preferred", 
		                  &myGetMapMultiScansCB, 
		                  "none", 
		                  "packets of '<string>: line' for header, followed by packets of '<byte4>: numPtsInPacket, (<double>:x, <double>:y)*' until numPtsInPacket == 0",
		                  "Map", "RETURN_UNTIL_EMPTY");
	  
    myServer->addData("getMapWithMaxCategory", 
                      "Requests the map with the specified maximum features; header and info are ascii, data points and lines are binary", 
                      &myGetMapMaxCategoryCB,
                      "string: category (one of the constants defined in ArMapInterface)", 
		                  "packets of '<string>: line' for header, followed by packets of '<byte4>: numPtsInPacket, (<double>:x, <double>:y)*' until numPtsInPacket == 0",
		                  "Map", "RETURN_UNTIL_EMPTY");
  
    
    
    myServer->addData("getMap", "gets the map as a set of ascii lines", 
		            &myGetMapCB, "none", 
			  "packets of '<string>: line' followed by a packet with an empty string to denote end (if only empty string then no map)",
			  "Map", "RETURN_UNTIL_EMPTY");
    myServer->addData("mapUpdated", "a single packet is sent to this when the map is updated and this denotes a new getMap and getMapName should be requested", 
		      NULL, "none", "none", 
		      "Map", "RETURN_SINGLE");
    myServer->addData("getGoals", "gets the list of goals", 
		      &myGetGoalsCB, "none", 
		      "<repeat> string: goal", "Map", "RETURN_SINGLE");
    myServer->addData("goalsUpdated", "a single packet is sent to this when the goals are updated and this denotes a new getGoals should be requested", 
		      NULL, "none", "none", 
		      "Map", "RETURN_SINGLE");

    myServer->addData("checkMap", 
                      "Requests that the server check whether the map needs to be read",
		                  &myCheckMapCB, 
                      "none", 
                      "none",
                      "Map", 
                      "RETURN_NONE|IDLE_PACKET");


  }
}

AREXPORT ArServerHandlerMap::~ArServerHandlerMap()
{
  
}

AREXPORT bool ArServerHandlerMap::loadMap(const char *mapFile)
{
  ArLog::log(ArLog::Normal,
             "ArServerHandlerMap::loadMap(%s)",
             mapFile);

  ArNetPacket emptyPacket;
  if (myMap != NULL)
  {
    if (myOwnMap)
      delete myMap;
    myMap = NULL;
  }
  myMap = new ArMapSimple(NULL); 
  
  myMapName = mapFile;
  myOwnMap = true;
  bool ret = myMap->readFile(mapFile);
  
  myServer->broadcastPacketTcp(&emptyPacket, "mapUpdated");
  myServer->broadcastPacketTcp(&emptyPacket, "goalsUpdated");
  
  return ret;
}


/**
   Use the map object given, note that this will not take ownership of the map unless you tell 
 **/
AREXPORT void ArServerHandlerMap::useMap(ArMapInterface *mapObj, 
					 bool takeOwnershipOfMap)
{
  ArNetPacket emptyPacket;
  if (myMap != NULL)
  {
    if (myOwnMap)
      delete myMap;
    myMap = NULL;
  }
  myMap = mapObj;
  myMapName = myMap->getFileName();
  myOwnMap = takeOwnershipOfMap;
  myServer->broadcastPacketTcp(&emptyPacket, "mapUpdated");
  myServer->broadcastPacketTcp(&emptyPacket, "goalsUpdated");
}

AREXPORT ArMapInterface *ArServerHandlerMap::getMap(void)
{
  return myMap;
}


/** @internal */
AREXPORT void ArServerHandlerMap::serverGetMapId(ArServerClient *client, 
                                                 ArNetPacket *packet)
{
   ArLog::log(ArLog::Normal,
              "ArServerHandlerMap::serverGetMapId() map ID requested by %s",
              ((client != NULL) ? client->getIPString() : "NULL"));

  // To force the problem described in Bug 11160 to become reproducible 
  // instead of highly intermittent, uncomment the following sleep.
  // ArUtil::sleep(1000);


  ArNetPacket sendPacket;
  ArMapId mapId;
  if (myMap != NULL)
  {
    bool isSuccess = myMap->getMapId(&mapId);
    if (isSuccess) {
    }
    else {
      mapId = ArMapId();
      ArLog::log(ArLog::Normal,
                 "ArServerHandlerMap::serverGetMapId() error getting map ID from map");
    }
  }
  else {
    ArLog::log(ArLog::Normal,
               "ArServerHandlerMap::serverGetMapId() map is NULL");

  }
  
  mapId.log("ArServerHandlerMap::serverGetMapId");


  ArMapId::toPacket(mapId,
                    &sendPacket);

  
  client->sendPacketTcp(&sendPacket);


} // end method serverGetMapId


/** @internal */
AREXPORT void ArServerHandlerMap::serverGetMapName(ArServerClient *client, 
                                                   ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  if (myMap == NULL)
  {
    sendPacket.strToBuf("");
    client->sendPacketTcp(&sendPacket);
  }
  else
  {
    sendPacket.strToBuf(myMap->getFileName());
    client->sendPacketTcp(&sendPacket);
  }
}

/** @internal */
AREXPORT void ArServerHandlerMap::writeMapToClient(const char *line,
					 ArServerClient *client)
{
  ArNetPacket sendPacket;
  sendPacket.strToBuf(line);
  client->sendPacketTcp(&sendPacket);
}

/** @internal */
AREXPORT void ArServerHandlerMap::writePointsToClient(
	int pointCount,
	std::vector<ArPose> *points,
	ArServerClient *client)
{
  ArNetPacket sendPacket;
  
  // Should never happen...
  if (points == NULL) {
    // Send 0 points just so the client doesn't hang
    sendPacket.byte4ToBuf(0);
    client->sendPacketTcp(&sendPacket);
    return;
  }
  
  ArLog::log(ArLog::Verbose, 
	     "ArServerHandlerMap::writePointsToClient() pointCount = %i, vector size = %i", 
	     pointCount, (int) points->size());
  
  // Neither should this, but just in case...
  if (pointCount > (int) points->size()) {
    pointCount = points->size();
  }
  
  int maxInPacketCount = 1000; // Maximum number of points sent in a packet
  int currentCount = 0;		// Number put into the current packet 
  int remainingCount = pointCount; // Total number of points remaining to be sent
  bool isStartPacket = true;	// Whether a new packet is being started
  
  int totalCount = 0;
  
  for (std::vector<ArPose>::iterator pointIt = points->begin(); 
       pointIt != points->end();
       pointIt++) {
    
    // Start a new packet if the previous one was sent (or there is no 
    // previous one)
    if (isStartPacket) {
      
      isStartPacket = false;
      
      sendPacket.empty();
      currentCount = 0;
      
      // Leftover points...
      if (maxInPacketCount > remainingCount) {
	maxInPacketCount = remainingCount;
      }
      
      // The first item in the packet is the number of points contained
      sendPacket.byte4ToBuf(maxInPacketCount);
      
    } // end if starting a new packet
    
    // Add the current point to the packet (think that the test should 
    // always be true)
    if (currentCount < maxInPacketCount) {
      
      currentCount++;
      remainingCount--;
      
      sendPacket.byte4ToBuf(
	      ArMath::roundInt((*pointIt).getX()));
      sendPacket.byte4ToBuf(
	      ArMath::roundInt((*pointIt).getY()));
    }
    
    // If the packet is full, send it and then start a new one
    if (currentCount == maxInPacketCount) {
      
      totalCount += currentCount;
      
      client->sendPacketTcp(&sendPacket);
      //ArUtil::sleep(1);
      
      isStartPacket = true;
    }
    
  } // end for each point
  
  ArLog::log(ArLog::Verbose, 
	     "ArServerHandlerMap::writePointsToClient() totalCount = %i", 
	     totalCount);
  
  // Send 0 points to indicate that all have been sent
  if (false) {
  sendPacket.empty();
  sendPacket.byte4ToBuf(0);
  client->sendPacketTcp(&sendPacket);
  }

} // end writePointsToClient

/** @internal */
AREXPORT void ArServerHandlerMap::writeLinesToClient(
	int lineCount,
	std::vector<ArLineSegment> *lines,
	ArServerClient *client)
{
  ArNetPacket sendPacket;
  
  // Should never happen...
  if (lines == NULL) {
    // Send 0 points just so the client doesn't hang
    sendPacket.byte4ToBuf(0);
    client->sendPacketTcp(&sendPacket);
    return;
  }
  
  ArLog::log(ArLog::Verbose, 
	     "ArServerHandlerMap::writeLinesToClient() pointCount = %i, vector size = %i", 
	     lineCount, (int) lines->size());
  
  // Neither should this, but just in case...
  if (lineCount > (int) lines->size()) {
    lineCount = lines->size();
  }
  
  int maxInPacketCount = 1000; // Maximum number of points sent in a packet
  int currentCount = 0;		// Number put into the current packet 
  int remainingCount = lineCount; // Total number of points remaining to be sent
  bool isStartPacket = true;	// Whether a new packet is being started
  
  int totalCount = 0;
  
  for (std::vector<ArLineSegment>::iterator lineIt = lines->begin(); 
       lineIt != lines->end();
       lineIt++) 
  {
    
    // Start a new packet if the previous one was sent (or there is no 
    // previous one)
    if (isStartPacket) {
      
      isStartPacket = false;
      
      sendPacket.empty();
      currentCount = 0;
      
      // Leftover points...
      if (maxInPacketCount > remainingCount) {
	maxInPacketCount = remainingCount;
      }
      
      // The first item in the packet is the number of points contained
      sendPacket.byte4ToBuf(maxInPacketCount);
      
    } // end if starting a new packet
    
    // Add the current point to the packet (think that the test should 
    // always be true)
    if (currentCount < maxInPacketCount) {
      
      currentCount++;
      remainingCount--;
      
      sendPacket.byte4ToBuf(
	      ArMath::roundInt((*lineIt).getX1()));
      sendPacket.byte4ToBuf(
	      ArMath::roundInt((*lineIt).getY1()));
      sendPacket.byte4ToBuf(
	      ArMath::roundInt((*lineIt).getX2()));
      sendPacket.byte4ToBuf(
	      ArMath::roundInt((*lineIt).getY2()));
    }
    
    // If the packet is full, send it and then start a new one
    if (currentCount == maxInPacketCount) {
      
      totalCount += currentCount;
      
      client->sendPacketTcp(&sendPacket);
      //ArUtil::sleep(1);
      
      isStartPacket = true;
    }
    
  } // end for each point
  
  ArLog::log(ArLog::Verbose, 
	     "ArServerHandlerMap::writePointsToClient() totalCount = %i", 
	     totalCount);
  
  // Send 0 points to indicate that all have been sent
  if (false) {
  sendPacket.empty();
  sendPacket.byte4ToBuf(0);
  client->sendPacketTcp(&sendPacket);
  }

} // end writePointsToClient


/** @internal */
AREXPORT void ArServerHandlerMap::serverGetMap(ArServerClient *client, 
					ArNetPacket *packet)
{
  ArLog::log(ArLog::Verbose, "Starting sending map to client");
  if (myMap == NULL)
  {
    writeMapToClient("", client);
    return;
  }

  myMap->lock();
  // This functor is used to send the entire map in text format.
  ArFunctor1<const char *> *functor = 
		new ArFunctor2C<ArServerHandlerMap, const char *, ArServerClient *>
					(this, 
					 &ArServerHandlerMap::writeMapToClient,
					 NULL,
					 client);
  // Send the map 
  myMap->writeToFunctor(functor, "");

  delete functor;
  // Send an empty packet to indicate the end of the map.
  writeMapToClient("", client);

  // send an empty packet to say we're done
  ArNetPacket emptyPacket;
  client->sendPacketTcp(&emptyPacket);

  myMap->unlock();
  ArLog::log(ArLog::Verbose, "Finished sending map to client");

}

AREXPORT void ArServerHandlerMap::serverGetMapBinary(ArServerClient *client, 
													                           ArNetPacket *packet)
{
  ArLog::log(ArLog::Verbose, "Starting sending map (binary) to client");
  if (myMap == NULL)
  {
    writeMapToClient("", client);
    return;
  }

  myMap->lock();
  // This functor is used to send the map header and objects in text format.
  ArFunctor1<const char *> *textFunctor = 
		new ArFunctor2C<ArServerHandlerMap, const char *, ArServerClient *>
					(this, 
					 &ArServerHandlerMap::writeMapToClient,
					 NULL,
					 client);

  // This functor is used to send the map data lines in binary format (since
  // it is faster).
  ArFunctor2<int, std::vector<ArLineSegment> *> *linesFunctor =
	  new ArFunctor3C<ArServerHandlerMap, int, std::vector<ArLineSegment> *, ArServerClient *>
				   (this,
				    &ArServerHandlerMap::writeLinesToClient,
				    0,
				    NULL,
				    client);

  // This functor is used to send the map data points in binary format (since
  // it is faster).
  ArFunctor2<int, std::vector<ArPose> *> *pointsFunctor =
	  new ArFunctor3C<ArServerHandlerMap, int, std::vector<ArPose> *, ArServerClient *>
				   (this,
				    &ArServerHandlerMap::writePointsToClient,
				    0,
				    NULL,
				    client);

  // Send the map up to but not including the DATA tag.
  myMap->writeObjectsToFunctor(textFunctor, 
                               "", 
                               true, // Only send one scan type
                               ArMapInterface::MAP_CATEGORY_2D);

  std::list<std::string> scanTypeList = myMap->getScanTypes();

  bool isAnyLinesExist = false;
  
  for (std::list<std::string>::iterator iter = scanTypeList.begin();
        iter != scanTypeList.end();
        iter++) {
    const char *scanType = (*iter).c_str();
    if ((myMap->getLines(scanType) != NULL) &&
        (!myMap->getLines(scanType)->empty())) {
      isAnyLinesExist = true;
      break;
    } 
  } // end for each scan type

  // see if we want to send the lines and make sure we have lines
  if ((myDataToSend == LINES || myDataToSend == BOTH) && 
      isAnyLinesExist) 
  {
    writeMapToClient("LINES", client);

    /***
    myMap->writeLinesToFunctor(linesFunctor, 
                               NULL, 
                               ARMAP_SUMMARY_SCAN_TYPE);
   
    ***/
    for (std::list<std::string>::iterator iter = scanTypeList.begin();
         iter != scanTypeList.end();
         iter++) {
      const char *scanType = (*iter).c_str();
      ArLog::log(ArLog::Normal,
                 "ArServerHandlerMap::serverGetMapBinary() sending lines for %s",
                 scanType);

      myMap->writeLinesToFunctor(linesFunctor, scanType, NULL);
    
    } // end for each scan type

    ArNetPacket sendPacket;
    sendPacket.empty();
    sendPacket.byte4ToBuf(0);
    client->sendPacketTcp(&sendPacket);

    /****/
  }

  // Always write the points (even if empty) because this signifies
  // the end of the map for the client.
  //if (myMap->getPoints()->begin() != myMap->getPoints()->end())
  //{
  writeMapToClient("DATA", client);
  // if we want to send points then send 'em
  if (myDataToSend == POINTS || myDataToSend == BOTH) 
  {
    // myMap->writePointsToFunctor(pointsFunctor, NULL, sendScanType);
    /**/
    for (std::list<std::string>::iterator iter = scanTypeList.begin();
         iter != scanTypeList.end();
         iter++) {
      const char *scanType = (*iter).c_str();
      ArLog::log(ArLog::Normal,
                 "ArServerHandlerMap::serverGetMapBinary() sending points for %s",
                 scanType);
      myMap->writePointsToFunctor(pointsFunctor, scanType, NULL);
    } // end for each scan type
    /**/
    ArNetPacket sendPacket;
    sendPacket.empty();
    sendPacket.byte4ToBuf(0);
    client->sendPacketTcp(&sendPacket);
  }
  // if not just say we're done
  else
    writeMapToClient("", client);
  //}

  // send an empty packet to say we're done
  ArNetPacket emptyPacket;
  client->sendPacketTcp(&emptyPacket);

  myMap->unlock();
  ArLog::log(ArLog::Verbose, "Finished sending map (binary) to client");
  
  // delete textFunctor;
  
  delete textFunctor;
  textFunctor = NULL;
  delete linesFunctor;
  linesFunctor = NULL;
  delete pointsFunctor;
  pointsFunctor = NULL;

}

AREXPORT void ArServerHandlerMap::serverGetMapMultiScans(ArServerClient *client, 
													                                 ArNetPacket *packet)
{

  sendMapWithMaxCategory(client, ArMapInterface::MAP_CATEGORY_2D_MULTI_SOURCES);

} // end method serverGetMapMultiScans


/// Requests that the server send the map with the specified maximum features.
AREXPORT void ArServerHandlerMap::serverGetMapWithMaxCategory(ArServerClient *client,
				                                                       ArNetPacket *packet)
{

  std::string category = ArMapInterface::MAP_CATEGORY_2D;
  if (packet != NULL) {

    char tempBuf[512];
    packet->bufToStr(tempBuf, sizeof(tempBuf));
    category = tempBuf;

    ArLog::log(ArLog::Normal,
               "ArServerHandlerMap::serverGetMapWithMaxCategory() %s requested",
               tempBuf);
  }

  if ((category.empty()) ||
      (ArUtil::strcasecmp(category, ArMapInterface::MAP_CATEGORY_2D) == 0) ) {
    
        serverGetMapBinary(client, packet);
    
  } 
  else if ((ArUtil::strcasecmp(category, ArMapInterface::MAP_CATEGORY_2D_MULTI_SOURCES) == 0) ||
           (ArUtil::strcasecmp(category, ArMapInterface::MAP_CATEGORY_2D_EXTENDED) == 0)) {

    sendMapWithMaxCategory(client, category.c_str());
  } 
  else { // unrecognized request

    ArLog::log(ArLog::Normal,
               "ArServerHandlerMap does not recognize request for %s, sending max known map (%s)",
               category.c_str(),
               ArMapInterface::MAP_CATEGORY_2D_EXTENDED);

    sendMapWithMaxCategory(client, ArMapInterface::MAP_CATEGORY_2D_EXTENDED);
  }

} // end method serverGetMapWithMaxCategory


AREXPORT void ArServerHandlerMap::sendMapWithMaxCategory(ArServerClient *client,
				                                                 const char *maxCategory)
{
  ArLog::LogLevel level = ArLog::Verbose;

  ArLog::log(level, 
             "Starting sending map (%s) to client", maxCategory);

  if (myMap == NULL)
  {
    ArLog::log(level, 
               "ArServerHandlerMap::sendMapWithMaxCategory() NULL map");

    writeMapToClient("", client);
    return;
  }

  myMap->lock();
  // This functor is used to send the map header and objects in text format.
  ArFunctor1<const char *> *textFunctor = 
		new ArFunctor2C<ArServerHandlerMap, const char *, ArServerClient *>
					(this, 
					 &ArServerHandlerMap::writeMapToClient,
					 NULL,
					 client);

  // This functor is used to send the map data lines in binary format (since
  // it is faster).
  ArFunctor2<int, std::vector<ArLineSegment> *> *linesFunctor =
	  new ArFunctor3C<ArServerHandlerMap, int, std::vector<ArLineSegment> *, ArServerClient *>
				   (this,
				    &ArServerHandlerMap::writeLinesToClient,
				    0,
				    NULL,
				    client);

  // This functor is used to send the map data points in binary format (since
  // it is faster).
  ArFunctor2<int, std::vector<ArPose> *> *pointsFunctor =
	  new ArFunctor3C<ArServerHandlerMap, int, std::vector<ArPose> *, ArServerClient *>
				   (this,
				    &ArServerHandlerMap::writePointsToClient,
				    0,
				    NULL,
				    client);

  // Send the map up to but not including the DATA tag.
  myMap->writeObjectsToFunctor(textFunctor, 
                               "", 
                               false,
                               maxCategory);

  std::list<std::string> scanTypeList = myMap->getScanTypes();

  if (!scanTypeList.empty()) {

    // see if we want to send the lines and make sure we have lines
    if (myDataToSend == LINES || myDataToSend == BOTH) {
    
      for (std::list<std::string>::iterator iter = scanTypeList.begin();
           iter != scanTypeList.end();
           iter++) {
        const char *scanType = (*iter).c_str();
        if ((myMap->getLines(scanType) != NULL) &&
            (!myMap->getLines(scanType)->empty())) {

          myMap->writeLinesToFunctor(linesFunctor,
                                     scanType,
                                     textFunctor);
          ArNetPacket sendPacket;
          sendPacket.empty();
          sendPacket.byte4ToBuf(0);
          client->sendPacketTcp(&sendPacket);
        }
      }
    }
    if (myDataToSend == POINTS || myDataToSend == BOTH) {
 
      // TODO This is different than getMapBinary -- in that DATA is not necessarily
      // always sent.  Is this going to be a problem?

      for (std::list<std::string>::iterator iter = scanTypeList.begin();
           iter != scanTypeList.end();
           iter++) {
        const char *scanType = (*iter).c_str();
        if ((myMap->getPoints(scanType) != NULL) &&
            (!myMap->getPoints(scanType)->empty())) {

          myMap->writePointsToFunctor(pointsFunctor,
                                      scanType,
                                      textFunctor);
          ArNetPacket sendPacket;
          sendPacket.empty();
          sendPacket.byte4ToBuf(0);
          client->sendPacketTcp(&sendPacket);
        }
      }
    }
  } // end if scan types
  
  // send an empty packet to say we're done
  ArNetPacket emptyPacket;
  client->sendPacketTcp(&emptyPacket);

  myMap->unlock();
  ArLog::log(level, "Finished sending map (%s) to client", maxCategory);
  
  // delete textFunctor;
  
  delete textFunctor;
  textFunctor = NULL;
  delete linesFunctor;
  linesFunctor = NULL;
  delete pointsFunctor;
  pointsFunctor = NULL;

} // end method sendMapWithMaxCategory



/** @internal */
AREXPORT void ArServerHandlerMap::serverGetGoals(ArServerClient *client, 
						 ArNetPacket *packet)
{
  std::list<ArMapObject *>::iterator objIt;
  ArMapObject* obj;
  ArPose goal;  
  ArNetPacket sendPacket;

  for (objIt = myMap->getMapObjects()->begin(); 
       objIt != myMap->getMapObjects()->end(); 
       objIt++)
  {
    //
    // Get the forbidden lines and fill the occupancy grid there.
    //
    obj = (*objIt);
    if (obj == NULL)
      break;
    if (strcasecmp(obj->getType(), "GoalWithHeading") == 0 ||
	strcasecmp(obj->getType(), "Goal") == 0)
    {
      sendPacket.strToBuf(obj->getName());
    }
  }
  client->sendPacketTcp(&sendPacket);
}

AREXPORT void ArServerHandlerMap::mapChanged(void)
{
  ArNetPacket emptyPacket;
  ArLog::log(ArLog::Normal,
             "ArServerHandlerMap::mapChanged() orig = %s new = %s",
             myMapFileName,
             myMap->getFileName());

  strncpy(myMapFileName, myMap->getFileName(), 512);
  myMapFileName[511] = 0;
  myServer->broadcastPacketTcp(&emptyPacket, "mapUpdated");
  myServer->broadcastPacketTcp(&emptyPacket, "goalsUpdated");
}


AREXPORT void ArServerHandlerMap::handleCheckMap(ArServerClient *client, 
                                                 ArNetPacket *packet)
{

  if (!myOwnMap) {
    ArLog::log(ArLog::Normal, "ArServerHandlerMap::handleCheckMap map not owned");
    myMap->refresh();

  }
  else {
    ArLog::log(ArLog::Normal, "ArServerHandlerMap::handleCheckMap map is owned, checking config");
    processFile();
  }

} // end method handleCheckMap


/** @internal */
AREXPORT bool ArServerHandlerMap::processFile(void)
{
  struct stat mapFileStat;

  if (myMapFileName[0] == '\0')
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerMap::processFile: No map file");
    return true;
  }

  if (stat(myMapFileName, &mapFileStat) != 0)
  {
    ArLog::log(ArLog::Terse, "Cannot stat file");
    return false;
  }
 // see if we need to load the map file
  if (!myAlreadyLoaded || 
      strcmp(myMapFileName, myLastMapFile) != 0 ||
      mapFileStat.st_mtime != myLastMapFileStat.st_mtime || 
      mapFileStat.st_ctime != myLastMapFileStat.st_ctime)
  {
    myAlreadyLoaded = true; 
    strcpy(myLastMapFile, myMapFileName);
    memcpy(&myLastMapFileStat, &mapFileStat, sizeof(mapFileStat));

    ArLog::log(ArLog::Terse, "Loading map %s", myMapFileName);
    if (!loadMap(myMapFileName))
    {
      ArLog::log(ArLog::Terse, "Failed loading map %s", myMapFileName);
      return false;
    }
    ArLog::log(ArLog::Terse, "Loaded map %s", myMapFileName);
    return true;
  }
  myAlreadyLoaded = true;
  ArLog::log(ArLog::Normal, "ArServerHandlerMapConfig: File did not need reloading");
  return true;
}

