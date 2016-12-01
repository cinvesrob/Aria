#include "Aria.h"
#include "ArExport.h"
#include "ArServerHandlerMapping.h"

/**
   @param server the server to add the handlers too

   @param robot the robot to map from

   @param laser the laser to map with

   @param baseDirectory the directory to put the map file into when
   its done, NULL or an empty string means into the working directory

   @param tempDirectory the directory to put the map file into while
   its being created, if this is NULL or empty it'll use the base directory
 **/
AREXPORT ArServerHandlerMapping::ArServerHandlerMapping(
	ArServerBase *server, ArRobot *robot, ArLaser *laser,
	const char *baseDirectory, const char *tempDirectory,
	bool useReflectorValues, ArLaser *laser2, 
	const char *suffix, const char *suffix2,
	std::list<ArLaser *> *extraLasers) :
  myTempDirectoryHelper(baseDirectory, tempDirectory),
  myZipFile(NULL),
  mySnapshotZipFile(NULL),
  myMappingStartCB(this, &ArServerHandlerMapping::serverMappingStart),
  myMappingEndCB(this, &ArServerHandlerMapping::serverMappingEnd),
  myMappingStatusCB(this, &ArServerHandlerMapping::serverMappingStatus),
  myPacketHandlerCB(this, &ArServerHandlerMapping::packetHandler),
  myLoopStartCB(this, &ArServerHandlerMapping::simpleLoopStart),
  myLoopEndCB(this, &ArServerHandlerMapping::simpleLoopEnd)
{
  myServer = server;
  myRobot = robot;
  myLaser = laser;
  myUseReflectorValues = useReflectorValues;

  myLaser2 = laser2;
  if (suffix != NULL)
    mySuffix = suffix;
  if (suffix2 != NULL)
    mySuffix2 = suffix2;

  myExtraLasers = extraLasers;

  if (myServer != NULL)
  {
    myServer->addData("mappingStart", "Starts making a map",
		      &myMappingStartCB, "string: name of map file",
		      "byte: 0 (started mapping), 1 (already mapping), 2 (could not start map)",
		      "Mapping", "RETURN_SINGLE");
    myServer->addData("mappingEnd", "Stops making a map",
		      &myMappingEndCB, "none",
		      "byte: 0 (ended mapping), 1 (no mapping to end), 2 (could not move temporary file to permanent file)", 
		      "Mapping", "RETURN_SINGLE");
    myServer->addData("mappingStatus", "Gets the status of mapping",
		      &myMappingStatusCB, "none",
	      "string: mapName if mapping, empty string if not mapping", 
		      "RobotInfo", "RETURN_SINGLE");
    myServer->addData("mappingStatusBroadcast", "Gets the status of mapping, also sent when mapping starts or ends (this is a new/better mappingStatus)",
		      &myMappingStatusCB, "none",
	      "string: mapName if mapping, empty string if not mapping", 
		      "RobotInfo", "RETURN_SINGLE");
  }
  myLaserLogger = NULL;
  myLaserLogger2 = NULL;
  myMapName = "";
  myFileName = "";
  myHandlerCommands = NULL;
  
  myPacketHandlerCB.setName("ArServerHandlerMapping");
  if (myRobot != NULL)
    myRobot->addPacketHandler(&myPacketHandlerCB);
}

AREXPORT ArServerHandlerMapping::~ArServerHandlerMapping()
{
  if (myLaserLogger != NULL)
  {
    delete myLaserLogger;
    myLaserLogger = NULL;
  }

  if (myLaserLogger2 != NULL)
  {
    delete myLaserLogger;
    myLaserLogger = NULL;
  }
}

AREXPORT void ArServerHandlerMapping::serverMappingStart(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  char buf[512];
  packet->bufToStr(buf, sizeof(buf));

  // see if we're already mapping
  if (myLaserLogger != NULL)
  {
    ArLog::log(ArLog::Normal, "MappingStart: Map already being made");
    sendPacket.byteToBuf(1);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    return;
  }


  myRobot->lock();

  // lower case everything (to avoid case conflicts)
  ArUtil::lower(buf, buf, sizeof(buf));
  myMapName = buf;

  myFileName = myMapName;
  if (!mySuffix.empty())
    myFileName += mySuffix;
  if (strstr(myMapName.c_str(), ".2d") == NULL)
    myFileName += ".2d";

  if (myLaser2 != NULL)
  {
    myFileName2 = myMapName;
    if (!mySuffix2.empty())
      myFileName2 += mySuffix2;
    if (strstr(myMapName.c_str(), ".2d") == NULL)
      myFileName2 += ".2d";
  }


  // call our mapping started callbacks
  std::list<ArFunctor *>::iterator fit;
  ArFunctor *functor;
  for (fit = myMappingStartCallbacks.begin(); 
       fit != myMappingStartCallbacks.end(); 
       fit++)
  {
    functor = (*fit);
    functor->invoke();
  }


  myLaserLogger = new ArLaserLogger(myRobot, myLaser, 300, 25, myFileName.c_str(),
				    true, Aria::getJoyHandler(), 
				    myTempDirectoryHelper.getTempDirectory(),
				    myUseReflectorValues,
				    Aria::getRobotJoyHandler(),
				    &myLocationDataMap,
				    myExtraLasers);
  if (myLaserLogger == NULL)
  {
    ArLog::log(ArLog::Normal, "MappingStart: myLaserLogger == NULL");
    sendPacket.byteToBuf(2);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    myMapName = "";
    myFileName = "";
    myRobot->unlock();
    return;
  }
  if (!myLaserLogger->wasFileOpenedSuccessfully())
  {
    ArLog::log(ArLog::Normal, "MappingStart: Cannot open map file %s", 
	       myFileName.c_str());
    sendPacket.byteToBuf(2);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    myMapName = "";
    myFileName = "";
    delete myLaserLogger;
    myLaserLogger = NULL;
    myRobot->unlock();
    return;
  }

  if (myLaser2 != NULL)
  {
    myLaserLogger2 = new ArLaserLogger(myRobot, myLaser2, 300, 25, 
				     myFileName2.c_str(),
				     true, Aria::getJoyHandler(), 
				     myTempDirectoryHelper.getTempDirectory(),
				     myUseReflectorValues,
				     Aria::getRobotJoyHandler(),
				     &myLocationDataMap);
  }

  // toss our strings for the start on there
  std::list<std::string>::iterator it;
  
  for (it = myStringsForStartOfLog.begin(); 
       it != myStringsForStartOfLog.end(); 
       it++)
  {
    myLaserLogger->addInfoToLogPlain((*it).c_str());
    if (myLaserLogger2 != NULL)
      myLaserLogger2->addInfoToLogPlain((*it).c_str());
  }

  // call our mapping started callbacks
  for (fit = myMappingBegunCallbacks.begin(); 
       fit != myMappingBegunCallbacks.end(); 
       fit++)
  {
    functor = (*fit);
    functor->invoke();
  }


  myRobot->unlock();
  
  
  ArLog::log(ArLog::Normal, "MappingStart: Map %s started", 
	     myMapName.c_str());
  sendPacket.byteToBuf(0);
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);
  
  ArNetPacket broadcastPacket;
  broadcastPacket.strToBuf(myFileName.c_str());
  myServer->broadcastPacketTcp(&broadcastPacket, "mappingStatusBroadcast");
}


AREXPORT void ArServerHandlerMapping::serverMappingEnd(
	ArServerClient *client, ArNetPacket *packet)
{
  std::list<ArFunctor *>::iterator fit;

  ArNetPacket sendPacket;
  if (myLaserLogger == NULL)
  {
    ArLog::log(ArLog::Normal, "MappingEnd: No map being made");
    sendPacket.byteToBuf(1);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    return;
  }

  myRobot->lock();

  delete myLaserLogger;
  myLaserLogger = NULL;

  bool haveFile2 = false;

  if (myLaserLogger2 != NULL)
  {
    haveFile2 = true;
    delete myLaserLogger2;
    myLaserLogger2 = NULL;
  }
    
    
  std::list<std::string> sourceFileNameList;
  sourceFileNameList.push_back(myFileName);
  if (haveFile2) {
    sourceFileNameList.push_back(myFileName2);
  }

  bool isSuccess = myTempDirectoryHelper.moveFilesToBaseDirectory
                                           (sourceFileNameList);
  
  if (isSuccess) {
    sendPacket.uByte2ToBuf(0);
  }
  else {
    sendPacket.uByte2ToBuf(2);
  } 


  ArLog::log(ArLog::Normal, "MappingEnd: Stopped mapping %s", 
	           myFileName.c_str());

  // call our mapping end callbacks
  for (fit = myMappingEndCallbacks.begin(); 
       fit != myMappingEndCallbacks.end(); 
       fit++)
    (*fit)->invoke();
  
  
  // Call the mapping ended callbacks
  for (fit = myMappingEndedCallbacks.begin(); 
       fit != myMappingEndedCallbacks.end(); 
       fit++) {
    (*fit)->invoke();
  }

  // Clear the internal file names
  myMapName = "";
  myFileName = "";

  myRobot->unlock();
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);

  ArNetPacket broadcastPacket;
  broadcastPacket.strToBuf(myFileName.c_str());
  myServer->broadcastPacketTcp(&broadcastPacket, "mappingStatusBroadcast");
}


AREXPORT void ArServerHandlerMapping::serverMappingStatus(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  sendPacket.strToBuf(myMapName.c_str());
  client->sendPacketTcp(&sendPacket);
}

AREXPORT void ArServerHandlerMapping::addStringForStartOfLogs(
	const char *str, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myStringsForStartOfLog.push_front(str);
  else if (position == ArListPos::LAST)
    myStringsForStartOfLog.push_back(str);
  else 
    ArLog::log(ArLog::Terse, "ArServerHandlerMapping::addStringForStartOfLogs: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remStringForStartOfLogs(
	const char *str)
{
  myStringsForStartOfLog.remove(str);
}

AREXPORT void ArServerHandlerMapping::addMappingStartCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myMappingStartCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myMappingStartCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addMappingStartCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remMappingStartCallback(
	ArFunctor *functor)
{
  myMappingStartCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerMapping::addMappingBegunCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myMappingBegunCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myMappingBegunCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addMappingBegunCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remMappingBegunCallback(
	ArFunctor *functor)
{
  myMappingBegunCallbacks.remove(functor);
}

AREXPORT void ArServerHandlerMapping::addMappingEndCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myMappingEndCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myMappingEndCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerHandlerMapping::addMappingEndCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remMappingEndCallback(
	ArFunctor *functor)
{
  myMappingEndCallbacks.remove(functor);
}


AREXPORT void ArServerHandlerMapping::addMappingEndedCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myMappingEndedCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myMappingEndedCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "ArServerHandlerMapping::addMappingEndedCallback: Invalid position.");
}

AREXPORT void ArServerHandlerMapping::remMappingEndedCallback(
	ArFunctor *functor)
{
  myMappingEndedCallbacks.remove(functor);
}




AREXPORT void ArServerHandlerMapping::addPreMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  myTempDirectoryHelper.addPreMoveCallback(functor, position);
}

AREXPORT void ArServerHandlerMapping::remPreMoveCallback(
	ArFunctor *functor)
{
  myTempDirectoryHelper.remPreMoveCallback(functor);
}

AREXPORT void ArServerHandlerMapping::addPostMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  myTempDirectoryHelper.addPostMoveCallback(functor, position);
}

AREXPORT void ArServerHandlerMapping::remPostMoveCallback(
	ArFunctor *functor)
{
  myTempDirectoryHelper.remPostMoveCallback(functor);
}



AREXPORT void ArServerHandlerMapping::addSimpleCommands(
	ArServerHandlerCommands *handlerCommands)
{
  myHandlerCommands = handlerCommands;
  myHandlerCommands->addStringCommand(
	  "mappingLoopStart", 
	  "If mapping is happening it starts a new loop with the tag of the given string", 
	  &myLoopStartCB);

  myHandlerCommands->addStringCommand(
	  "mappingLoopEnd", 
	  "If mapping is happening it ends a loop with the tag of the given string", 
	  &myLoopEndCB);
}

AREXPORT void ArServerHandlerMapping::simpleLoopStart(ArArgumentBuilder *arg)
{
  if (myLaserLogger != NULL) 
    myLaserLogger->addTagToLog("loop start %s", arg->getFullString());
  if (myLaserLogger2 != NULL) 
    myLaserLogger2->addTagToLog("loop start %s", arg->getFullString());
}

AREXPORT void ArServerHandlerMapping::simpleLoopEnd(ArArgumentBuilder *arg)
{
  if (myLaserLogger != NULL) 
    myLaserLogger->addTagToLog("loop stop %s", arg->getFullString());
  if (myLaserLogger2 != NULL) 
    myLaserLogger2->addTagToLog("loop stop %s", arg->getFullString());
}

/// The packet handler for starting/stopping scans from the lcd
AREXPORT bool ArServerHandlerMapping::packetHandler(ArRobotPacket *packet)
{
  // we return these as processed to help out the ArLaserLogger class
  if (packet->getID() == 0x96)
    return true;

  if (packet->getID() != 0x97)
    return false;

  myRobot->unlock();
  char argument = packet->bufToByte();
  if (argument == 1)
  {
    // see if we're already mapping
    if (myLaserLogger != NULL)
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerHandlerMapping::packetHandler: Start scan requested when already mapping");
    }
    else
    {
      // build a name
      time_t now;
      struct tm nowStruct;
      now = time(NULL);
      char buf[1024];

	  if (ArUtil::localtime(&now, &nowStruct))
      {
	sprintf(buf, "%02d%02d%02d_%02d%02d%02d", 
		(nowStruct.tm_year%100), nowStruct.tm_mon+1, nowStruct.tm_mday, 
		nowStruct.tm_hour, nowStruct.tm_min, nowStruct.tm_sec);
      }
      else
      {
	ArLog::log(ArLog::Normal, 
		   "ArServerHandlerMapping::packetHandler: Could not make good packet name (error getting time), so just naming it \"lcd\"");
	sprintf(buf, "lcd");
      }

      ArLog::log(ArLog::Normal, "Starting scan '%s' from LCD", buf);
      ArNetPacket fakePacket;
      fakePacket.strToBuf(buf);
      fakePacket.finalizePacket();
      serverMappingStart(NULL, &fakePacket);
    }
  }
  else if (argument == 0)
  {
    if (myLaserLogger == NULL)
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerHandlerMapping::packetHandler: Stop scan requested when not mapping");
    }
    else
    {
      ArLog::log(ArLog::Normal, "Stopping scan from LCD");
      serverMappingEnd(NULL, NULL);
    }
  }
  else
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerMapping::packetHandler: Unknown scan argument %d", argument);
  }
  myRobot->lock();
  return true;
}

AREXPORT void ArServerHandlerMapping::addTagToLog(const char *str)
{
  if (myLaserLogger != NULL)
    myLaserLogger->addTagToLogPlain(str);
  if (myLaserLogger2 != NULL)
    myLaserLogger2->addTagToLogPlain(str);
}

AREXPORT void ArServerHandlerMapping::addInfoToLog(const char *str)
{
  if (myLaserLogger != NULL)
    myLaserLogger->addInfoToLogPlain(str);
  if (myLaserLogger2 != NULL)
    myLaserLogger2->addInfoToLogPlain(str);
}

AREXPORT const char * ArServerHandlerMapping::getFileName(void)
{
  return myFileName.c_str();
}

AREXPORT const char * ArServerHandlerMapping::getMapName(void)
{
  return myMapName.c_str();
}

AREXPORT bool ArServerHandlerMapping::isMapping(void)
{
  if (myLaserLogger != NULL)
    return true;
  else
    return false;
}

AREXPORT bool ArServerHandlerMapping::addLocationData(
	const char *name, 
	ArRetFunctor3<int, ArTime, ArPose *, ArPoseWithTime *> *functor)
{
  if (myLocationDataMap.find(name) != myLocationDataMap.end())
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerMapping::addLocationData: Already have location data for %s", name);
    return false;
  }
  ArLog::log(ArLog::Normal, "ArServerHandlerMapping: Added location data %s", 
	     name);
  myLocationDataMap[name] = functor;
  return true;
}

  /// Get location data map (mostly for internal things)
AREXPORT const std::map<std::string, 
			ArRetFunctor3<int, ArTime, ArPose *, ArPoseWithTime *> *, 
			ArStrCaseCmpOp> *
ArServerHandlerMapping::getLocationDataMap(void)
{
  return &myLocationDataMap;
}


AREXPORT void ArServerHandlerMapping::addStringsForStartOfLogToMap(
	ArMap *arMap)
{
  std::list<std::string>::iterator it;
  std::string str;
  ArArgumentBuilder *builder;

  for (it = myStringsForStartOfLog.begin(); 
       it != myStringsForStartOfLog.end(); 
       it++)
  {
    str = (*it);
    builder = new ArArgumentBuilder;
    builder->add(str.c_str());
    if (strcasecmp(builder->getArg(0), "LogInfoStrip:") == 0)
    {
      builder->removeArg(0, true);
      printf("lis %s\n", builder->getFullString());

    }
    std::list<std::string> infoNames = arMap->getInfoNames();
    for (std::list<std::string>::iterator iter = infoNames.begin();
         iter != infoNames.end();
         iter++) 
    {
      const char *curName = (*iter).c_str();

      if (strcasecmp(builder->getArg(0), curName) == 0)
      {
	      builder->removeArg(0, true);
	      arMap->getInfo(curName)->push_back(builder);
	      builder = NULL;
	      break;
      }
    }
    if (builder != NULL)
      delete builder;
  }
}
  

/**
 *  When the optional zip file feature is used, this method should be called with
 *  a valid file in the context of a "mapping start" callback. The file should 
 *  have been opened in ZIP_MODE, and should remain open until after all of the 
 *  "mapping end" callbacks have completed.  The file should be closed and this
 *  method should be called with a NULL pointer within the context of a "mapping
 *  ended" callback. 
 *
 *  It is the application's responsibility to create and manage the zip file.
 *  Not all applications will use this feature.
 *
 *  @param zipFile a pointer to the optional ArZippable instance in which 
 *  scan results can be stored
**/
AREXPORT void ArServerHandlerMapping::setZipFile(ArZippable *zipFile)
{
  myZipFile = zipFile;
}

/**
 *  If snapshots are enabled when the optional zip file feature is used, this 
 *  method should be called with a valid file in the context of a "mapping start" 
 *  callback. The file should have been opened in ZIP_MODE, and should remain 
 *  open until after all of the "mapping end" callbacks have completed.  The file 
 *  should be closed and this method should be called with a NULL pointer within 
 *  the context of a "mapping ended" callback. The file should then be added to 
 *  the main mapping zip file.
 *
 *  It is the application's responsibility to create and manage the zip file.
 *  Not all applications will use this feature.
 *
 *  @param zipFile a pointer to the optional ArZippable instance in which 
 *  snapshots can be stored
**/
AREXPORT void ArServerHandlerMapping::setSnapshotZipFile(ArZippable *zipFile)
{
  mySnapshotZipFile = zipFile;
}

/**
 *  This method returns a pointer to the optional zip file associated with the
 *  mapping session. The file should be opened before the "mapping begun" 
 *  callbacks are invoked, and should be closed after the "mapping end" callbacks are 
 *  invoked.
 * 
 *  It is the application's responsibility to create and manage the zip file.
 *  Not all applications will use this feature.
 *
 *  @param zipFile a pointer to the optional ArZippable instance in which 
 *  scan results can be stored; NULL if there is no active zip file
**/
AREXPORT ArZippable *ArServerHandlerMapping::getZipFile()
{
  return myZipFile;

} // end method getZipFile

/**
 *  This method returns a pointer to a second optional zip file associated with the
 *  mapping session.  It contains the snapshot images, and is eventually added to the 
 *  main mapping zip file.  The file should be opened before the "mapping begun" 
 *  callbacks are invoked, and should be closed after the "mapping end" callbacks are 
 *  invoked.
 * 
 *  It is the application's responsibility to create and manage the zip file.
 *  Not all applications will use this feature.
 *
 *  @param zipFile a pointer to the optional ArZippable instance in which 
 *  scan results can be stored; NULL if there is no active zip file
**/
AREXPORT ArZippable *ArServerHandlerMapping::getSnapshotZipFile()
{
  return mySnapshotZipFile;

} // end method getZipFile


AREXPORT void ArServerHandlerMapping::forceReading(void)
{
  if (myLaserLogger != NULL)
    myLaserLogger->takeReading();
  if (myLaserLogger2 != NULL)
    myLaserLogger2->takeReading();
}
