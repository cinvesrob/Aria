#ifndef ARMAPCHANGER_H
#define ARMAPCHANGER_H

#include "ariaTypedefs.h"
#include "ariaUtil.h"

#include <list>

#include <ArMapInterface.h>
#include <ArMapUtils.h>
#include <ArMutex.h>

#include "ArNetPacket.h"
#include "ArCentralForwarder.h"
#include "ArClientBase.h"
#include "ArClientSwitchManager.h"
#include "ArServerBase.h"
#include "ArServerClient.h"
#include "ArMapChanger.h"

/// Utility class used to apply incremental changes to an Aria map.
/**
 *  ArMapChanger is a helper class that can send and receive ArMapChangeDetails
 *  over the network and apply them to an associated Aria map.  An instance of 
 *  ArMapChanger must be instantiated on both the client and the server side.
 *  (Note that there are two different versions of the constructors for this.)
 * 
 * TODO: Possibly subclass this into ArClientMapChanger and ArServerMapChanger?
 * 
 * @see ArMapChangeDetails
**/
class ArMapChanger 
{
public:
  
  /// Reply status for a map change request
  enum MapChangeReplyType {
    CHANGE_FAILED = 0,
    CHANGE_SUCCESS = 10
  };

  /// Name of the network packet that contains the incremental map changes.
  static const char *PROCESS_CHANGES_PACKET_NAME;

  /// Name of the network packet that contains incremental map changes originated by the robot.
  static const char *PROCESS_ROBOT_CHANGES_PACKET_NAME;

  /// Name of a network packet that is broadcast when the map is being changed.
  static const char *CHANGES_IN_PROGRESS_PACKET_NAME;

  /// Name of the network packet that is broadcast when map changes originated by the robot are complete.
  static const char *ROBOT_CHANGES_COMPLETE_PACKET_NAME;

  // ---------------------------------------------------------------------------
  // Constructors, Destructor
  // ---------------------------------------------------------------------------

  /// Constructs a server map changer.
  /**
   * The map changer will automatically apply the received map change details 
   * to the given map.
   * 
   * @param server the ArServerBase * that receives the network packets
   * @param map the ArMapInterface * to which to apply the map changes
  **/
  AREXPORT ArMapChanger(ArServerBase *server, 
						            ArMapInterface *map);

  /// Constructs a server map changer that can also originate changes (to the EM).
  /**
   * The map changer will automatically apply the received map change details 
   * to the given map.
   * 
   * @param clientSwitch the ArClientSwitchManager * that communicates to the EM
   * @param server the ArServerBase * that receives the network packets
   * @param map the ArMapInterface * to which to apply the map changes
  **/
  AREXPORT ArMapChanger(ArClientSwitchManager *clientSwitch,
                        ArServerBase *server, 
						            ArMapInterface *map);


  /// Constructs a client map changer.
  /**
   * The map changer will send map change details to the server.  The application
   * must request this by calling sendMapChanges.
   *
   * @param client the ArClientBase * which is used to send the map change details
   * @param infoNames the list of info names included in the map
  **/
  AREXPORT ArMapChanger(ArClientBase *client, 
                        const std::list<std::string> &infoNames);
  
  /// Constructs a stand-alone map changer that will apply changes to the given map.
  /**
   * This method is primarily used for debugging.
   *
   * @param map the ArMapInterface * to which to apply the map changes
  **/
  AREXPORT ArMapChanger(ArMapInterface *map);

  /// Destructor
  AREXPORT virtual ~ArMapChanger();
  
  // ---------------------------------------------------------------------------
  // Main Methods
  // ---------------------------------------------------------------------------

  /// Sends the given map changes from the client to the server.
  AREXPORT virtual bool sendMapChanges(ArMapChangeDetails *changeDetails);
  
  /// Sends the given map changes from the robot to the central server.
  AREXPORT virtual bool sendRobotMapChanges(ArMapChangeDetails *changeDetails);

  /// Applies the given map changes received from the client to the associated Aria map.
  AREXPORT virtual bool applyMapChanges(ArMapChangeDetails *changeDetails);

  /// Transmits the given map change packet list from the client to the server.
  AREXPORT virtual bool sendPacketList(const std::list<ArNetPacket *> &packetList);
 
  /// Transmit the given map change packet list from the robot to the central server. 
  AREXPORT virtual bool sendRobotPacketList(const std::list<ArNetPacket *> &packetList);

  // ---------------------------------------------------------------------------
  // Callback Methods
  // ---------------------------------------------------------------------------

  /// Adds a callback to be invoked after the map has been changed on the server.
  /**
   * This method is primarily used on the central server.  After the map changes
   * are successfully applied to its map, they are propagated to all of the 
   * connected robots.
  **/
  AREXPORT virtual bool addChangeCB
                          (ArFunctor2<ArServerClient *, 
                                      std::list<ArNetPacket *> *> *functor);

  /// Removes a callback from the map change list.
  AREXPORT virtual bool remChangeCB
                          (ArFunctor2<ArServerClient *, 
                                      std::list<ArNetPacket *> *> *functor);
  
  /// Adds a callback to be invoked after the remote reply has been received.
  /**
   * This method is primarily used on the robot.  After the ARCL originated 
   * changes have been applied by the Enterprise Manager, this callback list
   * is invoked for other interested parties.
  **/
  AREXPORT virtual bool addRobotChangeReplyCB
                          (ArFunctor2<ArServerClient *, ArNetPacket *>  *functor);

  /// Removes a callback from the remote reply list.
  AREXPORT virtual bool remRobotChangeReplyCB
                          (ArFunctor2<ArServerClient *, ArNetPacket *>  *functor);

  /// Adds a callback to be invoked before the map file is written.
  /**
   * This method is primarily used to temporarily make the server's directory
   * writeable, if necessary.
  **/
  AREXPORT virtual void addPreWriteFileCB(ArFunctor *functor,
                                          ArListPos::Pos position = ArListPos::LAST);

  /// Removes a callback from the pre-write file list.
  AREXPORT virtual void remPreWriteFileCB(ArFunctor *functor);

  /// Adds a callback to be invoked after the map file has been written.
  /**
   * This method is primarily used to restore the server's directory write status,
   * if necessary.
  **/
  AREXPORT virtual void addPostWriteFileCB(ArFunctor *functor,
                                           ArListPos::Pos position = ArListPos::LAST);

  /// Removes a callback from the post-write file list.
  AREXPORT virtual void remPostWriteFileCB(ArFunctor *functor);

// -----------------------------------------------------------------------------

protected:

  /// Indicates the current stage of the map change network packets.
  enum MapChangeCommand {
    START_CHANGES    = 0, ///< First packet that contains changes to be applied to the map
    CONTINUE_CHANGES = 1, ///< Request to continue applying changes to the map
    FINISH_CHANGES   = 2, ///< Last packet that contains changes to be applied to the map
    CANCEL_CHANGES   = 3, ///< Cancel the request to change the map
    LAST_CHANGE_COMMAND = CANCEL_CHANGES ///< Last value in the enumeration
  };

  /// Type of data contained in the map change network packet
  enum MapChangeDataType {
    NO_CHANGE = 0,       ///< No change data
    SUMMARY_DATA = 1,    ///< Summary data, e.g. min/max pos, number of points
    INFO_DATA = 2,       ///< Any of the info data, e.g. MapInfo, RouteInfo
    SUPPLEMENT_DATA = 3, ///< Miscellaneous supplemental data such as origin lat/long/alt
    OBJECTS_DATA = 4,    ///< Map objects, i.e. Cairn lines
    POINTS_DATA = 5,     ///< Map data points
    LINES_DATA = 6,      ///< Map data lines
    LAST_CHANGE_DATA_TYPE = LINES_DATA ///< Last value in the enumeration
  }; 

  /// Miscellaneous constants
  enum {
    CHANGE_DATA_TYPE_COUNT = LAST_CHANGE_DATA_TYPE + 1,
    CHANGE_COMMAND_COUNT = LAST_CHANGE_COMMAND + 1,
    MAX_POINTS_IN_PACKET = 1000,
    MAX_LINES_IN_PACKET = 500
  };


  // ---------------------------------------------------------------------------
  // Packet Handlers
  // ---------------------------------------------------------------------------

  /// Server handler for packets that contain map change details.
  AREXPORT virtual void handleChangePacket(ArServerClient *client, 
                                           ArNetPacket *packet);
  
  AREXPORT virtual void handleRobotChangeReplyPacket(ArServerClient *client, 
                                                     ArNetPacket *packet);

  /// Client handler for the results of applying the map changes on the server. 
  AREXPORT virtual void handleChangeReplyPacket(ArNetPacket *packet);
  
  /// Client handler for the map-changes-in-progress broadcast packet
  AREXPORT virtual void handleChangesInProgressPacket(ArNetPacket *packet);

  /// Client handler for the server's idle-processing-pending broadcast packet
  AREXPORT virtual void handleIdleProcessingPacket(ArNetPacket *packet);

  /// Client handler for when the robot disconnects or is shutdown.
  AREXPORT virtual void handleClientShutdown();

protected:

  // ---------------------------------------------------------------------------
  // Helper Methods
  // ---------------------------------------------------------------------------
  
  // bool applyMapChanges(std::list<ArNetPacket*> &packetList);

  /// Applies all scan data changes that are contained in the given change details.
  /**
   * Scan data include summary data, map data points, and map data lines.
   * If the map contains scan data for multiple sources, then this method 
   * applies all of the applicable changes.
   * 
   * An Aria map must have been previously associated with the map changer.
  **/
  bool applyScanChanges(ArMapChangeDetails *changeDetails);

  /// Applies scan data changes for the specified scan type.
  /**
   * @param changeDetails the ArMapChangeDetails * that describes how the map should
   * be modified; must be non-NULL
   * @param scanType the char * identifier of the scan type to be updated
   * @param parser the ArFileParser used to parse the changeDetails
  **/
  bool applyScanChanges(ArMapChangeDetails *changeDetails,
                        const char *scanType,
                        ArFileParser &parser);

  /// Applies the given map changes to the supplemental data in the map.
  /**
   * @param changeDetails the ArMapChangeDetails * that describes how the map should
   * be modified; must be non-NULL
  **/
  bool applySupplementChanges(ArMapChangeDetails *changeDetails);

  /// Applies the given map changes to the object data in the map.
  /**
   * @param changeDetails the ArMapChangeDetails * that describes how the map should
   * be modified; must be non-NULL
  **/
  bool applyObjectChanges(ArMapChangeDetails *changeDetails);

  /// Applies the given map changes to the info data in the map.
  /**
   * @param changeDetails the ArMapChangeDetails * that describes how the map should
   * be modified; must be non-NULL
  **/
  bool applyInfoChanges(ArMapChangeDetails *changeDetails);

  /// Determines whether the two given map objects are the same object.
  /**
   * If the objects have a name, then the name must be identical.  Otherwise, the type
   * and position must be the same.
  **/
  bool isMatchingObjects(ArMapObject *obj1, 
                         ArMapObject *obj2);
                         //ArMapChangeDetails *changeDetails);

  /// Creates a list of network packets for the given map change details.
  bool convertChangeDetailsToPacketList(ArMapChangeDetails *changeDetails,
                                        std::list<ArNetPacket *> *packetListOut,
                                        bool isRelay = false);

  /// Unpacks the given network packet list and populates the given map change details.
  bool convertPacketListToChangeDetails(std::list<ArNetPacket *> &packetList,
                                        ArMapChangeDetails *changeDetailsOut);
                                        
  /// Creates network packets for the specified map change data and adds them to the given list.
  /**
   * @param dataType the MapChangeDataType that specifies which map data is to be added
   * @param changeType the MapLineChangeType that specifies the type of map change (lines
   * added or deleted)
   * @param scanType the char * identifier of the scan source to add; valid only when 
   * dataType is SUMMARY_DATA
   * @param extra an optional const char * identifier that clarifies which data to add; 
   * when dataType is INFO_DATA, this is the info name; otherwise, ignored
   * @param fileLineSet the ArMapFileLineSet * component of the map change details that
   * is to be converted to network packets
   * @param packetListOut the list of ArNetPackets to which new packets are added
  **/
  bool addFileLineSetPackets(MapChangeDataType dataType, 
                             ArMapChangeDetails::MapLineChangeType changeType,
                             const char *scanType,
                             const char *extra,
                             ArMapFileLineSet *fileLineSet,
                             std::list<ArNetPacket *> *packetListOut);

  /// Inserts header information into the given network packet.
  /**
   * @param command the MapChangeCommand that specifies which command identifier to add to this packet
   * @param dataType the MapChangeDataType that specifies which map data is to be added
   * @param changeType the MapLineChangeType that specifies the type of map change (lines
   * added or deleted)
   * @param scanType the char * identifier of the scan source to add; valid only when 
   * dataType is SUMMARY_DATA
   * @param packet the ArNetPacket * to be modified; must be non-NULL
  **/
  void addHeaderToPacket(MapChangeCommand command,
                         MapChangeDataType dataType, 
                         ArMapChangeDetails::MapLineChangeType changeType,
                         const char *scanType,
                         ArNetPacket *packet);

  /// Creates network packets for the specified file line group and adds them to the given list.
  bool addGroupToPacketList(MapChangeDataType dataType, 
                            ArMapChangeDetails::MapLineChangeType changeType,
                            const char *scanType,
                            ArMapFileLineGroup &group,
                            std::list<ArNetPacket *> *packetListOut);

  /// Creates network packets for the specified file line and adds them to the given list.
  bool addFileLineToPacketList(MapChangeDataType dataType, 
                               ArMapChangeDetails::MapLineChangeType changeType,
                               const char *scanType,
                               const ArMapFileLine &fileLine,
                               std::list<ArNetPacket *> *packetListOut);

  /// Creates network packets for the given map data points and adds them to the given list.
  bool addPointsPackets(ArMapChangeDetails::MapLineChangeType changeType,
                        const char *scanType,
                        std::vector<ArPose> *pointList,
                        std::list<ArNetPacket *> *packetListOut);

  /// Creates network packets for the given map data lines and adds them to the given list.
  bool addLinesPackets(ArMapChangeDetails::MapLineChangeType changeType,
                       const char *scanType,
                       std::vector<ArLineSegment> *lineSegmentList,
                       std::list<ArNetPacket *> *packetListOut);

  /// Unpacks the header data from the given network packet.
  /**
   * @param packet the ArNetPacket * from which to extract the header information;
   * must be non-NULL
   * @param commandOut the MapChangeCommand * extracted from the packet
   * @param origMapIdOut the original ArMapId * extracted from the packet
   * @param newMapIdOut the optional new ArMapId * extracted from the packet
   * @param dataTypeOut the optional MapChangeDataType * extracted from the packet
   * @param changeTypeOut the optional MapLineChangeType * extracted from the packet
   * @param scanTypeOut the optional std::string * extracted from the packet
   * @return bool true if all of the header information was succesfully extracted;
   * false, otherwise
  **/
  bool unpackHeader(ArNetPacket *packet,
                    MapChangeCommand *commandOut,
                    ArMapId *origMapIdOut,
                    ArMapId *newMapIdOut = NULL,
                    MapChangeDataType *dataTypeOut = NULL,
                    ArMapChangeDetails::MapLineChangeType *changeTypeOut = NULL,
                    std::string *scanTypeOut = NULL);

  /// Unpacks the specified file line set from the given network packet.
  /**
   * The file line set is added to the given change details.
  **/
  bool unpackFileLineSet(ArNetPacket *packet,
                         MapChangeDataType dataType, 
                         ArMapChangeDetails::MapLineChangeType changeType,
                         const char *scanType,
                         int *numGroups,
                         int *numChildren,
                         ArMapChangeDetails *changeDetails);

  /// Unpacks the map data points for the specified scan from the given network packet.
  /**
   * The data points are added to the given change details.
  **/
  bool unpackPoints(ArNetPacket *packet,
                    ArMapChangeDetails::MapLineChangeType changeType,
                    const char *scanType,
                    int *numPoints,
                    ArMapChangeDetails *changeDetails);

  /// Unpacks the map data lines for the specified scan from the given network packet.
  /**
   * The data lines are added to the given change details.
  **/
  bool unpackLines(ArNetPacket *packet,
                   ArMapChangeDetails::MapLineChangeType changeType,
                   const char *scanType,
                   int *numLines,
                   ArMapChangeDetails *changeDetails);

  /// Resets all of the network packets in the given list so that they can be read again.
  void resetPacketList(std::list<ArNetPacket*> *packetList);

  /// Waits for a reply from the server.
  /**
   * If a reply is not received within 30 seconds, this method will timeout and return
   * false.
   * 
   * @return bool true if the reply was received; false otherwise
  **/
  bool waitForReply(ArTime &started);
  
  bool waitForCentralServerReply(ArTime &started);

  /// Determines whether idle processing is pending on the server.
  bool isIdleProcessingPending();
  
  /// Adds the given functor to the given callback list.
  AREXPORT void addToCallbackList(ArFunctor *functor,
                                  ArListPos::Pos position,
                                  std::list<ArFunctor*> *cbList);

  /// Removes the given functor from the given callback list.
  AREXPORT void remFromCallbackList(ArFunctor *functor,
                                    std::list<ArFunctor*> *cbList);

private:
  /// Disabled copy constructor
  ArMapChanger(const ArMapChanger &other);

  /// Disabled assignment operator
  ArMapChanger &operator=(const ArMapChanger &other);

protected:

  /// Accumulates the packet list that describes map changes received from a specified client.
  struct ClientChangeInfo
  {
  public:
    /// Constructor for changes received from a client
    ClientChangeInfo(ArServerClient *client);
   
    /// Constructor for changes received from a robot on the CS 
    ClientChangeInfo(ArCentralForwarder *forwarder);

    /// Destructor
    ~ClientChangeInfo();

    /// Adds the given packet to the list
    void addPacket(ArNetPacket *packet);

    /// Server client which sent the map changes...
    ArServerClient *myClient;

    /// Or the forwarder that sent the map changes    
    ArCentralForwarder *myForwarder;

    /// Time at which the first packet in this list was received
    ArTime myStartTime;
    /// Time at which the most recent packet in this list was received
    ArTime myLastActivityTime;
    /// List of network packets received from the client for these map changes
    std::list<ArNetPacket *> myPacketList;
  }; // end struct ClientChangeInfo


  /// Aria map currently in use
  ArMapInterface *myMap;
  /// Copy of the current Aria map, used to make sure the changes can be successfully made
  ArMapInterface *myWorkingMap;

  /// Change details to apply to the Aria map
  ArMapChangeDetails *myChangeDetails;
  /// Number of info types in the associated Aria map
  //int myInfoCount;

  /// List of info types in the associated info map
  std::list<std::string> myInfoNames;

  /// Associated server base; non-NULL only when changer instantiated on the server
  ArServerBase *myServer;

  /// Associated client switch manager; non-NULL when changer instantiated on robot with EM
  ArClientSwitchManager *myClientSwitch;

  /// Whether the client switch manager's serverClient has been initialized with a handler (for EM)
  bool myIsServerClientInit;


  /// Mutex that protects access to the myClient member
  ArMutex myClientMutex;
  /// Associated client base; non-NULL only when changer instantiated on the client
  ArClientBase *myClient;

  /// Mutex that protects access to the myClientInfo member
  ArMutex myClientInfoMutex;
  /// Information regarding the server client that is currently sending map changes
  ClientChangeInfo *myClientInfo;

  /// Mutex that protects access to the interleave data
  ArMutex myInterleaveMutex;
  /// Whether the client is ready to send another packet
  bool myReadyForNextPacket;
  /// Whether the client is waiting for a reply from the server
  bool myIsWaitingForReturn;

  /// Mutex that protects access to the myIsIdleProcessingPending member
  ArMutex myIdleProcessingMutex;
  /// Whether the server has data to process once it becomes idle
  bool myIsIdleProcessingPending;

  /// List of callbacks to be invoked before the changed map file is written
  std::list<ArFunctor*> myPreWriteCBList;
  /// List of callbacks to be invoked after the changed map file is written
  std::list<ArFunctor*> myPostWriteCBList;
          
  /// List of server client callbacks to be invoked after the map has been changed
  std::list< ArFunctor2<ArServerClient *, std::list<ArNetPacket *> *> *> 
                                                             myChangeCBList;
  
  /// List of server client callbacks to be invoked after the map has been changed
  std::list< ArFunctor2<ArServerClient *, ArNetPacket *> *>  myRobotChangeReplyCBList;

  /// Server handler for the network packets that describe map changes.
  ArFunctor2C<ArMapChanger, ArServerClient *, ArNetPacket *> myHandleChangePacketCB;
  
  ArFunctor2C<ArMapChanger, ArServerClient *, ArNetPacket *> myHandleRobotReplyPacketCB;
  
  /// Client handler for the map-changes-in-progress packet
  ArFunctor1C<ArMapChanger, ArNetPacket *>  myHandleChangesInProgressPacketCB;
  ///  Client handler for the reply packet
  ArFunctor1C<ArMapChanger, ArNetPacket *> myHandleReplyPacketCB;
  /// Client handler for the idle-processing-in-progress packet
  ArFunctor1C<ArMapChanger, ArNetPacket *> myHandleIdleProcessingPacketCB;
  /// Handler invoked when the client shuts down
  ArFunctorC<ArMapChanger> myClientShutdownCB;

}; // end class ArMapChanger

#endif // ARMAPCHANGER
