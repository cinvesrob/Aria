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
#include "ArCentralForwarder.h"


AREXPORT ArCentralForwarder::ArCentralForwarder(
	ArServerBase *mainServer, ArSocket *socket,
	const char *robotName, int port, 
	std::map<int, ArTime *> *usedPorts,
	ArFunctor2<ArCentralForwarder *,
		   ArServerClient *> *forwarderServerClientRemovedCB,
	const char *enforceProtocolVersion,
	ArServerCommands::Type enforceType) : 
  myReceiveDataFunctor(this, &ArCentralForwarder::receiveData),
  myInternalRequestChangedFunctor(this, 
				  &ArCentralForwarder::internalRequestChanged),
  myInternalRequestOnceFunctor(this, 
			       &ArCentralForwarder::internalRequestOnce,
			       NULL, NULL, true),
  myRobotServerClientRemovedCB(
	  this, &ArCentralForwarder::robotServerClientRemoved),
  myNetCentralHeartbeatCB(this, &ArCentralForwarder::netCentralHeartbeat),
  myClientServerClientRemovedCB(
	  this, &ArCentralForwarder::clientServerClientRemoved)
{
  myMainServer = mainServer;
  mySocket = socket;
  myRobotName = robotName;
  myStartingPort = port;
  myUsedPorts = usedPorts;
  myForwarderServerClientRemovedCB = forwarderServerClientRemovedCB;
  if (enforceProtocolVersion != NULL)
    myEnforceProtocolVersion = enforceProtocolVersion;
  else
    myEnforceProtocolVersion = "";
  myEnforceType = enforceType;

  myPrefix = "ArCentralForwarder_";
  myPrefix += myRobotName;
  myPrefix += ": ";
  myServer = NULL;
  myClient = NULL;
  myPort = 0;
  myState = STATE_STARTING;
  myBeingReplaced = false;
  myRobotHasCentralServerHeartbeat = false;
}

AREXPORT ArCentralForwarder::ArCentralForwarder() : 
  myReceiveDataFunctor(this, &ArCentralForwarder::receiveData),
  myInternalRequestChangedFunctor(this, 
				  &ArCentralForwarder::internalRequestChanged),
  myInternalRequestOnceFunctor(this, 
			       &ArCentralForwarder::internalRequestOnce,
			       NULL, NULL, true),
  myRobotServerClientRemovedCB(
	  this, &ArCentralForwarder::robotServerClientRemoved),
  myNetCentralHeartbeatCB(this, &ArCentralForwarder::netCentralHeartbeat),
  myClientServerClientRemovedCB(
	  this, &ArCentralForwarder::clientServerClientRemoved)
{
  myMainServer = NULL;
  mySocket = NULL;;
  myRobotName = "Unset";
  myStartingPort = 0;
  myUsedPorts = NULL;
  myForwarderServerClientRemovedCB = NULL;
  myEnforceProtocolVersion = "";
  myEnforceType = ArServerCommands::TYPE_UNSPECIFIED;

  myPrefix = "ArCentralForwarder_";
  myPrefix += myRobotName;
  myPrefix += ": ";
  myServer = NULL;
  myClient = NULL;
  myPort = 0;
  myState = STATE_STARTING;
  myBeingReplaced = false;
  myRobotHasCentralServerHeartbeat = false;
}


AREXPORT ArCentralForwarder::~ArCentralForwarder()
{
  if (myServer != NULL)
  {
    myServer->close();
    delete myServer;
  }
  
  if (myClient != NULL)
  {
    if (myClient->isConnected())
      myClient->disconnect();
    delete myClient;
  }

  // MPL adding this since it looks like it leaks this
  if (mySocket != NULL)
  {
    delete mySocket;
  }

  if (myRequestOnces.begin() != myRequestOnces.end())
    ArUtil::deleteSetPairs(myRequestOnces.begin(), myRequestOnces.end());
  myRequestOnces.clear();
  if (myLastRequest.begin() != myLastRequest.end())
    ArUtil::deleteSetPairs(myLastRequest.begin(), myLastRequest.end());
  myLastRequest.clear();
  if (myLastBroadcast.begin() != myLastBroadcast.end())
    ArUtil::deleteSetPairs(myLastBroadcast.begin(), myLastBroadcast.end());
  myLastBroadcast.clear();
}

AREXPORT bool ArCentralForwarder::callOnce(
	double heartbeatTimeout, double udpHeartbeatTimeout,
	double robotBackupTimeout, double clientBackupTimeout)
{
  if (myBeingReplaced)
  {
    ArLog::log(ArLog::Normal, "ArCentralForwarder::%s: being replaced by a duplicate, disconnecting", 
	       myRobotName.c_str());
    return false;
  }

  if (myState == STATE_CONNECTED)
  {
    return connectedCallOnce(heartbeatTimeout, udpHeartbeatTimeout,
			     robotBackupTimeout, clientBackupTimeout);
  }
  else if (myState == STATE_CONNECTING)
  {
    return connectingCallOnce(heartbeatTimeout, udpHeartbeatTimeout,
			      robotBackupTimeout, clientBackupTimeout);
  }
  else if (myState == STATE_GATHERING)
  {
    return gatheringCallOnce(heartbeatTimeout, udpHeartbeatTimeout,
			      robotBackupTimeout, clientBackupTimeout);
  }
  else if (myState == STATE_STARTING)
  {
    return startingCallOnce(heartbeatTimeout, udpHeartbeatTimeout,
			    robotBackupTimeout, clientBackupTimeout);
  }
  else
  {
    ArLog::log(ArLog::Normal, "%s in bad state, disconnecting", 
	       myRobotName.c_str());
    return false;
  }
  
}

AREXPORT bool ArCentralForwarder::startingCallOnce(
	double heartbeatTimeout, double udpHeartbeatTimeout, 
	double robotBackupTimeout, double clientBackupTimeout)
{
  myClient = new ArClientBase;
  myClient->enforceProtocolVersion(myEnforceProtocolVersion.c_str(), false);
  myClient->enforceType(myEnforceType, false);
  std::string name;
  name = "ArForwarderClient_" + myRobotName;
  myClient->setRobotName(name.c_str());
  
  if (myClient->internalNonBlockingConnectStart("", 0, true, "", "", 
						mySocket) != 
      ArClientBase::NON_BLOCKING_CONTINUE)
  {
    ArLog::log(ArLog::Normal, 
	       "%sCould not start connect to switching client %s from %s",
	       myPrefix.c_str(), myRobotName.c_str(), 
	       mySocket->getIPString());
    return false;
  }
  myState = STATE_CONNECTING;  
  myLastTcpHeartbeat.setToNow();
  myLastUdpHeartbeat.setToNow();
  return callOnce(heartbeatTimeout, udpHeartbeatTimeout,
		  robotBackupTimeout, clientBackupTimeout);
}

AREXPORT bool ArCentralForwarder::connectingCallOnce(
	double heartbeatTimeout, double udpHeartbeatTimeout, 
	double robotBackupTimeout, double clientBackupTimeout)
{

  ArClientBase::NonBlockingConnectReturn ret;

  if ((ret = myClient->internalNonBlockingConnectContinue()) == 
      ArClientBase::NON_BLOCKING_CONTINUE)
    return true;
  else if (ret == ArClientBase::NON_BLOCKING_FAILED)
  {
    ArLog::log(ArLog::Normal, 
	       "%sCould not connect to switching client %s from %s",
	       myPrefix.c_str(), myRobotName.c_str(), 
	       mySocket->getIPString());
    return false;
  }
  else if (ret == ArClientBase::NON_BLOCKING_CONNECTED)
  {
    myState = STATE_GATHERING;
    myLastTcpHeartbeat.setToNow();
    myLastUdpHeartbeat.setToNow();
    return callOnce(heartbeatTimeout, udpHeartbeatTimeout,
		    robotBackupTimeout, clientBackupTimeout);
  }
  else 
  {
    ArLog::log(ArLog::Normal, "%sIn unknown state connecting to %s", 
	       myPrefix.c_str(), myRobotName.c_str());
    return false;
  }
}

AREXPORT bool ArCentralForwarder::gatheringCallOnce(
	double heartbeatTimeout, double udpHeartbeatTimeout, 
	double robotBackupTimeout, double clientBackupTimeout)
{
  // if we have a heartbeat timeout make sure we've heard the
  // heartbeat within that range
  if (heartbeatTimeout >= -.00000001 && 
      myLastTcpHeartbeat.secSince() >= 5 && 
      myLastTcpHeartbeat.secSince() / 60.0 > heartbeatTimeout)
  {
    ArLog::log(ArLog::Normal, 
	       "%sHaven't connected in %g minutes, dropping connection", 
	       myPrefix.c_str(), heartbeatTimeout);
    return false;
  }

  if (!myClient->getReceivedDataList() || 
      !myClient->getReceivedArgRetList() || 
      !myClient->getReceivedGroupAndFlagsList())
  {
    myClient->loopOnce();
    return true;
  }

  ArLog::log(ArLog::Normal, "%Connected to switching client %s from %s",
	     myPrefix.c_str(), myRobotName.c_str(), mySocket->getIPString());
  //clientBase->logDataList();
  char serverName[1024];
  sprintf(serverName, "ArForwarderServer_%s", myRobotName.c_str());
  myServer = new ArServerBase(false, serverName, false, "", "", 
			      false, true,
			      false,
			      false, false);
  myServer->enforceProtocolVersion(myEnforceProtocolVersion.c_str());
  // there's no enforce of type here since this is the proxy for MP/ME
  // and such (robots don't connect here)

  myServer->addClientRemovedCallback(&myClientServerClientRemovedCB);

  ArTime startedOpening;
  startedOpening.setToNow();
  int port;
  bool foundPort;
  
  std::map<int, ArTime *>::iterator usedIt;
  // walk through our ports starting at our starting port
  for (port = myStartingPort, foundPort = false; 
       !foundPort && port < 65536; 
       port++)
  {
    // if we've used the port in the last 2 minutes then skip it
    if ((usedIt = myUsedPorts->find(port)) != myUsedPorts->end() && 
	(*usedIt).second != NULL &&
	((*usedIt).second->getSec() == 0 ||
	 (*usedIt).second->secSince() < 120))
    {

      ArLog::log(ArLog::Verbose, "%sSkipping port %d", myPrefix.c_str(), port);
      continue;
    }
    
    // try to open it
    if (myServer->open(port, myMainServer->getOpenOnIP()))
    {
      foundPort = true;
      myPort = port;
    }
  }
  
  if (!foundPort)
  {
    ArLog::log(ArLog::Normal, "%s Could not find port", 
	       myPrefix.c_str());
  }
  myServer->setUserInfo(myMainServer->getUserInfo());

  std::map<unsigned int, ArClientData *>::const_iterator dIt;
  ArClientData *clientData;

  myServer->addClientRemovedCallback(&myRobotServerClientRemovedCB);

  myClient->addHandler("centralHeartbeat", &myNetCentralHeartbeatCB);
  myClient->request("centralHeartbeat", 1000);

  if (myClient->dataExists("identSetSelfIdentifier"))
  {
    ArNetPacket sending;
    sending.strToBuf("CentralServer");
    myClient->requestOnce("identSetSelfIdentifier", &sending);
  }

  myLastTcpHeartbeat.setToNow();
  myLastUdpHeartbeat.setToNow();

  for (dIt = myClient->getDataMap()->begin(); 
       dIt != myClient->getDataMap()->end(); 
       dIt++)
  {
    clientData = (*dIt).second;

		// PS 10/22/13 - fix to crash, we don't know how
		// we got the NULL client data - this occured when
		// removing an addData from the robot side for 
		// EM packet support
		// So just test for null data and continue
		if (clientData == NULL) {
      ArLog::log(ArLog::Normal, 
		 "%sClientData is NULL",
		 myPrefix.c_str());

			continue;
		}

    if (myMainServer->dataHasFlag(clientData->getName(), 
				"MAIN_SERVER_ONLY"))
    {
      ArLog::log(ArLog::Normal, 
		 "%sNot forwarding %s since it is MAIN_SERVER_ONLY",
		 myPrefix.c_str(), clientData->getName());
      continue;
    }
    else if (clientData->hasDataFlag("DO_NOT_FORWARD"))
    {
      ArLog::log(ArLog::Normal, 
		 "%sNot forwarding %s since it is DO_NOT_FORWARD",
		 myPrefix.c_str(), clientData->getName());
      continue;
    }
    else if (clientData->hasDataFlag("RETURN_NONE"))
    {
      myReturnTypes[clientData->getCommand()] = RETURN_NONE;
    }
    else if (clientData->hasDataFlag("RETURN_SINGLE"))
    {
      myReturnTypes[clientData->getCommand()] = RETURN_SINGLE;
      myRequestOnces[clientData->getCommand()] = 
                    new std::list<ArServerClient *>;
    }
    else if (clientData->hasDataFlag("RETURN_VIDEO"))
    {
      ArLog::log(ArLog::Normal, 
		 "%sForwarding %s that is RETURN_VIDEO",
		 myPrefix.c_str(), clientData->getName());
      myReturnTypes[clientData->getCommand()] = RETURN_VIDEO;
      myRequestOnces[clientData->getCommand()] = 
                    new std::list<ArServerClient *>;
    }
    else if (clientData->hasDataFlag("RETURN_VIDEO_OPTIM"))
    {
      ArLog::log(ArLog::Normal, 
		 "%sForwarding %s that is RETURN_VIDEO_OPTIM",
		 myPrefix.c_str(), clientData->getName());
      myReturnTypes[clientData->getCommand()] = RETURN_VIDEO_OPTIM;
      myRequestOnces[clientData->getCommand()] = 
                    new std::list<ArServerClient *>;
    }
    else if (clientData->hasDataFlag("RETURN_UNTIL_EMPTY"))
    {
      myReturnTypes[clientData->getCommand()] = RETURN_UNTIL_EMPTY;
      myRequestOnces[clientData->getCommand()] = 
                    new std::list<ArServerClient *>;

    }
    else if (clientData->hasDataFlag("RETURN_COMPLEX"))
    {
      ArLog::log(ArLog::Normal, 
		 "%sNot forwarding %s since it is a complex return",
		 myPrefix.c_str(), clientData->getName());
      continue;
    }
    else
    {
      ArLog::log(ArLog::Normal, 
	  "%sNot forwarding %s since it is an unknown return (data flags %s)",
		 myPrefix.c_str(), clientData->getName(), 
		 clientData->getDataFlagsString());
      continue;
    }

    setLastRequest(clientData->getCommand());
    setLastBroadcast(clientData->getCommand());

    myServer->addDataAdvanced(
	    clientData->getName(), clientData->getDescription(),
	    NULL, clientData->getArgumentDescription(),
	    clientData->getReturnDescription(), clientData->getCommandGroup(),
	    clientData->getDataFlagsString(), clientData->getCommand(), 
	    &myInternalRequestChangedFunctor, 
	    &myInternalRequestOnceFunctor);

    myClient->addHandler(clientData->getName(), &myReceiveDataFunctor);
  }
  if (myClient->dataExists("centralServerHeartbeat"))
  {
    ArNetPacket sending;
    myRobotHasCentralServerHeartbeat = true; 
    myLastSentCentralServerHeartbeat.setToNow();
    myClient->requestOnce("centralServerHeartbeat", &sending, true);
    myClient->requestOnceUdp("centralServerHeartbeat", &sending, true);
  }
  myState = STATE_CONNECTED;
  return callOnce(heartbeatTimeout, udpHeartbeatTimeout,
		  robotBackupTimeout, clientBackupTimeout);
}

AREXPORT bool ArCentralForwarder::connectedCallOnce(
	double heartbeatTimeout, double udpHeartbeatTimeout, 
	double robotBackupTimeout, double clientBackupTimeout)
{
  if (!myClient->isConnected())
  {
    ArLog::log(ArLog::Normal, "%sLost connection to robot", 
	       myPrefix.c_str());
    return false;
  }

  myClient->setBackupTimeout(robotBackupTimeout);
  myServer->setBackupTimeout(clientBackupTimeout);

  if (myRobotHasCentralServerHeartbeat && 
      myLastSentCentralServerHeartbeat.mSecSince() >= 1000)
  {
    ArNetPacket sending;
    myLastSentCentralServerHeartbeat.setToNow();
    myClient->requestOnce("centralServerHeartbeat", &sending, true);
    myClient->requestOnceUdp("centralServerHeartbeat", &sending, true);
  }

  myClient->loopOnce();
  myServer->loopOnce();

  // if we have a heartbeat timeout make sure we've heard the
  // heartbeat within that range
  if (heartbeatTimeout >= -.00000001 && 
      myLastTcpHeartbeat.secSince() >= 5 && 
      myLastTcpHeartbeat.secSince() / 60.0 >= heartbeatTimeout)
  {
    ArLog::log(ArLog::Normal, "%sHaven't heard from robot in %g minutes, dropping connection", myPrefix.c_str(), heartbeatTimeout);
    return false;
  }

  // if we have a heartbeat timeout make sure we've heard the
  // heartbeat within that range
  if ((!myClient->isTcpOnlyFromServer() || !myClient->isTcpOnlyToServer()) &&
      udpHeartbeatTimeout >= -.00000001 && 
      myLastUdpHeartbeat.secSince() >= 5 && 
      myLastUdpHeartbeat.secSince() / 60.0 >= udpHeartbeatTimeout)
  {
    ArLog::log(ArLog::Normal, 
	       "%sSwitching to TCP only since haven't gotten UDP in %g minutes", 
	       myPrefix.c_str(), udpHeartbeatTimeout);
    myClient->setTcpOnlyFromServer();
    myClient->setTcpOnlyToServer();
  }

  return true;
}

void ArCentralForwarder::robotServerClientRemoved(ArServerClient *client)
{

  /*  MPL 2013_12_27 commenting this part out in favor of a simpler version 
  std::map<unsigned int, std::list<ArServerClient *> *>::iterator rIt;
  std::list<ArServerClient *> *requestList = NULL;
  std::list<ArServerClient *>::iterator scIt;
  std::list<ArServerClient *>::iterator scIt2;

  printf("Client disconnected\n");
  for (rIt = myRequestOnces.begin(); rIt != myRequestOnces.end(); rIt++)
  {
    requestList = (*rIt).second;
    // while we have entries for this client we loop and replace them
    // with NULLs
    bool foundOne = true;
    while (foundOne)
    {
      foundOne = false;
      // see if we find one... if we do we bail out of this loop
      // (since the iterators aren't valid anymore) and go back to the
      // while
      for (scIt = requestList->begin(); 
	   !foundOne && scIt != requestList->end(); 
	   scIt++)
      {
	if ((*scIt) == client)
	{
	  foundOne = true;
	  printf("Got...\n");
	  requestList->insert(scIt, (ArServerClient*)NULL);
	  for (scIt2 = requestList->begin(); 
	       scIt2 != requestList->end(); 
	       scIt2++)
	  {
	    if ((*scIt2) == client)
	    {
	      // erase it if we found one, then bail out of this,
	      // which should take us back to the while because of the
	      // check in the outer loop of !foundOne
	      requestList->erase(scIt2);
	      printf("Removed request for client %p\n", client);
	      break;
	    }
	  }
	}
      }
    }
  }
  */

  std::map<unsigned int, std::list<ArServerClient *> *>::iterator rIt;
  std::list<ArServerClient *> *requestList = NULL;
  std::list<ArServerClient *>::iterator scIt;
  bool printing = true;

  if (printing)
    ArLog::log(ArLog::Normal, "%sClient %s removed", myPrefix.c_str(),
	       client->getIPString());
  for (rIt = myRequestOnces.begin(); rIt != myRequestOnces.end(); rIt++)
  {
    requestList = (*rIt).second;
    // loop through the requests and replace any for this client with NULL
    for (scIt = requestList->begin(); 
	 scIt != requestList->end(); 
	 scIt++)
    {
      if ((*scIt) == client)
      {
	if (printing)
	  ArLog::log(ArLog::Normal, "%sRemoved client %s from list %u", myPrefix.c_str(), client->getIPString(), (*rIt).first);
	(*scIt) = NULL;
      }
    }
  }
}

void ArCentralForwarder::clientServerClientRemoved(ArServerClient *client)
{
  if (client->getState() != ArServerClient::STATE_DISCONNECTED)
    myForwarderServerClientRemovedCB->invoke(this, client);
}



void ArCentralForwarder::receiveData(ArNetPacket *packet)
{
  ReturnType returnType;
  std::list<ArServerClient *>::iterator it;
  ArServerClient *client;

  // chop off the old footer
  //packet->setLength(packet->getLength() - ArNetPacket::FOOTER_LENGTH);
  packet->setAddedFooter(true);

  /*
  if (strcmp(myClient->getName(packet->getCommand(), true), 
	     "getPictureCam1") == 0)
    printf("Got getPictureCam1...\n");
  */
  returnType = getReturnType(packet->getCommand());
  //printf("Got a packet in for %s %d\n", myClient->getName(packet->getCommand(), true), packet->getCommand());

  // this part is seeing if it came from a request_once, if so we
  // don't service anything else (so we take care of those things that
  // only happen once better then the ones that are mixing... but that
  // should be okay)
  checkRequestOnces(packet->getCommand());
  if ((returnType == RETURN_SINGLE || returnType == RETURN_UNTIL_EMPTY) &&
      (it = myRequestOnces[packet->getCommand()]->begin()) != 
      myRequestOnces[packet->getCommand()]->end())
  {
    //if (returnType == RETURN_UNTIL_EMPTY)
    //printf("Got a packet for %s with length %d %d\n", myClient->getName(packet->getCommand(), true), packet->getDataLength(), packet->getLength());

    client = (*it);
    if (client != NULL)
    {
      if (packet->getPacketSource() == ArNetPacket::TCP)
	client->sendPacketTcp(packet);
      else if (packet->getPacketSource() == ArNetPacket::UDP)
	client->sendPacketUdp(packet);
      else
      {
	client->sendPacketTcp(packet);
	ArLog::log(ArLog::Normal, 
		   "%sDon't know what type of packet %s is (%d)", 
		   myPrefix.c_str(), 
		   myClient->getName(packet->getCommand(), true), 
		   packet->getPacketSource());
      }
    }
    if ((returnType == RETURN_UNTIL_EMPTY && packet->getDataLength() == 0) || 
	returnType == RETURN_SINGLE)
    {
      //if (returnType == RETURN_UNTIL_EMPTY)
      //printf("Got final packet for for %s\n", myClient->getName(packet->getCommand(), true));
      myRequestOnces[packet->getCommand()]->pop_front();
    }
  }
  else if (returnType == RETURN_VIDEO)
  {
    // what we do here is send it to the ones that have requested it
    // but aren't listening for the broadcast... then we broadcast it
    // to everyone whose listening... this should ensure that everyone
    // just gets the packet once as often as we see it... 

    while ((it = myRequestOnces[packet->getCommand()]->begin()) != 
	   myRequestOnces[packet->getCommand()]->end())
    {
      //printf("Sent a single return_single_and_broadcast for %s\n", myClient->getName(packet->getCommand(), true));
      client = (*it);
      if (client != NULL && client->getFrequency(packet->getCommand()) == -2)
      {
	if (packet->getPacketSource() == ArNetPacket::TCP)
	  client->sendPacketTcp(packet);
	else if (packet->getPacketSource() == ArNetPacket::UDP)
	  client->sendPacketUdp(packet);
	else
	{
	  client->sendPacketTcp(packet);
	  ArLog::log(ArLog::Normal, 
		     "%sDon't know what type of packet %s is (%d)", 
		     myPrefix.c_str(), 
		     myClient->getName(packet->getCommand(), true), 
		     packet->getPacketSource());
	}
      }
      myRequestOnces[packet->getCommand()]->pop_front();
    }
    //printf("Broadcast return_single_and_broadcast for %s\n", myClient->getName(packet->getCommand(), true));
    myLastBroadcast[packet->getCommand()]->setToNow();
    if (packet->getPacketSource() == ArNetPacket::TCP)
    {
      myServer->broadcastPacketTcpByCommand(packet, packet->getCommand());
    }
    else if (packet->getPacketSource() == ArNetPacket::UDP)
    {
      myServer->broadcastPacketUdpByCommand(packet, packet->getCommand());
    }
    else
    {
      myServer->broadcastPacketTcpByCommand(packet, packet->getCommand());
      ArLog::log(ArLog::Normal, 
		 "%sDon't know what type of packet %s is (%d)", 
		 myPrefix.c_str(), 
		 myClient->getName(packet->getCommand(), true), 
		 packet->getPacketSource());
    }
  }
  else if (returnType == RETURN_VIDEO_OPTIM)
  {
    // what we do here is send it to the ones that have requested it
    while ((it = myRequestOnces[packet->getCommand()]->begin()) != 
	   myRequestOnces[packet->getCommand()]->end())
    {
      client = (*it);
      /*
      ArLog::log(ArLog::Normal, "%sSent a return_video_optim for %s to %s", 
		 myPrefix.c_str(), 
		 myClient->getName(packet->getCommand(), true),
		 client->getIPString());
      */
      if (client != NULL)
      {
	if (packet->getPacketSource() == ArNetPacket::TCP)
	  client->sendPacketTcp(packet);
	else if (packet->getPacketSource() == ArNetPacket::UDP)
	  client->sendPacketUdp(packet);
	else
	{
	  client->sendPacketTcp(packet);
	  ArLog::log(ArLog::Normal, 
		     "%sDon't know what type of packet %s is (%d)", 
		     myPrefix.c_str(),
		     myClient->getName(packet->getCommand(), true), 
		     packet->getPacketSource());
	}
      }
      myRequestOnces[packet->getCommand()]->pop_front();
    }
  }
  else
  {
    myLastBroadcast[packet->getCommand()]->setToNow();
    if (packet->getPacketSource() == ArNetPacket::TCP)
    {
      myServer->broadcastPacketTcpByCommand(packet, packet->getCommand());
    }
    else if (packet->getPacketSource() == ArNetPacket::UDP)
    {
      myServer->broadcastPacketUdpByCommand(packet, packet->getCommand());
    }
    else
    {
      myServer->broadcastPacketTcpByCommand(packet, packet->getCommand());
      ArLog::log(ArLog::Normal, 
		 "%sDon't know what type of packet %s is (%d)", 
		 myPrefix.c_str(), 
		 myClient->getName(packet->getCommand(), true), 
		 packet->getPacketSource());
    }
  }

}

void ArCentralForwarder::internalRequestChanged(long interval, 
						unsigned int command)
{
  /*
  if (strcmp(myClient->getName(command, true), 
	     "getPictureCam1") == 0)
    printf("Changed getPictureCam1 to %d...\n", interval);
  */
  if (interval == -2)
  {
    ArLog::log(ArLog::Verbose, "%sStopping request for %s", 
	       myPrefix.c_str(), myClient->getName(command, true));
    myClient->requestStopByCommand(command);
    setLastRequest(command);
  }
  else 
  {
    ReturnType returnType;
    
    returnType = getReturnType(command);
    if (returnType == RETURN_VIDEO && interval != -1)
    {
      ArLog::log(ArLog::Verbose, "%sIgnoring a RETURN_VIDEO attempted request of %s at %d interval since RETURN_VIDEOs cannot request at an interval", 
		 myPrefix.c_str(), myClient->getName(command, true), interval);
      return;
    }
    if (returnType == RETURN_VIDEO_OPTIM && interval != -1)
    {
      ArLog::log(ArLog::Verbose, "%sIgnoring a RETURN_VIDEO_OPTIM attempted request of %s at %d interval since RETURN_VIDEOs cannot request at an interval", 
		 myPrefix.c_str(), myClient->getName(command, true), interval);
      return;
    }
    
    ArLog::log(ArLog::Verbose, "%sRequesting %s at interval of %ld", 
	       myPrefix.c_str(), myClient->getName(command, true), interval);
    myClient->requestByCommand(command, interval);
    setLastRequest(command);
    // if the interval is -1 then also requestOnce it so that anyone
    // connecting after the first connection can actually get data too
    myClient->requestOnceByCommand(command);
  }
}

bool ArCentralForwarder::internalRequestOnce(
	ArServerClient *client, ArNetPacket *packet, bool tcp)
{
  ReturnType returnType = RETURN_NONE;
  
  returnType = getReturnType(packet->getCommand());

  // chop off the footer
  packet->setAddedFooter(true);
  /*
  if (strcmp(myClient->getName(packet->getCommand(), true), 
	     "getPictureCam1") == 0)
    printf("Request once for getPictureCam1...\n");
  */
  // if its video and the last broadcast or request was very recently
  // then ignore it so we don't wind up using up our wireless
  // bandwidth
  // MPL taking this out since it may be causing some problems
  /*
  if (returnType == RETURN_VIDEO && 
      (myLastBroadcast[packet->getCommand()]->mSecSince() < 25 ||
       myLastRequest[packet->getCommand()]->mSecSince() < 25))
  {
    ArLog::log(ArLog::Normal, "Ignoring a RETURN_VIDEO of %s since request or broadcast was recent", myClient->getName(packet->getCommand(), true));
    return;
  }
  */
  // if its a type where we keep track then put it into the list
  if (returnType == RETURN_SINGLE || returnType == RETURN_UNTIL_EMPTY || 
      returnType == RETURN_VIDEO || returnType == RETURN_VIDEO_OPTIM)
  {
    //if (returnType == RETURN_UNTIL_EMPTY)
    //printf("Trying to request once %s\n", myClient->getName(packet->getCommand(), true));
    checkRequestOnces(packet->getCommand());
    myRequestOnces[packet->getCommand()]->push_back(client);
  }
  
  bool ret;
  ArLog::log(ArLog::Verbose, "%sRequesting %s once", 
	     myPrefix.c_str(), myClient->getName(packet->getCommand()));
  if (tcp)
    ret = myClient->requestOnceByCommand(packet->getCommand(), packet);
  else
    ret = myClient->requestOnceByCommandUdp(packet->getCommand(), packet);

  setLastRequest(packet->getCommand());

  return ret;
}


AREXPORT void ArCentralForwarder::netCentralHeartbeat(ArNetPacket *packet)
{
  if (packet->getPacketSource() == ArNetPacket::TCP)
    myLastTcpHeartbeat.setToNow();
  else if (packet->getPacketSource() == ArNetPacket::UDP)
    myLastUdpHeartbeat.setToNow();
  else
    ArLog::log(ArLog::Normal, 
	       "%sGot unknown packet source for heartbeat packet", 
	       myPrefix.c_str());
}

AREXPORT bool ArCentralForwarder::addHandler(
	const char *name, ArFunctor1 <ArNetPacket *> *functor)
{
  if (myClient != NULL)
  {
    return myClient->addHandler(name, functor);
  }
  else 
  {
    ArLog::log(ArLog::Normal, 
	       "ArCentralForwarder::%s::addHandler: Tried to call while client was still NULL",
	       getRobotName());
    return false;
  }
}

AREXPORT bool ArCentralForwarder::remHandler(const char *name, ArFunctor1<ArNetPacket *> *functor)
{
  if (myClient != NULL)
  {
    return myClient->remHandler(name, functor);
  }
  else 
  {
    ArLog::log(ArLog::Normal, 
	       "ArCentralForwarder::%s::remHandler: Tried to call while client was still NULL",
	       getRobotName());
    return false;
  }
}

AREXPORT bool ArCentralForwarder::request(const char *name, long mSec)
{
  if (myServer != NULL)
  {
    return myServer->internalSetDefaultFrequency(name, mSec);
    
  }
  else 
  {
    ArLog::log(ArLog::Normal, 
	       "ArCentralForwarder::%s::remHandler: Tried to call while server was still NULL",
	       getRobotName());
    return false;
  }
}

AREXPORT bool ArCentralForwarder::requestOnce(const char *name,
					      ArNetPacket *packet,
					      bool quiet)
{
  if (myClient != NULL)
  {
    unsigned int commandNum = myClient->findCommandFromName(name);
    if (commandNum == 0)
    {
      if (!quiet)
	ArLog::log(ArLog::Normal, 
		   "ArCentralForwarder::%s::requestOnce: Can't find commandNum for %s",
		   getRobotName(), name);
      return false;
    }
    //return myClient->requestOnce(name, packet, quiet);
    if (packet != NULL)
    {
      packet->setCommand(commandNum);
      packet->finalizePacket();
      return internalRequestOnce(NULL, packet, true);
    }
    else
    {
      ArNetPacket tempPacket;
      tempPacket.setCommand(commandNum);
      tempPacket.finalizePacket();
      return internalRequestOnce(NULL, &tempPacket, true);
    }
  }
  else 
  {
    if (!quiet)
      ArLog::log(ArLog::Normal, 
		 "ArCentralForwarder::%s::requestOnce: Tried to call (for %s) while client was still NULL",
		 getRobotName(), name);
    return false;
  }
}

AREXPORT bool ArCentralForwarder::requestOnceUdp(const char *name, 
						 ArNetPacket *packet, 
						 bool quiet)
{
  if (myClient != NULL)
  {
    unsigned int commandNum = myClient->findCommandFromName(name);
    if (commandNum == 0)
    {
      if (!quiet)
	ArLog::log(ArLog::Normal, 
		   "ArCentralForwarder::%s::requestOnce: Can't find commandNum for %s",
		   getRobotName(), name);
      return false;
    }
    //return myClient->requestOnceUdp(name, packet, quiet);
    if (packet != NULL)
    {
      packet->setCommand(commandNum);
      packet->finalizePacket();
      return internalRequestOnce(NULL, packet, false);
    }
    else
    {
      ArNetPacket tempPacket;
      tempPacket.setCommand(commandNum);
      tempPacket.finalizePacket();
      return internalRequestOnce(NULL, &tempPacket, false);
    }
  }
  else 
  {
    if (!quiet)
      ArLog::log(ArLog::Normal, 
		 "ArCentralForwarder::%s::requestOnce: Tried to call (for %s) while client was still NULL",
		 getRobotName(), name);
    return false;
  }
}

AREXPORT bool ArCentralForwarder::requestOnceWithString(const char *name, const char *str)
{
  ArNetPacket tempPacket;
  tempPacket.strToBuf(str);

  return requestOnce(name, &tempPacket);
}

AREXPORT bool ArCentralForwarder::dataExists(const char *name)
{
  if (myClient != NULL)
  {
    return myClient->dataExists(name);
  }
  else 
  {
    ArLog::log(ArLog::Normal, 
	       "ArCentralForwarder::%s::dataExists: Tried to call while client was still NULL",
	       getRobotName());
    return false;
  }
}

ArCentralForwarder::ReturnType ArCentralForwarder::getReturnType(int command)
{
  if (myReturnTypes.find(command) != myReturnTypes.end())
    return myReturnTypes[command];
  else
    return RETURN_NONE;
}

void ArCentralForwarder::checkRequestOnces(unsigned int command)
{
  if (myRequestOnces.find(command) == myRequestOnces.end())
    myRequestOnces[command] = new std::list<ArServerClient *>;
}

void ArCentralForwarder::setLastRequest(unsigned int command)
{
  if (myLastRequest.find(command) == myLastRequest.end())
    myLastRequest[command] = new ArTime;
  myLastRequest[command]->setToNow();
}

void ArCentralForwarder::setLastBroadcast(unsigned int command)
{
  if (myLastBroadcast.find(command) == myLastBroadcast.end())
    myLastBroadcast[command] = new ArTime;
  myLastBroadcast[command]->setToNow();
}
