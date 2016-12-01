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
#include "ArServerClient.h"
#include "ArServerCommands.h"
#include "ArClientCommands.h"
#include "ArServerData.h"
#include "ArServerUserInfo.h"

/**
   In addition to the normal contructorness this contructor starts the
   connection between the server and the client going

   @param tcpSocket the socket that has been established between the
   client and the server

   @param udpPort the udp port the server is using

   @param authKey this authKey must be presented by the client to the
   server on the server's UDP port for the client to be recognized by
   the server

   @param introKey this key must be presented by the server to the
   client so that the client will recognize the server
   
   @param sendUdpCallback the callback used to send Udp packets (since
   the udp packets need to be sent from the server's udp port and the
   serverclient here doesn't have one)
   
   @param dataMap the map between command numbers and the command data
   
   @param passwordKey the key (just plain text thats used in an md5
   along with the serverKey and the password over on the client side)
   that was generated for the passwords on this exact connection

   @param serverKey the key (just plain text thats used in an md5
   along with the passwordKey and the password over on the client
   side) that should be used for any connections to this server

   @param debugLogging If true, log some debug messages

   @param logPasswordFailureVerbosely If true, log password errors at
   ArLog::Verbose
   level, if false, log them at ArLog::Normal level.

   @param userInfo the pointer to the item that holds our usernames
   and passwords
   
   @param rejecting 

   @param rejectingString string to reject with
 **/

AREXPORT ArServerClient::ArServerClient(
	ArSocket *tcpSocket, unsigned int udpPort, long authKey,
	long introKey, 	ArRetFunctor2<bool, ArNetPacket *, 
	struct sockaddr_in *> *sendUdpCallback,
	std::map<unsigned int, ArServerData *> *dataMap,
	const char *passwordKey, const char *serverKey,
	const ArServerUserInfo *userInfo, int rejecting, 
	const char *rejectingString, bool debugLogging,
	const char *serverClientName, bool logPasswordFailureVerbosely,
	bool allowSlowPackets, bool allowIdlePackets,
	const char *enforceProtocolVersion,
	ArServerCommands::Type enforceType) :

    myProcessPacketCB(this, &ArServerClient::processPacket, NULL, true),
 
    myRequestTransactionCount(0),
    myRequestTransactionMutex()

{
  ArNetPacket packet;

  // set our default to no command
  pushCommand(0);

  myAuthKey = authKey;
  myIntroKey = introKey;
  myTcpSocket.transfer(tcpSocket);
  myTcpSocket.setCloseCallback(tcpSocket->getCloseCallback());
  myTcpSocket.setNonBlock();
  myTcpReceiver.setSocket(&myTcpSocket);
  myTcpReceiver.setProcessPacketCB(&myProcessPacketCB);
  myTcpSender.setSocket(&myTcpSocket);

  mySendUdpCB = sendUdpCallback;
  myDataMap = dataMap;
  if (udpPort == 0)
    myTcpOnly = true;
  else
    myTcpOnly = false;
  mySentTcpOnly = myTcpOnly;

  myUserInfo = userInfo;
  myPasswordKey = passwordKey;
  myServerKey = serverKey;
  myRejecting = rejecting;
  if (rejectingString != NULL)
    myRejectingString = rejectingString;

  myDebugLogging = debugLogging;
  if (myDebugLogging)
    myVerboseLogLevel = ArLog::Normal;
  else
    myVerboseLogLevel = ArLog::Verbose;

  myTcpSender.setDebugLogging(myDebugLogging);

  myLogPrefix = serverClientName;
  myLogPrefix += ": ";
  myTcpSender.setLoggingPrefix(myLogPrefix.c_str());
  myTcpReceiver.setLoggingPrefix(myLogPrefix.c_str());

  myLogPasswordFailureVerbosely = logPasswordFailureVerbosely;

  if (enforceProtocolVersion != NULL)
    myEnforceProtocolVersion = enforceProtocolVersion;
  else
    myEnforceProtocolVersion = "";
  myEnforceType = enforceType;

  mySlowPacketsMutex.setLogName("ArServerClient::mySlowPacketsMutex");
  myIdlePacketsMutex.setLogName("ArServerClient::myIdlePacketsMutex");

  myAllowSlowPackets = allowSlowPackets;
  myAllowIdlePackets = allowIdlePackets;
    
  myRequestTransactionMutex.setLogName("ArServerClient::myRequestTransactionMutex");

  setIdentifier(ArServerClientIdentifier());
  internalSwitchState(STATE_SENT_INTRO);

  packet.empty();
  packet.setCommand(ArServerCommands::INTRODUCTION);
  packet.strToBuf("alpha");
  packet.uByte2ToBuf(udpPort);
  packet.uByte4ToBuf(myAuthKey);
  packet.uByte4ToBuf(myIntroKey);
  packet.strToBuf(myPasswordKey.c_str());
  packet.strToBuf(myEnforceProtocolVersion.c_str());
  sendPacketTcp(&packet);

  mySlowIdleThread = NULL;

  myHaveSlowPackets = false;
  myHaveIdlePackets = false;
  
  myCreationTime.setToNow();

  resetTracking();
}

AREXPORT ArServerClient::~ArServerClient()
{
  myTcpReceiver.setSocket(NULL);
  myTcpReceiver.setProcessPacketCB(NULL);
  myTcpSocket.close();
  
  /// MPL fixing a memory leak
  // new way to get the request change functor called
  std::list<ArServerClientData *>::iterator it;
  ArServerClientData *data;
  ArServerData *serverData;
  while ((it = myRequested.begin()) != myRequested.end())
  {
    data = (*it);
    serverData = (*it)->getServerData();
    myRequested.pop_front();
    delete data;
    serverData->callRequestChangedFunctor();
  }
  /* old way
  ArUtil::deleteSet(myRequested.begin(), myRequested.end());
  myRequested.clear();
  */
  ArUtil::deleteSetPairs(myTrackingSentMap.begin(), 
			 myTrackingSentMap.end());
  myTrackingSentMap.clear();

  ArUtil::deleteSetPairs(myTrackingReceivedMap.begin(), 
			 myTrackingReceivedMap.end());
  myTrackingReceivedMap.clear();

  ArUtil::deleteSet(mySlowPackets.begin(), mySlowPackets.end());
  mySlowPackets.clear();
  ArUtil::deleteSet(myIdlePackets.begin(), myIdlePackets.end());
  myIdlePackets.clear();

}

void ArServerClient::internalSwitchState(ServerState state)
{
  myState = state;
  myStateStart.setToNow();
  if (myDebugLogging)
    ArLog::log(ArLog::Normal, "%sSwitching to state %d", 
	       myLogPrefix.c_str(), (int)state);
}

AREXPORT void ArServerClient::setBackupTimeout(double timeoutInMins)
{
  myBackupTimeout = timeoutInMins;
  myTcpSender.setBackupTimeout(myBackupTimeout);
}

AREXPORT bool ArServerClient::tcpCallback(void)
{
  if (myState == STATE_REJECTED)
  {
    myTcpSender.sendData();
    ArUtil::sleep(10);
    return false;
  }

  if (myState == STATE_DISCONNECTED && (myHaveSlowPackets || 
					myHaveIdlePackets))
    return true;

  if (myState == STATE_DISCONNECTED)
    return false;

  if (myState == STATE_SENT_INTRO && myStateStart.secSince() >= 60)
  {
    ArLog::log(myVerboseLogLevel, "%sTook too long for %s to connect",
	       myLogPrefix.c_str(), getIPString());
    return false;
  }

  if (myTcpOnly && !mySentTcpOnly)
  {
    mySentTcpOnly = true;
    ArNetPacket sending;
    sending.setCommand(ArServerCommands::TCP_ONLY);
    sendPacketTcp(&sending);
  }

  if (!myTcpReceiver.readData())
  {
    ArLog::log(myVerboseLogLevel, "%sTrouble receiving tcp data from %s",
	       myLogPrefix.c_str(), getIPString());
    internalSwitchState(STATE_DISCONNECTED);
    return tcpCallback();
    //return false; 
  }
  if (!myTcpSender.sendData())
  {
    ArLog::log(myVerboseLogLevel, "%sTrouble sending tcp data to %s", 
	       myLogPrefix.c_str(), getIPString());
    internalSwitchState(STATE_DISCONNECTED);
    return tcpCallback();
    //return false;
  }

  return true;
}

AREXPORT void ArServerClient::handleRequests(void)
{
  if (myState != STATE_CONNECTED)
    return;  

  std::list<ArServerClientData *>::iterator it;
  ArServerClientData *data;  
  ArServerData *serverData;
  ArTime lastSent;

  // walk through our list
  for (it = myRequested.begin(); it != myRequested.end(); ++it)
  {
    data = (*it);
    lastSent = data->getLastSent();
    // see if this needs to be called
    if (data->getMSec() != -1 && 
	(data->getMSec() == 0 || lastSent.mSecSince() > data->getMSec()))
    {
      serverData = data->getServerData();
      // call it, then set it so we know we did
      pushCommand(serverData->getCommand());
      pushForceTcpFlag(false);
      if (serverData->getFunctor() != NULL)
	serverData->getFunctor()->invoke(this, data->getPacket());
      popCommand();
      popForceTcpFlag();
      data->setLastSentToNow();
    }
  }
}


void ArServerClient::sendListPacket(void)
{
  ArNetPacket packet;
  std::map<unsigned int, ArServerData *>::iterator it;
  unsigned int count;
  unsigned int shortLen;
  unsigned int longLen;
  ArServerData *serverData;

  // First we send a list of numbers, names and descriptions
  packet.setCommand(ArServerCommands::LIST);
  
  // number of entries (we'll overwrite it later with the right number)
  shortLen = packet.getLength();
  packet.uByte2ToBuf(0);
  // loop through the data to build the packet
  for (it = myDataMap->begin(), count = 0; it != myDataMap->end(); it++)
  {
    serverData = (*it).second;
    if (myUserInfo == NULL || 
	serverData->getCommandGroup() == NULL || 
	serverData->getCommandGroup()[0] == '\0' || 
	myGroups.count(serverData->getCommandGroup()) != 0 || 
	myGroups.count("all") != 0)
    {
      count++;
      packet.uByte2ToBuf(serverData->getCommand());
      packet.strToBuf(serverData->getName());
      packet.strToBuf(serverData->getDescription());
    }
  }
  // put the real number of entries in the right spot 
  longLen = packet.getLength();
  packet.setLength(shortLen);
  packet.uByte2ToBuf(count);
  packet.setLength(longLen);

  sendPacketTcp(&packet);

  // then we send a list of the arguments and returns... they aren't
  // combined so that the packet structure doesn't need to change
  packet.empty();
  packet.setCommand(ArServerCommands::LISTARGRET);
  
  // number of entries (we'll overwrite it later with the right number)
  shortLen = packet.getLength();
  packet.uByte2ToBuf(0);
  
  // loop through the data to build the packet
  for (it = myDataMap->begin(), count = 0; it != myDataMap->end(); it++)
  {
    serverData = (*it).second;
    if (myUserInfo == NULL || 
	serverData->getCommandGroup() == NULL || 
	serverData->getCommandGroup()[0] == '\0' || 
	myGroups.count(serverData->getCommandGroup()) != 0 || 
	myGroups.count("all") != 0)
    {      
      // this if is a quick and dirty way to fix to prevent overruns,
      // before we overrun it sends the packet with what we have, then
      // it sends another packet for the rest...  we need a real
      // solution later MPL TODO 2013_12_13 but this should be fine
      // (since the client doesn't generally care as much about if the
      // args/rets are there)
      if (packet.getLength() + 2 + ArNetPacket::FOOTER_LENGTH + 
	  strlen(serverData->getArgumentDescription()) + 
	  strlen(serverData->getReturnDescription()) > ArNetPacket::MAX_LENGTH)
      {
	// put the real number of entries in the right spot
	longLen = packet.getLength();
	packet.setLength(shortLen);
	packet.uByte2ToBuf(count);
	packet.setLength(longLen);
	sendPacketTcp(&packet);
	
	/// reset it all back down to 0
	packet.empty();
	packet.setCommand(ArServerCommands::LISTARGRET);
	
	// number of entries (we'll overwrite it later with the right number)
	shortLen = packet.getLength();
	packet.uByte2ToBuf(0);
	count = 0;
      }
      count++;
      packet.uByte2ToBuf(serverData->getCommand());
      packet.strToBuf(serverData->getArgumentDescription());
      packet.strToBuf(serverData->getReturnDescription());
    }
  }
  // put the real number of entries in the right spot
  longLen = packet.getLength();
  packet.setLength(shortLen);
  packet.uByte2ToBuf(count);
  packet.setLength(longLen);
  sendPacketTcp(&packet);


  // then we send a list of command groups... they aren't
  // combined so that the packet structure doesn't need to change
  packet.empty();
  packet.setCommand(ArServerCommands::LISTGROUPANDFLAGS);
  
  // number of entries (we'll overwrite it later with the right number)
  shortLen = packet.getLength();
  packet.uByte2ToBuf(0);
  
  // loop through the data to build the packet
  for (it = myDataMap->begin(), count = 0; it != myDataMap->end(); it++)
  {
    serverData = (*it).second;
    if (myUserInfo == NULL || 
	serverData->getCommandGroup() == NULL || 
	serverData->getCommandGroup()[0] == '\0' || 
	myGroups.count(serverData->getCommandGroup()) != 0 || 
	myGroups.count("all") != 0)
    {
      count++;
      packet.uByte2ToBuf(serverData->getCommand());
      packet.strToBuf(serverData->getCommandGroup());
      packet.strToBuf(serverData->getDataFlagsString());
    }

  }
  // put the real number of entries in the right spot
  longLen = packet.getLength();
  packet.setLength(shortLen);
  packet.uByte2ToBuf(count);
  packet.setLength(longLen);
  sendPacketTcp(&packet);
}

AREXPORT void ArServerClient::processPacket(ArNetPacket *packet, bool tcp)
{
  std::string str;
  struct sockaddr_in sin;
  unsigned int clientUdpPort;
  ArNetPacket retPacket;

  //printf("Command number %d\n", packet->getCommand());
  // if we're in intro mode and received back the intro
  if (myState == STATE_SENT_INTRO && 
      packet->getCommand() == ArClientCommands::INTRODUCTION)
  {
    char user[512];
    unsigned char password[16];
    char enforceProtocolVersion[256];
    clientUdpPort = packet->bufToUByte2();
    packet->bufToStr(user, sizeof(user));
    packet->bufToData((char *)password, 16);
    packet->bufToStr(enforceProtocolVersion, sizeof(enforceProtocolVersion));
    ArServerCommands::Type clientType = (ArServerCommands::Type)packet->bufToByte();

    if (myRejecting != 0)
    {
      retPacket.empty();
      retPacket.setCommand(ArServerCommands::REJECTED);
      retPacket.byte2ToBuf(myRejecting);
      retPacket.strToBuf(myRejectingString.c_str());
      sendPacketTcp(&retPacket);
      ArLog::log(ArLog::Normal, 
		 "%sRejected connection from %s (%d, %s)",
		 myLogPrefix.c_str(), getIPString(), myRejecting,
		 myRejectingString.c_str());
      internalSwitchState(STATE_REJECTED);
      return;
    }

    if (!myEnforceProtocolVersion.empty() && 
	strcmp(enforceProtocolVersion, myEnforceProtocolVersion.c_str()) != 0)
    {
      retPacket.empty();
      retPacket.setCommand(ArServerCommands::REJECTED);
      retPacket.byte2ToBuf(4);
      retPacket.strToBuf("Server rejecting client connection since protocol version does not match");
      sendPacketTcp(&retPacket);
      ArLog::log(ArLog::Normal, "%sServer rejecting client connection since protocol version does not match", myLogPrefix.c_str());
      internalSwitchState(STATE_REJECTED);
    }

    if (myEnforceType != ArServerCommands::TYPE_UNSPECIFIED &&
	(myEnforceType != clientType ||
	 myEnforceType == ArServerCommands::TYPE_NONE))
    {
      retPacket.empty();
      retPacket.setCommand(ArServerCommands::REJECTED);
      retPacket.byte2ToBuf(6);
      retPacket.strToBuf("Server rejecting client connection since type (real or sim) does not match (but is specified)");
      sendPacketTcp(&retPacket);
      ArLog::log(ArLog::Normal, "%sServer rejecting client connection since type (real or sim) does not match (but is specified)", myLogPrefix.c_str());
      internalSwitchState(STATE_REJECTED);
    }

    // if user info is NULL we're not checking passwords
    if (myUserInfo != NULL && 
	!myUserInfo->matchUserPassword(user, password, myPasswordKey.c_str(),
				       myServerKey.c_str(),
				       myLogPasswordFailureVerbosely))
    {
      retPacket.empty();
      retPacket.setCommand(ArServerCommands::REJECTED);
      retPacket.byte2ToBuf(1);
      retPacket.strToBuf("");
      sendPacketTcp(&retPacket);
      ArLog::log(ArLog::Normal, "%sRejected user '%s' or password from %s",
		 myLogPrefix.c_str(), user, getIPString());
      internalSwitchState(STATE_REJECTED);
      return;
    }
    if (myUserInfo != NULL)
      myGroups = myUserInfo->getUsersGroups(user);
    else
      myGroups.clear();
    sin.sin_family = AF_INET;
    sin.sin_addr = *myTcpSocket.inAddr();
    sin.sin_port = ArSocket::hostToNetOrder(clientUdpPort);
    if (myUserInfo != NULL)
      ArLog::log(ArLog::Normal, 
		 "%sClient connected from %s with user %s", 
		 myLogPrefix.c_str(), getIPString(), user);
    else
      ArLog::log(ArLog::Normal, 
		 "%sClient connected from %s", myLogPrefix.c_str(), 
		 getIPString());

    setUdpAddress(&sin);
    // send that we've connected
    retPacket.empty();
    retPacket.setCommand(ArServerCommands::CONNECTED);
    sendPacketTcp(&retPacket);
    // note that we're connected
    internalSwitchState(STATE_CONNECTED);

    // send them the list
    sendListPacket();
    // send the udp introduction if we're using udp
    if (!myTcpOnly)
    {
      retPacket.empty();
      retPacket.setCommand(ArServerCommands::UDP_INTRODUCTION);
      retPacket.byte4ToBuf(myIntroKey);
      sendPacketUdp(&retPacket);
    }
  }
  // if we aren't in intro mode and got an intro somethings wrong
  else if (packet->getCommand() == ArClientCommands::INTRODUCTION)
  {
    ArLog::log(ArLog::Terse, 
	       "%sReceived introduction when not in intro mode",
	       myLogPrefix.c_str());
    return;
  }
  // if we got this over tcp then they only want tcp
  else if (packet->getCommand() == ArClientCommands::UDP_INTRODUCTION)
  {
    if (!myTcpOnly)
    {
      ArLog::log(ArLog::Normal, "%sGot UDP introduction over tcp, assuming client only wants tcp data.", myLogPrefix.c_str());
      myTcpOnly = true;
    }
    return;
  }
  // if we're connected and got a udp confirmation
  else if ((myState == STATE_CONNECTED || 
	    myState == STATE_SENT_INTRO) && 
	   packet->getCommand() == ArClientCommands::UDP_CONFIRMATION)
  {
    myUdpConfirmedTo = true;
    ArLog::log(myVerboseLogLevel, 
	       "%s: udp connection to client confirmed.", myLogPrefix.c_str());
    return;
  }
  // if we're not connected (or close) and got a confirmation
  else if (packet->getCommand() == ArClientCommands::UDP_CONFIRMATION)
  {
    ArLog::log(ArLog::Normal, 
	       "%sReceived udp confirmation when not connected.",
	       myLogPrefix.c_str());
    return;
  }
  else if (packet->getCommand() == ArClientCommands::TCP_ONLY)
  {
    ArLog::log(myVerboseLogLevel, "%sClient only wants tcp data.", myLogPrefix.c_str());
    myTcpOnly = true;
    return;
  }
  else if (packet->getCommand() == ArClientCommands::SHUTDOWN)
  {
    ArLog::log(ArLog::Normal, "%sClient from %s has disconnected.",
	       myLogPrefix.c_str(), getIPString());
    internalSwitchState(STATE_DISCONNECTED);
    return;
  }
  // if we're connected its a request, then set all that up
  else if (myState == STATE_CONNECTED && 
	   packet->getCommand() == ArClientCommands::REQUEST)
  {
    std::list<ArServerClientData *>::iterator it;
    ArServerClientData *data;
    ArServerData *serverData;
    unsigned int command;
    long mSec;
    // see which one they requested
    command = packet->bufToUByte2();
    mSec = packet->bufToByte4();

    // first we see if we already have this one
    for (it = myRequested.begin(); it != myRequested.end(); ++it)
    {
      data = (*it);
      serverData = data->getServerData();
      if (serverData->getCommand() == command)
      {
	trackPacketReceived(packet, command);
	data->setMSec(mSec);
	data->setPacket(packet);
	data->getPacket()->setCommand(command);
	serverData->callRequestChangedFunctor();
	ArLog::log(myVerboseLogLevel, 
	   "%sRevised request for command %s to %d mSec with new argument", 
		   myLogPrefix.c_str(), 
		   findCommandName(serverData->getCommand()), mSec);
	return;
      }
    }
    // we didn't have it, so make a new one
    std::map<unsigned int, ArServerData *>::iterator sdit;
    if ((sdit = myDataMap->find(command)) == myDataMap->end())
    {
      ArLog::log(ArLog::Terse, 
  	      "%sGot request for command %d which doesn't exist", 
		 myLogPrefix.c_str(), command);
      return;
    }
    serverData = (*sdit).second;
    if (serverData == NULL) 
    {
      ArLog::log(ArLog::Terse, 
		 "%sprocessPackets request handler has NULL serverData", 
		 myLogPrefix.c_str());
    }
    if (myUserInfo != NULL && 
	serverData->getCommandGroup() != NULL &&
	serverData->getCommandGroup()[0] != '\0' &&
	myGroups.count(serverData->getCommandGroup()) == 0 &&
	myGroups.count("all") == 0)
    {
      ArLog::log(ArLog::Normal, 
		 "%s%s tried to request command '%s' but it doesn't have access to that command", 
		 myLogPrefix.c_str(), getIPString(), 
		 serverData->getName());
      return;
    }
    trackPacketReceived(packet, command);
    data = new ArServerClientData(serverData, mSec, packet);
    data->getPacket()->setCommand(command);
    ArLog::log(myVerboseLogLevel, 
	       "%sadded request for command %s every %d mSec", 
	       myLogPrefix.c_str(), serverData->getName(), mSec);
    if (mSec == 0)
      ArLog::log(ArLog::Normal, "%sClient from %s requested command %s every at 0 msec", myLogPrefix.c_str(), 
		 getIPString(), serverData->getName());
    myRequested.push_front(data);
    serverData->callRequestChangedFunctor();
    pushCommand(command);
    pushForceTcpFlag(false);
    if (serverData->getFunctor() != NULL)
      serverData->getFunctor()->invoke(this, data->getPacket());
    popCommand();
    popForceTcpFlag();
    return;
  }
  // if we got a request when we're not connected
  else if (packet->getCommand() == ArClientCommands::REQUEST)
  {
    ArLog::log(ArLog::Normal, 
	       "Got a request while not connected.",
	       myLogPrefix.c_str());
    return;
  }
  // if we're connected its a requestStop, then set all that up
  else if (myState == STATE_CONNECTED && 
	   packet->getCommand() == ArClientCommands::REQUESTSTOP)
  {
    std::list<ArServerClientData *>::iterator it;
    ArServerClientData *data;
    ArServerData *serverData;
    unsigned int command;
    // see which one they requested
    command = packet->bufToUByte2();

    // first we see if we have this one
    for (it = myRequested.begin(); it != myRequested.end(); ++it)
    {
      data = (*it);
      serverData = data->getServerData();
      // we have a match, so set the new params then return
      if (data->getServerData()->getCommand() == command)
      {
	trackPacketReceived(packet, command);
	myRequested.erase(it);
	ArLog::log(myVerboseLogLevel, "%sStopped request for command %s", 
		   myLogPrefix.c_str(), 
		   findCommandName(serverData->getCommand()));
	delete data;
	serverData->callRequestChangedFunctor();
	return;
      }
    }
    // if we don't have it... that means that it wasn't here
    
    // find out what to call it
    std::map<unsigned int, ArServerData *>::iterator sdit;
    if ((sdit = myDataMap->find(command)) == myDataMap->end())
    {
      ArLog::log(ArLog::Terse, 
		 "%sGot a requeststop for command %d which doesn't exist", 
		 myLogPrefix.c_str(), command);
      return;
    }
    trackPacketReceived(packet, command);
    serverData = (*sdit).second;
    if (serverData == NULL) 
      ArLog::log(ArLog::Terse, 
		 "%srequeststop handler has NULL serverData on back command %d", 
		 myLogPrefix.c_str(), command);
    else
      ArLog::log(ArLog::Normal, "%s: Got a stop request for command %s that isn't requested", myLogPrefix.c_str(), serverData->getName());
    return;
  }
  // if we got a requestStop when we're not connected
  else if (packet->getCommand() == ArClientCommands::REQUESTSTOP)
  {
    ArLog::log(ArLog::Normal, 
	       "%sGot a requeststop while not connected.", 
	       myLogPrefix.c_str());
    return;
  }
  // if we're connected and its a command to execute just once
  else if (myState == STATE_CONNECTED)
  {
    unsigned int command;
    std::map<unsigned int, ArServerData *>::iterator it;
    ArServerData *serverData;

    command = packet->getCommand();
    if ((it = myDataMap->find(command)) == myDataMap->end())
    {
      ArLog::log(ArLog::Terse, 
  	      "%sArServerClient got request for command %d which doesn't exist", 
		 myLogPrefix.c_str(), command);
      return;
    }
    serverData = (*it).second;
    if (myUserInfo != NULL && 
	serverData->getCommandGroup() != NULL &&
	serverData->getCommandGroup()[0] != '\0' &&
	myGroups.count(serverData->getCommandGroup()) == 0 &&
	myGroups.count("all") == 0)
    {
      ArLog::log(ArLog::Normal, 
		 "%s%s tried to request command '%s' once but it doesn't have access to that command", myLogPrefix.c_str(),
		 getIPString(), 
		 serverData->getName());
      return;
    }
    trackPacketReceived(packet, command);

    // copy it out and return if its an idle packet
    if (myAllowIdlePackets && serverData->isIdlePacket())
    {
      myHaveIdlePackets = true;
      if (command <= 255)
	ArLog::log(myVerboseLogLevel, "%sStoring idle command %d", 
		   myLogPrefix.c_str(), command);
      else
	ArLog::log(myVerboseLogLevel, "%sStoring idle command %s", 
		   myLogPrefix.c_str(), serverData->getName());
      myIdlePacketsMutex.lock();
      ArNetPacket *idlePacket = new ArNetPacket(packet->getLength() + 5);
      idlePacket->duplicatePacket(packet);
      myIdlePackets.push_back(idlePacket);
      myIdlePacketsMutex.unlock();
      return;
    }
    // If its a slow or an idle packet (and we're not allowing the
    // idle behavior) and we allow slow packets then copy it
    else if (myAllowSlowPackets && (serverData->isSlowPacket() || 
				    serverData->isIdlePacket()))
    {
      myHaveSlowPackets = true;
      if (command <= 255)
	ArLog::log(myVerboseLogLevel, "%sStoring slow command %d", 
		   myLogPrefix.c_str(), command);
      else
	ArLog::log(myVerboseLogLevel, "%sStoring slow command %s", 
		   myLogPrefix.c_str(), serverData->getName());
      mySlowPacketsMutex.lock();
      ArNetPacket *slowPacket = new ArNetPacket(packet->getLength() + 5);
      slowPacket->duplicatePacket(packet);
      mySlowPackets.push_back(slowPacket);
      mySlowPacketsMutex.unlock();
      return;
    }


    if (command <= 255)
      ArLog::log(myVerboseLogLevel, "%sGot command %s", 
		 myLogPrefix.c_str(), serverData->getName());
    else
      ArLog::log(ArLog::Verbose, "%sGot command %s", 
		 myLogPrefix.c_str(), serverData->getName());
    pushCommand(command);
    pushForceTcpFlag(tcp);
    if (serverData->getFunctor() != NULL)
      serverData->getFunctor()->invoke(this, packet);
    if (serverData->getRequestOnceFunctor() != NULL)
      serverData->getRequestOnceFunctor()->invokeR(this, packet);
    popCommand();
    popForceTcpFlag();
    return;
  }
  else
  {
    ArLog::log(ArLog::Terse, 
	       "%sRogue packet command %s in state %d", myLogPrefix.c_str(),
	       findCommandName(packet->getCommand()), myState);
  }
}

AREXPORT void ArServerClient::forceDisconnect(bool quiet)
{
  if (!quiet)
    ArLog::log(ArLog::Normal, "Client from %s has been forcibly disconnected.",
	       getIPString());
  internalSwitchState(STATE_DISCONNECTED);
}

AREXPORT void ArServerClient::processAuthPacket(ArNetPacket *packet, 
				       struct sockaddr_in *sin)
{
  ArNetPacket retPacket;
  long authKey;
  // check the auth key again, just in case

  authKey = packet->bufToUByte4();
  if (authKey != myAuthKey)
  {
    ArLog::log(ArLog::Terse, "ArServerClient: authKey given does not match actual authKey, horrible error.");
    return;
  }
  
  if (mySin.sin_port != sin->sin_port)
  {
    ArLog::log(myVerboseLogLevel, 
	       "Client says it is using port %u but is using port %u\n", 
	       ArSocket::netToHostOrder(mySin.sin_port),
	       ArSocket::netToHostOrder(sin->sin_port));
  }
  myUdpConfirmedFrom = true;
  mySin.sin_port = sin->sin_port;
  mySin.sin_addr = sin->sin_addr;
  ArLog::log(myVerboseLogLevel, "Client connected to server on udp port %u", 
	     ArSocket::netToHostOrder(mySin.sin_port));
  // TODO put some state info here to note that its fully udp connected
  retPacket.empty();
  retPacket.setCommand(ArServerCommands::UDP_INTRODUCTION);
  retPacket.byte4ToBuf(myIntroKey);
  sendPacketUdp(&retPacket);
  retPacket.empty();
  retPacket.setCommand(ArServerCommands::UDP_CONFIRMATION);
  sendPacketTcp(&retPacket);
}

AREXPORT void ArServerClient::broadcastPacketTcp(ArNetPacket *packet)
{
  std::list<ArServerClientData *>::iterator it;
  ArServerClientData *data;  
  ArServerData *serverData;
  ArTime lastSent;

  // walk through our list
  for (it = myRequested.begin(); it != myRequested.end(); ++it)
  {
    data = (*it);
    serverData = data->getServerData();
    // see if this is our data, if it is send the packet
    if (serverData->getCommand() == packet->getCommand())
    {
      sendPacketTcp(packet);
      return;
    }
  }
  // we didn't have the data to send
}

AREXPORT void ArServerClient::broadcastPacketUdp(ArNetPacket *packet)
{
  std::list<ArServerClientData *>::iterator it;
  ArServerClientData *data;  
  ArServerData *serverData;
  ArTime lastSent;
  
  // walk through our list
  for (it = myRequested.begin(); it != myRequested.end(); ++it)
  {
    data = (*it);
    serverData = data->getServerData();
    // see if this is our data, if it is send the packet
    if (serverData->getCommand() == packet->getCommand())
    {
      sendPacketUdp(packet);
      return;
    }
  }
  // we didn't have the data to send
}

AREXPORT bool ArServerClient::sendPacketTcp(ArNetPacket *packet)
{
  if (!setupPacket(packet))
  {
    if (myDebugLogging && packet->getCommand() <= 255)
      ArLog::log(ArLog::Normal, 
		 "%s sendPacket: could not set up tcp command %d",
		 myLogPrefix.c_str(), packet->getCommand());
    return false;
  }
  else
  {
    trackPacketSent(packet, true);

    if (myDebugLogging && packet->getCommand() <= 255)
      ArLog::log(ArLog::Normal, "%sSending tcp command %d", 
		 myLogPrefix.c_str(), packet->getCommand());

    myTcpSender.sendPacket(packet, myLogPrefix.c_str());
    return true;
  }
}

AREXPORT bool ArServerClient::sendPacketUdp(ArNetPacket *packet)
{
  if (myTcpOnly || getForceTcpFlag())
    return sendPacketTcp(packet);
  
  if (!setupPacket(packet))
  {
    if (myDebugLogging && packet->getCommand() <= 255)
      ArLog::log(ArLog::Normal, 
		 "%s sendPacket: could not set up udp command %d",
		 myLogPrefix.c_str(), packet->getCommand());
    return false;
  }
  else if (mySendUdpCB != NULL)
  {
    trackPacketSent(packet, false);
    if (myDebugLogging && packet->getCommand() <= 255)
      ArLog::log(ArLog::Normal, "%sSending udp command %d", 
		 myLogPrefix.c_str(), packet->getCommand());
    return mySendUdpCB->invokeR(packet, &mySin);
  }
  else
  {
    if (myDebugLogging && packet->getCommand() <= 255)
      ArLog::log(ArLog::Normal, "%sCould not send udp command %d", 
		 myLogPrefix.c_str(), packet->getCommand());
    return false;
  }
}

AREXPORT  bool ArServerClient::setupPacket(ArNetPacket *packet)
{
  if (packet->getCommand() == 0)
  {
   if (getCommand() != 0)
   {
     packet->setCommand(getCommand());
   }
   else
   {
     ArLog::log(ArLog::Terse, "ArServerClient::sendPacket: packet to send has no command and we have no command in the stack");
     return false;
   }
  }

  if (myState == STATE_DISCONNECTED)
  {
    if (myDebugLogging && packet->getCommand() <= 255)
      ArLog::log(myVerboseLogLevel, "%s sendPacket: command %s trying to be sent while disconnected", myLogPrefix.c_str(), findCommandName(packet->getCommand()));
    return false;
  }

  if (packet->getLength() > ArNetPacket::MAX_LENGTH)
  {
    if (findCommandName(packet->getCommand()) != NULL)
      ArLog::log(ArLog::Terse, 
		 "% send packet: Packet for command %s packet is bad at %d", 
		 myLogPrefix.c_str(), findCommandName(packet->getCommand()), 
		 packet->getLength());
    else
      ArLog::log(ArLog::Terse, 
	 "% send packet: Packet for command number %d packet is bad at %d", 
		 myLogPrefix.c_str(), packet->getCommand(), 
		 packet->getLength());
    return false;
  }

  packet->finalizePacket();
  return true;
} 


AREXPORT void ArServerClient::setUdpAddress(struct sockaddr_in *sin)
{
  mySin = *sin;
}

AREXPORT struct sockaddr_in *ArServerClient::getUdpAddress(void)
{
  return &mySin;
}

AREXPORT long ArServerClient::getAuthKey(void)
{
  return myAuthKey;
}

unsigned int ArServerClient::getCommand(void)
{
  if (mySlowIdleThread == NULL || ArThread::self() != mySlowIdleThread)
  {
    if (!myCommandStack.empty())
      return myCommandStack.front();
    else
      return 0;
  }
  else
  {
    if (!mySlowIdleCommandStack.empty())
      return mySlowIdleCommandStack.front();
    else
      return 0;
  }
}

void ArServerClient::pushCommand(unsigned int command)
{
  myCommandStack.push_front(command);
}

void ArServerClient::popCommand(void)
{
  if (myCommandStack.size() == 0)
  {
    ArLog::log(ArLog::Terse, 
      "ArServerClient::popCommand: popCommand tried to be called when stack empty.");
  }
  myCommandStack.pop_front();
}

void ArServerClient::pushSlowIdleCommand(unsigned int command)
{
  mySlowIdleCommandStack.push_front(command);
}

void ArServerClient::popSlowIdleCommand(void)
{
  if (mySlowIdleCommandStack.size() == 0)
  {
    ArLog::log(ArLog::Terse, 
      "ArServerClient::popCommand: popSlowIdleCommand tried to be called when stack empty.");
  }
  mySlowIdleCommandStack.pop_front();
}


bool ArServerClient::getForceTcpFlag(void)
{
  if (mySlowIdleThread == NULL || ArThread::self() != mySlowIdleThread)
  {
    if (!myForceTcpStack.empty())
      return myForceTcpStack.front();
    else
      return false;

  }
  else
  {
    if (!mySlowIdleForceTcpStack.empty())
      return mySlowIdleForceTcpStack.front();
    else
      return false;
  }
}

void ArServerClient::pushForceTcpFlag(bool forceTcp)
{
  myForceTcpStack.push_front(forceTcp);
}

void ArServerClient::popForceTcpFlag(void)
{
  if (myForceTcpStack.size() == 0)
  {
    ArLog::log(ArLog::Terse, 
      "ArServerClient::popForceTcp: popForceTcpFlag tried to be called when stack empty.");
  }
  myForceTcpStack.pop_front();
}


void ArServerClient::pushSlowIdleForceTcpFlag(bool forceTcp)
{
  mySlowIdleForceTcpStack.push_front(forceTcp);
}

void ArServerClient::popSlowIdleForceTcpFlag(void)
{
  if (mySlowIdleForceTcpStack.size() == 0)
  {
    ArLog::log(ArLog::Terse, 
      "ArServerClient::popForceTcp: popSlowIdleForceTcpFlag tried to be called when stack empty.");
  }
  mySlowIdleForceTcpStack.pop_front();
}

AREXPORT void ArServerClient::shutdown(void)
{
  ArNetPacket packet;
  
  packet.setCommand(ArServerCommands::SHUTDOWN);
  sendPacketTcp(&packet);
  myTcpSender.sendData();
}

AREXPORT const char *ArServerClient::getIPString(void) const
{
  return myIPString.c_str(); 
}

AREXPORT ArServerClientIdentifier ArServerClient::getIdentifier(void) const
{
  return myIdentifier;
}

AREXPORT void ArServerClient::setIdentifier(
	ArServerClientIdentifier identifier)
{
  std::string oldIPString = myIPString;
  
  myIdentifier = identifier;
  myIPString = myTcpSocket.getRawIPString();
  if (myIdentifier.getIDString() != 0 && 
      myIdentifier.getIDString()[0] != '\0')
  {
    myIPString += " ";
    myIPString += myIdentifier.getIDString(); 
  }
  
  if (!oldIPString.empty() && oldIPString != myIPString)
    ArLog::log(ArLog::Normal, "%s%s changed to %s", myLogPrefix.c_str(),
	       oldIPString.c_str(), getIPString());
  myTcpSocket.setIPString(getIPString());
}

void ArServerClient::trackPacketSent(ArNetPacket *packet, bool tcp)
{
  if (myTrackingSentMap.find(packet->getCommand()) == myTrackingSentMap.end())
    myTrackingSentMap[packet->getCommand()] = new Tracker;

  if (tcp)
  {
    myTrackingSentMap[packet->getCommand()]->myPacketsTcp++;
    myTrackingSentMap[packet->getCommand()]->myBytesTcp += packet->getLength();
  }
  else
  {
    myTrackingSentMap[packet->getCommand()]->myPacketsUdp++;
    myTrackingSentMap[packet->getCommand()]->myBytesUdp += packet->getLength();
  }
}

void ArServerClient::trackPacketReceived(ArNetPacket *packet, 
					 ArTypes::UByte2 command)
{
  if (myTrackingReceivedMap.find(command) == myTrackingReceivedMap.end())
    myTrackingReceivedMap[command] = new Tracker;

  if (packet->getPacketSource() == ArNetPacket::TCP)
  {
    myTrackingReceivedMap[command]->myPacketsTcp++;
    myTrackingReceivedMap[command]->myBytesTcp += packet->getLength();
  }
  else
  {
    myTrackingReceivedMap[command]->myPacketsUdp++;
    myTrackingReceivedMap[command]->myBytesUdp += packet->getLength();
  }
}

AREXPORT void ArServerClient::logTracking(bool terse)
{
  std::map<ArTypes::UByte2, Tracker *>::iterator it;

  ArTypes::UByte2 command;
  Tracker *tracker = NULL;
  long seconds;

  seconds = myTrackingStarted.secSince();
  if (seconds == 0)
    seconds = 1;

  char name[512];

  long packetsReceivedTcp = 0;
  long bytesReceivedTcp = 0;
  long packetsReceivedUdp = 0;
  long bytesReceivedUdp = 0;

  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "Received tracking for %s (active %d seconds):", 
	     getIPString(), seconds);
  for (it = myTrackingReceivedMap.begin(); it != myTrackingReceivedMap.end(); it++)
  {
    command = (*it).first;
    tracker = (*it).second;

    packetsReceivedTcp += tracker->myPacketsTcp;
    bytesReceivedTcp += tracker->myBytesTcp;
    packetsReceivedUdp += tracker->myPacketsUdp;
    bytesReceivedUdp += tracker->myBytesUdp;

    std::map<unsigned int, ArServerData *>::iterator nameIt;
    if ((nameIt = myDataMap->find(command)) != myDataMap->end())
      snprintf(name, sizeof(name), "%s", (*nameIt).second->getName());
    // if we're command 255 or less and there's no name its probably
    // one of the server commands we don't really need to track
    else if (command <= 255)
      continue;
    // we should know what the name of everything other then the
    // server command is, but print if we don't, just in case
    else
      snprintf(name, sizeof(name), "#%d", command);
    if (terse)
    {
      ArLog::log(ArLog::Terse, 
		 "%35s %7ld pkts %10ld B %7ld B/sec", 
		 name, tracker->myPacketsTcp + tracker->myPacketsUdp, 
		 tracker->myBytesTcp + tracker->myBytesUdp,
		 ((tracker->myBytesTcp + tracker->myBytesUdp)/
		  seconds));
    }
    else
    {
      ArLog::log(ArLog::Terse, 
	 "%35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/s ", 
		 name, tracker->myPacketsTcp, tracker->myBytesTcp, 
		 tracker->myBytesTcp/seconds,
		 tracker->myPacketsUdp, tracker->myBytesUdp,
		 tracker->myBytesUdp/seconds);
    }
  }
  
  ArLog::log(ArLog::Terse, "");
  if (terse)
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld pkts %10ld B %7ld B/sec", 
	       "Total Received", packetsReceivedTcp + packetsReceivedUdp, 
	       bytesReceivedTcp + bytesReceivedUdp,
	       (bytesReceivedTcp + bytesReceivedUdp) / seconds);
  }
  else
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",  
	       "Total Received", packetsReceivedTcp, bytesReceivedTcp, 
	       bytesReceivedTcp/seconds, packetsReceivedUdp, bytesReceivedUdp, 
	       bytesReceivedUdp/seconds);
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp rcvs %10ld tcp B %7ld tcp B/S",
	       "Low level TCP Received", myTcpSocket.getRecvs(), 
	       myTcpSocket.getBytesRecvd(), 
	       myTcpSocket.getBytesRecvd()/seconds);
  }

  long packetsSentTcp = 0;
  long bytesSentTcp = 0;
  long packetsSentUdp = 0;
  long bytesSentUdp = 0;

  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "Sent tracking for %s (active %d seconds):", 
	     getIPString(), seconds);
  for (it = myTrackingSentMap.begin(); it != myTrackingSentMap.end(); it++)
  {
    command = (*it).first;
    tracker = (*it).second;

    packetsSentTcp += tracker->myPacketsTcp;
    bytesSentTcp += tracker->myBytesTcp;
    packetsSentUdp += tracker->myPacketsUdp;
    bytesSentUdp += tracker->myBytesUdp;

    std::map<unsigned int, ArServerData *>::iterator nameIt;
    if ((nameIt = myDataMap->find(command)) != myDataMap->end())
      snprintf(name, sizeof(name), "%s", (*nameIt).second->getName());
    // if we're command 255 or less and there's no name its probably
    // one of the server commands we don't really need to track
    else if (command <= 255)
      continue;
    // we should know what the name of everything other then the
    // server command is, but print if we don't, just in case
    else
      snprintf(name, sizeof(name), "#%d", command);
    if (terse)
    {
      ArLog::log(ArLog::Terse, 
		 "%35s %7ld pkts %10ld B %7ld B/sec", 
		 name, tracker->myPacketsTcp + tracker->myPacketsUdp, 
		 tracker->myBytesTcp + tracker->myBytesUdp,
		 ((tracker->myBytesTcp + tracker->myBytesUdp)/
		  seconds));
    }
    else
    {
      ArLog::log(ArLog::Terse, 
	 "%35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/s ", 
		 name, tracker->myPacketsTcp, tracker->myBytesTcp, 
		 tracker->myBytesTcp/seconds,
		 tracker->myPacketsUdp, tracker->myBytesUdp,
		 tracker->myBytesUdp/seconds);
    }
  }

  ArLog::log(ArLog::Terse, "");
  if (terse)
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld pkts %10ld B %7ld B/sec", 
	       "Total Sent", packetsSentTcp + packetsSentUdp, 
	       bytesSentTcp + bytesSentUdp,
	       (bytesSentTcp + bytesSentUdp) / seconds);    
    ArLog::log(ArLog::Terse, "");
    ArLog::log(ArLog::Terse, "%-35s %7ld pkts %10ld B %7ld B/sec", 
	       "Total Sent and Received", 
	       (packetsSentTcp + packetsSentUdp + 
		packetsReceivedTcp + packetsReceivedUdp),
	       (bytesSentTcp + bytesSentUdp + 
		bytesReceivedTcp + bytesReceivedUdp),
	       (bytesSentTcp + bytesSentUdp + 
		bytesReceivedTcp + bytesReceivedUdp) / seconds);
  }
  else
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",  
	       "Total Sent", packetsSentTcp, bytesSentTcp, 
	       bytesSentTcp / seconds,
	       packetsSentUdp, bytesSentUdp, bytesSentUdp / seconds);
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp snds %10ld tcp B %7ld tcp B/S",
	       "Low level TCP Sent", myTcpSocket.getSends(), 
	       myTcpSocket.getBytesSent(), 
	       myTcpSocket.getBytesSent() / seconds);

    ArLog::log(ArLog::Terse, "");
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",  
	       "Total Sent and Received", packetsSentTcp = packetsReceivedTcp, 
	       bytesSentTcp + bytesReceivedTcp, 
	       (bytesSentTcp + bytesReceivedTcp) / seconds,
	       packetsSentUdp + packetsReceivedUdp, 
	       bytesSentUdp + bytesReceivedUdp, 
	       (bytesSentUdp + bytesReceivedUdp) / seconds);
  }

  ArLog::log(ArLog::Terse, "");
}


AREXPORT void ArServerClient::resetTracking(void)
{
  std::map<ArTypes::UByte2, Tracker *>::iterator it;

  myTrackingStarted.setToNow();

  for (it = myTrackingSentMap.begin(); it != myTrackingSentMap.end(); it++)
    (*it).second->reset();

  for (it = myTrackingReceivedMap.begin(); 
       it != myTrackingReceivedMap.end(); 
       it++)
    (*it).second->reset();

  myTcpSocket.resetTracking();
}

AREXPORT bool ArServerClient::hasGroupAccess(const char *group)
{
  if (myUserInfo == NULL || group == NULL || group[0] == '\0' || 
      myGroups.count(group) > 0 || myGroups.count("all") > 0)
    return true;
  else
    return false;  
}

/**
   @param command the command number, you can use findCommandFromName
   
   @return returns lowest amount of time requested for this packet,
   note that 0 and higher means thats how often it was asked for, -1
   means nothing requested the data at an interval but wants it when
   its been pushed, and -2 means that nothing wants the data
**/

AREXPORT long ArServerClient::getFrequency(ArTypes::UByte2 command)
{
  std::list<ArServerClientData *>::iterator it;
  ArServerClientData *data;  
  ArServerData *serverData;
  
  // walk through our list
  for (it = myRequested.begin(); it != myRequested.end(); ++it)
  {
    data = (*it);
    serverData = data->getServerData();
    // see if this is our data, if it is send the packet
    if (serverData->getCommand() == command)
    {
      if (data->getMSec() >= 0)
	return data->getMSec();
      else
	return -1;
    }
  }
  return -2;
}
  

AREXPORT void ArServerClient::startRequestTransaction()
{
  myRequestTransactionMutex.lock();
  myRequestTransactionCount++;
  myRequestTransactionMutex.unlock();

} // end method startRequestTransaction


AREXPORT bool ArServerClient::endRequestTransaction()
{
  bool isSuccess = false;
  myRequestTransactionMutex.lock();
  if (myRequestTransactionCount > 0) {
    myRequestTransactionCount--;
    isSuccess = true;
  }
  else {
    ArLog::log(ArLog::Normal,
               "ArServerClient::endRequestTransaction() transaction not in progress");
  }
  myRequestTransactionMutex.unlock();

  return isSuccess;

} // end method endRequestTransaction


AREXPORT int ArServerClient::getRequestTransactionCount()
{
  myRequestTransactionMutex.lock();
  int c = myRequestTransactionCount;
  myRequestTransactionMutex.unlock();

  return c;

} // end method getRequestTransactionCount

/**
 *  Note that this method is not very efficient; it performs a linear search
 *  of all commands. 
**/
AREXPORT unsigned int ArServerClient::findCommandFromName(const char *commandName) const
{
  if (ArUtil::isStrEmpty(commandName)) {
    return 0;
  }

  for (std::map<unsigned int, ArServerData *>::const_iterator iter =
          myDataMap->begin();
       iter != myDataMap->end();
       iter++) {
    if (strcmp(commandName, (*iter).second->getName()) == 0) {
      return iter->first;
    }
  }
  return 0;

} // end method findCommandFromName


const char *ArServerClient::findCommandName(unsigned int command) const 
{
  std::map<unsigned int, ArServerData *>::const_iterator nameIt;
  if ((nameIt = myDataMap->find(command)) != myDataMap->end())
    return (*nameIt).second->getName();
  else
    return NULL;
}


AREXPORT bool ArServerClient::slowPacketCallback(void)
{
  ArNetPacket *slowPacket;
  unsigned int command;
  std::map<unsigned int, ArServerData *>::iterator it;
  ArServerData *serverData;

  if (mySlowIdleThread == NULL)
	mySlowIdleThread = ArThread::self();

  mySlowPacketsMutex.lock();
  while (!mySlowPackets.empty())
  {
    slowPacket = mySlowPackets.front();
    mySlowPackets.pop_front();
    mySlowPacketsMutex.unlock();

    command = slowPacket->getCommand();
    if ((it = myDataMap->find(command)) == myDataMap->end())
    {
      ArLog::log(ArLog::Terse, 
  	      "%sArServerClient got request for command %d which doesn't exist during slow... very odd", 
		 myLogPrefix.c_str(), command);
      delete slowPacket;
      return false;
    }
    serverData = (*it).second;

    ArLog::log(myVerboseLogLevel, "Processing slow command %s", 
	       serverData->getName());

    pushSlowIdleCommand(command);
    pushSlowIdleForceTcpFlag(true);

    if (serverData->getFunctor() != NULL)
      serverData->getFunctor()->invoke(this, slowPacket);
    if (serverData->getRequestOnceFunctor() != NULL)
      serverData->getRequestOnceFunctor()->invokeR(this, slowPacket);

    popSlowIdleCommand();
    popSlowIdleForceTcpFlag();

    delete slowPacket;
    mySlowPacketsMutex.lock();
  }
  mySlowPacketsMutex.unlock();  
  myHaveSlowPackets = false;
  return true;
}

AREXPORT bool ArServerClient::idlePacketCallback(void)
{
  ArNetPacket *idlePacket;
  unsigned int command;
  std::map<unsigned int, ArServerData *>::iterator it;
  ArServerData *serverData;

  if (mySlowIdleThread == NULL)
    mySlowIdleThread = ArThread::self();
  
  myIdlePacketsMutex.lock();
  while (!myIdlePackets.empty())
  {
    idlePacket = myIdlePackets.front();
    myIdlePackets.pop_front();
    myIdlePacketsMutex.unlock();

    command = idlePacket->getCommand();
    if ((it = myDataMap->find(command)) == myDataMap->end())
    {
      ArLog::log(ArLog::Terse, 
  	      "%sArServerClient got request for command %d which doesn't exist during idle... very odd", 
		 myLogPrefix.c_str(), command);
      delete idlePacket;
      return false;
    }
    serverData = (*it).second;

    ArLog::log(myVerboseLogLevel, "Processing idle command %s", 
	       serverData->getName());
    
    pushSlowIdleCommand(command);
    pushSlowIdleForceTcpFlag(true);

    if (serverData->getFunctor() != NULL)
      serverData->getFunctor()->invoke(this, idlePacket);
    if (serverData->getRequestOnceFunctor() != NULL)
      serverData->getRequestOnceFunctor()->invokeR(this, idlePacket);

    popSlowIdleCommand();
    popSlowIdleForceTcpFlag();

    delete idlePacket;
    myIdlePacketsMutex.lock();
  }
  myIdlePacketsMutex.unlock();  

  myHaveIdlePackets = false;
  return true;
}
