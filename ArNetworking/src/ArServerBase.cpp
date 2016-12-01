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
#include "ArServerBase.h"
#include "ArServerCommands.h"
#include "ArClientCommands.h"
#include "ArServerMode.h"

//#define ARDEBUG_SERVERBASE

#if (defined(ARDEBUG_SERVERBASE))
//#if ((_DEBUG) && defined(ARDEBUG_SERVERBASE))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

/**
   @param addAriaExitCB whether to add an exit callback to aria or not
   @param serverName the name for logging
   @param addBackupTimeoutToConfig whether to add the backup timeout
   (to clients of this server) as a config option, if this is true it
   sets a default value for the timeout of 2 minutes, if its false it
   has no timeout (but you can set one, see
   ArServerBase::setBackupTimeout())
   @param backupTimeoutName the name of backup timeout
   @param backupTimeoutName the description of backup timeout
   @param masterServer If this is a master server (ie a central manager)
   @param slaveServer If this is a slave server (ie a central forwarder)
   @param logPasswordFailureVerbosely Whether we log password failure verbosely or not
   @param allowSlowPackets Whether we allow slow packets (that go in
   their own thread) or not... if we don't then they're processed in
   the main thread like they have always been
   @param allowIdlePackets Whether we allow idle packets (that go in
   their own thread and are processed only when the robot is idle) or
   not... if we don't then they're processed in the main thread like
   they have always been
   @param numClientsAllowed This is the number of client connections
   allowed, negative values mean allow any number (the default is -1)
**/

AREXPORT ArServerBase::ArServerBase(bool addAriaExitCB, 
				    const char * serverName,
				    bool addBackupTimeoutToConfig,
				    const char *backupTimeoutName,
				    const char *backupTimeoutDesc,
				    bool masterServer, bool slaveServer,
				    bool logPasswordFailureVerbosely,
				    bool allowSlowPackets,
				    bool allowIdlePackets,
				    int maxClientsAllowed, 
				    int warningTimeMSec) :
  myProcessPacketCB(this, &ArServerBase::processPacket),
  mySendUdpCB(this, &ArServerBase::sendUdp),
  myAriaExitCB(this, &ArServerBase::close),
  myGetFrequencyCB(this, &ArServerBase::getFrequency, 0, true),
  myProcessFileCB(this, &ArServerBase::processFile),
  myIdentGetConnectionIDCB(this, &ArServerBase::identGetConnectionID),
  myIdentSetConnectionIDCB(this, &ArServerBase::identSetConnectionID),
  myIdentSetSelfIdentifierCB(this, &ArServerBase::identSetSelfIdentifier),
  myIdentSetHereGoalCB(this, &ArServerBase::identSetHereGoal),
	myStartRequestTransactionCB(this, &ArServerBase::handleStartRequestTransaction),
	myEndRequestTransactionCB(this, &ArServerBase::handleEndRequestTransaction),
  myIdleProcessingPendingCB(this, &ArServerBase::netIdleProcessingPending)
{


  myDataMutex.setLogName("ArServerBase::myDataMutex");
  myClientsMutex.setLogName("ArServerBase::myClientsMutex");
  myCycleCallbacksMutex.setLogName("ArServerBase::myCycleCallbacksMutex");
  myAddListMutex.setLogName("ArServerBase::myAddListMutex");
  myRemoveSetMutex.setLogName("ArServerBase::myRemoveSetMutex");
  myProcessingSlowIdleMutex.setLogName(
	  "ArServerBase::myProcessingSlowIdleMutex");
  myIdleCallbacksMutex.setLogName(
	  "ArServerBase::myIdleCallbacksMutex");
  myBackupTimeoutMutex.setLogName("ArServerBase::myBackupTimeoutMutex");
  

  if (serverName != NULL && serverName[0] > 0)
    myServerName = serverName;
  else
    myServerName = "ArServer";

  myLogPrefix = myServerName + "Base: ";
  myDebugLogging = false;
  myVerboseLogLevel = ArLog::Verbose;

  myTimeChecker.setName(myServerName.c_str());
  myTimeChecker.setDefaultMSecs(warningTimeMSec);

  setThreadName(myServerName.c_str());
  myTcpPort = 0;
  myUdpPort = 0;
  myRejecting = 0;
  myOpened = false;
  myUdpReceiver.setProcessPacketCB(&myProcessPacketCB);
  if (slaveServer)
    myNextDataNumber = 40000;
  else
    myNextDataNumber = 256;
  myUserInfo = NULL;
  myAriaExitCB.setName("ArServerBaseExit");
  if (addAriaExitCB)
    Aria::addExitCallback(&myAriaExitCB, 20);
  
  myLogPasswordFailureVerbosely = logPasswordFailureVerbosely;
  myConnectionNumber = 1;
  
  myMostClients = 0;
  myUsingOwnNumClients = true;
  myNumClients = 0;
  
  myLoopMSecs = 10;
  /*
  Aria::getConfig()->addParam(
	  ArConfigArg("ArServerBase_MSecToSleepInLoop",
		      &myLoopMSecs,
		      "The number of MS to sleep in the loop (default 1)", 1),
	  "Misc Testing", 
	  ArPriority::EXPERT);
  */

  if (slaveServer)
  {
    myAllowSlowPackets = false;
    myAllowIdlePackets = false;
  }
  else
  {
    myAllowSlowPackets = allowSlowPackets;
    myAllowIdlePackets = allowIdlePackets;
  }
  mySlowIdleThread = NULL;

  if (myAllowSlowPackets || myAllowIdlePackets)
    mySlowIdleThread = new SlowIdleThread(this);
  
  myHaveSlowPackets = false;
  myHaveIdlePackets = false;
  myHaveIdleCallbacks = false;

  myMaxClientsAllowed = maxClientsAllowed;

  myEnforceType = ArServerCommands::TYPE_UNSPECIFIED;

  if (addBackupTimeoutToConfig)
  {
    myBackupTimeout = 10;
    Aria::getConfig()->addParam(
	    ArConfigArg(backupTimeoutName,
			&myBackupTimeout,
			backupTimeoutDesc, -1),
	    "Connection timeouts", 
	    ArPriority::DETAILED);
    myProcessFileCB.setName("ArServerBase");
    Aria::getConfig()->addProcessFileCB(&myProcessFileCB, -100);
  }
  else
    myBackupTimeout = -1;

  myNewBackupTimeout = false;

  if (masterServer)
  {
    addData("identGetConnectionID", 
	    "gets an id for this connection, this id should then be set on any connections to the other servers associated with this one (ie the ones for individual robots) so that all the connections to one client can be tied together",
	    &myIdentGetConnectionIDCB, "none", "ubyte4: id",
	    "RobotInfo", "RETURN_SINGLE");
  }

  if (slaveServer)
  {
    addData("identSetConnectionID", 
	    "Sets an id for this connection, this id should then be from the getID on the main connection for this server, this is so that all the connections to one client can be tied together",
	    &myIdentSetConnectionIDCB, "ubyte4: id", "none",
	    "RobotInfo", "RETURN_SINGLE");
  }

  addData("identSetSelfIdentifier", 
	  "Sets an arbitrary self identifier for this connection, this can be used by programs to send information back to the same client (if this and the identSetHereGoal)",
	  &myIdentSetSelfIdentifierCB, "string: selfID", "none",
	  "RobotInfo", "RETURN_SINGLE");
  
  addData("identSetHereGoal", 
	  "Sets an string for the here goal for this connection, this can be used by programs to send information back to the same client (if this and the identSetSelfID match)",
	  &myIdentSetHereGoalCB, "string: hereGoal", "none",
	  "RobotInfo", "RETURN_SINGLE");

  addData("requestOnceReturnIsAlwaysTcp", 
	  "The presence of this command means that any requestOnce commands will have the data sent back to them as tcp... any requestOnceUdp will be returned either as tcp or udp (that is up to the callback sending the data)",
	  &myIdentSetHereGoalCB, "none", "none",
	  "RobotInfo", "RETURN_NONE");

  if (myAllowIdlePackets)
    addData("idleProcessingPending",
	    "The presence of this command means that the server will process packets flagged IDLE_PACKET only when the robot is idle.  This packet will be broadcast when the state of idle processing changes.",
	    &myIdleProcessingPendingCB, "none", "byte: 1 for idle processing pending, 0 for idle processing not pending",
	    "RobotInfo", "RETURN_SINGLE");

  addData("requestClientRestart", 
	  "If certain changes on the robot happen the clients may need to restart, this message is broadcast when those changes occur.",
	  NULL,
	  "none",
	  "byte: restartNumber, which is 1 for a restart because of config changes (there will likely be more reasons later); string: human readable reason for restart.   (The restartNumber is intended for heavy weight clients, the string is intended for lighter weight clients.)",
	  "RobotInfo", "RETURN_NONE");
	


  // The start/end-RequestTransaction packets encapsulate a sequence of related 
  // messages. Their primary purpose is to delay the execution of any queued
  // idle packets until the sequence has completed. (This somewhat implies that
  // they shouldn't be used to create a sequence of idle packets.) 
  //
  // Currently, these packets are only used by the Central Server when 
  // it is determining whether the map file needs to be downloaded to the robot.
  // The checkMap or setConfig* commands are executed in the idle thread after
  // the download has completed.
  //
  addData("startRequestTransaction", 
          "Special packet that marks the beginning of a multiple-packet request-response sequence. Must be followed by an endRequestTransaction packet upon completion.",
	        &myStartRequestTransactionCB,
	        "none",
	        "",
	        "RobotInfo", "RETURN_NONE");
  
  addData("endRequestTransaction", 
          "Special packet that marks the end of a multiple-packet request-response sequence. Idle processing does not proceed until transaction is ended.",
	        &myEndRequestTransactionCB,
	        "none",
	        "",
	        "RobotInfo", "RETURN_NONE");
	
}

AREXPORT ArServerBase::~ArServerBase()
{
  close();

  if (mySlowIdleThread != NULL)
  {
    delete mySlowIdleThread;
    mySlowIdleThread = NULL;
  }
}

/**
   This opens the server up on the given port, you will then need to
   run the server for it to actually listen and respond to client connections.

   @param port the port to open the server on 
   @param openOnIP If not null, only listen on the local network interface with this address
   @param tcpOnly If true, only accept TCP connections. If false, use both TCP
   and UDP communications with clients.

   @return true if the server could open the port, false otherwise
**/
AREXPORT bool ArServerBase::open(unsigned int port, const char *openOnIP,
				 bool tcpOnly)
{
  myDataMutex.lock();
  myTcpPort = port;
  myTcpOnly = tcpOnly;
  if (openOnIP != NULL)
    myOpenOnIP = openOnIP;
  else
    myOpenOnIP = "";


  if (myTcpSocket.open(myTcpPort, ArSocket::TCP, openOnIP))
  {
    // This can be taken out since the open does this now
    //myTcpSocket.setLinger(0);
    myTcpSocket.setNonBlock();
  }
  else
  {
    myOpened = false;
    myDataMutex.unlock();
    return false;
  }

  if (myTcpOnly)
  {
    if (openOnIP != NULL)
      ArLog::log(ArLog::Normal, 
		 "%sStarted on tcp port %d on ip %s.", 
		 myLogPrefix.c_str(), myTcpPort, openOnIP);
    else
      ArLog::log(ArLog::Normal, "%sStarted on tcp port %d.",
		 myLogPrefix.c_str(), myTcpPort);
    myOpened = true;
    myUdpPort = 0;
    myDataMutex.unlock();
    return true;
  }
  
  if (myUdpSocket.create(ArSocket::UDP) &&
      myUdpSocket.findValidPort(myTcpPort, openOnIP) &&
      myUdpSocket.setNonBlock())
  {
 // not possible on udp?   myUdpSocket.setLinger(0);
    myUdpPort = ArSocket::netToHostOrder(myUdpSocket.inPort());
    myUdpReceiver.setSocket(&myUdpSocket);
    if (openOnIP != NULL)
      ArLog::log(ArLog::Normal, 
		 "%sStarted on tcp port %d and udp port %d on ip %s.", 
		 myLogPrefix.c_str(), myTcpPort, myUdpPort, openOnIP);
    else
      ArLog::log(ArLog::Normal, "%sStarted on tcp port %d and udp port %d.", 
		 myLogPrefix.c_str(), myTcpPort, myUdpPort);
  
  }
  else
  {
    myOpened = false;
    myTcpSocket.close();
    myDataMutex.unlock();
    return false;
  }
  myOpened = true;
  myDataMutex.unlock();
  return true;
}

AREXPORT void ArServerBase::close(void)
{
  std::list<ArServerClient *>::iterator it;
  ArServerClient *client;

  myClientsMutex.lock();
  if (!myOpened)
  {
    myClientsMutex.unlock();
    return;
  }

  ArLog::log(ArLog::Normal, "%sServer shutting down.", 
	     myLogPrefix.c_str());
  myOpened = false;
  // now send off the client packets to shut down
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    client->shutdown();
    printf("Sending shutdown\n");
  }

  ArUtil::sleep(10);

  while ((it = myClients.begin()) != myClients.end())
  {
    client = (*it);
    myClients.pop_front();
    delete client;
  }
  myTcpSocket.close();
  if (!myTcpOnly)
    myUdpSocket.close();
  myClientsMutex.unlock();
  
  /// MPL adding this since it looks like its leaked
  myDataMutex.lock();
  ArUtil::deleteSetPairs(myDataMap.begin(), myDataMap.end());
  myDataMap.clear();
  myDataMutex.unlock();
}

void ArServerBase::acceptTcpSockets(void)
{
  //ArSocket socket;

  //myClientsMutex.lock();

  // loop while we have new connections to make... we need to make a
  // udp auth key for any accepted sockets and send our introduction
  // to the client
  while (myTcpSocket.accept(&myAcceptingSocket) && 
	 myAcceptingSocket.getFD() >= 0)
  {
    finishAcceptingSocket(&myAcceptingSocket, false);
  }
  //myClientsMutex.unlock();
}

AREXPORT ArServerClient *ArServerBase::makeNewServerClientFromSocket(
	ArSocket *socket, bool overrideGeneralReject)
{
  ArServerClient *serverClient = NULL;
  //myDataMutex.lock();
  serverClient = finishAcceptingSocket(socket, true, 
				       overrideGeneralReject);
  //myDataMutex.unlock();
  return serverClient;
}

AREXPORT ArServerClient *ArServerBase::finishAcceptingSocket(
	ArSocket *socket, bool skipPassword, 
	bool overrideGeneralReject)
{
  ArServerClient *client;
  bool foundAuthKey;
  std::list<ArServerClient *>::iterator it;
  bool thisKeyBad;
  long authKey;
  long introKey;
  ArNetPacket packet;
  int iterations;

  ArLog::log(ArLog::Normal, "%sNew connection from %s", 
	     myLogPrefix.c_str(), socket->getIPString());

  myClientsMutex.lock();
  // okay, now loop until we find a key or until we've gone 10000
  // times (huge error but not worth locking up on)... basically
  // we're making sure that none of the current clients already have
  // this key... they should never have it if random is working
  // right... but...
  for (foundAuthKey = false, authKey = ArMath::random(), iterations = 0; 
       !foundAuthKey && iterations < 10000;
       iterations++, authKey = ArMath::random())
  {
    // see if any of the current clients have this authKey
    for (thisKeyBad = false, it = myClients.begin(); 
	 !thisKeyBad && it != myClients.end();
	 ++it)
    {
      if (authKey == (*it)->getAuthKey())
	thisKeyBad = true;
    }
    // if the key wasn't found above it means its clean, so we found
    // a good one and are done
    if (!thisKeyBad)
      foundAuthKey = true;
  }
  myClientsMutex.unlock();
  if (!foundAuthKey)
    authKey = 0;
  // now we pick an introKey which introduces the server to the client
  // this one isn't critical that it be unique though, so we just
  // set it straight out
  introKey = ArMath::random();
  std::string passwordKey;
  int numInPasswordKey = ArMath::random() % 100;
  int i;
  passwordKey = "";
  if (!skipPassword)
  {
    for (i = 0; i < numInPasswordKey; i++)
      passwordKey += '0' + ArMath::random() % ('z' - '0');
  }
  int reject;
  const char *rejectString;

  char tooManyClientsBuf[1024];
    
  // if we have too many clients, reject this because of that...
  if (myMaxClientsAllowed > 0 && myNumClients + 1 > myMaxClientsAllowed)
  {
    sprintf(tooManyClientsBuf, "Server rejecting connection since it would exceed the maximum number of allowed connections (which is %d)",
	    myMaxClientsAllowed);
    reject = 5;
    rejectString = tooManyClientsBuf;

    myTooManyClientsCBList.invoke(socket->getIPString());
  }
  // if we're overriding the blanket reject it's probably because we
  // are rejecting direct connections and this connection is from the
  // client switch....
  else if (overrideGeneralReject)
  {
    reject = 0;
    rejectString = "";
  }
  else
  {
    reject = myRejecting;
    rejectString = myRejectingString.c_str();
  }
  std::string serverClientName = myServerName + "Client_" + socket->getIPString();
  ArLog::log(myVerboseLogLevel, "%sHanding off connection to %s to %s",
	     myLogPrefix.c_str(), socket->getIPString(), 
	     serverClientName.c_str());
  client = new ArServerClient(socket, myUdpPort, authKey, introKey,
			      &mySendUdpCB, &myDataMap, 
			      passwordKey.c_str(), myServerKey.c_str(),
			      myUserInfo, reject, rejectString,
			      myDebugLogging, serverClientName.c_str(),
			      myLogPasswordFailureVerbosely,
			      myAllowSlowPackets, myAllowIdlePackets,
			      myEnforceProtocolVersion.c_str(),
			      myEnforceType);
  //client->setUdpAddress(socket->sockAddrIn());
  // put the client onto our list of clients...
  //myClients.push_front(client);
  myAddListMutex.lock();
  myAddList.push_front(client);
  myAddListMutex.unlock();
  return client;
}

AREXPORT int ArServerBase::getNumClients(void)
{
  return myNumClients;
}

AREXPORT void ArServerBase::run(void)
{
  runInThisThread();
}

AREXPORT void ArServerBase::runAsync(void)
{
  create();
}

AREXPORT void *ArServerBase::runThread(void *arg)
{
  threadStarted();
  while (myRunning)
  {
    loopOnce();
    ArUtil::sleep(myLoopMSecs);
  }
  close();
  threadFinished();
  return NULL;
}


/**
   This will broadcast this packet to any client that wants this
   data.... this no longer excludes things because that doesn't work
   well with the central server, so use
   broadcastPacketTcpWithExclusion for that if you really want it...
   
   @param packet the packet to send
   @param name the type of data to send
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcp(ArNetPacket *packet, 
					       const char *name)
{
  return broadcastPacketTcpWithExclusion(packet, name, NULL);
}


/**
   This will broadcast this packet to any client that wants this
   data.... 
   
   @param packet the packet to send
   @param name the type of data to send
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpToMatching(
	ArNetPacket *packet, const char *name, 
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  return broadcastPacketTcpWithExclusion(packet, name, NULL, true, identifier, 
					 matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this data.
   
   @param packet the packet to send
   @param name the type of data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpWithExclusion(
	ArNetPacket *packet, const char *name, ArServerClient *excludeClient,
	bool match, ArServerClientIdentifier identifier, 
	bool matchConnectionID)
{
  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  unsigned int command = 0;

//  ArLog::log(ArLog::Normal,
//             "ArServerBase::broadcastPacketTcpWithExclusion() called");
  
  myDataMutex.lock();  
//  ArLog::log(ArLog::Normal,
//             "ArServerBase::broadcastPacketTcpWithExclusion() data lock obtained");
  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
      command = (*it).second->getCommand();
  }  
  if (command == 0)
  {
    ArLog::log(myVerboseLogLevel, 
	       "ArServerBase::broadcastPacket: no command by name of \"%s\"", 
	       name);
    myDataMutex.unlock();  
    return false;
  }
  myDataMutex.unlock();  
  return broadcastPacketTcpByCommandWithExclusion(
	  packet, command, excludeClient, match, identifier, 
	  matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this
   data.... this no longer excludes things because that doesn't work
   well with the central server, so use
   broadcastPacketTcpByCommandWithExclusion for that...
   
   @param packet the packet to send
   @param command the command number of the data to send

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpByCommand(
	ArNetPacket *packet, unsigned int command)
{
  return broadcastPacketTcpByCommandWithExclusion(packet, command, NULL);
}

/**
   This will broadcast this packet to any client that wants this data.
   
   @param packet the packet to send
   @param command the command number of the data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketTcpByCommandWithExclusion(
	ArNetPacket *packet, unsigned int command, 
	ArServerClient *excludeClient, bool match, 
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  std::list<ArServerClient *>::iterator lit;
  ArServerClient *serverClient;
  ArNetPacket emptyPacket;
  
//  ArLog::log(ArLog::Normal,
//             "ArServerBase::broadcastPacketTcpByCommandWithExclusion() called");

  myClientsMutex.lock();  
//  ArLog::log(ArLog::Normal,
//             "ArServerBase::broadcastPacketTcpByCommandWithExclusion() client lock obtained");
  emptyPacket.empty();

  if (!myOpened)
  {
    ArLog::log(ArLog::Verbose, "ArServerBase::broadcastPacket: server not open to send packet.");
    myClientsMutex.unlock();  
    return false;
  }

  if (packet == NULL)
    packet = &emptyPacket;

  packet->setCommand(command);
  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
  {
    serverClient = (*lit);
    if (excludeClient != NULL && serverClient == excludeClient)
      continue;
    if (match && 
	!serverClient->getIdentifier().matches(identifier, matchConnectionID))
      continue;
    serverClient->broadcastPacketTcp(packet);
  }

  myClientsMutex.unlock();  
  return true;
}


/**
   This will broadcast this packet to any client that wants this
   data.... this no longer excludes things because that doesn't work
   well with the central server, so use
   broadcastPacketUdpWithExclusion for that if you really want to

   @param packet the packet to send
   @param name the type of data to send

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdp(ArNetPacket *packet, 
					       const char *name)
{
  return broadcastPacketUdpWithExclusion(packet, name, NULL);
}

/**
   This will broadcast this packet to any client that wants this
   data.... 

   @param packet the packet to send
   @param name the type of data to send
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdpToMatching(
	ArNetPacket *packet, const char *name,
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  return broadcastPacketUdpWithExclusion(packet, name, NULL, true, 
					 identifier, matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this data.

   @param packet the packet to send
   @param name the type of data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/

AREXPORT bool ArServerBase::broadcastPacketUdpWithExclusion(
	ArNetPacket *packet, const char *name, ArServerClient *excludeClient,
	bool match, ArServerClientIdentifier identifier, 
	bool matchConnectionID)
{
  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  unsigned int command = 0;

  myDataMutex.lock();  
  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
      command = (*it).second->getCommand();
  }  
  if (command == 0)
  {
    ArLog::log(myVerboseLogLevel, 
	       "ArServerBase::broadcastPacket: no command by name of \"%s\"", 
	       name);
    myDataMutex.unlock();  
    return false;
  }
  myDataMutex.unlock();  
  return broadcastPacketUdpByCommandWithExclusion(
	  packet, command, excludeClient, match, identifier, 
	  matchConnectionID);
}

/**
   This will broadcast this packet to any client that wants this
   data.... 
   
   (To restrict what clients the command is sent to, use the
   broadcastPacketUdpByCommandWithExclusion() method.)

   @param packet the packet to send
   @param command the type of data to send

   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdpByCommand(
	ArNetPacket *packet, unsigned int command)
{
  return broadcastPacketUdpByCommandWithExclusion(packet, command, NULL);
}

/**
   This will broadcast this packet to any client that wants this data.

   @param packet the packet to send
   @param command the type of data to send
   @param excludeClient don't send data to this client (NULL (the default) just
   ignores this feature)
   @param match whether to match the identifier or not
   @param identifier the identifier to match
   @param matchConnectionID true to match the connection ID, false to
   match the other ID
   @return false if there is no data for this name
**/
AREXPORT bool ArServerBase::broadcastPacketUdpByCommandWithExclusion(
	ArNetPacket *packet, unsigned int command,
	ArServerClient *excludeClient, bool match, 
	ArServerClientIdentifier identifier, bool matchConnectionID)
{
  std::list<ArServerClient *>::iterator lit;
  ArServerClient *serverClient;
  ArNetPacket emptyPacket;

  myClientsMutex.lock();    
  emptyPacket.empty();

  if (!myOpened)
  {
    ArLog::log(ArLog::Verbose, "ArServerBase::broadcastPacket: server not open to send packet.");
    myClientsMutex.unlock();  
    return false;
  }

  if (packet == NULL)
    packet = &emptyPacket;

  packet->setCommand(command);
  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
  {
    serverClient = (*lit);
    if (excludeClient != NULL && serverClient == excludeClient)
      continue;
    if (match && 
	!serverClient->getIdentifier().matches(identifier, matchConnectionID))
      continue;
    serverClient->broadcastPacketUdp(packet);
  }
  myClientsMutex.unlock();  
  return true;
}

/**
   @param name Name is the name of the command number to be found

   @return the number of the command or 0 if no command has that name
**/

AREXPORT unsigned int ArServerBase::findCommandFromName(const char *name)
{
  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  unsigned int num = 0;
  myDataMutex.lock();    

  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
      num = (*it).second->getCommand();
  }  
  if (num == 0)
  {
    ArLog::log(myVerboseLogLevel, 
	    "ArServerBase::findCommandFromName: no command by name of \"%s\"", 
	       name);
    myDataMutex.unlock();  
    return 0;
  }
  myDataMutex.unlock();  
  return num;
}

/**
   This runs the server loop once, normally you'll probably just use
   run() or runAsync(), but if you want to drive this server from within your own
   loop just call this repeatedly, but do note it takes up no time
   at all, so you'll want to put in your own millisecond sleep so it
   doesn't monopolize the CPU.
 **/
AREXPORT void ArServerBase::loopOnce(void)
{
  std::list<ArServerClient *>::iterator it;
  std::set<ArServerClient *>::iterator setIt;
  // for speed we'd use a list of iterators and erase, but for clarity
  // this is easier and this won't happen that often
  //std::list<ArServerClient *> removeList;
  ArServerClient *client;

  myTimeChecker.start();

  myDataMutex.lock();  

  myTimeChecker.check("lock");

  if (!myOpened)
  {
    myDataMutex.unlock();
    myTimeChecker.finish();
    return;
  }
  myDataMutex.unlock();

  acceptTcpSockets();

  myTimeChecker.check("acceptTcpSockets");

  if (!myTcpOnly)
    myUdpReceiver.readData();

  myTimeChecker.check("receiveUDPData");

  if (myProcessingSlowIdleMutex.tryLock() == 0)
  {
    myClientsMutex.lock();
    myAddListMutex.lock();
    while (!myAddList.empty())
    {
      myClients.push_back(myAddList.front());
      myAddList.pop_front();
    }
    myAddListMutex.unlock();
    myClientsMutex.unlock();
    myProcessingSlowIdleMutex.unlock();
  }

  myTimeChecker.check("addedClients");


  bool needIdleProcessing = idleProcessingPending();
  if (needIdleProcessing != myLastIdleProcessingPending)
  {
    ArNetPacket sendingPacket;
    if (needIdleProcessing)
      sendingPacket.byteToBuf(1);
    else
      sendingPacket.byteToBuf(0);
    broadcastPacketTcp(&sendingPacket, "idleProcessingPending");
    myLastIdleProcessingPending = needIdleProcessing;
  }

  myTimeChecker.check("idleProcessing");

  myBackupTimeoutMutex.lock();

  myTimeChecker.check("backupTimeoutLock");

  double backupTimeout = myBackupTimeout;
  bool newBackupTimeout = myNewBackupTimeout;
  myNewBackupTimeout = false;

  myBackupTimeoutMutex.unlock();

  myTimeChecker.check("backupTimeoutUnlock");

  bool haveSlowPackets = false;
  bool haveIdlePackets = false;
  int numClients = 0;


  // need a recursive lock before we can lock here but we should be
  //okay without a lock here (and have been for ages)
  //myClientsMutex.lock(); first let the clients handle new data
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    numClients++;

    if (newBackupTimeout)
      client->setBackupTimeout(backupTimeout);      

    if (!client->tcpCallback())
    {
      client->forceDisconnect(true);
      myRemoveSetMutex.lock();
      myRemoveSet.insert(client);
      myRemoveSetMutex.unlock();
    }
    else
    {
      if (client->hasSlowPackets())
	haveSlowPackets = true;
      if (client->hasIdlePackets())
	haveIdlePackets = true;
    }
      
  }

  myTimeChecker.check("tcpCallback");

  myHaveSlowPackets = haveSlowPackets;
  myHaveIdlePackets = haveIdlePackets;

  // if we're using our own num clients then check the max and set it
  if (myUsingOwnNumClients)
  {
    // remember how many we've had at max so its easier to track down memory
    if (numClients > myMostClients)
      myMostClients = numClients;
    
    myNumClients = numClients;
  }
  else
  {
    // if not then check the max
    if (myNumClients > myMostClients)
      myMostClients = myNumClients;
  }


  myTimeChecker.check("numClientsAccounting");

  //printf("Before...\n");
  if (myProcessingSlowIdleMutex.tryLock() == 0)
  {
    //printf("Almost...\n");
    myClientsMutex.lock();
    myRemoveSetMutex.lock();
    //printf("In...\n");
    while ((setIt = myRemoveSet.begin()) != myRemoveSet.end())
    {
      client = (*setIt);
      myClients.remove(client);
      for (std::list<ArFunctor1<ArServerClient*> *>::iterator rci = myClientRemovedCallbacks.begin();
	   rci != myClientRemovedCallbacks.end();
	   rci++) {
	if (*rci) {
	  (*rci)->invoke(client);
	}
      }
      
      myRemoveSet.erase(client);
      delete client;
    }
    myRemoveSetMutex.unlock();
    myClientsMutex.unlock();
    //printf("out...\n");
    myProcessingSlowIdleMutex.unlock();
  }

  myTimeChecker.check("removeClients");

  // now let the clients send off their packets
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    client->handleRequests();
  }

  myTimeChecker.check("handleRequests");

  // need a recursive lock before we can lock here but we should be
  //okay without a lock here (and have been for ages)
  //myClientsMutex.unlock();
  myCycleCallbacksMutex.lock();

  myTimeChecker.check("cycleCallbackLock");
  // call cycle callbacks
  for(std::list<ArFunctor*>::const_iterator f = myCycleCallbacks.begin();
          f != myCycleCallbacks.end(); f++) 
  {
    if(*f) (*f)->invoke();
  }
  myCycleCallbacksMutex.unlock();

  myTimeChecker.check("cycleCallbacks");
  myTimeChecker.finish();
}

AREXPORT void ArServerBase::processPacket(ArNetPacket *packet, struct sockaddr_in *sin)
{
  std::list<ArServerClient *>::iterator it;
  unsigned char *bytes = (unsigned char *)&sin->sin_addr.s_addr;
  ArServerClient *client;
  struct sockaddr_in *clientSin;

  myClientsMutex.lock();
  // if its a udp packet then see if we have its owner
  if (packet->getCommand() == ArClientCommands::UDP_INTRODUCTION)
  {
    long authKey;
    bool matched;

    authKey = packet->bufToUByte4();
    for (matched = false, it = myClients.begin();
	 !matched && it != myClients.end(); ++it)
    {
      client = (*it);
      if (client->getAuthKey() == authKey)
      {
	packet->resetRead();
	/*
	ArLog::log(ArLog::Normal, "Got process auth from %d.%d.%d.%d %d\n", 
	       bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));	     
	*/
	client->processAuthPacket(packet, sin);
      }
    }
    myClientsMutex.unlock();
    return;
  }

  // it wasn't the introduction so handle it the normal way

  // walk through our list of clients and see if it was one of them,
  // if so return
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    clientSin = client->getUdpAddress();
    if (clientSin->sin_port == sin->sin_port &&
	clientSin->sin_addr.s_addr == sin->sin_addr.s_addr)
    {
      client->processPacket(packet, false);
      myClientsMutex.unlock();
      return;
    }
    /*
    else
    {
      bytes = (unsigned char *)&clientSin->sin_addr.s_addr;
      printf("Wanted %d.%d.%d.%d %d\n", bytes[0], 
	     bytes[1], bytes[2], bytes[3], 
	     ArSocket::netToHostOrder(clientSin->sin_port));
    }
    */
  }
  // if it wasn't one of our clients it was from somewhere bogus
  //bytes = (unsigned char *)&clientSin->sin_addr.s_addr;
  bytes = (unsigned char *)&sin->sin_addr.s_addr;
  if (myDataMap.find(packet->getCommand()) != myDataMap.end())
    ArLog::log(ArLog::Normal, 
	       "UDP Packet %s from bogus source %d.%d.%d.%d %d\n", 
	       myDataMap[packet->getCommand()]->getName(),
	       bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));
  else
    ArLog::log(ArLog::Normal, 
	       "UDP Packet unknown from bogus source %d.%d.%d.%d %d\n", 
	       bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));
  myClientsMutex.unlock();
}

AREXPORT bool ArServerBase::sendUdp(ArNetPacket *packet, struct sockaddr_in *sin)
{
  bool ret;
  // this doesn't lock since it should only be called from 
  ret = myUdpSocket.sendTo(packet->getBuf(), packet->getLength(), sin);
  return ret;
}

/**
 * @note A callback is called from the server run loop, and if this callback
 * does not return in a timely manner data reception will be delayed (blocked).
  
 
   @param name the name of the data 

   @param description the more verbose description of what the data is for

   @param functor the functor to call when this data is requested

   @param argumentDescription a description of the arguments expected

   @param returnDescription a description of what the data returns

   @param commandGroup the name of the group this command is in

   @param dataFlags Most people won't need this, its for some advanced
   server things... this is a list of data flags separated by |
   characters, the flags are listed in ArClientData docs 

   @pynote Pass the name of a function or a lambda expression for @arg functor.
   @javanote Use a subclass of ArFunctor_ServerData instead of the ArFunctor2 template @arg functor.
**/
AREXPORT bool ArServerBase::addData(
	const char *name, const char *description,
	ArFunctor2<ArServerClient *, ArNetPacket *> * functor,
	const char *argumentDescription, const char *returnDescription, 
	const char *commandGroup, const char *dataFlags)
{
  return addDataAdvanced(name, description, functor, argumentDescription, 
		  returnDescription, commandGroup, dataFlags);
}

/**
   @copydoc ArServerBase::addData() 

   @param advancedCommandNumber 0 for most people... it is the optional number
   of the command, this is a very advanced option and is made for some
   server forwarding things so basically no one should use it, 0 means
   use the auto way... anything else does it manually... the number
   needs to be the same or higher as the next number available

   @param requestChangedFunctor functor called with the lowest amount
   of time requested for this packet, called whenever the requests for
   the packet change, note that 0 and higher means thats how often it
   was asked for, -1 means something has requested the data but just
   when it is called, and -2 means that nothing wants the data
   
   @param requestOnceFunctor functor called if this was a request
   once... with the server and client just like the normal functor,
   but its only called if its a request once, note that both are
   called (as long as they aren't NULL of course)

   @sa addData()
**/

AREXPORT bool ArServerBase::addDataAdvanced(
	const char *name, const char *description,
	ArFunctor2<ArServerClient *, ArNetPacket *> * functor,
	const char *argumentDescription, const char *returnDescription, 
	const char *commandGroup, const char *dataFlags,
	unsigned int advancedCommandNumber,
	ArFunctor2<long, unsigned int> *requestChangedFunctor, 
	ArRetFunctor2<bool, ArServerClient *, ArNetPacket *> *requestOnceFunctor)
{
  ArServerData *serverData;
  std::map<unsigned int, ArServerData *>::iterator it;

  myDataMutex.lock();

  //printf("%s %s\n", name, description);
  // if we already have one we can't do this
  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), name))
    {
      ArLog::log(ArLog::Verbose, "ArServerBase::addData: already have data for name \"%s\", could not add it.", name);
      myDataMutex.unlock();
      return false;
    }
  }
  ArLog::log(ArLog::Verbose, "ArServerBase::addData: name \"%s\" mapped to number %d", name, myNextDataNumber);
  if (advancedCommandNumber != 0)
  {
    // old check from unified command numbers
    /* if (advancedCommandNumber < myNextDataNumber)
    {
      ArLog::log(ArLog::Normal, "ArServerBase::addData: Advanced command number given for %s but the number is too low, it wasn't added", name);
      myDataMutex.unlock();
      return false;
    }
    */
    if (myDataMap.find(advancedCommandNumber) != myDataMap.end())
    {
      ArLog::log(ArLog::Normal, "ArServerBase::addData: Advanced command number given for %s is already used, it wasn't added", name);
      myDataMutex.unlock();
      return false;
    }
    serverData = new ArServerData(name, description, advancedCommandNumber,
				  functor, argumentDescription, 
				  returnDescription, commandGroup, dataFlags,
				  &myGetFrequencyCB, requestChangedFunctor, 
				  requestOnceFunctor);
    if (myAdditionalDataFlags.size() > 0)
      serverData->addDataFlags(myAdditionalDataFlags.c_str());
    myDataMap[advancedCommandNumber] = serverData;
    myDataMutex.unlock();
    return true;

  }
  else
  {
    serverData = new ArServerData(name, description, myNextDataNumber,
				  functor, argumentDescription, 
				  returnDescription, commandGroup, dataFlags,
				  &myGetFrequencyCB, requestChangedFunctor, 
				  requestOnceFunctor);
    if (myAdditionalDataFlags.size() > 0)
      serverData->addDataFlags(myAdditionalDataFlags.c_str());
    myDataMap[myNextDataNumber] = serverData;
    myNextDataNumber++;
    myDataMutex.unlock();
    return true;
  }
}


/** Cycle callbacks are called every cycle, in no particular order.
 * @param functor Functor to add.
 */
AREXPORT void ArServerBase::addCycleCallback(ArFunctor* functor)
{
  myCycleCallbacksMutex.lock();
  myCycleCallbacks.push_back(functor);
  myCycleCallbacksMutex.unlock();
}

/** Cycle callbacks are called every cycle, in no particular order.
 * @param functor Functor to remove. If it's not currently in the cycle callback
 * list, nothing is done.
 */
AREXPORT void ArServerBase::remCycleCallback(ArFunctor* functor)
{
  myCycleCallbacksMutex.lock();
  myCycleCallbacks.remove(functor);
  myCycleCallbacksMutex.unlock();
}

/**
 *  @javanote Use ArFunctor_ServerClient instead of ArFunctor1<ArServerClient*>
 *  for @a functor.
 */
AREXPORT void ArServerBase::addClientRemovedCallback(ArFunctor1<ArServerClient *> *functor)
{
  myDataMutex.lock();
  myClientRemovedCallbacks.push_back(functor);
  myDataMutex.unlock();
}

/**
 *  @javanote Use ArFunctor_ServerClient instead of ArFunctor1<ArServerClient*>
 *  for @a functor.
 */
AREXPORT void ArServerBase::remClientRemovedCallback(ArFunctor1<ArServerClient *> *functor)
{
  myDataMutex.lock();
  myClientRemovedCallbacks.remove(functor);
  myDataMutex.unlock();
}


AREXPORT bool ArServerBase::loadUserInfo(const char *fileName,
					 const char *baseDirectory)
{
  if (myUserInfo != NULL)
  {
    delete myUserInfo;
    myUserInfo = NULL;
  }
  ArServerUserInfo *userInfo = new ArServerUserInfo;
  userInfo->setBaseDirectory(baseDirectory);
  if (!userInfo->readFile(fileName))
  {
    ArLog::log(ArLog::Terse, "%sloadUserInfo: Could not load user info for %s", myLogPrefix.c_str());
    delete userInfo;
    return false;
  }
  if (!userInfo->doNotUse())
  {
    ArLog::log(ArLog::Normal, "%sLoaded user information",
	       myLogPrefix.c_str());
    myDataMutex.lock();
    myUserInfo = userInfo;
    myDataMutex.unlock();
  }
  else
  {
    ArLog::log(ArLog::Normal, 
	       "%sLoaded user information but not using it",
	       myLogPrefix.c_str());
    delete userInfo;
  }
  return true;
}

AREXPORT void ArServerBase::logUserInfo(void)
{
  myDataMutex.lock();
  if (myUserInfo == NULL)
    ArLog::log(ArLog::Terse, 
	       "%sNo user name or password needed to connect",
	       myLogPrefix.c_str());
  else
    myUserInfo->logUsers();
  myDataMutex.unlock();
}

/**
   Logs the different groups of commands.  It logs the command names
   first along with the list of commands in that group.  Then it
   outputs a list of groups.  Useful for building the user/pass file.
**/
AREXPORT void ArServerBase::logCommandGroups(void)
{
  logCommandGroupsToFile(NULL);
}

/**
   For a description of what this outputs see logCommandGroups
**/
AREXPORT void ArServerBase::logCommandGroupsToFile(const char *fileName)
{
  std::map<unsigned int, ArServerData *>::iterator dit;

  std::multimap<std::string, std::string> groups;
  std::multimap<std::string, std::string>::iterator git;

  myDataMutex.lock();
  for (dit = myDataMap.begin(); dit != myDataMap.end(); dit++)
  {
    std::string group;
    std::string command;
    command = (*dit).second->getName();
    if ((*dit).second->getCommandGroup() == NULL)
      group = "";
    else
      group = (*dit).second->getCommandGroup();
    
    groups.insert(std::pair<std::string, std::string>(group, command));
  }

  FILE *file = NULL;
  if (fileName != NULL)
  {
    file = ArUtil::fopen(fileName, "w");
  }

  char descLine[10000];
  std::string line;
  std::string lastGroup;
  bool first = true;
  bool firstGroup = true;
  std::map<std::string, std::string>::iterator dIt; 
  std::string listOfGroups = "Groups";

  for (git = groups.begin(); git != groups.end(); git++)
  {
    if (ArUtil::strcasecmp((*git).first, lastGroup) != 0 || firstGroup)
    {
      if (!firstGroup)
      {
	if (file != NULL)
	  fprintf(file, "%s", line.c_str());
	else
	  ArLog::log(ArLog::Terse, "%s", line.c_str());
      }
      first = true;
      firstGroup = false;
      lastGroup = (*git).first;
      listOfGroups += " ";
      if (lastGroup.size() == 0)
	listOfGroups += "None";
      else
	listOfGroups += lastGroup;
    }
    if (first)
    {
      line = "CommandGroup ";
      if ((*git).first.size() == 0)
      {
	line += "None";
      }
      else
      {
	// output the groups description if therei s one
	if ((dIt = myGroupDescription.find((*git).first)) != 
	    myGroupDescription.end())
	  snprintf(descLine, sizeof(descLine), 
		   "CommandGroup %s has description %s", 
		   (*git).first.c_str(), (*dIt).second.c_str());
	else
	  snprintf(descLine, sizeof(descLine), 
		   "CommandGroup %s has no description", 
		   (*git).first.c_str());
	if (file != NULL)
	  fprintf(file, "%s\n", descLine);
	else
	  ArLog::log(ArLog::Terse, "%s", descLine);

	line += (*git).first.c_str();
      }
      line += " is";


      }
    line += " ";
    line += (*git).second.c_str();
    first = false;
  }
  if (!firstGroup && !first)
  {
    if (file != NULL)
      fprintf(file, "%s\n", line.c_str());
    else
      ArLog::log(ArLog::Terse, "%s", line.c_str());
  }
  if (file != NULL)
    fprintf(file, "%s\n", listOfGroups.c_str());
  else
    ArLog::log(ArLog::Terse, "%s", listOfGroups.c_str());
  myDataMutex.unlock();
}


///     
///=========================================================================///
///   Function: setGroupDescription                                         ///
///   Created:  17-Nov-2005                                                 ///
///   Purpose:  To permit the user to update the Command Group Descriptions.///
///   Notes:                                                                ///
///                                                                         ///
///   Revision History:                                                     ///
///      WHEN       WHO         WHAT and/or WHY                             ///
///    17-Nov-2005   NJ    Created this function.                           ///
///                                                                         ///
///=========================================================================///
///
AREXPORT void ArServerBase::setGroupDescription(const char *cmdGrpName, const char *cmdGrpDesc)
{
  myDataMutex.lock();
  myGroupDescription.insert(std::pair<std::string, std::string>(cmdGrpName, cmdGrpDesc));
  myDataMutex.unlock();
}





//   Created:  17-Nov-2005                                                 //
//   Purpose:  
//   Notes:                                                                //
//             Assumption: Command Group Name Map exists already.          //
//                                                                         //
//   Revision History:                                                     //
//      WHEN       WHO         WHAT and/or WHY                             //
//   17-Nov-2005   NJ    Created this function.                            //

AREXPORT void ArServerBase::logGroupDescriptions(void)
{
  // -- This wrapper insures that NULL is the only input passed to the
  //    logCommandGroupNamesToFile routine.   
  //    This is used if the log is desired over a filename.  
  logGroupDescriptionsToFile(NULL);	// NULL is the only value passed.
}

AREXPORT void ArServerBase::logGroupDescriptionsToFile(const char *fileName)
{ 
  myDataMutex.lock();
  std::map<std::string, std::string>::iterator d_itr;	// establish map iterator		
  char line[1024];
  FILE *file = NULL;
  if (fileName != NULL)
    file = ArUtil::fopen(fileName, "w");			// open file if exists

  // -- Assemble output strings and send to file or log
  for (d_itr = myGroupDescription.begin(); d_itr != myGroupDescription.end(); d_itr++)
  {
    snprintf(line, sizeof(line), "%-29s %s", (*d_itr).first.c_str(), (*d_itr).second.c_str());
    
    if (file != NULL)
      fprintf(file, "%s\n", line);		// send string to file
    else
      ArLog::log(ArLog::Terse, "%s", line);	// send string to log
  }
  if (fileName != NULL)			// task complete notification
    fclose (file);
  myDataMutex.unlock();
}	



/**
   Text string that is the key required when using user and password
   information... this is NOT used if there is no user and password
   information.
**/ 
AREXPORT void ArServerBase::setServerKey(const char *serverKey)
{
  myDataMutex.lock();
  // if this is setting it to empty and its already empty don't print
  // a message
  if ((serverKey == NULL || serverKey[0] == '\0') && myServerKey.size() > 0)
    ArLog::log(ArLog::Normal, "Clearing server key");
  if (serverKey != NULL && serverKey[0] != '\0')
    ArLog::log(ArLog::Normal, "Setting new server key");
  if (serverKey != NULL)
    myServerKey = serverKey;
  else
    myServerKey = "";
  myDataMutex.unlock();
}

AREXPORT void ArServerBase::rejectSinceUsingCentralServer(
	const char *centralServerIPString)
{
  myDataMutex.lock();
  myRejecting = 2;
  myRejectingString = centralServerIPString;
  myDataMutex.unlock();
}



AREXPORT void ArServerBase::enforceProtocolVersion(const char *protocolVersion)
{
  myDataMutex.lock();
  if (protocolVersion != NULL)
    myEnforceProtocolVersion = protocolVersion;
  else
    myEnforceProtocolVersion = "";
  myDataMutex.unlock();
  ArLog::log(ArLog::Normal, "%sNew enforceProtocolVersionSet", myLogPrefix.c_str());
}

AREXPORT void ArServerBase::enforceType(ArServerCommands::Type type)
{
  myDataMutex.lock();
  myEnforceType = type;
  myDataMutex.unlock();
  ArLog::log(ArLog::Normal, "%sNew enforce type: %s", 
	     myLogPrefix.c_str(), ArServerCommands::toString(type));
	     
}

/**
   Sets up the backup timeout, if there are packets to send to clients
   and they haven't been sent for longer than this then the connection
   is closed.  Less than 0 means this won't happen.  If this is
   positive but less than 5 seconds then 5 seconds is used.
**/
AREXPORT void ArServerBase::setBackupTimeout(double timeoutInMins)
{
  myBackupTimeoutMutex.lock();
  myBackupTimeout = timeoutInMins;
  myNewBackupTimeout = true;
  myBackupTimeoutMutex.unlock();

  /* Taking this out, since it can cause deadlocks in some central
   * server circumstances... and doing it next time through is good enough

  std::list<ArServerClient *>::iterator it;
//  ArLog::log(ArLog::Normal, "Setting server base backup time to %g\n", myBackupTimeout);
  myClientsMutex.lock();
  for (it = myClients.begin(); it != myClients.end(); it++)
    (*it)->setBackupTimeout(myBackupTimeout);
  myClientsMutex.unlock();
  */
}

AREXPORT void ArServerBase::logTracking(bool terse)
{
  std::list<ArServerClient *>::iterator lit;

  myClientsMutex.lock();  

  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
    (*lit)->logTracking(terse);

  ArLog::log(ArLog::Terse, "");
  
  if (!terse)
  {
    ArLog::log(ArLog::Terse, "%-85s %7ld udp rcvs %10ld udp B",
	       "Low Level UDP Received (all conns)", myUdpSocket.getRecvs(), 
	       myUdpSocket.getBytesRecvd());
    ArLog::log(ArLog::Terse, "%-85s %7ld udp snds %10ld udp B",
	       "Low Level UDP Sent (all conns)", myUdpSocket.getSends(), 
	       myUdpSocket.getBytesSent());

    ArLog::log(ArLog::Terse, "");
  }
  myClientsMutex.unlock();  
}

AREXPORT void ArServerBase::resetTracking(void)
{
  std::list<ArServerClient *>::iterator lit;

  myClientsMutex.lock();  

  for (lit = myClients.begin(); lit != myClients.end(); ++lit)
    (*lit)->resetTracking();
  myClientsMutex.unlock();  
  myDataMutex.lock();
  myUdpSocket.resetTracking();
  myDataMutex.unlock();
}

AREXPORT const ArServerUserInfo* ArServerBase::getUserInfo(void) const
{
  return myUserInfo;
}

AREXPORT void ArServerBase::setUserInfo(const ArServerUserInfo *userInfo)
{
  myDataMutex.lock();
  myUserInfo = userInfo;
  myDataMutex.unlock();
}

/** 
    @param command the command name as a string

    @param frequency the frequency to get the data, > 0 ==
    milliseconds, -1 == on broadcast, -2 == never...  This function
    won't set the value to anything slower than has already been
    requested
*/
AREXPORT bool ArServerBase::internalSetDefaultFrequency(
	const char *command, int frequency)
{
 

  // first find our number so each client doesn't have to
  std::map<unsigned int, ArServerData *>::iterator it;  
  ArServerData *data = NULL;
  unsigned int commandNumber = 0;

  myClientsMutex.lock();
  myDataMutex.lock();    

  for (it = myDataMap.begin(); it != myDataMap.end(); it++)
  {
    if (!strcmp((*it).second->getName(), command))
    {
      data = (*it).second;
      commandNumber = data->getCommand();
    }
  }  

  if (commandNumber == 0)
  {
    myDataMutex.unlock();
    myClientsMutex.unlock();
    ArLog::log(ArLog::Normal, 
	       "%s::setDefaultFrequency: No command of name %s", 
	       myServerName.c_str(), command);
    return false;
  }

  int freq = -2;
  if (myDefaultFrequency.find(commandNumber) != myDefaultFrequency.end())
    freq = myDefaultFrequency[commandNumber];

  // if the freq is an interval and so is this but this
  // is a smaller interval
  if (frequency >= 0 && (freq < 0 || (freq >= 0 && frequency < freq)))
    freq = frequency;
  // if this just wants the data when pushed but freq is still
  // at never then set it to when pushed
  else if (frequency == -1 && freq == -2)
    freq = -1;

  myDefaultFrequency[commandNumber] = freq;

  data->callRequestChangedFunctor();

  ArLog::log(ArLog::Normal, "%s: Set default frequency for %s (%u) to %d",
	     myServerName.c_str(), command, commandNumber, freq);
  
  myDataMutex.unlock();
  myClientsMutex.unlock();
  return true;
}


/**
   @param command the command number, you can use findCommandFromName
   
   @param internalCall whether its an internal call or not (whether to
   lock or not)
   
   @return returns lowest amount of time requested for this packet,
   note that 0 and higher means thats how often it was asked for, -1
   means nothing requested the data at an interval but wants it when
   its been pushed, and -2 means that nothing wants the data
**/

AREXPORT long ArServerBase::getFrequency(unsigned int command,
					 bool internalCall)
{
  std::list<ArServerClient *>::iterator it;
  long ret = -2;
  long clientFreq;

  if (!internalCall)
    myClientsMutex.lock();

  // if we have a default frequency start with that
  if (myDefaultFrequency.find(command) != myDefaultFrequency.end())
    ret = myDefaultFrequency[command];
    
  for (it = myClients.begin(); it != myClients.end(); it++)
  {
    clientFreq = (*it)->getFrequency(command);
    // if the ret is an interval and so is this client but this client
    // is a smalelr interval
    if (clientFreq >= 0 && (ret < 0 || (ret >= 0 && clientFreq < ret)))
      ret = clientFreq;
    // if this client just wants the data when pushed but ret is still
    // at never then set it to when pushed
    else if (clientFreq == -1 && ret == -2)
      ret = -1;
  }
  
  if (!internalCall)
    myClientsMutex.unlock();

  return ret;
}

/**
   Additional data flags to be added to addData calls (these are added
   in with what is there).  You can do multiple flags by separating
   them with a | character.
 **/
AREXPORT void ArServerBase::setAdditionalDataFlags(
	const char *additionalDataFlags)
{
  myDataMutex.lock();
  if (additionalDataFlags == NULL || additionalDataFlags[0] == '\0')
    myAdditionalDataFlags = "";
  else  
    myAdditionalDataFlags = additionalDataFlags;
  myDataMutex.unlock();
}


AREXPORT bool ArServerBase::dataHasFlag(const char *name, 
					const char *dataFlag)
{
  unsigned int command;
  if ((command = findCommandFromName(name)) == 0)
  {
    ArLog::log(ArLog::Verbose, 
	   "ArServerBase::dataHasFlag: %s is not data that is on the server", 
	       name);
    return false;
  }

  return dataHasFlagByCommand(findCommandFromName(name), dataFlag);
}

AREXPORT bool ArServerBase::dataHasFlagByCommand(unsigned int command, 
						 const char *dataFlag)
{
  std::map<unsigned int, ArServerData *>::iterator dIt;
  bool ret;

  myDataMutex.lock();
  if ((dIt = myDataMap.find(command)) == myDataMap.end())
    ret = false;
  else
    ret = (*dIt).second->hasDataFlag(dataFlag);
  myDataMutex.unlock();
  
  return ret;
}

AREXPORT unsigned int ArServerBase::getTcpPort(void)
{
  return myTcpPort;
}

AREXPORT unsigned int ArServerBase::getUdpPort(void)
{
  return myUdpPort;  
}

AREXPORT const char *ArServerBase::getOpenOnIP(void)
{
  if (myOpenOnIP.empty())
    return NULL;
  else
    return myOpenOnIP.c_str();
}

bool ArServerBase::processFile(void)
{
  setBackupTimeout(myBackupTimeout);
  return true;
}

AREXPORT void ArServerBase::identGetConnectionID(ArServerClient *client, 
						 ArNetPacket *packet)
{
  ArNetPacket sending;
  ArServerClientIdentifier identifier = client->getIdentifier();
  
  // if there is an id then just send that ConnectionID
  if (identifier.getConnectionID() != 0)
  {
    sending.uByte4ToBuf(identifier.getConnectionID());
    client->sendPacketTcp(&sending);
  }
  // otherwise we need to find an ConnectionID for it (don't lock while doing
  // this since the client bases can only be active from within the
  // server base)
  
  // we have the id number, but make sure it isn't already taken
  // (maybe we'll wrap a 4 byte number, but its unlikely, but better
  // safe than sorry)
  bool duplicate;
  std::list<ArServerClient *>::iterator it;
  bool hitZero;

  // this does both the stepping to find a good ID, the for also winds
  // up incrementing it after we find one, which I don't like since
  // its a side effect, but oh well
  for (duplicate = true, hitZero = false;
       duplicate;
       myConnectionNumber++)
  {
    if (myConnectionNumber == 0)
    {
      // if we hit zero once already and are back there, just use 0
      // for an id
      if (hitZero)
      {
	ArLog::log(ArLog::Terse, "ConnectionID hit zero again, giving up");
	break;
      }
      ArLog::log(ArLog::Terse, "ConnectionID lookup wrapped");
      // otherwise set that we hit zero and go to 1
      hitZero = true;
      myConnectionNumber++;
    }

    // see if we have any duplicates
    for (it = myClients.begin(); 
	 it != myClients.end();
	 it++)
    {
      if (myConnectionNumber != (*it)->getIdentifier().getConnectionID())
      {
	duplicate = false;
	break;
      }
    }
  }
  
  identifier.setConnectionID(myConnectionNumber);
  client->setIdentifier(identifier);

  sending.uByte4ToBuf(identifier.getConnectionID());
  client->sendPacketTcp(&sending);
}

AREXPORT void ArServerBase::identSetConnectionID(ArServerClient *client, ArNetPacket *packet)
{
  ArServerClientIdentifier identifier = client->getIdentifier();
  identifier.setConnectionID(packet->bufToUByte4());
  client->setIdentifier(identifier);
}

AREXPORT void ArServerBase::identSetSelfIdentifier(ArServerClient *client, 
				     ArNetPacket *packet)
{
  char buf[32000];
  packet->bufToStr(buf, sizeof(buf));

  ArServerClientIdentifier identifier = client->getIdentifier();
  identifier.setSelfIdentifier(buf);
  client->setIdentifier(identifier);
}

AREXPORT void ArServerBase::identSetHereGoal(ArServerClient *client, ArNetPacket *packet)
{
  char buf[32000];
  packet->bufToStr(buf, sizeof(buf));

  ArServerClientIdentifier identifier = client->getIdentifier();
  identifier.setHereGoal(buf);
  client->setIdentifier(identifier);
}
  
AREXPORT void ArServerBase::handleStartRequestTransaction(ArServerClient *client, ArNetPacket *packet)
{
  client->startRequestTransaction();
  
  ArLog::log(ArLog::Verbose,
             "ArServerBase::handleStartRequestTransaction() from %s complete",
             client->getIPString());

} // end method handleStartRequestTransaction


AREXPORT void ArServerBase::handleEndRequestTransaction(ArServerClient *client, ArNetPacket *packet)
{
  if (client->endRequestTransaction()) {
    ArLog::log(ArLog::Verbose,
               "ArServerBase::handleEndRequestTransaction() from %s complete",
               client->getIPString());

  }
  else {
    ArLog::log(ArLog::Verbose,
               "ArServerBase::handleEndRequestTransaction() error from %s" ,
               client->getIPString());

  }
    

} // end method handleEndRequestTransaction


AREXPORT int ArServerBase::getRequestTransactionCount()
{
  myClientsMutex.lock();
  int c = 0;
  for (std::list<ArServerClient *>::iterator iter = myClients.begin();
       iter != myClients.end();
       iter++) {
    ArServerClient *client = *iter;
    if (client != NULL) {
      c += client->getRequestTransactionCount();
    }

  } // end for each client
  
  myClientsMutex.unlock(); 

  return c;

} // end method getRequestTransactionCount


AREXPORT void ArServerBase::closeConnectionID(ArTypes::UByte4 idNum)
{
  if (idNum == 0)
  {
    ArLog::log(ArLog::Verbose, "%s: Cannot close connection to id 0", 
	       myLogPrefix.c_str());
    return;
  }

  std::list<ArServerClient *>::iterator it;
  // for speed we'd use a list of iterators and erase, but for clarity
  // this is easier and this won't happen that often
  //std::list<ArServerClient *> removeList;
  ArServerClient *client;

  myClientsMutex.lock(); 
  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    if (client->getIdentifier().getConnectionID() == idNum)
    {
      client->forceDisconnect(true);
      myRemoveSetMutex.lock();
      myRemoveSet.insert(client);
      myRemoveSetMutex.unlock();
    }
  }
  /*
  while ((it = removeList.begin()) != removeList.end())
  {
    client = (*it);
    if (myDebugLogging)
      ArLog::log(ArLog::Normal,
		 "%sClosing (by connection id) connection to %s", 
		 myLogPrefix.c_str(), client->getIPString());
    myClients.remove(client);
    for (std::list<ArFunctor1<ArServerClient*> *>::iterator rci = myClientRemovedCallbacks.begin();
         rci != myClientRemovedCallbacks.end();
	 rci++) {
      if (*rci) {
        (*rci)->invoke(client);
      }
    }

    delete client;
    removeList.pop_front();
  }
  */
  myClientsMutex.unlock(); 
}

AREXPORT void ArServerBase::logConnections(const char *prefix)
{
  std::list<ArServerClient *>::iterator it;
  ArServerClient *client;

  myClientsMutex.lock(); 

  ArLog::log(ArLog::Normal, "%sConnections for %s (%d now, %d max)", prefix, 
	     myServerName.c_str(), myClients.size(), myMostClients);

  for (it = myClients.begin(); it != myClients.end(); ++it)
  {
    client = (*it);
    ArLog::log(ArLog::Normal, "%s\t%s", prefix, client->getIPString());
  }

  myClientsMutex.unlock(); 
}
/// Sets debug logging 
AREXPORT void ArServerBase::setDebugLogging(bool debugLogging)
{
  myDebugLogging = debugLogging; 
  if (myDebugLogging)
    myVerboseLogLevel = ArLog::Normal;
  else
    myVerboseLogLevel = ArLog::Verbose;
}

AREXPORT bool ArServerBase::getDebugLogging(void)
{
  return myDebugLogging; 
}

AREXPORT int ArServerBase::getMostClients(void)
{ 
  return myMostClients; 
}

void ArServerBase::slowIdleCallback(void)
{
  if (!myOpened)
  {
    ArUtil::sleep(myLoopMSecs);
    return;
  }

  bool isRequestTransactionInProgress = (getRequestTransactionCount() > 0);
  
  std::list<ArServerClient *>::iterator it;
  ArServerClient *client = NULL;

  std::list<ArFunctor *>::iterator cIt;
  ArFunctor *functor = NULL;

  if (myAllowSlowPackets)
  {
    myProcessingSlowIdleMutex.lock();
    //printf("SLOW...\n");
    for (it = myClients.begin(); it != myClients.end(); ++it)
    {
      client = (*it);
      if (!client->slowPacketCallback())
      {
	client->forceDisconnect(true);
	myRemoveSetMutex.lock();
	myRemoveSet.insert(client);
	myRemoveSetMutex.unlock();
      }
    }
    //printf("done slow...\n");
    myProcessingSlowIdleMutex.unlock();
  }

  int activityTime = ArServerMode::getActiveModeActivityTimeSecSince();

  // This check is solely for the purpose of logging a message if idle 
  // processing is being delayed because a request transaction is still 
  // in progress. (Could/should be improved so that it does not spam.)
  // 
  // The if statement is similar to the following one -- except with
  // isRequestTransactionInProgress inverted.
  if (myAllowIdlePackets && 
      (activityTime < 0 || activityTime > 1) &&
      isRequestTransactionInProgress) {
    ArLog::log(ArLog::Verbose,
               "ArServerBase::slowIdleCallback() waiting until request transaction complete");
  } // end if 


  // Don't process idle packets until all "request transactions" are complete.
  // These are multi-step handshake sequences that should not be interrupted.
  //
  if (myAllowIdlePackets && 
      (activityTime < 0 || activityTime > 1) &&
      !isRequestTransactionInProgress)
  {
    /*
    if (idleProcessingPending() && ArServerMode::getActiveMode() != NULL)
      ArLog::log(myVerboseLogLevel, "Idle processing pending, idle %ld msecs", 
		 ArServerMode::getActiveMode()->getActivityTime().mSecSince());
    */

    //printf("IDLE...\n");

    myProcessingSlowIdleMutex.lock();

    myIdleCallbacksMutex.lock();
    if (myHaveIdleCallbacks)
    {
      while ((cIt = myIdleCallbacks.begin()) != myIdleCallbacks.end())
      {
	functor = (*cIt);
	myIdleCallbacks.pop_front();
	myIdleCallbacksMutex.unlock();
	if (functor->getName() != NULL && functor->getName()[0] != '\0')
	  ArLog::log(myVerboseLogLevel, "Calling idle callback functor %s", 
		     functor->getName());
	else
	  ArLog::log(myVerboseLogLevel, 
		     "Calling anonymous idle callback functor");
	functor->invoke();
	myIdleCallbacksMutex.lock();
      }
      myHaveIdleCallbacks = false;
    }
    myIdleCallbacksMutex.unlock();

    for (it = myClients.begin(); it != myClients.end(); ++it)
    {
      client = (*it);
      if (!client->idlePacketCallback())
      {
	client->forceDisconnect(true);
	myRemoveSetMutex.lock();
	myRemoveSet.insert(client);
	myRemoveSetMutex.unlock();
      }
    }
    //printf("done idle...\n");
    myProcessingSlowIdleMutex.unlock();
  }
  ArUtil::sleep(myLoopMSecs);
}

/**
 * Call this method to delay the current thread after a packet has been sent.  
 * This allows the packet to be transmitted before any subsequent action.
 *
 * If the call is made in the main server thread, then loop once will be called
 * until the specified time has passed.  If the call is made in the idle thread,
 * then ArUtil::sleep is used.  (Note that if ArUtil::sleep() were called in the
 * main thread, then the packet would not actually be transmitted until after
 * the sleep exits.) 
**/
void ArServerBase::sleepAfterSend(int msecDelay)
{
  if (msecDelay < 0) {
    ArLog::log(ArLog::Normal,
               "ArServerBase::sleepAfterSend() unexpected msecDelay = %i",
               msecDelay);
    return;
  }

  if (ArThread::self() == mySlowIdleThread) {
    IFDEBUG(ArLog::log(ArLog::Normal,
                       "ArServerBase::sleepAfterSend() about to sleep in idle thread"));

    ArUtil::sleep(msecDelay);

    IFDEBUG(ArLog::log(ArLog::Normal,
                       "ArServerBase::sleepAfterSend() finished sleep in idle thread"));
  }
  else { // not idle thread

    IFDEBUG(ArLog::log(ArLog::Normal,
                       "ArServerBase::sleepAfterSend() about to loop in main thread"));

    ArTime startTime;
    while (startTime.mSecSince() < msecDelay) {
      loopOnce();
      ArUtil::sleep(1);
    } // end while keep delaying
    
    IFDEBUG(ArLog::log(ArLog::Normal,
                       "ArServerBase::sleepAfterSend() finished loop in main thread"));

  }

} // end method sleepAfterSend


AREXPORT bool ArServerBase::hasSlowPackets(void)
{
  return myHaveSlowPackets; 
}

AREXPORT bool ArServerBase::hasIdlePackets(void)
{
  return myHaveIdlePackets; 
}

AREXPORT bool ArServerBase::hasIdleCallbacks(void)
{
  return myHaveIdleCallbacks;
}

AREXPORT bool ArServerBase::idleProcessingPending(void)
{
  return myHaveIdleCallbacks || myHaveIdlePackets;
}

AREXPORT void ArServerBase::netIdleProcessingPending(ArServerClient *client,
						     ArNetPacket *packet)
{
  ArNetPacket sendingPacket;
  
  if (idleProcessingPending())
    sendingPacket.byteToBuf(1);
  else
    sendingPacket.byteToBuf(0);

  client->sendPacketTcp(&sendingPacket);
}

AREXPORT bool ArServerBase::allowingIdlePackets(void)
{
  return myAllowIdlePackets;
}

/**
   Note that if idle packets are not allowed then idle callbacks will
   not work.
   
   @return true if the callback was added, false if not
**/
AREXPORT bool ArServerBase::addIdleSingleShotCallback(ArFunctor *functor)
{
  if (!myAllowIdlePackets)
    return false;

  myIdleCallbacksMutex.lock();
  myIdleCallbacks.push_back(functor);
  myHaveIdleCallbacks = true;
  if (functor->getName() != NULL && functor->getName()[0] != '\0')
    ArLog::log(myVerboseLogLevel, 
	       "Adding idle singleshot callback functor %s", 
	       functor->getName());
  else
    ArLog::log(myVerboseLogLevel, 
	       "Adding anonymous idle singleshot callback functor");
  myIdleCallbacksMutex.unlock();
  return true;
}

AREXPORT void ArServerBase::internalSetNumClients(int numClients)
{
  myUsingOwnNumClients = false;
  myNumClients = numClients;
}


AREXPORT void ArServerBase::internalLockup(void)
{
  ArLog::log(ArLog::Normal, "%s locking up by request", myServerName.c_str());
  myDataMutex.lock();  
}

ArServerBase::SlowIdleThread::SlowIdleThread(ArServerBase *serverBase)
{
  setThreadName("ArServerBase::SlowIdleThread");
  myServerBase = serverBase;
  runAsync();
}

ArServerBase::SlowIdleThread::~SlowIdleThread()
{
  
}

void * ArServerBase::SlowIdleThread::runThread(void *arg)
{
  threadStarted();

  while (getRunning())
  {
    myServerBase->slowIdleCallback();
  }
  
  threadFinished();
  return NULL;
}
