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
#include "ArClientSwitchManager.h"

AREXPORT ArClientSwitchManager::ArClientSwitchManager(
	ArServerBase *serverBase, ArArgumentParser *parser,
	const char *serverDesc, const char *clientSoftwareDesc) : 
  myParseArgsCB(this, &ArClientSwitchManager::parseArgs),
  myLogOptionsCB(this, &ArClientSwitchManager::logOptions),
  mySocketClosedCB(this, &ArClientSwitchManager::socketClosed),
  mySwitchCB(this, &ArClientSwitchManager::clientSwitch),
  myNetCentralHeartbeatCB(this, &ArClientSwitchManager::netCentralHeartbeat),
  myNetCentralServerHeartbeatCB(this, 
			   &ArClientSwitchManager::netCentralServerHeartbeat),
  myFileUserCB(this, &ArClientSwitchManager::fileUserCallback),
  myFilePasswordCB(this, &ArClientSwitchManager::filePasswordCallback),
  myFileServerKeyCB(this, &ArClientSwitchManager::fileServerKeyCallback),
  myProcessFileCB(this, &ArClientSwitchManager::processFile)
{
  myMutex.setLogName("ArClientSwitchManager::myDataMutex");
  myServer = serverBase;
  myParser = parser;
  myServerDesc = serverDesc;
  myClientSoftwareDesc = clientSoftwareDesc;
  
  setThreadName("ArClientSwitchManager");

  mySwitchCB.setName("ArClientSwitchManager");

  myParseArgsCB.setName("ArClientSwitchManager");
  Aria::addParseArgsCB(&myParseArgsCB, 49);
  myLogOptionsCB.setName("ArClientSwitchManager");
  Aria::addLogOptionsCB(&myLogOptionsCB, 49);

  myNetCentralHeartbeatCB.setName("ArClientSwitchManager");
  myServer->addData("centralHeartbeat", 
		    "a packet that is requested and used by the central server to make sure there is still a connection, it should not be used by anyone or anything else, note that this sends a response over tcp and udp at the same time",
		    &myNetCentralHeartbeatCB, "none", "none", "RobotInfo", 
		    "RETURN_SINGLE");

  myServer->addData("centralServerHeartbeat", 
		    "a packet that is sent by central server to make sure there is still a connection to the robot, it should not be used by anyone or anything else",
		    &myNetCentralServerHeartbeatCB, "none", "none", "RobotInfo", 
		    "RETURN_SINGLE");

  myServerBackupTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("RobotToCentralServerTimeoutInMins", 
		      &myServerBackupTimeout,
		      "The amount of time the robot can go without sending a packet to the central server successfully (when there are packets to send).  A number less than 0 means this won't happen.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", 
		      -1),
	  "Connection timeouts", ArPriority::DETAILED);

  myServerHeartbeatTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("RobotFromCentralServerTimeoutInMins", 
		      &myServerHeartbeatTimeout,
		      "The amount of time a robot can go without getting the heartbeat from the central server before disconnecting it.  A number less than 0 means that the central server will never timeout.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);
  
  myServerUdpHeartbeatTimeout = 2;
  Aria::getConfig()->addParam(
	  ArConfigArg("RobotFromCentralServerUdpTimeoutInMins", 
		      &myServerUdpHeartbeatTimeout,
		      "The amount of time a robot can go without getting the udp heartbeat from the central server before disconnecting it.  A number less than 0 means that the central server will never timeout.  The time is in minutes but takes doubles (ie .5) (5 seconds is used if the value is positive, but less than that amount)", -1),
	  "Connection timeouts", ArPriority::DETAILED);


  myProcessFileCB.setName("ArClientSwitchManager");
  Aria::getConfig()->addProcessFileCB(&myProcessFileCB, -1000);
  
  myConfigFirstProcess = true;
  myConfigConnectToCentralServer = false;
  myConfigCentralServer[0] = '\0';
  myConfigIdentifier[0] = '\0';
  
  switchState(IDLE);
  myClient = NULL;
  myServerClient = NULL;
  myCentralServerPort = 5000;

  myFileUserCB.setName("ArClientSwitchManager::user");
  myFileParser.addHandler("user", &myFileUserCB);
  myFilePasswordCB.setName("ArClientSwitchManager::password");
  myFileParser.addHandler("password", &myFilePasswordCB);
  myFileServerKeyCB.setName("ArClientSwitchManager::serverKey");
  myFileParser.addHandler("serverKey", &myFileServerKeyCB);
}
 
AREXPORT ArClientSwitchManager::~ArClientSwitchManager()
{
}

AREXPORT bool ArClientSwitchManager::isConnected(void)
{
  return myState == CONNECTED;
}

AREXPORT void ArClientSwitchManager::switchState(State state)
{
  myState = state;
  myStartedState.setToNow();
  //myGaveTimeWarning = false;
}

AREXPORT bool ArClientSwitchManager::parseArgs(void)
{
  const char *centralServer = NULL;
  const char *identifier = NULL;

  if (!myParser->checkParameterArgumentString("-centralServer", 
					      &centralServer) || 
      !myParser->checkParameterArgumentString("-cs", &centralServer) || 
      !myParser->checkParameterArgumentInteger("-centralServerPort", 
				     &myCentralServerPort) || 
      !myParser->checkParameterArgumentInteger("-csp", 
					       &myCentralServerPort) || 
      !myParser->checkParameterArgumentString("-identifier", &identifier) || 
      !myParser->checkParameterArgumentString("-id", &identifier))
    return false;


  bool wasReallySetOnlyTrue = myParser->getWasReallySetOnlyTrue();
  myParser->setWasReallySetOnlyTrue(true);
  
  bool wasReallySet = false;
  const char *centralServerInfoFile = NULL;
  while (myParser->checkParameterArgumentString(
		 "-centralServerInfoFile", &centralServerInfoFile, 
		 &wasReallySet, true) && 
	 wasReallySet)
  {
    if (centralServerInfoFile != NULL && !parseFile(centralServerInfoFile))
    {
      myParser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
      return false;
    }
    wasReallySet = false;
  }
  
  myParser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);

  myDataMutex.lock();
  if (centralServer != NULL && centralServer[0] != '\0')
  {
    myCentralServer = centralServer;
    myState = TRYING_CONNECTION;
  }

  if (identifier != NULL && identifier[0] != '\0')
  {
    myIdentifier = identifier;
  }
  /* Just don't set it so the server can just set it to the IP
  else 
  {
    int nameLen;
    char name[512];
    int i;
    
    nameLen = ArMath::random() % 15 + 1;
    for (i = 0; i < nameLen; i++)
      name[i] = 'a' + (ArMath::random() % ('z' - 'a'));
    name[nameLen] = '\0';
    
    myIdentifier = name;
  }
  */
  myDataMutex.unlock();

  return true;
}

AREXPORT void ArClientSwitchManager::logOptions(void) const
{
  ArLog::log(ArLog::Terse, "ArClientSwitchManager options:");
//  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "-centralServer <host>");
  ArLog::log(ArLog::Terse, "-cs <host>");
  ArLog::log(ArLog::Terse, "-centralServerPort <port>");
  ArLog::log(ArLog::Terse, "-csp <port>");
  ArLog::log(ArLog::Terse, "-identifier <identifier>");
  ArLog::log(ArLog::Terse, "-id <identifier>");
  /*
  ArLog::log(ArLog::Terse, "-centralServerUser <user>");
  ArLog::log(ArLog::Terse, "-csu <user>");
  ArLog::log(ArLog::Terse, "-centralServerPassword <password>");
  ArLog::log(ArLog::Terse, "-csp <password>");
  ArLog::log(ArLog::Terse, "-centralServerKey <serverKey>");
  ArLog::log(ArLog::Terse, "-csk <password>");
  */
  ArLog::log(ArLog::Terse, "-centralServerInfoFile <fileName>");
}

AREXPORT void ArClientSwitchManager::clientSwitch(ArNetPacket *packet)
{
  //myDataMutex.lock();
  ArLog::log(ArLog::Normal, "Switch acknowledged, switching");
  
  myServerClient = myServer->makeNewServerClientFromSocket(
	  myClient->getTcpSocket(), true);
  myServerClient->getTcpSocket()->setCloseCallback(&mySocketClosedCB);
  myServerClient->setBackupTimeout(myServerBackupTimeout);

  ArSocket emptySocket;
  myClient->getTcpSocket()->transfer(&emptySocket);

  myLastTcpHeartbeat.setToNow();
  myLastUdpHeartbeat.setToNow();
  switchState(CONNECTED);
  
  char switchStr[1024];
  sprintf(switchStr, "Recovered connection to %s at %s, now connected.", 
    myServerDesc.c_str(), myCentralServer.c_str());
  myConnectedCBList.invoke(switchStr);
  //myDataMutex.unlock();
}

AREXPORT void ArClientSwitchManager::socketClosed(void)
{
  myDataMutex.lock();
  if (myState == CONNECTED)
  {
    myServerClient = NULL;
    ArLog::log(ArLog::Normal, "ArClientSwitchManager: Lost connection to central server");
    switchState(LOST_CONNECTION);

    char failedStr[1024];
    sprintf(failedStr, "Lost connection to %s at %s, restarting connection.", 
	    myServerDesc.c_str(), myCentralServer.c_str());

    myDataMutex.unlock();
    myFailedConnectCBList.invoke(failedStr);
  }
  else
  {
    myDataMutex.unlock();
  }
}

AREXPORT void *ArClientSwitchManager::runThread(void *arg)
{
  threadStarted();
  
  while (getRunning())
  {
    myDataMutex.lock();
    if (myState == IDLE)
    {
    }
    else if (myState == TRYING_CONNECTION)
    {
      myLastConnectionAttempt.setToNow();
      ArLog::log(ArLog::Normal, "Trying to connect to central server %s",
		 myCentralServer.c_str());
      myClient = new ArClientBase;
      myClient->setRobotName("ClientSwitch", myDebugLogging);
      myClient->setServerKey(myServerKey.c_str(), false);
      myClient->enforceProtocolVersion(myEnforceProtocolVersion.c_str(), false);
      myClient->enforceType(myEnforceType, false);
      myLastTcpHeartbeat.setToNow();
      myLastUdpHeartbeat.setToNow();
      if (!myClient->blockingConnect(myCentralServer.c_str(), 
				     myCentralServerPort, false,
				     myUser.c_str(), myPassword.c_str(),
				     myServer->getOpenOnIP()))
      {
	char failedStr[10000];
	char verboseFailedStr[10000];
	if (myClient->wasRejected())
	{
	  //ArLog::log(ArLog::Normal, 
	             //"Could not connect to %s because it rejected connection (%d, %s)",
		     //myCentralServer.c_str(), myClient->getRejected(), myClient->getRejectedString());
	  if (myClient->getRejected() == 1)
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nBad username and password.", 
		    myServerDesc.c_str(), myCentralServer.c_str());
	    sprintf(failedStr, 
		    "Could not connect to %s (Bad username and password)", 
		    myCentralServer.c_str());
	  }
	  else if (myClient->getRejected() == 2)
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nIt rejected this connection because it is not direct.", 
		    myServerDesc.c_str(), myCentralServer.c_str());
	    sprintf(failedStr, 
		    "Could not connect to %s (rejected because not a direct connection).", 
		    myCentralServer.c_str());
	  }
	  else if (myClient->getRejected() == 3)
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nIt is a version not supported by this robot's %s.", 
		    myServerDesc.c_str(), myCentralServer.c_str(),
		    myClientSoftwareDesc.c_str());
	    sprintf(failedStr, 
		    "Could not connect to %s (it is not a version supported by this robot's %s)", 
		    myCentralServer.c_str(), myClientSoftwareDesc.c_str());
	  }
	  else if (myClient->getRejected() == 4)
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nIt does not support this robot's %s version.", 
		    myServerDesc.c_str(), myCentralServer.c_str(),
		    myClientSoftwareDesc.c_str());
	    sprintf(failedStr, 
		    "Could not connect to %s at %s (It does not support this robot's %s version)", 
		    myServerDesc.c_str(), myCentralServer.c_str(),
		    myClientSoftwareDesc.c_str());
	  }
	  else if (myClient->getRejected() == 5)
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nIt's number of licenses has been exceeded.\n\nPlease contact your robot provider for assistance purchasing more licenses.", 
		    myServerDesc.c_str(), myCentralServer.c_str());
	    sprintf(failedStr, 
		    "Could not connect to %s (It's number of licenses has been exceeded)",
		    myCentralServer.c_str());
	  }
	  else if (myClient->getRejected() == 6)
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nIt does not allow connections from %s robots.\n\nPlease contact your robot administrator for assistance.", 
		    myServerDesc.c_str(), myCentralServer.c_str(), 
		    ArServerCommands::toString(myEnforceType));
	    sprintf(failedStr, 
		    "Could not connect to %s (It does not allow connections from %s robots)",
		    myCentralServer.c_str(), 
		    ArServerCommands::toString(myEnforceType));
	  }
	  else
	  {
	    sprintf(verboseFailedStr, 
		    "Could not connect to %s at %s\n\nThe reason is '%s'", 
		    myServerDesc.c_str(), myCentralServer.c_str(), myClient->getRejectedString());
	    sprintf(failedStr, 
		    "Could not connect to %s (reason %d '%s')", 
		    myCentralServer.c_str(), myClient->getRejected(), 
		    myClient->getRejectedString());
	  }
	}
	else
	{
	  ArLog::log(ArLog::Verbose, 
	     "Could not connect to %s to switch with, not doing anything",
		     myCentralServer.c_str());
	  sprintf(verboseFailedStr, "Could not connect to %s at %s\n\nIt may not be reachable by the robot.", myServerDesc.c_str(), myCentralServer.c_str());
	  sprintf(failedStr, "Could not connect to %s", myCentralServer.c_str());
	}
	myClient->getTcpSocket()->close();
	delete myClient;
	myClient = NULL;
	switchState(LOST_CONNECTION);
	ArLog::log(ArLog::Normal, "%s", failedStr);
	myDataMutex.unlock();
	myFailedConnectCBList.invoke(verboseFailedStr);
	continue;
      }
      
      if (!myClient->dataExists("switch"))
      {
	ArLog::log(ArLog::Normal, 
		   "ArClientSwitchManager: Connected to central server %s but it isn't a central server, going to idle");
	myClient->disconnect();
	delete myClient;
	myClient = NULL;
	switchState(LOST_CONNECTION);

	char failedStr[1024];
	sprintf(failedStr, "Connected to %s at %s, but it is inappropriate software, disconnecting.\n\nLikely this robot is pointing at another robot.", 
		myServerDesc.c_str(), myCentralServer.c_str());
	myDataMutex.unlock();
	myFailedConnectCBList.invoke(failedStr);
	continue;
      }

      ArNetPacket sendPacket;
      ArLog::log(ArLog::Verbose, "Putting in %s\n", myIdentifier.c_str());
      sendPacket.strToBuf(myIdentifier.c_str());

      if (myClient->dataExists("centralServerHeartbeat"))
      {
	ArLog::log(ArLog::Normal, "Requesting switch (have heartbeat)");
	myServerHasHeartbeat = true;
      }
      else
      {
	ArLog::log(ArLog::Normal, "Requesting switch (no heartbeat)");
	myServerHasHeartbeat = false;
      }
      
      myClient->addHandler("switch", &mySwitchCB);
      myClient->requestOnce("switch", &sendPacket);
      switchState(CONNECTING);
    }
    else if (myState == CONNECTING)
    {
      /* old behavior that just warned
      if (!myGaveTimeWarning && myStartedState.secSince() > 10)
      {
	ArLog::log(ArLog::Normal, "ArClientSwitchManager: Connecting has taken over 10 seconds, probably a problem");
	myGaveTimeWarning = true;
      }
      myClient->loopOnce();
      myDataMutex.unlock();
      ArUtil::sleep(1);
      continue;      
      */
      // new behavior that starts over
      myClient->loopOnce();
      if (myStartedState.secSince() >= 15 && 
	  myLastTcpHeartbeat.secSince() / 60.0 >= myServerHeartbeatTimeout)
      {

	ArLog::log(ArLog::Normal, "ArClientSwitchManager: Connecting to central server has taken %.2f minutes, restarting connection", myStartedState.secSince() / 60.0); // this had / 4.0 for no apparent reason, changed it (11/13/2012 MPL)
	/// added this to try and eliminate the occasional duplicates
	myClient->getTcpSocket()->close();
	delete myClient;
	myClient = NULL;
	switchState(LOST_CONNECTION);

	char failedStr[1024];
	sprintf(failedStr, "Connection to %s at %s took over %.2f minutes, restarting connection.", 
		myServerDesc.c_str(), myCentralServer.c_str(), myStartedState.secSince() / 60.0);
	myDataMutex.unlock();
	myFailedConnectCBList.invoke(failedStr);
	continue;
      }
      myDataMutex.unlock();
      ArUtil::sleep(1);
      continue;
    }
    else if (myState == CONNECTED)
    {
      if (myClient != NULL)
      {
	delete myClient;
	myClient = NULL;
      }
      // if we have a heartbeat timeout make sure we've heard the
      // heartbeat within that range
      if (myServerHasHeartbeat && myServerHeartbeatTimeout >= -.00000001 && 
	  myLastTcpHeartbeat.secSince() >= 5 && 
	  myLastTcpHeartbeat.secSince() / 60.0 >= myServerHeartbeatTimeout)
      {
	ArLog::log(ArLog::Normal, 
		   "ArClientSwitchManager: Dropping connection since haven't heard from central server in %g minutes", 
		   myServerHeartbeatTimeout);
	myServerClient->forceDisconnect(false);
	myServerClient = NULL;
	switchState(LOST_CONNECTION);

	char failedStr[1024];
	sprintf(failedStr, "Dropping connection to %s at %s since the robot hasn't heard from it in over %g minutes, restarting connection.", 
		myServerDesc.c_str(), myCentralServer.c_str(), myServerHeartbeatTimeout);
	myDataMutex.unlock();
	myFailedConnectCBList.invoke(failedStr);
	continue;
      }
      else if 
	(myServerHasHeartbeat && !myServerClient->isTcpOnly() && 
	 myServerUdpHeartbeatTimeout >= -.00000001 && 
	 myLastUdpHeartbeat.secSince() >= 5 && 
	 myLastUdpHeartbeat.secSince() / 60.0 >= myServerUdpHeartbeatTimeout)
      {
	ArLog::log(ArLog::Normal, "ArClientSwitchManager: Switching to TCP only since gotten UDP from central server in %g minutes", 
		   myServerUdpHeartbeatTimeout);
	myServerClient->useTcpOnly();
      }
      
    }
    else if (myState == LOST_CONNECTION)
    {
      if (myLastConnectionAttempt.secSince() > 10)
      {
	switchState(TRYING_CONNECTION);
      }
    }
    myDataMutex.unlock();
    ArUtil::sleep(100);
  }
  threadFinished();
  return NULL;
}

AREXPORT void ArClientSwitchManager::netCentralHeartbeat(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sending;
  client->sendPacketTcp(&sending);
  client->sendPacketUdp(&sending);
}

AREXPORT void ArClientSwitchManager::netCentralServerHeartbeat(
	ArServerClient *client, ArNetPacket *packet)
{
  myDataMutex.lock();
  if (client != myServerClient)
  {
    ArLog::log(ArLog::Normal, "Got a central server heartbeat packet from someone that isn't the central server %s", client->getIPString());
    myDataMutex.unlock();
    return;
  }
  if (packet->getPacketSource() == ArNetPacket::TCP)
    myLastTcpHeartbeat.setToNow();
  else if (packet->getPacketSource() == ArNetPacket::UDP)
    myLastUdpHeartbeat.setToNow();
  else
    ArLog::log(ArLog::Normal, 
       "Got unknown packet source for central server heartbeat packet");
  myDataMutex.unlock();
}

AREXPORT bool ArClientSwitchManager::parseFile(const char *fileName)
{
  ArLog::log(ArLog::Normal, "Loading central server user/password from %s", 
	     fileName);
  if (!myFileParser.parseFile(fileName))
  {
    
    ArLog::log(ArLog::Normal, "Failed parsing central server user/password file %s", 
	     fileName);
    return false;
  }
  return true;
}

bool ArClientSwitchManager::fileUserCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad user line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myUser = "";
  else
    myUser = arg->getArg(0);
  return true;
}


bool ArClientSwitchManager::filePasswordCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad password line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myPassword = "";
  else
    myPassword = arg->getArg(0);
  return true;
}

bool ArClientSwitchManager::fileServerKeyCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad serverKey line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myServerKey = "";
  else
    myServerKey = arg->getArg(0);
  return true;
}

AREXPORT const char *ArClientSwitchManager::getCentralServerHostName(void)
{
  if (myCentralServer.size() <= 0)
    return NULL;
  else
    return myCentralServer.c_str();
}

AREXPORT const char *ArClientSwitchManager::getIdentifier(void)
{
  if (myIdentifier.size() <= 0)
    return NULL;
  else
    return myIdentifier.c_str();
}

bool ArClientSwitchManager::processFile(void)
{
  myDataMutex.lock();
  if (myServerClient != NULL)
    myServerClient->setBackupTimeout(myServerBackupTimeout);

  // we only process this once, that's fine since changing it causes
  // the software to restart
  if (myConfigFirstProcess)
  {
    // if the config wants to connect to the central server and there
    // isn't already a central server set, use the one in the config
    if (myConfigConnectToCentralServer && 
	myCentralServer.empty() && myConfigCentralServer[0] != '\0')
    {
      myCentralServer = myConfigCentralServer;
      myState = TRYING_CONNECTION;
    }
    
    // if we have no identifier set, but the config has one, copy it
    // over
    if (myIdentifier.empty() && myConfigIdentifier[0] != '\0')
    {
      myIdentifier = myConfigIdentifier;
    }
    myConfigFirstProcess = false;
  }

  myDataMutex.unlock();
  return true;
}


AREXPORT void ArClientSwitchManager::addToConfig(
	const char *configSection, 
	const char *connectName, const char *connectDesc, 
	const char *addressName, const char *addressDesc)
{
  myConfigFirstProcess = true;

  if (myCentralServer.empty())
  {
    Aria::getConfig()->addParam(
	    ArConfigArg(connectName, &myConfigConnectToCentralServer,
			connectDesc),
	    configSection, ArPriority::ADVANCED, "Checkbox", 
	    ArConfigArg::RESTART_SOFTWARE);

    myConfigDisplayHint = "Visible:";
    myConfigDisplayHint += connectName;
    myConfigDisplayHint += "=true";
    
    Aria::getConfig()->addParam(
	    ArConfigArg(addressName, myConfigCentralServer,
			addressDesc,
			sizeof(myConfigCentralServer)),
	    configSection, ArPriority::ADVANCED, 
	    getConfigDisplayHint(),
	    ArConfigArg::RESTART_SOFTWARE);
  }

  if (myIdentifier.empty())
  {
    Aria::getConfig()->addParam(
	    ArConfigArg("Identifier", myConfigIdentifier,
			"The identifier to use for this robot...  After initial setup this should not be changed",
			sizeof(myConfigIdentifier)),
	    configSection, ArPriority::CALIBRATION, 
	    getConfigDisplayHint(),
	    ArConfigArg::RESTART_SOFTWARE);
  }
}


/// Enforces the that the server is using this protocol version
AREXPORT void ArClientSwitchManager::enforceProtocolVersion(const char *protocolVersion)
{
  myDataMutex.lock();
  if (protocolVersion != NULL)
    myEnforceProtocolVersion = protocolVersion;
  else
    myEnforceProtocolVersion = "";
  myDataMutex.unlock();
  ArLog::log(ArLog::Normal, "ArClientSwitchManager: New enforceProtocolVersionSet");

}

AREXPORT void ArClientSwitchManager::enforceType(ArServerCommands::Type type)
{
  myDataMutex.lock();
  myEnforceType = type;
  myDataMutex.unlock();
  ArLog::log(ArLog::Normal, "ArClientSwitchManager: New enforce type: %s", 
	     ArServerCommands::toString(type));
	     
}
