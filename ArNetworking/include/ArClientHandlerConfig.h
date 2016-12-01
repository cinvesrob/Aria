#ifndef ARCLIENTCONFIGHANDLER_H
#define ARCLIENTCONFIGHANDLER_H

#include "Aria.h"
#include "ArClientBase.h"

/// Client handler for receiving and updating ArConfig data via ArNetworking.
/**
 * ArClientHandlerConfig processes the network packets that describe a
 * robot's ArConfig.  It also provides a means to save the modified 
 * configuration data to the robot server.  This class is designed to 
 * work in conjunction with the ArServerHandlerConfig.  See the server
 * handler documentation for a complete description of the networking
 * interface.
 *
 * This class should be thread safe, with the exception of
 * unThreadSafeGetConfig. (If you want to use this method, surround it 
 * with calls to lock() and unlock().) 
 *
 * Note that you can't add callbacks or remove callbacks from within a
 * callback function.
**/
class ArClientHandlerConfig
{
public:
  /// Constructor
  AREXPORT ArClientHandlerConfig(ArClientBase *client,
				                         bool ignoreBounds = false,
                                 const char *robotName = NULL,
                                 const char *logPrefix = NULL);
  /// Destructor
  AREXPORT virtual ~ArClientHandlerConfig(void);

  /// Requests the config from the server
  AREXPORT void requestConfigFromServer(void);
  /// Tells the server to reload the configuration 
  AREXPORT void reloadConfigOnServer(void);

  /// Threadsafe way to get the config to play with
  AREXPORT ArConfig getConfigCopy(void);

  /// Adds a gotConfig callback
  AREXPORT void addGotConfigCB(ArFunctor *functor, 
			       ArListPos::Pos position = ArListPos::LAST);
  /// Removes a gotConfig callback
  AREXPORT void remGotConfigCB(ArFunctor *functor);
  /// Adds a save config to server succeeded callback
  AREXPORT void addSaveConfigSucceededCB(ArFunctor *functor, 
			   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a save config to server succeeded callback
  AREXPORT void remSaveConfigSucceededCB(ArFunctor *functor);
  /// Adds a save config to server failed callback
  AREXPORT void addSaveConfigFailedCB(ArFunctor1<const char *> *functor, 
			   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a save config to server failed callback
  AREXPORT void remSaveConfigFailedCB(ArFunctor1<const char *> *functor);

  /// Returns true if config gotten
  AREXPORT bool haveGottenConfig(void);
  /// Sends the config back to the server 
  AREXPORT void saveConfigToServer(void);
  /// Sends the config back to the server 
  AREXPORT void saveConfigToServer(
	  ArConfig *config, 
	  const std::set<std::string, 
	  ArStrCaseCmpOp> *ignoreTheseSections = NULL);

  /// Returns if we've requested some defaults
  AREXPORT bool haveRequestedDefaults(void);
  /// Returns if we've gotten our requested defaults
  AREXPORT bool haveGottenDefaults(void);

  /// Sees if we can request defaults (both types)
  AREXPORT bool canRequestDefaults(void);

  AREXPORT bool requestDefaultConfigFromServer(void);
  AREXPORT ArConfig *getDefaultConfig();

  /// Requests defaults for all sections from the server; modifies the config
  AREXPORT bool requestConfigDefaults(void);
  /// Requests defaults for one section from the server; modifies the config
  AREXPORT bool requestSectionDefaults(const char *section);
  

  /// Adds a got config defaults callback
  AREXPORT void addGotConfigDefaultsCB(ArFunctor *functor, 
			   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a got config defaults callback
  AREXPORT void remGotConfigDefaultsCB(ArFunctor *functor);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Last Editable Priority
  //
  //    This value designates the last priority (highest numerical value) for 
  //    which parameters may be edited.  For example, when this value is set to 
  //    EXPERT, then the user may not edit FACTORY parameters.
  //
  //    If editable priority levels are supported and the client wishes to
  //    receive parameters of ineditable priorities, then the method 
  //    requestLastEditablePriority should be called (and completed) before 
  //    requestConfigFromServer is called.
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /// Returns whether the server supports the last editable priority request.
  AREXPORT bool isLastEditablePriorityAvailable();
  
  /// Requests the last editable priority from the server
  AREXPORT bool requestLastEditablePriorityFromServer();

  /// Returns whether the last editable priority has been received from the server
  AREXPORT bool haveGottenLastEditablePriority();
  /// Returns the last editable priority of the config
  AREXPORT ArPriority::Priority getLastEditablePriority();

  /// Adds callback invoked when the last editable priority packet is received
  AREXPORT void addGotLastEditablePriorityCB
                  (ArFunctor *functor, 
			             int position = 50);
  /// Removes the specified callback from the list of last editable priority callbacks
  AREXPORT void remGotLastEditablePriorityCB(ArFunctor *functor);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /// Unthreadsafe way to get the config to play with (see long docs)
  AREXPORT ArConfig *getConfig(void);
  /// Locks the config for if you're using the unthreadsafe getConfig
  AREXPORT int lock(void);
  /// Try to lock for the config for if you're using the unthreadsafe getConfig
  AREXPORT int tryLock(void);
  /// Unlocks the config for if you're using the unthreadsafe getConfig
  AREXPORT int unlock(void);

  /// Turn on this flag to reduce the number of verbose log messages.
  AREXPORT void setQuiet(bool isQuiet);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Packet Handlers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 
  /// Handles the packet from the GetConfigBySectionsV4
  AREXPORT void handleGetConfigBySectionsV4(ArNetPacket *packet);
  
   /// Handles the packet from the GetConfigBySectionsV3
  AREXPORT void handleGetConfigBySectionsV3(ArNetPacket *packet);

  /// Handles the packet from the GetConfigBySectionsV2
  AREXPORT void handleGetConfigBySectionsV2(ArNetPacket *packet);

  /// Handles the packet from the GetConfigBySections
  AREXPORT void handleGetConfigBySections(ArNetPacket *packet);

  /// Handles the packet from the GetConfigSectionFlags
  AREXPORT void handleGetConfigSectionFlags(ArNetPacket *packet);

  /// Handles the packet from the getConfig
  AREXPORT void handleGetConfig(ArNetPacket *packet);

  /// Handles the return packet from the setConfig (saveConfigToServer)
  AREXPORT void handleSetConfig(ArNetPacket *packet);

  /// Handles the return packet from the setConfigBySections (saveConfigToServer)
  AREXPORT void handleSetConfigBySections(ArNetPacket *packet);

  /// Handles the return packet from the setConfigBySectionsV2 (saveConfigToServer)
  AREXPORT void handleSetConfigBySectionsV2(ArNetPacket *packet);

  /// Handles the return packet from getConfigDefaults
  AREXPORT void handleGetConfigDefaults(ArNetPacket *packet);

  /// Handles the return packet from getLastEditablePriority
  AREXPORT void handleGetLastEditablePriority(ArNetPacket *packet);

protected:

  AREXPORT void handleGetConfigData(ArNetPacket *packet,
                                    bool isMultiplePackets,
                                    int version);


protected:

  std::string myRobotName;
  std::string myLogPrefix;

  std::list<ArFunctor *> myGotConfigCBList;
  std::list<ArFunctor *> mySaveConfigSucceededCBList;
  std::list<ArFunctor1<const char *> *> mySaveConfigFailedCBList;
  std::list<ArFunctor *> myGotConfigDefaultsCBList;
  ArCallbackList myGotLastEditablePriorityCBList;

  ArClientBase *myClient;
  
  ArConfig myConfig;
  ArConfig *myDefaultConfig;
  ArPriority::Priority myLastEditablePriority;

  ArMutex myDataMutex;
  ArMutex myCallbackMutex;

  bool myHaveRequestedLastEditablePriority;
  bool myHaveGottenLastEditablePriority;
  bool myHaveRequestedConfig;
  bool myHaveGottenConfig;
  bool myHaveRequestedDefaults;
  bool myHaveGottenDefaults;
  bool myHaveRequestedDefaultCopy;

  bool myIsQuiet;

  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleGetConfigBySectionsV4CB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleGetConfigBySectionsV3CB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleGetConfigBySectionsV2CB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleGetConfigBySectionsCB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleGetConfigCB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleSetConfigCB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleSetConfigBySectionsCB;
  ArFunctor1C<ArClientHandlerConfig, ArNetPacket *> myHandleSetConfigBySectionsV2CB;
  ArFunctor1C<ArClientHandlerConfig, 
              ArNetPacket *> myHandleGetConfigDefaultsCB;
  ArFunctor1C<ArClientHandlerConfig, 
              ArNetPacket *> myHandleGetDefaultConfigCB;
  ArFunctor1C<ArClientHandlerConfig, 
	            ArNetPacket *> myHandleGetConfigSectionFlagsCB;
  ArFunctor1C<ArClientHandlerConfig, 
              ArNetPacket *> myHandleGetLastEditablePriorityCB;
};

#endif
