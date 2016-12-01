#ifndef ARSERVERHANDLERMAPPING_H
#define ARSERVERHANDLERMAPPING_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArServerHandlerCommands.h"
#include "ArTempDirectoryHelper.h"

/// Class that handles the starting, stopping, and status of mapping
class ArServerHandlerMapping
{
public:
  /// Constructor
  AREXPORT ArServerHandlerMapping(ArServerBase *server, ArRobot *robot,
				  ArLaser *laser, 
				  const char *baseDirectory = "",
				  const char *tempDirectory = "",
				  bool useReflectorValues = false,
				  ArLaser *laser2 = NULL, 
				  const char *sickSuffix = NULL,
				  const char *sick2Suffix = NULL,
				  std::list<ArLaser *> *extraLasers = NULL);
  /// Deconstructor
  AREXPORT virtual ~ArServerHandlerMapping();
  /// Starts a new map unless ones already made
  AREXPORT void serverMappingStart(ArServerClient *client, 
				   ArNetPacket *packet);
  /// Ends a map
  AREXPORT void serverMappingEnd(ArServerClient *client, 
				 ArNetPacket *packet);
  /// Gets the mapping status
  AREXPORT void serverMappingStatus(ArServerClient *client, 
				    ArNetPacket *packet);
  /// Returns if we're mapping or not
  AREXPORT bool isMapping(void);

  /// Forces a reading to be taken
  AREXPORT void forceReading(void);

  /// Gets the filename we're using (make sure the robot's locked)
  AREXPORT const char *getFileName(void);
  /// Gets the mapname we're using (make sure the robot's locked)
  AREXPORT const char *getMapName(void);
  /// Adds the simple commands to the simple command handler passed in
  AREXPORT void addSimpleCommands(ArServerHandlerCommands *handlerCommands);
  /// Adds a loop start to the log
  AREXPORT void simpleLoopStart(ArArgumentBuilder *arg);
  /// Adds a loop end to the log
  AREXPORT void simpleLoopEnd(ArArgumentBuilder *arg);
  /// Adds a string for adding to the log when we start
  AREXPORT void addStringForStartOfLogs(
	  const char *str, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a string for adding to the log when we start
  AREXPORT void remStringForStartOfLogs(const char *str);
  /// Adds a tag to the logg (has robot pose and stuff) (make sure the
  /// robot's locked)
  AREXPORT void addTagToLog(const char *str);
  /// Adds an info to the log (has no robot pose) (make sure the
  /// robot's locked)
  AREXPORT void addInfoToLog(const char *str);
  /// Adds a new type of location data to the logger
  AREXPORT bool addLocationData(
	  const char *name, 
	  ArRetFunctor3<int, ArTime, ArPose *, ArPoseWithTime *> *functor);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Callback Methods
  //
  //   The mapping callbacks are invoked in the order in which they appear
  //   in this header file.  Namely:
  //      1. MappingStart
  //      2. MappingBegun (this is logically "mapping started")
  //      3. MappingEnd
  //      4. MappingEnded
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /// Adds a callback for the start of mapping
  AREXPORT void addMappingStartCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for the start of mapping
  AREXPORT void remMappingStartCallback(ArFunctor *functor);

  /// Adds a callback for just after mapping has started
  AREXPORT void addMappingBegunCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for just after mapping has started
  AREXPORT void remMappingBegunCallback(ArFunctor *functor);

  /// Adds a callback for the end of mapping
  AREXPORT void addMappingEndCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for the end of mapping
  AREXPORT void remMappingEndCallback(ArFunctor *functor);

  /// Adds a callback that is invoked after mapping has been ended. 
  AREXPORT void addMappingEndedCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback from the list invoked after mapping has been ended.
  AREXPORT void remMappingEndedCallback(ArFunctor *functor);
  

  /// Adds a callback to be called before moving from temp dir to base dir
  AREXPORT void addPreMoveCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called before moving from temp dir to base dir
  AREXPORT void remPreMoveCallback(ArFunctor *functor);
  
  /// Adds a callback to be called after moving from temp dir to base dir
  AREXPORT void addPostMoveCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called after moving from temp dir to base dir
  AREXPORT void remPostMoveCallback(ArFunctor *functor);


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Misc Methods 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /// The packet handler for starting/stopping scans from the lcd
  AREXPORT bool packetHandler(ArRobotPacket *packet);
  /// Get location data map (mostly for internal things)
  AREXPORT const std::map<std::string, ArRetFunctor3<int, ArTime, ArPose *, ArPoseWithTime *> *, 
		 ArStrCaseCmpOp> *getLocationDataMap(void);
  /// A method to add the strings for the start of a log just straight to an ArMap
  AREXPORT void addStringsForStartOfLogToMap(ArMap *arMap);


  /// Returns a pointer to the optional zip file installed on this handler.
  AREXPORT ArZippable *getZipFile();
  
  /// Returns a pointer to a second optional zip file used to contain snapshot images.
  AREXPORT ArZippable *getSnapshotZipFile();
 
  /// Sets the optional zip file installed on this handler.
  AREXPORT void setZipFile(ArZippable *zipFile);
  
  /// Sets the optional zip file installed on this handler.
  AREXPORT void setSnapshotZipFile(ArZippable *zipFile);

protected:


  AREXPORT std::string makeFilePathName(const char *fileName);

  ArServerBase *myServer;
  ArRobot *myRobot;
  ArLaser *myLaser;
  ArLaser *myLaser2;
  bool myUseReflectorValues;
  
  std::list<ArFunctor *> myMappingStartCallbacks;
  std::list<ArFunctor *> myMappingBegunCallbacks;
  std::list<ArFunctor *> myMappingEndCallbacks;
  std::list<ArFunctor *> myMappingEndedCallbacks;
  
  std::list<ArFunctor *> myPreMoveCallbacks;
  std::list<ArFunctor *> myPostMoveCallbacks;

  std::list<std::string> myStringsForStartOfLog;
  ArLaserLogger *myLaserLogger;
  ArLaserLogger *myLaserLogger2;
  std::string myMapName;
  std::string myFileName;
  std::string myFileName2;
  
  std::string mySuffix;
  std::string mySuffix2;

  std::list<ArLaser *> *myExtraLasers;
  
  ArTempDirectoryHelper myTempDirectoryHelper;

  /// Optional zip file in which mapping results may be saved
  ArZippable *myZipFile;
  /// Second optional zip file in which snaphots may be saved 
  ArZippable *mySnapshotZipFile;

  std::map<std::string, ArRetFunctor3<int, ArTime, ArPose *, 
				      ArPoseWithTime *> *, 
	   ArStrCaseCmpOp> myLocationDataMap;

  ArFunctor2C<ArServerHandlerMapping, ArServerClient *, ArNetPacket *> myMappingStartCB;
  ArFunctor2C<ArServerHandlerMapping, ArServerClient *, ArNetPacket *> myMappingEndCB;
  ArFunctor2C<ArServerHandlerMapping, ArServerClient *, ArNetPacket *> myMappingStatusCB;
  ArServerHandlerCommands *myHandlerCommands;
  ArRetFunctor1C<bool, ArServerHandlerMapping, ArRobotPacket *> myPacketHandlerCB;
  ArFunctor1C<ArServerHandlerMapping, ArArgumentBuilder *> myLoopStartCB;
  ArFunctor1C<ArServerHandlerMapping, ArArgumentBuilder *> myLoopEndCB;
};



#endif // ARSERVERHANDLERMAPPING_H
