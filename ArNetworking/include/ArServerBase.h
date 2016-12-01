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
#ifndef NLSERVERBASE_H
#define NLSERVERBASE_H

#include "Aria.h"
#include "ArServerCommands.h"
#include "ArServerClient.h"
#include "ArServerData.h"
#include "ArNetPacket.h"
#include "ArNetPacketReceiverUdp.h"
#include "ArServerUserInfo.h"

/**
   Base server for all networking services.

   This class is the "base" server for network services. Start
   this server by calling open(), and then running the loop with run() or runAsync(),
   (or loopOnce() if it is absolutely neccesary to use your own loop; see the
   implementation of run()).
   You can then pass this object to the various "ArServerInfo", "ArServerHandler", and ArServerMode classes. 

   Data request callbacks are added to an ArServerBase object by calling
   addData().

   This class takes care of locking in its own function calls so you
   don't need to worry about locking and unlocking the class before
   you do things.

    The counterpart of this class for clients is ArClientBase.

   You can require user names and passwords for making connections to
   the server, so each user has a set of permissions for different
   command groups. To do this, load configuration from a userInfo file 
   with loadUserInfo().  You can log the
   user info (and what commands users have) with logUserInfo().  You can
   log the command groups with logCommandGroups() or
   logCommandGroupsToFile().  For a description of the file see
   ArServerUserInfo.  There is also an example in
   <code>ArNetworking/examples/serverDemo.userInfo</code>.

   In addition to the user and password requirement you can set a server "key"
   (a special string of text) required (in addition to user and password)
   to access the server with setServerKey(), though this is ONLY used if
   user and password information is requied (so that older clients can
   connect if there's no user and password information required).  

   The way that the passwords are transmitted across the network is this:
   Upon connection the server sends a randomly generated (for that
   connection) string of text to the client which we'll call the
   passwordKey.  The client then combines the serverKey (a string which must be
   known to connect to the server at all), the password, and the
   passwordKey (that string that is generated uniquely for each
   connection) and comes up with an md5 of that string and sends that
   md5 value across the line.  The server then compares the md5 from
   the client with the md5 generated with the correct information on
   the server... if they match the password is good, otherwise it
   rejects the connection.  This is NOT perfect security but should be
   much better then plaintext.  If you care about perfect security
   then use an ssh tunnel with something like PUTTY from the machine
   the client is on to the machine the server is on with an openssh
   server set up (this'll likely even work in windows with cygwin) and
   set up a firewall on the machine the server is on that disallows
   port the server port connection from outside and only allows ssh
   from outside (and get a book or three and read those).

   This class now handles identifiers for the ArServerClients... all
   server bases will let a client set a self identifier string, and a
   here goal... those may be used by any server.  Messages can be sent
   to just server clients that match that message (so that its easier
   to send messages to people who sent commands).  

   Master servers (the main connection server base on the central
   server) can also give each connection an id number (with
   identGetConnectionID).  Things that then connect to the other
   server bases on a master server (slave servers) should then set
   their ConnectionID (with identSetConnectionID) so that things can
   relate the different connections together.

   @sa ArServerSimpleOpener
   @sa ArClientBase
   @sa ArServerMode
**/

class ArServerBase: public ArASyncTask
{
public:
  /// Constructor
  AREXPORT ArServerBase(
	  bool addAriaExitCB = true, 
	  const char *serverName = "",
	  bool addBackupTimeoutToConfig = true,
	  const char *backupTimeoutName = "RobotToClientTimeoutInMins",
	  const char *backupTimeoutDesc = "The amount of time the central server can go without sending a packet to the robot successfully (when there are packets to send).  A number less than 0 means this won't happen.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)",
	  bool masterServer = false, bool slaveServer = false,
	  bool logPasswordFailureVerbosely = false, 
	  bool allowSlowPackets = true, bool allowIdlePackets = true,
	  int maxClientsAllowed = -1,
	  int warningTimeMSec = 250);

  /// Destructor
  AREXPORT virtual ~ArServerBase();
  
  /// Opens the server to accept new client connections
  AREXPORT bool open(unsigned int port, const char *openOnIP = NULL,
		     bool tcpOnly = false);
  /// Closes the server
  AREXPORT void close(void);
  /// Runs the server loop once
  AREXPORT void loopOnce(void);

  /// Adds a callback to be called when requests for some data are recieved.
  AREXPORT bool addData(const char *name, const char *description,
			ArFunctor2<ArServerClient *, ArNetPacket *> *functor,
			const char *argumentDescription,
			const char *returnDescription,
			const char *commandGroup = NULL,
			const char *dataFlags = NULL);

  /// Loads up a set of usernames/passwords/permissions from a file
  AREXPORT bool loadUserInfo(const char *fileName, 
			     const char *baseDirectory = "");
  
  /// Logs the users and their groups
  AREXPORT void logUserInfo(void);

  /// Logs the groups and commands in those groups
  AREXPORT void logCommandGroups(void);  

  /// Logs the groups and commands in those groups to a file
  AREXPORT void logCommandGroupsToFile(const char *fileName);  

  /// Logs the command group names and command group descriptions for those names
  AREXPORT void setGroupDescription(const char *GrpName, const char *GrpDesc);  

  /// Logs the command group names and descriptions
  AREXPORT void logGroupDescriptions(void);  

  /// Logs the command group names and descriptions to a file
  AREXPORT void logGroupDescriptionsToFile(const char *fileName);  

  /// Sets a 'key' needed to access the server through any account
  AREXPORT void setServerKey(const char *serverKey);


  /// Tells the server to reject connectings because we're usinga central server
  AREXPORT void rejectSinceUsingCentralServer(
	  const char *centralServerIPString);

  /// Enforces that the server is using this protocol version
  AREXPORT void enforceProtocolVersion(const char *protocolVersion);

  /// Enforces that the robots that connect are this type
  AREXPORT void enforceType(ArServerCommands::Type type);

  /// Sets the backup timeout
  AREXPORT void setBackupTimeout(double timeoutInMins);

  /// Runs the server in this thread
  AREXPORT virtual void run(void);
  
  /// Runs the server in its own thread
  AREXPORT virtual void runAsync(void);

  /// Logs the connections
  AREXPORT void logConnections(const char *prefix = "");
  
  /// Gets the number of clients connected
  AREXPORT int getNumClients(void);

  /// Gets the number of a command
  AREXPORT unsigned int findCommandFromName(const char *command);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketTcp(ArNetPacket *packet, const char *name);
  
  /// Broadcasts packets to any client wanting this data that matches the ID
  AREXPORT bool broadcastPacketTcpToMatching(
	  ArNetPacket *packet, const char *name, 
	  ArServerClientIdentifier identifier, bool matchConnectionID = false);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketTcpWithExclusion(
	  ArNetPacket *packet, const char *name, 
	  ArServerClient *excludeClient, bool match = false, 
	ArServerClientIdentifier identifier = ArServerClientIdentifier(), 
	bool matchConnectionID = false);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketUdp(ArNetPacket *packet, const char *name);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketUdpWithExclusion(
        ArNetPacket *packet, const char *name, ArServerClient *excludeClient,
	bool match = false, 
	ArServerClientIdentifier identifier = ArServerClientIdentifier(), 
	bool matchConnectionID = false);

  /// Broadcasts packets to any client wanting this data that matches
  AREXPORT bool broadcastPacketUdpToMatching(
	  ArNetPacket *packet, const char *name, 
	  ArServerClientIdentifier identifier, bool matchConnectionID);
 
  /// Delays the current thread by the specified msecs, for use after packet sent.
  AREXPORT void sleepAfterSend(int msecDelay);

  /// Sees if we have any idle processing pending (idle packets or callbacks)
  AREXPORT bool idleProcessingPending(void);

  /// Processes udp packets
  AREXPORT void processPacket(ArNetPacket *packet, 
			      struct sockaddr_in *sin);

  /// Sends Udp packets
  AREXPORT bool sendUdp(ArNetPacket *packet, 
			struct sockaddr_in *sin);
  
  /// Logs the tracking on each client (how many packets and bytes were sent)
  AREXPORT void logTracking(bool terse = true);

  /// Clears the tracking on all the clients (resets counters)
  AREXPORT void resetTracking(void);

  // the function for the thread
  AREXPORT virtual void * runThread(void *arg);

  /// Gets the tcp port we're using
  AREXPORT unsigned int getTcpPort(void);
  /// Gets the udp port we're using
  AREXPORT unsigned int getUdpPort(void);
  /// Gets the IP we're opening on (or NULL if there's none)
  AREXPORT const char *getOpenOnIP(void);

  /// Add a callback to be called at every cycle
  AREXPORT void addCycleCallback(ArFunctor* functor);

  /// Remove a callback to be called at every cycle
  AREXPORT void remCycleCallback(ArFunctor* functor);

  /// Add a callback to be invoked when a client has been removed (but before it is deleted)
  AREXPORT void addClientRemovedCallback(ArFunctor1<ArServerClient *> *functor);
  /// Remove the callback invoked when a client has been removed
  AREXPORT void remClientRemovedCallback(ArFunctor1<ArServerClient *> *functor);
  /// Makes a new serverclient from this socket (for switching, needs no password since this was an outgoing connection to a trusted server)
  AREXPORT ArServerClient *makeNewServerClientFromSocket(
	  ArSocket *socket, bool overrideGeneralReject);
  /// Gets the user info we're using (mostly internal for switching)
  AREXPORT const ArServerUserInfo *getUserInfo(void) const;
  /// Sets the user info we'll use (mostly internal for switching)
  AREXPORT void setUserInfo(const ArServerUserInfo *userInfo);
  /// Adds a callback for requests in a more advanced manner, for internal use
  AREXPORT bool addDataAdvanced(
	  const char *name, const char *description,
	  ArFunctor2<ArServerClient *, ArNetPacket *> *functor,
	  const char *argumentDescription, const char *returnDescription,
	  const char *commandGroup = NULL, const char *dataFlags = NULL, 
	  unsigned int commandNumber = 0,
	  ArFunctor2<long, unsigned int> *requestChangedFunctor = NULL, 
	  ArRetFunctor2<bool, ArServerClient *, ArNetPacket *> 
	  *requestOnceFunctor = NULL);
  /// Sets the data flags to add in addition to those passed in
  AREXPORT void setAdditionalDataFlags(const char *additionalDataFlags);
  /// Gets the frequncy a command is requested (mostly internal)
  AREXPORT long getFrequency(unsigned int command, 
			     bool internalCall = false);
  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketTcpByCommand(
	  ArNetPacket *packet, unsigned int command);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketTcpByCommandWithExclusion(
	  ArNetPacket *packet, unsigned int command,
	  ArServerClient *excludeClient, bool match = false, 
	  ArServerClientIdentifier identifier = ArServerClientIdentifier(), 
	  bool matchConnectionID = false);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketUdpByCommand(
	  ArNetPacket *packet, unsigned int command);

  /// Broadcasts packets to any client wanting this data
  AREXPORT bool broadcastPacketUdpByCommandWithExclusion(
	  ArNetPacket *packet, unsigned int command,
	  ArServerClient *excludeClient, bool match = false, 
	  ArServerClientIdentifier identifier = ArServerClientIdentifier(), 
	  bool matchConnectionID = false);
  
  /// Closes connection with a given connection ID
  AREXPORT void closeConnectionID(ArTypes::UByte4 idNum);
  /// Returns if a command has a data flag
  AREXPORT bool dataHasFlag(const char *name, const char *dataFlag);
  /// Returns if a command has a data flag by command number
  AREXPORT bool dataHasFlagByCommand(unsigned int command, 
				     const char *dataFlag);
  /// Sets debug logging 
  AREXPORT void setDebugLogging(bool debugLogging = false);
  /// Gets if this is using debug logging 
  AREXPORT bool getDebugLogging(void);
  /// Gets the most clients we've had connected
  AREXPORT int getMostClients(void); 

  /// Gets if we're allowing idle packets
  AREXPORT bool allowingIdlePackets(void);

  /// Adds an idle callback to be called once next time the robot is idle
  AREXPORT bool addIdleSingleShotCallback(ArFunctor *functor);

  /// Internal, Sees if we have any slow packets we are waiting to process
  AREXPORT bool hasSlowPackets(void);

  /// Internal, Sees if we have any idle packets we are waiting to process
  AREXPORT bool hasIdlePackets(void);

  /// Internal, Sees if we have any idle callbacks we are waiting to process
  AREXPORT bool hasIdleCallbacks(void);
  
  /// Internal, sets the maximum number of clients 
  AREXPORT void internalSetNumClients(int numClients);

  /// Adds a callback when we switch states while starting
  AREXPORT void addTooManyClientsCB(
	  ArFunctor1<const char *> *functor, int position = 50) 
    { myTooManyClientsCBList.addCallback(functor, position); }
  /// Removes a callback when we switch to running
  AREXPORT void remTooManyClientsCB(ArFunctor1<const char *> *functor)
    { myTooManyClientsCBList.remCallback(functor); }

  /// Internal call that sets the default frequency as a helper for forwarding (if it's faster than the current default)
  AREXPORT bool internalSetDefaultFrequency(const char *command, int frequency);

  /// Internal call to lockup the server (for testing)
  AREXPORT void internalLockup(void);
protected:

  /// The command get the ConnectionID of this server client connection
  AREXPORT void identGetConnectionID(ArServerClient *client, 
				     ArNetPacket *packet);
  /// The command set the ConnectionID of this server client connection
  AREXPORT void identSetConnectionID(ArServerClient *client, 
				     ArNetPacket *packet);
  /// The command set the self identifier of this server client connection
  AREXPORT void identSetSelfIdentifier(ArServerClient *client, 
				       ArNetPacket *packet);
  /// The command set the here goal of this server client connection
  AREXPORT void identSetHereGoal(ArServerClient *client, ArNetPacket *packet);
  

  /// Handles the "startRequestTransaction" packet, and updates the state of the client. 
  /**
   *  "Request transactions" are used to indicate that a sequence of request /
   *  reply packets must be completed before idle processing can proceed. 
   *  Any "startRequestTransaction" MUST be followed by an "endRequestTransaction".
   *
   *  The packets are meant to be used sparingly. They are currently only used
   *  when the Central Server is determining whether the map file needs to 
   *  be downloaded to the client robot. 
   * 
   *  @param client the ArServerClient* that sent the request; updated to indicate
   *  a new transaction is in progress
   *  @param packet the ArNetPacket* that contains the request (no additional data)
  **/
  AREXPORT void handleStartRequestTransaction(ArServerClient *client, 
                                              ArNetPacket *packet);

  /// Handles the "endRequestTransaction" packet, and updates the state of the client. 
  /**
   *  @param client the ArServerClient* that sent the request; updated to indicate
   *  that a transaction has completed
   *  @param packet the ArNetPacket* that contains the request (no additional data)
   *  @see handleStartRequestTransaction
  **/
	AREXPORT void handleEndRequestTransaction(ArServerClient *client, 
                                            ArNetPacket *packet);

  /// Returns the number of request transactions in progress for all connected clients.
  /**
   * @see handleStartRequestTransaction
  **/
  AREXPORT int getRequestTransactionCount();


  /// accepts new sockets and moves them into the client list
  void acceptTcpSockets(void);
  /// Internal function for server/client switching
  AREXPORT ArServerClient *finishAcceptingSocket(
	  ArSocket *socket, bool skipPassword = false, 
	  bool overrideGeneralReject = false);
  bool processFile(void);

  /// callback for our slow idle thread, which is down below...
  /// The command that returns if there is idle processing to do
  AREXPORT void netIdleProcessingPending(ArServerClient *client, 
					 ArNetPacket *packet);

  void slowIdleCallback(void);
  

  
  std::string myServerName;
  std::string myLogPrefix;
  ArMutex myDataMutex;
  ArMutex myClientsMutex;
  ArMutex myCycleCallbacksMutex;
  bool myTcpOnly;
  bool myDebugLogging;
  ArLog::LogLevel myVerboseLogLevel;

  unsigned int myTcpPort;
  unsigned int myUdpPort;
  std::string myOpenOnIP;
  // our mapping of the ints to the serverData
  std::map<unsigned int, ArServerData *> myDataMap;
 /// our mapping of the group names
  std::map<std::string, std::string> myGroupDescription;			

  // default frequency (helper for forwarding)
  std::map<unsigned int, int> myDefaultFrequency;

  int myRejecting;
  std::string myRejectingString;

  std::string myEnforceProtocolVersion;
  ArServerCommands::Type myEnforceType;

  double myBackupTimeout;
  bool myNewBackupTimeout;
  ArMutex myBackupTimeoutMutex;

  ArCallbackList1<const char *> myTooManyClientsCBList;

  // the number we're on for the current data
  unsigned int myNextDataNumber;
  std::string myAdditionalDataFlags;
  bool myOpened;
  std::list<ArServerClient *> myClients;
  ArNetPacketReceiverUdp myUdpReceiver;
  ArFunctor2C<ArServerBase, ArNetPacket *, struct sockaddr_in *> myProcessPacketCB;
  ArRetFunctor2C<bool, ArServerBase, ArNetPacket *, struct sockaddr_in *> mySendUdpCB;  
  ArSocket myTcpSocket;
  ArSocket myUdpSocket;
  std::list<ArFunctor*> myCycleCallbacks;
  std::list<ArFunctor1<ArServerClient *> *> myClientRemovedCallbacks;
  
  ArTimeChecker myTimeChecker;

  ArTypes::UByte4 myConnectionNumber;

  ArSocket myAcceptingSocket;
  
  int myMostClients;
  bool myUsingOwnNumClients;
  int myNumClients;
  

  int myLoopMSecs;

  ArMutex myAddListMutex;
  std::list<ArServerClient *> myAddList;
  ArMutex myRemoveSetMutex;
  std::set<ArServerClient *> myRemoveSet;

  const ArServerUserInfo *myUserInfo;
  bool myLogPasswordFailureVerbosely;
  std::string myServerKey;
  ArFunctorC<ArServerBase> myAriaExitCB;
  ArRetFunctor2C<long, ArServerBase, unsigned int, bool> myGetFrequencyCB;
  ArRetFunctorC<bool, ArServerBase> myProcessFileCB;

  ArFunctor2C<ArServerBase, ArServerClient *, 
	      ArNetPacket *> myIdentGetConnectionIDCB;
  ArFunctor2C<ArServerBase, ArServerClient *, 
	      ArNetPacket *> myIdentSetConnectionIDCB;
  ArFunctor2C<ArServerBase, ArServerClient *, 
	      ArNetPacket *> myIdentSetSelfIdentifierCB;
  ArFunctor2C<ArServerBase, ArServerClient *, 
	      ArNetPacket *> myIdentSetHereGoalCB;

  ArFunctor2C<ArServerBase, ArServerClient *, 
	                          ArNetPacket *> myStartRequestTransactionCB;
  ArFunctor2C<ArServerBase, ArServerClient *, 
	                          ArNetPacket *> myEndRequestTransactionCB;

  ArFunctor2C<ArServerBase, ArServerClient *, 
	      ArNetPacket *> myIdleProcessingPendingCB;

  bool myAllowSlowPackets;
  bool myAllowIdlePackets;

  int myMaxClientsAllowed;

  ArMutex myProcessingSlowIdleMutex;

  
  class SlowIdleThread : public ArASyncTask
  {
  public:
    /// Constructor
    SlowIdleThread(ArServerBase *serverBase);
    /// Destructor
    virtual ~SlowIdleThread(void);
    virtual void *runThread(void *arg);

    /// Add a callback to be called at every cycle
    AREXPORT void addCycleCallback(ArFunctor* functor);

    /// Remove a callback to be called at every cycle
    AREXPORT void remCycleCallback(ArFunctor* functor);

  protected:
    ArServerBase *myServerBase;
    ArMutex myCycleCallbacksMutex;
    std::list<ArFunctor*> myCycleCallbacks;
  
  };
  friend class ArServerBase::SlowIdleThread;

  SlowIdleThread *mySlowIdleThread;
  bool myHaveSlowPackets;
  bool myHaveIdlePackets;

  bool myLastIdleProcessingPending;

  ArMutex myIdleCallbacksMutex;
  bool myHaveIdleCallbacks;
  std::list<ArFunctor *> myIdleCallbacks;
};

#endif
