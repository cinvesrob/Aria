#ifndef NLCLIENTBASE_H
#define NLCLIENTBASE_H

#include "Aria.h"
#include "ArClientCommands.h"
#include "ArServerCommands.h"
#include "ArNetPacket.h"
#include "ArNetPacketSenderTcp.h"
#include "ArNetPacketReceiverTcp.h"
#include "ArNetPacketReceiverUdp.h"
#include "ArClientData.h"

/**
   @brief The base client class

   You need to connect a client to a server using blockingConnect() with
   the address for the server.  Then you should add handlers to
   the client to receive information from the server (with addHandler(),
   request(), and requestOnce()).  Then call run() or runAsync()
   (or loopOnce() if you really know what you're doing).
   You can also add a callback that will get called every cycle of the
   client.
   
   User and password information may be required by the server.
   For details on that see ArServerBase.  The most important thing is
   the modification to blockingConnect(), it can now take a user and
   password argument and upon failure you'll want to check wasRejected()
   to determine whether the client couldn't find the server or if the user and
   password were rejected.  Another relevant function is setServerKey()
   to set the key (string of text) we need to connect to the server.

   This class should be thread safe... The only thing you cannot do is
   call 'addHandler' or 'remHandler' from within a handler for that
   specific piece of data.   
 **/

class ArClientBase : public ArASyncTask
{
public:
  
  /// The state of the connection
  enum ClientState {
    STATE_NO_CONNECTION, ///< The client has not connected
    STATE_FAILED_CONNECTION, ///< The client tried to connect and failed
    STATE_OPENED_SOCKET, ///< Client opened socket, waiting for intro from srvr
    STATE_EXCHANGED_INTROS, ///< Client and server have exchanged introductions
    STATE_REJECTED, ///< Client was rejected by server
    STATE_WAITING_LIST, ///< Client was connected to server, waiting for list
    STATE_CONNECTED, ///< Client is connected to server
    STATE_LOST_CONNECTION ///< Client lost connection to server
  };
  
  enum {
    CLIENT_KEY_LENGTH = 16
  };

  enum NonBlockingConnectReturn
  {
    NON_BLOCKING_CONTINUE, ///< Client didn't connect or fail to connect yet
    NON_BLOCKING_CONNECTED, ///< Client connected
    NON_BLOCKING_FAILED ///< Client couldn't connect
  };

  /// Constructor
  AREXPORT ArClientBase();
  /// Destructor
  AREXPORT virtual ~ArClientBase();
  
  /// Sets the optional name of the connected robot for debugging purposes.
  AREXPORT virtual void setRobotName(const char *name, 
				                             bool debugLogging = false,
                                     int robotId = 0);

  /// Returns the optional name of the connected robot for debugging purposes.
  AREXPORT virtual const char *getRobotName() const;

  /// Returns the entire string used to prefix log entries for this client base.
  AREXPORT virtual const char *getLogPrefix() const;

  /// Returns if we're doing debug logging or not
  AREXPORT bool getDebugLogging(void);

  /// Connect to a server
  AREXPORT bool blockingConnect(const char *host, int port, bool log = true,
				const char *user = NULL, 
				const char *password = NULL,
				const char *openOnIP = NULL);

  /// Disconnect from a server
  /**
   * This method blocks while the connection is being shutdown. 
  **/
  AREXPORT bool disconnect(void);

  /// Disconnect from a server next time through the loop
  /**
   * This method does not block while the connection is being
   * shutdown, and the disconnect only happens the next time the
   * client base is in its loop
  **/
  AREXPORT void disconnectSoon(void);

  /** @return true if we're connected to a server */
  AREXPORT bool isConnected(void) { return myState == STATE_CONNECTED;}
  
  /** @return true if a server connection attempt failed because the server rejected the username or password, false if the connection failed for another reason, or the username/password were accepted. */
  AREXPORT bool wasRejected(void) { return myState == STATE_REJECTED; }

  /// Gets the state of the client
  AREXPORT ClientState getState(void) { return myState; }

  /// Adds a functor for some particular data
  AREXPORT bool addHandler(const char *name, 
			    ArFunctor1 <ArNetPacket *> *functor);

  /// Removes a functor for some particular data by name
  AREXPORT bool remHandler(const char *name, ArFunctor1<ArNetPacket *> *functor);

  /// Request some data every @a mSec milliseconds
  AREXPORT bool request(const char *name, long mSec, 
				ArNetPacket *packet = NULL);

  /// Don't want this data anymore
  AREXPORT bool requestStop(const char *name);
  
  /// Request some data (or send a command) just once
  AREXPORT bool requestOnce(const char *name, 
			                      ArNetPacket *packet = NULL,
                            bool quiet = false);

  /// Request some data (or send a command) just once by UDP 
  AREXPORT bool requestOnceUdp(const char *name, 
			       ArNetPacket *packet = NULL, 
			       bool quiet = false);

  /// Request some data (or send a command) just once with a string as argument
  AREXPORT bool requestOnceWithString(const char *name, const char *str);
  
  /// Sees if this data exists
  AREXPORT bool dataExists(const char *name);

  /// Gets the name of the host we tried to connect to
  AREXPORT const char *getHost(void);

  
  /// Sets the 'key' needed to connect to the server
  AREXPORT void setServerKey(const char *serverKey, bool log = true);
  
  /// Enforces the that the server is using this protocol version
  AREXPORT void enforceProtocolVersion(const char *protocolVersion, 
				       bool log = true);

  /// Enforces that the robots that connect are this type
  AREXPORT void enforceType(ArServerCommands::Type type, bool log = true);

  /// Gets the last time a packet was received
  AREXPORT ArTime getLastPacketReceived(void);

  /// Sets the backup timeout
  AREXPORT void setBackupTimeout(double timeoutInMins);

  /// Runs the client in this thread
  AREXPORT virtual void run(void);
  
  /// Runs the client in its own thread
  AREXPORT virtual void runAsync(void);

  /// Stops the thread and sets the stopped flag
  AREXPORT virtual void stopRunning(void);

  /// Returns whether the client has been stopped (regardless of whether its in its own thread)
  AREXPORT virtual bool isStopped();

  /// Print out or data with descriptions
  AREXPORT void logDataList(void);

  /// Adds a functor to call every cycle
  AREXPORT void addCycleCallback(ArFunctor *functor);
  
  /// Removes a functor called every cycle
  AREXPORT void remCycleCallback(ArFunctor *functor);

  /// Send a packet over TCP
  AREXPORT bool sendPacketTcp(ArNetPacket *packet);
  /// Send a packet over UDP (unless client only wants tcp then sends over tcp)
  AREXPORT bool sendPacketUdp(ArNetPacket *packet);

  /// Sets the time to allow for connection (default 3)
  AREXPORT void setConnectTimeoutTime(int sec);

  /// Gets the time allowed for connection
  AREXPORT int getConnectTimeoutTime(void);

  /// Logs the tracking information (packet and byte counts)
  AREXPORT void logTracking(bool terse);
  
  /// Clears the tracking information (resets counters)
  AREXPORT void resetTracking(void);

  /// Adds a call for when the server shuts down 
  AREXPORT void addServerShutdownCB(ArFunctor *functor,
			       ArListPos::Pos position = ArListPos::LAST);

  /// Removes a call for when the server shuts down
  AREXPORT void remServerShutdownCB(ArFunctor *functor);

  /// Adds a call for when a disconnection has occured because of error
  AREXPORT void addDisconnectOnErrorCB(ArFunctor *functor,
			       ArListPos::Pos position = ArListPos::LAST);

  /// Removes a call for when a disconnection has occured because of error
  AREXPORT void remDisconnectOnErrorCB(ArFunctor *functor);

  /// Run the loop once
  AREXPORT void loopOnce(void);

  /// Process the packet wherever it came from
  AREXPORT void processPacket(ArNetPacket *packet, bool tcp);

  /// Process a packet from udp (just hands off to processPacket)
  AREXPORT void processPacketUdp(ArNetPacket *packet,
				struct sockaddr_in *sin);

  /// Sets it so we only get TCP data from the server not UDP
  AREXPORT void setTcpOnlyFromServer(void);
  /// Sets it so we only send TCP data to the server
  AREXPORT void setTcpOnlyToServer(void);

  /// Returns whether we only get TCP data from the server not UDP
  AREXPORT bool isTcpOnlyFromServer(void);
  /// Returns whether we only send TCP data to the server
  AREXPORT bool isTcpOnlyToServer(void);
  /// Gets a (probably) unique key that can be used to identify the client (after connection)
  AREXPORT void getClientKey(unsigned char key[CLIENT_KEY_LENGTH]);
  

  /// Starts the process of disconnecting from the server.
  /**
   * This method should be called in threads that should not block
   * (such as the main GUI thread).  The call should be followed by 
   * a delay of one second, followed by a call to 
   * finishNonBlockingDisconnect().
  **/
  AREXPORT bool startNonBlockingDisconnect();

  /// Finishes the process of disconnecting from the server.
  /**
   * This method should be used in conjunction with 
   * startNonBlockingDisconnect in threads that should not block
   * (such as the main GUI thread).  
  **/
  AREXPORT void finishNonBlockingDisconnect();

  /// Gets the name of the data a packet is for
  AREXPORT const char *getName(ArNetPacket *packet, bool internalCall = false);
  /// Gets the name of the data a command is
  AREXPORT const char *getName(unsigned int command, bool internalCall = false);
  // the function for the thread
  AREXPORT virtual void * runThread(void *arg);
  /// Internal function to get the socket (no one should need this)
  AREXPORT struct in_addr *getTcpAddr(void) { return myTcpSocket.inAddr(); }
  /// Internal function that'll do the connection (mainly for switch connections)
  AREXPORT bool internalBlockingConnect(
	  const char *host, int port, bool log, const char *user, 
	  const char *password, ArSocket *tcpSocket, 
	  const char *openOnIP = NULL);
  /// Internal function that'll start the non blocking connection (mainly for switch connections)
  AREXPORT NonBlockingConnectReturn internalNonBlockingConnectStart(
	  const char *host, int port, bool log, const char *user, 
	  const char *password, ArSocket *tcpSocket, 
	  const char *openOnIP = NULL);
  /// Internal function that'll start the non blocking connection (mainly for switch connections)
  AREXPORT NonBlockingConnectReturn internalNonBlockingConnectContinue(void);
  /// Internal function to get the tcp socket
  AREXPORT ArSocket *getTcpSocket(void) { return &myTcpSocket; }
  /// Internal function to get the udp socket
  AREXPORT ArSocket *getUdpSocket(void) { return &myUdpSocket; }
  /// Internal function get get the data map
  AREXPORT const std::map<unsigned int, ArClientData *> *getDataMap(void)
    { return &myIntDataMap; }
  /// Returns the command number from the name
  AREXPORT unsigned int findCommandFromName(const char *name);
  /// Request some data every mSec milliseconds by command not name
  AREXPORT bool requestByCommand(unsigned int command, long mSec, 
				 ArNetPacket *packet = NULL);
  /// Don't want this data anymore, by command not name
  AREXPORT bool requestStopByCommand(unsigned int command);
  
  /// Request some data (or send a command) just once by command not name
  AREXPORT bool requestOnceByCommand(unsigned int command,
			    ArNetPacket *packet = NULL);
  /// Request some data (or send a command) just once by command not name
  AREXPORT bool requestOnceByCommandUdp(unsigned int command,
			    ArNetPacket *packet = NULL);
  /// Gets if we received the list of commands
  bool getReceivedDataList(void) { return myReceivedDataList; }
  /// Gets if we received the list of arguments and
  bool getReceivedArgRetList(void) { return myReceivedArgRetList; }
  /// Gets if we received the list of commands
  bool getReceivedGroupAndFlagsList(void) { return myReceivedGroupAndFlagsList; }
  /// Tells us the reason the connection was rejected (see ArServerCommands for details)
  int getRejected(void) { return myRejected; }
  /// Tells us the reason the connection was rejected (see ArServerCommands for details)
  const char *getRejectedString(void) { return myRejectedString; }
protected:
  
  /// Optional robot name for logging
  std::string myRobotName;
  /// Optional prefix to be inserted at the start of log messages
  std::string myLogPrefix;
  /// If we're enforcing the version of the protocol or not
  std::string myEnforceProtocolVersion;
  /// if we're a particular type
  ArServerCommands::Type myEnforceType;

  AREXPORT bool setupPacket(ArNetPacket *packet);
  ArTime myLastPacketReceived;
  std::list<ArFunctor *> myServerShutdownCBList;
  std::list<ArFunctor *> myDisconnectOnErrorCBList;
  std::list<ArFunctor *> myCycleCallbacks;
  void clear(void);
  // does the first part of connection
  bool internalConnect(const char *host, int port, bool obsolete,
		       const char *openOnIP);
  void internalStartUdp(void);
  void buildList(ArNetPacket *packet);
  void internalSwitchState(ClientState state);
  bool myReceivedDataList;
  bool myReceivedArgRetList;
  bool myReceivedGroupAndFlagsList;
  ClientState myState;
  ArTime myStateStarted;
  bool myUdpConfirmedFrom;
  bool myUdpConfirmedTo;
  // if we only send tcp
  bool myTcpOnlyTo;
  // if we only receive tcp from the server
  bool myTcpOnlyFrom;

  bool myIsRunningAsync;
  bool myDisconnectSoon;
  bool myIsStartedDisconnect;
  bool myIsStopped;

  bool myQuiet;
  bool myDebugLogging;
  ArLog::LogLevel myVerboseLogLevel;
  std::string myHost;
  std::string myUser;
  std::string myPassword;
  // the time we allow for connections
  int myTimeoutTime;
  // the time we started our connection
  ArTime myStartedConnection;
  ArTime myStartedUdpConnection;
  ArTime myStartedShutdown;
  
  enum NonBlockingConnectState
  {
    NON_BLOCKING_STATE_NONE,
    NON_BLOCKING_STATE_TCP,
    NON_BLOCKING_STATE_UDP
  };
  
  NonBlockingConnectState myNonBlockingConnectState;
  

  ArMutex myDataMutex;
  ArMutex myClientMutex;
  ArMutex myMapsMutex;
  ArMutex myStateMutex;
  ArMutex myCallbackMutex;
  ArMutex myCycleCallbackMutex;
  ArMutex myPacketTrackingMutex;
  // our map of names to ints
  std::map<std::string, unsigned int> myNameIntMap;
  // our map of ints to functors
  std::map<unsigned int, ArClientData *> myIntDataMap;
  
  struct sockaddr_in myUdpSin;
  bool myUdpSinValid;
  // the port the server said it was using
  unsigned int myServerReportedUdpPort;
  // the port the server actually is using
  unsigned int myServerSentUdpPort;
  unsigned int myUdpPort;
  long myAuthKey;
  long myIntroKey;
  std::string myServerKey;
  double myBackupTimeout;
  // this is a key we have for identifying ourselves moderately uniquely
  unsigned char myClientKey[16];
  ArNetPacketSenderTcp myTcpSender;
  ArNetPacketReceiverTcp myTcpReceiver;
  ArNetPacketReceiverUdp myUdpReceiver;
  ArSocket myTcpSocket;
  ArSocket myUdpSocket;
  ArFunctor2C<ArClientBase, ArNetPacket *, bool> myProcessPacketCB;
  ArFunctor2C<ArClientBase, ArNetPacket *, struct sockaddr_in *> myProcessPacketUdpCB;

  int myRejected;
  char myRejectedString[32000];

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
  AREXPORT void trackPacketReceived(ArNetPacket *packet, bool tcp);
  std::map<unsigned int, Tracker *> myTrackingSentMap;
  std::map<unsigned int, Tracker *> myTrackingReceivedMap;

};

#endif // NLCLIENTBASE_H
