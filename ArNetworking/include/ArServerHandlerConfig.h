#ifndef ARSERVERCONFIGHANDLER_H
#define ARSERVERCONFIGHANDLER_H

#include "Aria.h"
#include "ArServerBase.h"

class ArServerClient;

/// Class for sending and receiving ArConfig data via ArNetworking.
/**
 * ArServerHandlerConfig defines the network packet handlers used to transmit
 * ArConfig objects to a client, and to modify them based on information 
 * received from the client.  Since the packet structure for the ArConfig is 
 * rather complex, this class is best used in conjunction with the 
 * ArClientHandlerConfig.
 * 
 * This class handles the following requests:
 *  <ul>
 *    <li>getConfigBySections/getConfigBySectionsV2:  Replies with multiple 
 *        packets, one for each ArConfig section plus an empty packet that 
 *        terminates the reply.  Each packet contains the following header 
 *        information:
 *        <ol> 
 *          <li>Section Indicator: Always set to 'S' (byte)</li>
 *          <li>Section Name: (string)</li>
 *          <li>Section Comment: (string) </li>  
 *          <li>Section Category Name: (string) Only for getConfigBySectionsV2 </li>
 *        </ol>
 *        For each parameter in the section, the packet then contains a 
 *        a Parameter Indicator (always set to 'P' (byte)) followed by a 
 *        complete description of the parameter (display hints are included).  
 *        See ArClientArgUtils for more information.
 *    </li>
 *    <li>getConfig:  This request has been superceded by getConfigBySections. (It
 *        replies with a single packet containing all of the ArConfig 
 *        sections as described above.  If the ArConfig is large, then it 
 *        may not be sent successfully.  In addition, it contains no parameter
 *        display hints.)
 *    </li>
 *    <li>setConfig: Parses the received packet and updates the robot's 
 *        ArConfig (and saves it to the file).  For each modified section, 
 *        the received packet is expected to contain:
 *        <ol>
 *          <li>Section Indicator: Always set to "Section" (string)</li>
 *          <li>Section Name: (string) </li>
 *        </ol>
 *        Followed by a brief "text" description of each modified parameter.
 *        This is the parameter name (string) followed by the parameter value
 *        formatted as text (string).  See ArClientArgUtils for more information.
 *        
 *        A reply packet containing a string is sent to the client. If the 
 *        string is empty, then the config was successfully updated.  Otherwise,
 *        the string contains the name of the first parameter that caused an
 *        error during the update handling.
 *    </li>
 *    <li>reloadConfig: Reloads the ArConfig from the text file. 
 *    </li>
 *    <li>configUpdated: An empty packet is broadcast to all interested clients
 *        after setConfig or reloadConfig has been completed.  The clients may
 *        then request the modified ArConfig data with getConfigBySections
 *        (or getConfig).
 *    </li>
 *    <li>getConfigDefaults: If a "default" configuration file is available,
 *        then this command can be used to send the preferred default values
 *        to the client.  The received packet should contain a single string,
 *        which is the name of the section to be retrieved, or an empty string
 *        to indicate all sections.  
 *        
 *        For each requested section, the reply packet contains:
 *        <ol>
 *          <li>Section Indicator: Always set to "Section" (string)</li>
 *          <li>Section Name: (string) </li>
 *        </ol>
 *        Followed by a brief "text" description of each default parameter value.
 *        This is the parameter name (string) followed by the parameter value
 *        formatted as text (string).  See ArClientArgUtils for more information.
 *    </li>
 *  </ul>
 * 
 *  If you are using this class with the default file option you'll
 *  want to make it AFTER you're done adding things to the config, ie
 *  last, so that the default code can work correctly (it needs to know
 *  about all the info).
**/
class ArServerHandlerConfig
{
public:

  /// Constructor
  /**
   * @param server the ArServerBase * used to send and receive network packets;
   * must be non-NULL
   * @param config the ArConfig * that is maintained by this server handler
   * @param defaultFile the char * name of the file that contains the default
   * values for the ArConfig; if NULL, then getConfigDefaults will not be
   * supported
   * @param defaultFileBaseDirectory the char * name of the directory that
   * contains the default file
  **/
  AREXPORT ArServerHandlerConfig(ArServerBase *server, 
                                 ArConfig *config,
                                 const char *defaultFile = NULL, 
                                 const char *defaultFileBaseDirectory = NULL,
                                 bool allowFactory = true,
                                 const char *robotName = NULL,
                                 bool preventChanges = false,
                                 const char *preventChangesString = NULL);
  
  /// Destructor
  AREXPORT virtual ~ArServerHandlerConfig();

  // ---------------------------------------------------------------------------
  // Network Packet Handlers 
  // ---------------------------------------------------------------------------

  /// Handles the "reloadConfig" request.
  AREXPORT void reloadConfig(ArServerClient *client, ArNetPacket *packet);

  /// Handles the "getConfigBySections" request.
  AREXPORT void getConfigBySections(ArServerClient *client, ArNetPacket *packet);

  /// Handles the "getConfigBySectionsV2" request.
  AREXPORT void getConfigBySectionsV2(ArServerClient *client, ArNetPacket *packet);

  /// Handles the "getConfigBySectionsV3" request.
  AREXPORT void getConfigBySectionsV3(ArServerClient *client, ArNetPacket *packet);
  
  /// Handles the "getConfigBySectionsV4" request.
  AREXPORT void getConfigBySectionsV4(ArServerClient *client, ArNetPacket *packet);

  /// Handles the (deprecated) "getConfig" request.
  AREXPORT void getConfig(ArServerClient *client, ArNetPacket *packet);
  
  /// Handles the "setConfig" request.
  AREXPORT void setConfig(ArServerClient *client, ArNetPacket *packet);
  
  /// Handles the "setConfigParam" request.
  AREXPORT void setConfigParam(ArServerClient *client, ArNetPacket *packet);
  
  AREXPORT void setConfigBySections(ArServerClient *client, ArNetPacket *packet);
  
  AREXPORT void setConfigBySectionsV2(ArServerClient *client, ArNetPacket *packet);

  /// Handles the "getConfigDefaults" request.
  AREXPORT void getConfigDefaults(ArServerClient *client, ArNetPacket *packet);

  /// Handles the "getConfigSectionFlags" request.
  AREXPORT void getConfigSectionFlags(ArServerClient *client, 
				                              ArNetPacket *packet);

  /// Handles the "getLastEditablePriority" request.
  AREXPORT void getLastEditablePriority(ArServerClient *client,
                                        ArNetPacket *packet);

  // ---------------------------------------------------------------------------
  // Callback Methods
  // ---------------------------------------------------------------------------

  /// Adds a callback to be called before writing to disk
  AREXPORT void addPreWriteCallback(ArFunctor *functor, 
                                    ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called before writing to disk
  AREXPORT void remPreWriteCallback(ArFunctor *functor);

  /// Adds a callback to be called after writing to disk
  AREXPORT void addPostWriteCallback(ArFunctor *functor, 
                                     ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called after writing to disk
  AREXPORT void remPostWriteCallback(ArFunctor *functor);  

  /// Adds a callback to be called when the config is updated
  AREXPORT void addConfigUpdatedCallback(ArFunctor *functor, 
				                ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called when the config is updated
  AREXPORT void remConfigUpdatedCallback(ArFunctor *functor);  

  /// Restarts the IO manually (mostly for because of a config change)
  AREXPORT void restartIO(const char *reason);
  /// Restarts the software manually (mostly for because of a config change)
  AREXPORT void restartSoftware(const char *reason);
  /// Restarts the hardware manually (mostly for because of a config change)
  AREXPORT void restartHardware(const char *reason);

  /// Adds a callback for when the IO is changed
  void addRestartIOCB(ArFunctor *functor, int position = 50) 
    { myRestartIOCBList.addCallback(functor, position); }
  /// Adds a callback for when the IO is changed
  void remRestartIOCB(ArFunctor *functor)
    { myRestartIOCBList.remCallback(functor); }

  /// Sets a callback for when a RESTART_SERVER config param is changed
  AREXPORT void setRestartSoftwareCB(ArFunctor *restartServerCB);

  /// Gets the callback for when a RESTART_SERVER config param is changed
  AREXPORT ArFunctor *getRestartSoftwareCB(void);

  /// Sets a callback for when a RESTART_ROBOT config param is changed
  AREXPORT void setRestartHardwareCB(ArFunctor *restartRobotCB);

  /// Gets the callback for when a RESTART_ROBOT config param is changed
  AREXPORT ArFunctor *getRestartHardwareCB(void);

  /// Locks the config so we don't do anything with it
  AREXPORT int lockConfig(void) { return myConfigMutex.lock(); }
  /// Tries to lock the config so we don't do anything with it
  AREXPORT int tryLockConfig(void) { return myConfigMutex.tryLock(); }
  /// Unlocks the config so we can use it again
  AREXPORT int unlockConfig(void) { return myConfigMutex.unlock(); }

  /// Writes the config out
  AREXPORT bool writeConfig(void);
  /// Notifies the clients that the config was updated
  AREXPORT bool configUpdated(ArServerClient *client = NULL);
  
  /// Changes the variables that prevent changes
  AREXPORT void setPreventChanges(bool preventChanges = false,
			 const char *preventChangesString = NULL);

  /// loads the whole of a default file (for internal use)
  AREXPORT bool loadDefaultsFromFile(void);
  /// Parses a line of the default config (for internal use)
  AREXPORT bool loadDefaultsFromPacket(ArNetPacket *packet);
  /// Creates an empty default config...
  AREXPORT void createEmptyConfigDefaults(void);

  /// Changes if factory is allowed... this is internal, only for when
  /// that decision is deferred
  void setAllowFactory(bool allowFactory) 
    {  myPermissionAllowFactory = allowFactory; }
  /// Changes if factory is allowed... this is internal, only for when
  /// that decision is deferred
  bool getAllowFactory(void)
    { return myPermissionAllowFactory; }
protected:

  AREXPORT void doGetConfigBySections(ArServerClient *client, 
                                      ArNetPacket *packet,
                                      int version);

  /// Helper method for the get config callbacks (e.g. getConfigBySections, getConfig, ...).
  AREXPORT void handleGetConfig(ArServerClient *client, 
                                ArNetPacket *packet,
                                bool isMultiplePackets,
                                ArPriority::Priority lastPriority,
                                bool isSendIneditablePriorities,
                                int version);

  /// Helper method for the get config callbacks (e.g. getConfigBySections, getConfig, ...).
  AREXPORT bool handleGetConfigSection(ArNetPacket &sending,
                                       ArServerClient *client, 
                                      ArNetPacket *packet,
                                      bool isMultiplePackets,
                                      ArPriority::Priority lastPriority,
                                      bool isSendIneditablePriorities,
                                      int version,
                                      ArConfigSection *section,
                                      int startIndex,
                                      int paramCount,
                                      int sectionIndex,
                                      std::set<std::string> &sentParams);


  /// Internal method that handles a setConfig packet for myConfig or
  /// myDefaults
  bool internalSetConfig(ArServerClient *client, 
                         ArNetPacket *packet,
                         int version,
                         bool isMultiplePackets = false,
                         bool isSingleParam = false);

  /// just creates the default config... (internal, don't use)
  void createDefaultConfig(const char *defaultFileBaseDir);
  /// Adds the default config callbacks;
  void addDefaultServerCommands(void);

  /// Determines the last editable priority based on the input flags
  ArPriority::Priority findLastEditablePriority();

  ArPriority::Priority convertToPriority(int priorityVal,
                                         ArPriority::Priority defaultPriority);


protected:
  
  std::string myRobotName;
  std::string myLogPrefix;

  ArServerBase *myServer;
  ArConfig *myConfig;


  ArConfig *myDefault;
  std::string myDefaultFile;
  std::string myDefaultFileBaseDir;
  ArMutex myDefaultConfigMutex;
  bool myAddedDefaultServerCommands;

  bool myPermissionAllowFactory;
  bool myPreventChanges;
  std::string myPreventChangesString;

  ArMutex myConfigMutex;
  
  std::list<ArFunctor *> myPreWriteCallbacks;
  std::list<ArFunctor *> myPostWriteCallbacks;
  std::list<ArFunctor *> myConfigUpdatedCallbacks;

  ArCallbackList myRestartIOCBList;
  ArFunctor *myRestartSoftwareCB;
  bool myRestartSoftwareCBSet;
  ArFunctor *myRestartHardwareCB;
  bool myRestartHardwareCBSet;

  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigBySectionsCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigBySectionsV2CB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigBySectionsV3CB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigBySectionsV4CB;

  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> mySetConfigCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> mySetConfigParamCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> mySetConfigBySectionsCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> mySetConfigBySectionsV2CB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myReloadConfigCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigDefaultsCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetConfigSectionFlagsCB;
  ArFunctor2C<ArServerHandlerConfig, ArServerClient*, ArNetPacket *> myGetLastEditablePriorityCB;

};

#endif
