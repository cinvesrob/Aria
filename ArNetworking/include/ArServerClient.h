#ifndef NLSERVERCLIENT_H
#define NLSERVERCLIENT_H

#include "ArNetPacket.h"
#include "ArServerCommands.h"
#include "ArNetPacketReceiverTcp.h"
#include "ArNetPacketSenderTcp.h"

class ArServerData;
class ArServerUserInfo;

#include "ArServerClientData.h"
#include "ArServerClientIdentifier.h"

/**
   This class represents the server's connection to the client, and
   contains the socket that the server uses to talk to the client and
   the information for sending packets to the client.
**/

class ArServerClient
{
public:
    /// The state of the connection
  enum ServerState {
    STATE_SENT_INTRO, ///<  opened socket, sent our intro to the server
    STATE_REJECTED, ///< Client was rejected by server
    STATE_CONNECTED, ///< Client is connected to server
    STATE_DISCONNECTED ///< Client has disconnected from server
  };


  /// Constructor
  AREXPORT ArServerClient(
	  ArSocket *tcpSocket, unsigned int udpPort, long authKey, 
	  long introKey, ArRetFunctor2<bool, ArNetPacket *, 
	  struct sockaddr_in *> *sendUdpCallback,
	  std::map<unsigned int, ArServerData *> *dataMap,
	  const char *passwordKey, const char *serverKey,
	  const ArServerUserInfo *userInfo = NULL,
	  int rejecting = 0, const char *rejectingString = "", 
	  bool debugLogging = false, 
	  const char *serverClientName = "ArServerBase_unknown",
	  bool logPasswordFailureVerbosely = false,
	  bool allowSlowPackets = true, bool allowIdlePackets = true,
	  const char *enforceProtocolVersion = "",
	  ArServerCommands::Type enforceType = ArServerCommands::TYPE_UNSPECIFIED);
  /// Destructor
  AREXPORT virtual ~ArServerClient();
  
  /// The callback for taking care of the TCP connection
  AREXPORT bool tcpCallback(void);

  /// The callback for taking care of slow packets 
  AREXPORT bool slowPacketCallback(void);

  /// The callback for taking care of idle packets (logic on idle elsewhere)
  AREXPORT bool idlePacketCallback(void);

  /// Sets the backup timeout
  AREXPORT void setBackupTimeout(double timeoutInMins);

  /// Process the packet whever it came from
  AREXPORT void processPacket(ArNetPacket *packet, bool tcp = true);
  
  /// Send a packet over TCP. 
  /// The command ID of the outgoing packet will be set to the current command ID 
  /// (from the incoming packet).
  AREXPORT virtual bool sendPacketTcp(ArNetPacket *packet);
  /// Send a packet over UDP (unless client only wants TCP; then sends over TCP).
  /// The command ID of the outgoing packet will be set to the current command ID 
  /// (from the incoming packet).
  AREXPORT virtual bool sendPacketUdp(ArNetPacket *packet);

  /// Sees if this client has access to a given group
  AREXPORT bool hasGroupAccess(const char *group);

  /** Broadcasts a packet over TCP if this client wants this data -- For internal
   * use only!
   * @internal 
   */
  AREXPORT void broadcastPacketTcp(ArNetPacket *packet); 

  /**  Broadcasts a packet over UDP if this client wants this data (unless client only wants tcp then sends over tcp)  -- For internal ArNetworking use only!
   * @internal 
   */
  AREXPORT void broadcastPacketUdp(ArNetPacket *packet);

  /// Logs the tracking information (packet and byte counts)
  AREXPORT void logTracking(bool terse);
  
  /// Clears the tracking information (resets counters)
  AREXPORT void resetTracking(void);

  /// Gets the IP string of the client
  AREXPORT const char *getIPString(void) const;

  /// Gets the identifier of this server client
  AREXPORT ArServerClientIdentifier getIdentifier(void) const;

  /// Sets the identifier of this server client
  AREXPORT void setIdentifier(ArServerClientIdentifier identifier);

  /// Sends a shutdown command to the socket
  AREXPORT void shutdown(void);

  /// Sets the sin (network address) for the UDP socket
  AREXPORT void setUdpAddress(struct sockaddr_in *sin);

  /// Gets the sin (network address) for the UDP socket
  AREXPORT struct sockaddr_in *getUdpAddress(void);
 
  /// Gets the authKey used for setting up UDP from this client to the server
  AREXPORT long getAuthKey(void);

  /// Processes the auth packets that came from udp
  AREXPORT void processAuthPacket(ArNetPacket *packet, struct sockaddr_in *sin);
  
  /// Handles the requests for packets 
  AREXPORT void handleRequests(void);

  /// Internal function to get the tcp socket
  AREXPORT ArSocket *getTcpSocket(void) { return &myTcpSocket; }
  /// Forcibly disconnect a client (for client/server switching)
  AREXPORT void forceDisconnect(bool quiet);
  /// Gets how often a command is asked for
  AREXPORT long getFrequency(ArTypes::UByte2 command);
  /// Sets the TCP only flag
  AREXPORT void useTcpOnly(void) { myTcpOnly = true; }
  /// Gets the tcp only flag
  AREXPORT bool isTcpOnly(void) { return myTcpOnly; }
  /// Gets the state
  AREXPORT ServerState getState(void) { return myState; }
  /// Gets if we have any slow packets to process
  AREXPORT bool hasSlowPackets(void) { return myHaveSlowPackets; }
  /// Gets if we have any idle packets to process
  AREXPORT bool hasIdlePackets(void) { return myHaveIdlePackets; }


  /// Starts a new request transaction, incrementing the count. 
  /**
   *  This method is intended to be called solely by the ArServerBase.
   *  It MUST be followed by a call to endRequestTransaction().
   *
   *  Request transactions are used under certain circumstances to 
   *  indicate that a number of related request/reply packets must
   *  be completely processed before background idle processing can 
   *  proceed. See ArServerBase for more information.
  **/
  AREXPORT void startRequestTransaction();

  /// Ends the most recent request transaction, decrementing the count.
  /**
   *  This method is intended to be called solely by the ArServerBase.
   *  @see startRequestTransaction().
  **/
  AREXPORT bool endRequestTransaction();

  /// Returns the number of request transactions that are currently in progress.
  /**
   *  This method is intended to be called solely by the ArServerBase.
   *  @see startRequestTransaction().
  **/
  AREXPORT int getRequestTransactionCount();


  /// Returns the command ID for the specified name, or 0 if it is not found
  AREXPORT unsigned int findCommandFromName(const char *commandName) const;

  /// Get creation time
  AREXPORT ArTime getCreationTime(void) { return myCreationTime; }
protected:
  const char *findCommandName(unsigned int command) const;
  // Some members so that we don't have to pass the number around
  std::list<unsigned int> myCommandStack;
  std::list<bool> myForceTcpStack;  
  std::list<unsigned int> mySlowIdleCommandStack;
  std::list<bool> mySlowIdleForceTcpStack;  

  AREXPORT bool setupPacket(ArNetPacket *packet);
  // Pushes a new number onto our little stack of numbers
  void pushCommand(unsigned int num);
  // Pops the command off the stack
  void popCommand(void);
  // Pushes a new number onto our little stack of numbers
  void pushSlowIdleCommand(unsigned int num);
  // Pops the command off the stack
  void popSlowIdleCommand(void);
  // Gets the command we're on
  unsigned int getCommand();
  // Gets the command we're on
  bool getForceTcpFlag();
  // Pushes a new forceTcp flag onto our little stack of flags
  void pushForceTcpFlag(bool forceTcp);
  // Pops the command off the stack
  void popForceTcpFlag(void);
  // Pushes a new forceTcp flag onto our little stack of flags
  void pushSlowIdleForceTcpFlag(bool forceTcp);
  // Pops the command off the stack
  void popSlowIdleForceTcpFlag(void);

  // this sends a list packet to our client
  void sendListPacket(void);

  // this could just be dealth with by seeing if myUserInfo is NULL,
  // but that may be confusing
  const ArServerUserInfo *myUserInfo;
  std::set<std::string, ArStrCaseCmpOp> myGroups;  
  ArNetPacketSenderTcp myTcpSender;
  std::map<unsigned int, ArServerData *> *myDataMap;
  std::list<ArServerClientData *> myRequested;
  void internalSwitchState(ServerState state);
  ServerState myState;
  ArTime myStateStart;
  bool myUdpConfirmedFrom;
  bool myUdpConfirmedTo;
  ArSocket myTcpSocket;
  ArNetPacketReceiverTcp myTcpReceiver;
  ArFunctor2C<ArServerClient, ArNetPacket *, bool> myProcessPacketCB;
  ArRetFunctor2<bool, ArNetPacket *, struct sockaddr_in *> *mySendUdpCB;
  struct sockaddr_in mySin;
  long myAuthKey;
  long myIntroKey;
  bool myTcpOnly;
  bool mySentTcpOnly;
  std::string myPasswordKey;
  std::string myServerKey;
  bool myDebugLogging;
  ArLog::LogLevel myVerboseLogLevel;
  std::string myLogPrefix;
  ArServerClientIdentifier myIdentifier;
  std::string myIPString;

  double myBackupTimeout;

  int myRejecting;
  std::string myRejectingString;
  
  std::string myEnforceProtocolVersion;
  ArServerCommands::Type myEnforceType;

  ArTime myTrackingStarted;
  class Tracker
  {
  public:
    Tracker() { reset(); }
    virtual ~Tracker() {}
    void reset(void) 
      { myPacketsTcp = 0; myBytesTcp = 0; myPacketsUdp = 0; myBytesUdp = 0; }
    long myPacketsTcp;
    long myBytesTcp;
    long myPacketsUdp;
    long myBytesUdp;
  };
  AREXPORT void trackPacketSent(ArNetPacket *packet, bool tcp);
  AREXPORT void trackPacketReceived(ArNetPacket *packet, ArTypes::UByte2);
  std::map<ArTypes::UByte2, Tracker *> myTrackingSentMap;
  std::map<ArTypes::UByte2, Tracker *> myTrackingReceivedMap;
  bool myLogPasswordFailureVerbosely;  

  bool myAllowSlowPackets;
  bool myAllowIdlePackets;

  ArThread *mySlowIdleThread;
  
  bool myHaveSlowPackets;
  bool myHaveIdlePackets;

  ArMutex mySlowPacketsMutex;
  std::list<ArNetPacket *> mySlowPackets;
  ArMutex myIdlePacketsMutex;
  std::list<ArNetPacket *> myIdlePackets;
  
  /// Number of "request transactions" that are currently in progress for this client.
  int myRequestTransactionCount;
  /// Mutex for multi-threaded access to the request transaction count.
  ArMutex myRequestTransactionMutex;

  ArTime myCreationTime;
};



#endif
