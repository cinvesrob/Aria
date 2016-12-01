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
#include "ArMapChanger.h"

#include <algorithm>
#include <iterator>

#include "ArFileParser.h"
#include "ArMapUtils.h"
#include "ArMD5Calculator.h"

//#define ARDEBUG_MAP_CHANGER
#ifdef ARDEBUG_MAP_CHANGER
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

const char *ArMapChanger::PROCESS_CHANGES_PACKET_NAME = 
                              "processMapChanges";

const char *ArMapChanger::PROCESS_ROBOT_CHANGES_PACKET_NAME = 
                              "processRobotMapObjectChange";

const char *ArMapChanger::CHANGES_IN_PROGRESS_PACKET_NAME = 
                              "mapChangesInProgress";
  
const char *ArMapChanger::ROBOT_CHANGES_COMPLETE_PACKET_NAME =
                              "robotMapObjectChangeComplete";

AREXPORT ArMapChanger::ArMapChanger(ArMapInterface *map) :
  myMap(map),
  myWorkingMap(NULL),
  myChangeDetails(NULL),
  //myInfoCount((map != NULL) ? map->getInfoCount() : 0),
  myInfoNames(),
  myServer(NULL),
  myClientSwitch(NULL),
  myIsServerClientInit(false),
  myClientMutex(),
  myClient(NULL),
  myClientInfoMutex(),
  myClientInfo(NULL),
  myInterleaveMutex(),
  myReadyForNextPacket(false),
  myIsWaitingForReturn(false),
  myIdleProcessingMutex(),
  myIsIdleProcessingPending(false),
  myPreWriteCBList(),
  myPostWriteCBList(),
  myChangeCBList(),
  myHandleChangePacketCB(this, &ArMapChanger::handleChangePacket),
  myHandleRobotReplyPacketCB(this, &ArMapChanger::handleRobotChangeReplyPacket),
  myHandleChangesInProgressPacketCB(this, &ArMapChanger::handleChangesInProgressPacket),
  myHandleReplyPacketCB(this, &ArMapChanger::handleChangeReplyPacket),
  myHandleIdleProcessingPacketCB(this, &ArMapChanger::handleIdleProcessingPacket),
  myClientShutdownCB(this, &ArMapChanger::handleClientShutdown)
{
  if (myMap != NULL) {
    myInfoNames = myMap->getInfoNames();
  }
} // end ctor

AREXPORT ArMapChanger::ArMapChanger(ArServerBase *server,
                                    ArMapInterface *map) :
  myMap(map),
  myWorkingMap(NULL),
  myChangeDetails(NULL),
 //myInfoCount((map != NULL) ? map->getInfoCount() : 0),
  myInfoNames(),
  myServer(server),
  myClientSwitch(NULL),
  myIsServerClientInit(false),
  myClientMutex(),
  myClient(NULL),
  myClientInfoMutex(),
  myClientInfo(NULL),
  myInterleaveMutex(),
  myReadyForNextPacket(false),
  myIsWaitingForReturn(false),
  myIdleProcessingMutex(),
  myIsIdleProcessingPending(false),
  myPreWriteCBList(),
  myPostWriteCBList(),
  myChangeCBList(),
  myHandleChangePacketCB(this, &ArMapChanger::handleChangePacket),
  myHandleRobotReplyPacketCB(this, &ArMapChanger::handleRobotChangeReplyPacket),
  myHandleChangesInProgressPacketCB(this, &ArMapChanger::handleChangesInProgressPacket),
  myHandleReplyPacketCB(this, &ArMapChanger::handleChangeReplyPacket),
  myHandleIdleProcessingPacketCB(this, &ArMapChanger::handleIdleProcessingPacket),
  myClientShutdownCB(this, &ArMapChanger::handleClientShutdown)
{
  if (myMap != NULL) {
    myInfoNames = myMap->getInfoNames();
  }
  if (myServer == NULL) {

    return;
  }
  myServer->addData(PROCESS_CHANGES_PACKET_NAME, 
                    "Applies changes to the map",
		                &myHandleChangePacketCB, 
                    "TODO", 
                    "TODO", 
                    "Map", 
                    "RETURN_SINGLE|IDLE_PACKET"); 
  
  myServer->addData(PROCESS_ROBOT_CHANGES_PACKET_NAME, 
                    "Applies changes originated by the robot to the map",
		                &myHandleChangePacketCB,  // Same handler
                    "Complex packet containing the map changes. See ArMapChangeDetails.", 
                    "None",
                    "Map", 
                    "RETURN_SINGLE|IDLE_PACKET"); 
 
  myServer->addData(CHANGES_IN_PROGRESS_PACKET_NAME, 
                    "Notifies clients that changes are being applied to the map",
		                NULL, 
                    "TODO", 
                    "TODO", 
                    "Map", 
                    "RETURN_NONE"); 

} // end ctor


AREXPORT ArMapChanger::ArMapChanger(ArClientSwitchManager *clientSwitch,
                                    ArServerBase *server,
                                    ArMapInterface *map) :
  myMap(map),
  myWorkingMap(NULL),
  myChangeDetails(NULL),
 //myInfoCount((map != NULL) ? map->getInfoCount() : 0),
  myInfoNames(),
  myServer(server),
  myClientSwitch(clientSwitch),
  myIsServerClientInit(false),
  myClientMutex(),
  myClient(NULL),
  myClientInfoMutex(),
  myClientInfo(NULL),
  myInterleaveMutex(),
  myReadyForNextPacket(false),
  myIsWaitingForReturn(false),
  myIdleProcessingMutex(),
  myIsIdleProcessingPending(false),
  myPreWriteCBList(),
  myPostWriteCBList(),
  myChangeCBList(),
  myHandleChangePacketCB(this, &ArMapChanger::handleChangePacket),
  myHandleRobotReplyPacketCB(this, &ArMapChanger::handleRobotChangeReplyPacket),
  myHandleChangesInProgressPacketCB(this, &ArMapChanger::handleChangesInProgressPacket),
  myHandleReplyPacketCB(this, &ArMapChanger::handleChangeReplyPacket),
  myHandleIdleProcessingPacketCB(this, &ArMapChanger::handleIdleProcessingPacket),
  myClientShutdownCB(this, &ArMapChanger::handleClientShutdown)
{
  if (myMap != NULL) {
    myInfoNames = myMap->getInfoNames();
  }
  if (myServer != NULL) {

    myServer->addData(PROCESS_CHANGES_PACKET_NAME, 
                      "Applies changes to the map",
                      &myHandleChangePacketCB, 
                      "TODO", 
                      "TODO", 
                      "Map", 
                      "RETURN_SINGLE|IDLE_PACKET"); 
    
    myServer->addData(CHANGES_IN_PROGRESS_PACKET_NAME, 
                      "Notifies clients that changes are being applied to the map",
                      NULL, 
                      "TODO", 
                      "TODO", 
                      "Map", 
                      "RETURN_NONE"); 
  }

  if ((myClientSwitch != NULL) && 
      (myClientSwitch->getCentralServerHostName() != NULL)) {

    myServer->addData(PROCESS_ROBOT_CHANGES_PACKET_NAME, 
                      "Applies ARCL/robot-originated changes to the map",
                      NULL,
                      "Complex packet containing the map changes. See ArMapChangeDetails.", 
                      "None",
                      "Map", 
                      "RETURN_SINGLE|IDLE_PACKET"); 
    
    myServer->addData(ROBOT_CHANGES_COMPLETE_PACKET_NAME,
                      "Handles notification that the ARCL/robot-originated changes are complete",
                      &myHandleRobotReplyPacketCB, 
                      "None",
                      "string: robot originating changes; uByte2: status (0 = failed, 10 = success)" ); 
  } // end if 

} // end ctor

AREXPORT ArMapChanger::ArMapChanger(ArClientBase *client,
                                    const std::list<std::string> &infoNames) :
  myMap(NULL),
  myWorkingMap(NULL),
  myChangeDetails(NULL),
  //myInfoCount(infoCount),
  myInfoNames(infoNames),
  myServer(NULL),
  myClientSwitch(NULL),
  myClientMutex(),
  myClient(client),
  myClientInfoMutex(),
  myClientInfo(NULL),
  myInterleaveMutex(),
  myReadyForNextPacket(false),
  myIsWaitingForReturn(false),
  myIdleProcessingMutex(),
  myIsIdleProcessingPending(false),
  myPreWriteCBList(),
  myPostWriteCBList(),
  myChangeCBList(),
  myHandleChangePacketCB(this, &ArMapChanger::handleChangePacket),
  myHandleChangesInProgressPacketCB(this, &ArMapChanger::handleChangesInProgressPacket),
  myHandleReplyPacketCB(this, &ArMapChanger::handleChangeReplyPacket),
  myHandleIdleProcessingPacketCB(this, &ArMapChanger::handleIdleProcessingPacket),
  myClientShutdownCB(this, &ArMapChanger::handleClientShutdown)
{
  // myMap is null, so don't do this
  // if (myMap != NULL) {
  //  myInfoNames = myMap->getInfoNames();
  //}

  myClientMutex.setLogName("ArMapChanger_ClientMutex");
  myClientInfoMutex.setLogName("ArMapChanger_ClientInfoMutex");

  if (myClient == NULL) {

    return;
  }
  myClientMutex.lock();

  if (myClient && myClient->dataExists(PROCESS_CHANGES_PACKET_NAME)) {

    ArLog::log(ArLog::Normal,
               "ArMapChanger::ctor() server supports map changes");

    myClient->addHandler(PROCESS_CHANGES_PACKET_NAME,
                         &myHandleReplyPacketCB);

  } // end if processMapChanges available
  
  if (myClient && myClient->dataExists(CHANGES_IN_PROGRESS_PACKET_NAME)) {
  
    ArLog::log(ArLog::Normal,
              "ArMapChanger::ctor() server notifies of map changes");

    myClient->addHandler(CHANGES_IN_PROGRESS_PACKET_NAME,
                         &myHandleChangesInProgressPacketCB);

    myClient->requestOnce(CHANGES_IN_PROGRESS_PACKET_NAME);
    myClient->request(CHANGES_IN_PROGRESS_PACKET_NAME, -1);
  }

  if (myClient && myClient->dataExists("idleProcessingPending")) {
  
    ArLog::log(ArLog::Normal,
              "ArMapChanger::ctor() server notifies of idle processing");

    myClient->addHandler("idleProcessingPending",
                         &myHandleIdleProcessingPacketCB);

    myClient->requestOnce("idleProcessingPending");
    myClient->request("idleProcessingPending", -1);
  }
  if (myClient) {
    myClient->addServerShutdownCB(&myClientShutdownCB);
    myClient->addDisconnectOnErrorCB(&myClientShutdownCB);
  }
  myClientMutex.unlock();

} // end ctor


AREXPORT ArMapChanger::~ArMapChanger()
{
  if (myClient != NULL) {
    myClient->remHandler(PROCESS_CHANGES_PACKET_NAME,
                         &myHandleReplyPacketCB);
    myClient->remHandler(CHANGES_IN_PROGRESS_PACKET_NAME,
                         &myHandleChangesInProgressPacketCB);
    myClient->remHandler("idleProcessingPending",
                         &myHandleIdleProcessingPacketCB);
    myClient->remServerShutdownCB(&myClientShutdownCB);
    myClient->remDisconnectOnErrorCB(&myClientShutdownCB);
  }
}

AREXPORT bool ArMapChanger::sendMapChanges(ArMapChangeDetails *changeDetails)
{
  if (changeDetails == NULL) {
    return false;
  }
  if (myClient == NULL) {
    return false;
  }

  bool isInterleaved = true;
  myInterleaveMutex.lock();
  if (myIsWaitingForReturn)
  {
    ArLog::log(ArLog::Terse, 
	             "ArMapChanger::sendMapChanges() already busy sending changes");
    myInterleaveMutex.unlock();
    return false;
  }
  myIsWaitingForReturn = true;
  myReadyForNextPacket = false;
  ArLog::log(ArLog::Normal, 
	             "ArMapChanger::sendMapChanges() set myIsWaitingForReturn = true && myReadyForNextPacket = false");
   myInterleaveMutex.unlock();



  ArMapId givenOrigMapId;
  ArMapId givenNewMapId;

  changeDetails->getOrigMapId(&givenOrigMapId);
  changeDetails->getNewMapId(&givenNewMapId);


  // A giant hack to strip out the source name if we're sending the
  // changes back to the original source.
  myClientMutex.lock();
  const char *robotName = ((myClient != NULL) ? myClient->getRobotName() : "");
  myClientMutex.unlock();

  if (!ArUtil::isStrEmpty(robotName) && 
      !ArUtil::isStrEmpty(givenOrigMapId.getSourceName())) {
    if (ArUtil::strcasecmp(robotName, givenOrigMapId.getSourceName()) == 0) {
      ArMapId modOrigMapId = givenOrigMapId;
      modOrigMapId.setSourceName(NULL);
      modOrigMapId.setTimestamp(-1);
      changeDetails->setOrigMapId(modOrigMapId);
    }
  }
  if (!ArUtil::isStrEmpty(robotName) && 
      !ArUtil::isStrEmpty(givenNewMapId.getSourceName())) {
    if (ArUtil::strcasecmp(robotName, givenNewMapId.getSourceName()) == 0) {
      ArMapId modNewMapId = givenNewMapId;
      modNewMapId.setSourceName(NULL);
      modNewMapId.setTimestamp(-1);
      changeDetails->setNewMapId(modNewMapId);
    }
  }


  // TODO Probably just want to send each packet as created...
  std::list<ArNetPacket*> packetList;
  bool isSuccess = convertChangeDetailsToPacketList(changeDetails,
                                                    &packetList);


  if (!isSuccess) {

    ArLog::log(ArLog::Normal,
               "ArMapChanger::sendMapChanges() error converting change details to network packets");
    return false;
  }
  
  isSuccess = sendPacketList(packetList);

  changeDetails->setOrigMapId(givenOrigMapId);
  changeDetails->setNewMapId(givenNewMapId);

  ArLog::log(ArLog::Normal,
             "ArMapChanger::sendMapChanges() changes sent (%i), deleting packet list...",
             isSuccess);

  ArUtil::deleteSet(packetList.begin(), packetList.end());
  packetList.clear();


  return isSuccess;

} // end method sendMapChanges
  

/// Sends the given map changes from the robot to the central server.
AREXPORT bool ArMapChanger::sendRobotMapChanges(ArMapChangeDetails *changeDetails)
{
  // KMC TODO This looks a lot like sendMapChanges and should be refactored
  // (presuming it works)

  if (changeDetails == NULL) {
    return false;
  }
  if ((myClientSwitch == NULL) ||
      (myClientSwitch->getServerClient() == NULL)) {
    return false;
  }


  bool isInterleaved = true;
  myInterleaveMutex.lock();
  if (myIsWaitingForReturn)
  {
    ArLog::log(ArLog::Terse, 
	             "ArMapChanger: already busy sending changes");
    myInterleaveMutex.unlock();
    return false;
  }
  myIsWaitingForReturn = true;
  myReadyForNextPacket = false;
  ArLog::log(ArLog::Normal, 
	             "ArMapChanger::sendRobotMapChanges() set myIsWaitingForReturn = true && myReadyForNextPacket = false");
  myInterleaveMutex.unlock();



  ArMapId givenOrigMapId;
  ArMapId givenNewMapId;

  changeDetails->getOrigMapId(&givenOrigMapId);
  changeDetails->getNewMapId(&givenNewMapId);


  // A giant hack to strip out the source name if we're sending the
  // changes back to the original source.
  myClientMutex.lock();
  std::string robotName = ((myClientSwitch != NULL) ? myClientSwitch->getIdentifier() : "");
  myClientMutex.unlock();

  if (!ArUtil::isStrEmpty(robotName.c_str()) && 
      !ArUtil::isStrEmpty(givenOrigMapId.getSourceName())) {
    if (ArUtil::strcasecmp(robotName.c_str(), givenOrigMapId.getSourceName()) == 0) {
      ArMapId modOrigMapId = givenOrigMapId;
      modOrigMapId.setSourceName(NULL);
      modOrigMapId.setTimestamp(-1);
      changeDetails->setOrigMapId(modOrigMapId);
    }
  }
  if (!ArUtil::isStrEmpty(robotName.c_str()) && 
      !ArUtil::isStrEmpty(givenNewMapId.getSourceName())) {
    if (ArUtil::strcasecmp(robotName.c_str(), givenNewMapId.getSourceName()) == 0) {
      ArMapId modNewMapId = givenNewMapId;
      modNewMapId.setSourceName(NULL);
      modNewMapId.setTimestamp(-1);
      changeDetails->setNewMapId(modNewMapId);
    }
  }


  // TODO Probably just want to send each packet as created...
  std::list<ArNetPacket*> packetList;
  bool isSuccess = convertChangeDetailsToPacketList(changeDetails,
                                                    &packetList);


  if (!isSuccess) {

    ArLog::log(ArLog::Normal,
               "ArMapChanger::sendRobotMapChanges() error converting change details to network packets");
    return false;
  }
  
  isSuccess = sendRobotPacketList(packetList);

  changeDetails->setOrigMapId(givenOrigMapId);
  changeDetails->setNewMapId(givenNewMapId);

  ArLog::log(ArLog::Normal,
             "ArMapChanger::sendMapChanges() changes sent (%i), deleting packet list...",
             isSuccess);

  ArUtil::deleteSet(packetList.begin(), packetList.end());
  packetList.clear();


  return isSuccess;

} // end method sendRobotMapChanges


bool ArMapChanger::sendPacketList(const std::list<ArNetPacket *> &packetList) {
 
  bool isInterleaved = true;

  ArTime started;
  started.setToNow();

  for (std::list<ArNetPacket*>::const_iterator iter = packetList.begin();
       iter != packetList.end();
       iter++) {

    started.setToNow();

    ArNetPacket *packet = *iter;
    if (packet == NULL) {
      continue;
    }
    myClientMutex.lock();
    if (myClient != NULL) {
      myClient->requestOnce(PROCESS_CHANGES_PACKET_NAME,
                            packet);
    }
    else {
      ArLog::log(ArLog::Terse, 
                 "ArMapChanger::sendPacketList aborted because connection lost");
      myClientMutex.unlock();
      return false;
   }

    myClientMutex.unlock();

    /***
    if (isInterleaved) {
      if (!waitForReply(started)) {
        return false;
      }
    } // end if interleaved
     ***/
  } // end for each packet

  if (!packetList.empty()) {
    if (isInterleaved) {
      if (!waitForReply(started)) {
        return false;
      }
    } // end if interleaved
  }

  return true;
  
} // end method sendPacketList



bool ArMapChanger::sendRobotPacketList(const std::list<ArNetPacket *> &packetList) {
 
  bool isInterleaved = true;
  ArLog::log(ArLog::Normal,
             "ArMapChanger::sendRobotPacketList() count = %i", packetList.size());

  ArTime started;
  started.setToNow();

  for (std::list<ArNetPacket*>::const_iterator iter = packetList.begin();
       iter != packetList.end();
       iter++) {

    started.setToNow();

    ArNetPacket *packet = *iter;
    if (packet == NULL) {
      continue;
    }
    myClientMutex.lock();
    if ((myClientSwitch != NULL) &&
        (myClientSwitch->getServerClient() != NULL)) {
      
      unsigned int mapChangesCommand = myClientSwitch->getServerClient()->
                                          findCommandFromName(PROCESS_ROBOT_CHANGES_PACKET_NAME);

      packet->setCommand(mapChangesCommand);

      myClientSwitch->getServerClient()->broadcastPacketTcp(packet);

//                                                            PROCESS_ROBOT_CHANGES_PACKET_NAME);
    }
    else {
      ArLog::log(ArLog::Terse, 
                 "ArMapChanger::sendRobotPacketList aborted because connection lost");
      myClientMutex.unlock();
      return false;
   }

    myClientMutex.unlock();

    /***
    if (isInterleaved) {
      if (!waitForReply(started)) {
        return false;
      }
    } // end if interleaved
     ***/
  } // end for each packet

/*** KMC Don't actually want to wait for the packet
  if (!packetList.empty()) {
    if (isInterleaved) {
      if (!waitForCentralServerReply(started)) {
        return false;
      }
    } // end if interleaved
  }
***/
  return true;
  
} // end method sendRobotPacketList



bool ArMapChanger::waitForReply(ArTime &started)
{
  myInterleaveMutex.lock();

  // myReadyForNextPacket is set to true when the response is received
  // from the server.
  while (!myReadyForNextPacket) 
  {
	  if (!myIsWaitingForReturn)
	  {
	    ArLog::log(ArLog::Normal, 
                  "ArMapChanger::waitForReply: Put was cancelled or failed.");
	    myInterleaveMutex.unlock();
	    return false;
	  }
	  myInterleaveMutex.unlock();


    myClientMutex.lock();

    if (myClient == NULL) {
	    ArLog::log(ArLog::Normal, 
                  "ArMapChanger::waitForReply: Client was shutdown.");
      myClientMutex.unlock();
      return false;
    }
    myClientMutex.unlock();

	  ArUtil::sleep(1);

    if (!isIdleProcessingPending()) {
	    if (started.secSince() > 30)
	    {
        myInterleaveMutex.lock();
        ArLog::log(ArLog::Normal, 
	                 "ArMapChanger::waitForReply() set myIsWaitingForReturn = false");
        myIsWaitingForReturn = false;
        myInterleaveMutex.unlock();

	      ArLog::log(ArLog::Normal, 
                  "ArMapChanger::waitForReply: No return from client within 30 seconds, failing put.");	 
	      return false;
	    }
    }
    else {
      started.setToNow();
    }
	  myInterleaveMutex.lock();
	  
  } // end while not ready for next packet

  // Reset the flag so we'll wait for a response during the next loop iteration.
  myReadyForNextPacket = false;
  ArLog::log(ArLog::Normal, 
	             "ArMapChanger::waitForReply() set myReadyForNextPacket = false");

  myInterleaveMutex.unlock(); 

  return true;

} // end method waitForReply


bool ArMapChanger::waitForCentralServerReply(ArTime &started)
{
  ArLog::log(ArLog::Normal,
             "ArMapChanger::waitForCentralServerReply() called");

  myInterleaveMutex.lock();

  // myReadyForNextPacket is set to true when the response is received
  // from the server.
  while (!myReadyForNextPacket) 
  {
	  if (!myIsWaitingForReturn)
	  {
	    ArLog::log(ArLog::Normal, 
                  "ArMapChanger::waitForCentralServerReply: Put was cancelled or failed.");
	    myInterleaveMutex.unlock();
	    return false;
	  }
	  myInterleaveMutex.unlock();

/*** KMC TODO
    myClientMutex.lock();

    if (myClient == NULL) {
	    ArLog::log(ArLog::Normal, 
                  "ArMapChanger::waitForCentralServerReply: Client was shutdown.");
      myClientMutex.unlock();
      return false;
    }
    myClientMutex.unlock();
******/

	  ArUtil::sleep(1);

    if (true || !isIdleProcessingPending()) {
	    if (started.secSince() > 30)
	    {
        myInterleaveMutex.lock();
        myIsWaitingForReturn = false;
        ArLog::log(ArLog::Normal,
                  "ArMapChanger::waitForCentralServerReply() set myIsWaitingForReturn = false (timeout)");
        myInterleaveMutex.unlock();

	      ArLog::log(ArLog::Normal, 
                  "ArMapChanger::waitForCentralServerReply: No return from client within 30 seconds, failing put.");	 
	      return false;
	    }
    }
    else {
      started.setToNow();
    }
	  myInterleaveMutex.lock();
	  
  } // end while not ready for next packet

  // Reset the flag so we'll wait for a response during the next loop iteration.
  ArLog::log(ArLog::Normal,
             "ArMapChanger::waitForCentralServerReply() set myReadyForNextPacket = false");
  myReadyForNextPacket = false;

  myInterleaveMutex.unlock(); 

  return true;

} // end method waitForCentralServerReply

bool ArMapChanger::isIdleProcessingPending()
{
  myIdleProcessingMutex.lock();
  bool isPending = myIsIdleProcessingPending;
  myIdleProcessingMutex.unlock();

  return isPending;

} // end method isIdleProcessingPending


AREXPORT void ArMapChanger::handleChangeReplyPacket(ArNetPacket *packet)
{ 
  IFDEBUG(ArLog::log(ArLog::Normal,
                     "ArMapChanger::handleChangeReplyPacket()"));
 
  myInterleaveMutex.lock();
  if (!myIsWaitingForReturn)
  {
    myInterleaveMutex.unlock();
    return;
  }
  
  int ret = packet->bufToUByte2();

  // packet->bufToStr(fileName, sizeof(fileName));
  // if (myInterleaved && ret == 10)
  if (ret == CHANGE_SUCCESS)
  {
    myReadyForNextPacket = true;
    ArLog::log(ArLog::Normal,
               "ArMapChanger::handleChangeReplyPacket() set myReadyForNextPacket = false");

  }
  else
  {
    myIsWaitingForReturn = false;
    ArLog::log(ArLog::Normal,
               "ArMapChanger::handleChangeReplyPacket() set myIsWaitingForReturn = false");
  }
  myInterleaveMutex.unlock();

  //if (done)
  //  callFileSentCallbacks(ret);
} // end method handleChangeReplyPacket


AREXPORT void ArMapChanger::handleChangesInProgressPacket(ArNetPacket *packet)
{
  if (packet == NULL) {
    return;
  }
  bool isInProgress = (packet->bufToUByte() != 0);

  ArLog::log(ArLog::Normal,
             "ArMapChanger::handleChangesInProgressPacket() in progress = %i",
             isInProgress);
             
} // end method handleChangesInProgressPacket


AREXPORT void ArMapChanger::handleIdleProcessingPacket(ArNetPacket *packet)
{
  if (packet == NULL) {
    return;
  }
	unsigned char isPending = packet->bufToByte();

  ArLog::log(ArLog::Normal,
             "ArMapChanger::handleIdleProcessingPacket() idleProcessingPending = %i",
             isPending);
 
  myIdleProcessingMutex.lock();
  myIsIdleProcessingPending = isPending;
  myIdleProcessingMutex.unlock();
           
} // end method handleIdleProcessingPacket


AREXPORT void ArMapChanger::handleChangePacket(ArServerClient *client, 
                                               ArNetPacket *packet)
{
  if (packet == NULL) {
    return;
  }

  if (myMap == NULL) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::handleChangePacket no map to change");
    return;
  }

  ArNetPacket replyPacket;
  bool isChangesStarted = false;
  bool isChangesFinished = false;
  
  ArMapId thisMapId;
  myMap->getMapId(&thisMapId);

  MapChangeCommand command;
  ArMapId origMapId;
  ArMapId newMapId;

  bool isSuccess = unpackHeader(packet, &command, &origMapId, &newMapId);
  
  if (!isSuccess) {
    ArLog::log(ArLog::Terse,
               "ArMapChanger::handleChangePacket() cannot unpack header");

    replyPacket.uByte2ToBuf(CHANGE_FAILED);
    client->sendPacketTcp(&replyPacket);

    return;

  }

  // KMC TODO Figure out appropriate "same map" check for deltas from robot
  bool isSameMap = true;

  if (!origMapId.isNull()) {
    isSameMap = thisMapId.isSameFile(origMapId);

    if (isSameMap) {
      if (ArUtil::strcasecmp(thisMapId.getFileName(), 
                             newMapId.getFileName()) != 0) {
        // Hopefully this won't happen, but make sure that the client isn't 
        // renaming the map file.
        isSameMap = false;
      }
    }
  } // end if same map

  // TODO: Not sure how to handle this... Right now just allowing changes
  // on current map.  But may want a big map manager thing later.
  if (!isSameMap) {


    // In the case of changes propagated from the central server, no one
    // will be waiting for the replies -- and so the FINISH_CHANGES message
    // will be received.  Send this server's map ID to the central server
    // so that the CS can refresh the file if necessary.  (Most likely in
    // the case in which the CS and the robot have different 
    // AramMapInfoMinder's.)
    if (command == FINISH_CHANGES) {

      ArLog::log(ArLog::Normal,
                "ArMapChanger::handleChangePacket() packet received for other map");
      thisMapId.log("This Map");
      origMapId.log("Received Map - Original");
      newMapId.log("Received Map - New");

      replyPacket.uByte2ToBuf(CHANGE_FAILED);
      client->sendPacketTcp(&replyPacket);

      // Have no idea whether this is going to work...

      ArNetPacket mapIdPacket;

      unsigned int mapIdCommand = client->findCommandFromName("getMapId");

      mapIdPacket.setCommand(mapIdCommand);
      ArMapId::toPacket(thisMapId,
                        &mapIdPacket);

      ArLog::log(ArLog::Normal,
                 "ArMapChanger::internalChangeMap() packet sending map Id packet command = %i",
                 mapIdCommand);
      
      client->sendPacketTcp(&mapIdPacket);

    }

    return;

  } // end if not same map


  switch (command) {

  case START_CHANGES:
    {
      myClientInfoMutex.lock();
      // if other client making changes then
      if (myClientInfo != NULL) {
      //   if other client has timed out then
        if (myClientInfo->myLastActivityTime.secSince() > 15) {
          // TODO notify other client that its being dumped

          delete myClientInfo;
          myClientInfo = NULL;
        }
        else {

          // TODO notify this client that it can't change yet

          myClientInfoMutex.unlock();
          return;
        } // end else other client has not timed out
      } // end if other client is currently changing same map

      // setup changes for this client
      myClientInfo = new ClientChangeInfo(client);
        
      myClientInfo->addPacket(packet);

      isChangesStarted = true;
      

      myClientInfoMutex.unlock();


    }
    break;

  case CONTINUE_CHANGES:
    {

      // append changes for this client
      myClientInfoMutex.lock();
      if ((myClientInfo != NULL) && (myClientInfo->myClient == client)) {

        // TODO: Not entirely sure about this... We could build up the 
        // change details and then apply them...

        // Save a copy of the packet... Want to receive all changes before applying them

        // TODO The copy ctor for ArNetPacket should be disabled if it's not going to work
        myClientInfo->addPacket(packet);
      }
      else {
        
        // notify the client that something has gone terribly wrong
      }
      myClientInfoMutex.unlock();
   }
    break;
  
  case FINISH_CHANGES:
    {
      // apply changes for this client
      myClientInfoMutex.lock();

      if ((myClientInfo != NULL) && (myClientInfo->myClient == client)) {

        // TODO: Probably need to do the apply in a separate thread and have a 
        // success/fail callback
        
        myClientInfo->addPacket(packet);
  
        ArMapChangeDetails *changeDetails = new ArMapChangeDetails();

        isSuccess = convertPacketListToChangeDetails(myClientInfo->myPacketList,
                                                     changeDetails);

        changeDetails->log();

        ArMapId newMapId;
        changeDetails->getNewMapId(&newMapId);
        bool isRepackageChanges = (!newMapId.isValidTimestamp());
       

        if (isSuccess) {

          isSuccess = applyMapChanges(changeDetails);

          if (isSuccess) {

            ArLog::log(ArLog::Normal,
                      "ArMapChanger::internalChangeMap Change details successfully applied to map");
          
          }
          else { // error applying map changes
            ArLog::log(ArLog::Normal,
                      "ArMapChanger::internalChangeMap Error applying change details to map");
          } // end else error applying map changes
        } 
        else { // error converting packet list
          ArLog::log(ArLog::Normal,
                    "ArMapChanger::internalChangeMap Error converting packet list to map change details");
        } // end else error converting packet list


        if (isSuccess && !myChangeCBList.empty()) {

          std::list<ArNetPacket *> relayPacketList;
          if (!isRepackageChanges) {
            ArLog::log(ArLog::Normal,
                       "ArMapChanger::internalChangeMap Relaying original change packets");
            relayPacketList = myClientInfo->myPacketList;
          }
          else {
            ArLog::log(ArLog::Normal,
                       "ArMapChanger::internalChangeMap Recreating change packets");
            convertChangeDetailsToPacketList(changeDetails,
                                             &relayPacketList,
                                             true);
          }

          for (std::list< ArFunctor2<ArServerClient *, 
                                     std::list<ArNetPacket *> *> *>::iterator cbIter = 
                   myChangeCBList.begin();
               cbIter != myChangeCBList.end();
               cbIter++) {

            ArFunctor2<ArServerClient *, std::list<ArNetPacket *> *> *functor =
                                                  *cbIter;

            resetPacketList(&myClientInfo->myPacketList);

            functor->invoke(myClientInfo->myClient, 
                            &relayPacketList);
          }

        } // end if changes successfully applied and callbacks


        delete changeDetails;

        delete myClientInfo;
        myClientInfo = NULL;

      }
      else {
        
        // notify the client that something has gone terribly wrong
        ArLog::log(ArLog::Normal,
                   "ArMapChanger::handleChangePacket() cannot finish changes from other client");
        isSuccess = false;

      }
      
      isChangesFinished = true;

      myClientInfoMutex.unlock();
    }
    break;

  case CANCEL_CHANGES:
    {
      // cancel changes for this client
      // reset
      // apply changes for this client
       myClientInfoMutex.lock();
      if ((myClientInfo != NULL) && (myClientInfo->myClient == client)) {
        delete myClientInfo;
        myClientInfo = NULL;
        ArLog::log(ArLog::Normal,
                   "ArMapChanger::handleChangePacket() cancelling changes");
      }
      else {
        
        // notify the client that something has gone terribly wrong
        ArLog::log(ArLog::Normal,
                   "ArMapChanger::handleChangePacket() cannot cancel changes from other client");
        isSuccess = false;
      }
      
      isChangesFinished = true;
                                  
      myClientInfoMutex.unlock();
    }
    break;

  } // end switch


  if ((isChangesStarted) && (myServer != NULL)) {
    // Broadcast that changes are in progress in order to prevent others from 
    // applying changes during this time.
    //
    ArNetPacket inProgressPacket;
    inProgressPacket.uByteToBuf(true);
    myServer->broadcastPacketTcp(&inProgressPacket,
                                 CHANGES_IN_PROGRESS_PACKET_NAME);
    
  }

  if (isChangesFinished) {
    if (isSuccess) {
      replyPacket.uByte2ToBuf(CHANGE_SUCCESS);
    }
    else {
      replyPacket.uByte2ToBuf(CHANGE_FAILED);
      // TODO Think about adding an error message
    }
    client->sendPacketTcp(&replyPacket);
 
 
    if (myServer != NULL) {
      // Broadcast that changes are no longer in progress.
      //
      ArNetPacket inProgressPacket;
      inProgressPacket.uByteToBuf(false);
      myServer->broadcastPacketTcp(&inProgressPacket,
                                  CHANGES_IN_PROGRESS_PACKET_NAME);
    }
  } // end if 

} // end method handleChangePacket


AREXPORT void ArMapChanger::handleClientShutdown()
{
  ArLog::log(ArLog::Normal,
             "ArMapChanger::handleClientShutdown() received");

  // Doing this so that we don't attempt to remove the handlers in the destructor --
  // which wreaks havoc if the client base has already been deleted.

  myClientMutex.lock();
  myClient = NULL;
  myClientMutex.unlock();

} // end method handleClientShutdown


void ArMapChanger::resetPacketList(std::list<ArNetPacket*> *packetList) 
{
  if (packetList == NULL) {
    return;
  }

  for (std::list<ArNetPacket*>::iterator iter = packetList->begin();
       iter != packetList->end();
       iter++) {
    ArNetPacket *packet = *iter;
    if (packet == NULL) {
      continue;
    }
    packet->resetRead();
  }

} // end method packetList

/***
bool ArMapChanger::applyMapChanges(std::list<ArNetPacket*> &packetList)
{
  ArMapChangeDetails *changeDetails = new ArMapChangeDetails();

  bool isSuccess = convertPacketListToChangeDetails(packetList,
                                                    changeDetails);

  if (isSuccess) {
    isSuccess = applyMapChanges(changeDetails);

    if (isSuccess) {

      ArLog::log(ArLog::Normal,
                "Change details successfully applied to map");
    
    }
    else { // error applying map changes
      ArLog::log(ArLog::Normal,
                "Error applying change details to map");
    } // end else error applying map changes
  } 
  else { // error converting packet list
    ArLog::log(ArLog::Normal,
               "Error converting packet list to map change details");
  } // end else error converting packet list

  changeDetails->log();

  delete changeDetails;

  return isSuccess;
}
***/

AREXPORT bool ArMapChanger::applyMapChanges(ArMapChangeDetails *changeDetails)
{

  if (changeDetails == NULL) {
    return false;
  }
  if (myMap == NULL) {
    return false;
  }

  myMap->lock();

  ArLog::log(ArLog::Normal,
             "ArMapChanger::applyMapChanges() begin");

  myWorkingMap = myMap->clone();

  bool isSuccess = true;

  // --------------------------------------------------------------------------
  // Apply Scan changes...
  //
  isSuccess = applyScanChanges(changeDetails) && isSuccess;

  // --------------------------------------------------------------------------
  // Apply Supplement changes...
  //
  isSuccess = applySupplementChanges(changeDetails) && isSuccess;

  // --------------------------------------------------------------------------
  // Apply Object changes...
  //
  isSuccess = applyObjectChanges(changeDetails) && isSuccess;

  // --------------------------------------------------------------------------
  // Apply Info changes...
  //
  isSuccess = applyInfoChanges(changeDetails) && isSuccess;
  

  ArMapId mapId;
  ArMapId changesMapId;
  ArMapId newMapId;
  
  myMap->getMapId(&mapId);
    
    

  if (isSuccess) {

    changeDetails->getNewMapId(&changesMapId);
  
    unsigned char tempChecksum[ArMD5Calculator::DIGEST_LENGTH];
    myWorkingMap->calculateChecksum(tempChecksum, 
                                    ArMD5Calculator::DIGEST_LENGTH);

    if ((changesMapId.getChecksum () != NULL) &&
        (memcmp(changesMapId.getChecksum(), 
               tempChecksum,
               ArMD5Calculator::DIGEST_LENGTH) != 0)) {

      char tempChecksumDisplay[ArMD5Calculator::DISPLAY_LENGTH];
      ArMD5Calculator::toDisplay(tempChecksum,
                                 ArMD5Calculator::DIGEST_LENGTH,
                                 tempChecksumDisplay,
                                 ArMD5Calculator::DISPLAY_LENGTH);

      // Different checksums
      ArLog::log(ArLog::Normal,
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\
Map Checksums different after applying changes\n\
   From Client: %s\n\
   Calculated : %s\n\
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
                 changesMapId.getDisplayChecksum(),
                 tempChecksumDisplay);

      isSuccess = false;

      /***
      // Re-insert this to debug why map changes weren't applied 
      // successfully.  (Run diff on MapChangeError.map and robot's 
      // current map.)
      bool isWriteBadMap = true;

      if (isWriteBadMap) {
        myWorkingMap->writeFile("MapChangeError.map");
      }
      ***/

    } // end if different checksums

  } // end if changes successfully applied

  if (isSuccess) {

    myMap->set(myWorkingMap);

    ArLog::log(ArLog::Normal,
               "Successfully applied changes to map %s, saving file",
               mapId.getFileName());

    std::list<ArFunctor*>::iterator iter = myPreWriteCBList.begin();

    for (iter = myPreWriteCBList.begin(); 
         iter != myPreWriteCBList.end(); 
         iter++) {
      myMap->addPreWriteFileCB(*iter);
    }
    for (iter = myPostWriteCBList.begin(); 
         iter != myPostWriteCBList.end(); 
         iter++) {
      myMap->addPostWriteFileCB(*iter);
    }

    isSuccess = myMap->writeFile(mapId.getFileName(), 
                                 true,
                                 NULL, 0,
                                 changesMapId.getTimestamp());

    for (iter = myPreWriteCBList.begin(); 
         iter != myPreWriteCBList.end(); 
         iter++) {
      myMap->remPreWriteFileCB(*iter);
    }
    for (iter = myPostWriteCBList.begin(); 
         iter != myPostWriteCBList.end(); 
         iter++) {
      myMap->remPostWriteFileCB(*iter);
    }

    myMap->getMapId(&newMapId);

    if (changesMapId.isNull() || (changesMapId == newMapId)) {
      // This will actually insert the timestamp into the map ID 
      changeDetails->setNewMapId(newMapId);

    }
    else {

      ArLog::log(ArLog::Normal,
"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\
 Map IDs different after applying changes\n\
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
      changesMapId.log("Changes Map ID");
      newMapId.log("New Map ID");

      isSuccess = false;

    }
  } // end if successful

  if (isSuccess) {

    ArLog::log(ArLog::Normal,
               "Successfully saved file for map %s, emitting mapChanged",
               mapId.getFileName());

    myMap->mapChanged(); // ??? TODO
  }
  else { // error occurred

    ArLog::log(ArLog::Terse,
               "An error occurred while applying changes to map %s",
               mapId.getFileName());

    // TODO Notify client of error (and hopefully get full map file)
  
  } // end else error occurred

  delete myWorkingMap;
  myWorkingMap = NULL;

  myMap->unlock();

  ArLog::log(ArLog::Normal,
             "ArMapChanger::applyMapChanges() end");

  return isSuccess;

} // end method applyMapChanges


bool ArMapChanger::applyScanChanges(ArMapChangeDetails *changeDetails)
{
  if ((changeDetails == NULL) || (myWorkingMap == NULL)) {
    return false;
  }

  std::list<std::string> *scanTypeList = changeDetails->getScanTypes();

  if ((scanTypeList == NULL) || (scanTypeList->empty())) {
    return true;
  }
  ArFileParser parser;
  bool isSuccess = myWorkingMap->addToFileParser(&parser);

  for (std::list<std::string>::iterator iter = scanTypeList->begin();
       (isSuccess && (iter != scanTypeList->end()));
       iter++) {

    const char *scanType = (*iter).c_str();
    isSuccess = applyScanChanges(changeDetails, scanType, parser);
  }

  myWorkingMap->remFromFileParser(&parser);

  return isSuccess;

} // end for each scan type


bool ArMapChanger::applyScanChanges(ArMapChangeDetails *changeDetails,
                                    const char *scanType,
                                    ArFileParser &parser)
{

  ArLog::log(ArLog::Normal,
             "ArMapChanger::applyScanChanges() for scan type %s",
             scanType);

  ArMapFileLineSet *newSummaryLines   = changeDetails->getChangedSummaryLines
                                                (ArMapChangeDetails::ADDITIONS,
                                                 scanType);
  if (newSummaryLines == NULL) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::applyScanChanges() for scan type %s, error getting added summary lines",
               scanType);
    return false;
  }

  bool isSuccess = true;

  char buf[10000];

  for (ArMapFileLineSet::iterator iter = newSummaryLines->begin();
       iter != newSummaryLines->end();
       iter++) {
    ArMapFileLineGroup &group = *iter;
    strncpy(buf, group.getParentLine()->getLineText(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    ArLog::log(ArLog::Normal,
               "Parsing summary line: %s",
               buf);
    
    isSuccess = parser.parseLine(buf) && isSuccess;
    if (!isSuccess) {

      ArLog::log(ArLog::Normal,
                 "Error parsing summary line: %s",
                 group.getParentLine()->getLineText());
    }

  } // end for each new summary line

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Process point changes...
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  std::vector<ArPose> *deletedPoints = changeDetails->getChangedPoints
                                              (ArMapChangeDetails::DELETIONS,
                                               scanType);
  std::vector<ArPose> *addedPoints   = changeDetails->getChangedPoints
                                              (ArMapChangeDetails::ADDITIONS,
                                               scanType);

  std::vector<ArPose> *mapPoints = myWorkingMap->getPoints(scanType);

  if ((deletedPoints == NULL) || (addedPoints == NULL) || (mapPoints == NULL)) {
    return false;
  }
  ArLog::log(ArLog::Normal,
             "ArMapChanger::applyScanChanges() for scan type %s initial points: %i  deletedPoints: %i  addedPoints: %i, numPointsInMap: %i",
             scanType, 
             mapPoints->size(),
             deletedPoints->size(),
             addedPoints->size(),
             myWorkingMap->getNumPoints());
 
	std::sort(mapPoints->begin(), mapPoints->end());
	std::sort(deletedPoints->begin(), deletedPoints->end());
	std::sort(addedPoints->begin(), addedPoints->end());
  
  /****/
  // Handle the simple but unlikely case that all points were deleted
  if (deletedPoints->size() >= mapPoints->size()) {
    mapPoints->clear();
    *mapPoints = *addedPoints;
  }
  else if (deletedPoints->empty()) {
   if (!addedPoints->empty()) {
      std::vector<ArPose> tempPoints;
      tempPoints.reserve(myWorkingMap->getNumPoints(scanType) + addedPoints->size());
      merge(mapPoints->begin(), mapPoints->end(),
            addedPoints->begin(), addedPoints->end(),
            std::inserter(tempPoints, 
                     tempPoints.begin()));
      *mapPoints = tempPoints;
    } // end if points were added (but not deleted)
  }
  else { // some but not all points were deleted 

    ArTime timeToDelete;
    std::vector<ArPose> tempPoints;
    tempPoints.reserve(myWorkingMap->getNumPoints(scanType));

    set_difference(mapPoints->begin(), mapPoints->end(), 
                   deletedPoints->begin(), deletedPoints->end(),
                   std::inserter(tempPoints, 
                            tempPoints.begin()));
    long int elapsed = timeToDelete.mSecSince();

    ArLog::log(ArLog::Normal,
               "ArMapChanger::applyScanChanges() took %i msecs to apply deletions in %i points (map points size = %i, deleted points size = %i, temp size = %i)",
               elapsed,
               myWorkingMap->getNumPoints(scanType),
               mapPoints->size(),
               deletedPoints->size(),
               tempPoints.size());

    if (!addedPoints->empty()) {
      mapPoints->clear();
      mapPoints->reserve(tempPoints.size() + addedPoints->size());
      merge(tempPoints.begin(), tempPoints.end(),
            addedPoints->begin(), addedPoints->end(),
            std::inserter(*mapPoints, 
                    mapPoints->begin()));
    }
    else {
      ArLog::log(ArLog::Normal,
                 "ArMapChanger::applyScanChanges() copying tempPoints to map points");

      *mapPoints = tempPoints;
    }

  } // end if some but not all points were deleted

  if (myWorkingMap->getNumPoints(scanType) != mapPoints->size()) {
    ArLog::log(ArLog::Normal,
                "Error changing points: map's numPoints = %i, num points in map = %i",
                myWorkingMap->getNumPoints(scanType),
                mapPoints->size());
  }


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Process line changes...
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  std::vector<ArLineSegment> *deletedLines = changeDetails->getChangedLineSegments
                                              (ArMapChangeDetails::DELETIONS,
                                               scanType);
  std::vector<ArLineSegment> *addedLines   = changeDetails->getChangedLineSegments
                                              (ArMapChangeDetails::ADDITIONS,
                                               scanType);

  std::vector<ArLineSegment> *mapLines = myWorkingMap->getLines(scanType);
  if ((deletedLines == NULL) || (addedLines == NULL) || (mapLines == NULL)) {
    return false;
  }
  
  /****/
  // Handle the simple but unlikely case that all lines were deleted
  if (deletedLines->size() >= mapLines->size()) {
    mapLines->clear();
    *mapLines = *addedLines;
  }
  else if (deletedLines->empty()) {
   if (!addedLines->empty()) {
      std::vector<ArLineSegment> tempLines;
      tempLines.reserve(myWorkingMap->getNumLines(scanType) + addedLines->size());
      merge(mapLines->begin(), mapLines->end(),
            addedLines->begin(), addedLines->end(),
            std::inserter(tempLines, 
                     tempLines.begin()));
      *mapLines = tempLines;
    } // end if lines were added (but not deleted)
  }
  else { // some but not all lines were deleted 

    ArTime timeToDelete;
    std::vector<ArLineSegment> tempLines;
    tempLines.reserve(myWorkingMap->getNumLines(scanType));

    set_difference(mapLines->begin(), mapLines->end(), 
                   deletedLines->begin(), deletedLines->end(),
                   std::inserter(tempLines, 
                            tempLines.begin()));
    long int elapsed = timeToDelete.mSecSince();

    ArLog::log(ArLog::Normal,
               "ArMapChanger::applyScanChanges() took %i msecs to apply deletions in %i lines",
               elapsed,
               myWorkingMap->getNumLines(scanType));

    if (!addedLines->empty()) {
      mapLines->clear();
      mapLines->reserve(tempLines.size() + addedLines->size());
      merge(tempLines.begin(), tempLines.end(),
            addedLines->begin(), addedLines->end(),
            std::inserter(*mapLines, 
                    mapLines->begin()));
    }
    else {
      *mapLines = tempLines;
    }

  }

  if (myWorkingMap->getNumLines(scanType) != mapLines->size()) {
    ArLog::log(ArLog::Normal,
                "Error changing lines: map's numLines = %i, num lines in map = %i",
                myWorkingMap->getNumLines(scanType),
                mapLines->size());
  }

  return isSuccess;

} // end method applyScanChanges



bool ArMapChanger::applySupplementChanges(ArMapChangeDetails *changeDetails)
{

  ArMapFileLineSet *newSupplementLines   = changeDetails->getChangedSupplementLines
                                                (ArMapChangeDetails::ADDITIONS);
  if (newSupplementLines == NULL) {
    return false;
  }
  ArFileParser parser;
  bool isSuccess = myWorkingMap->addToFileParser(&parser);

  char buf[10000];

  for (ArMapFileLineSet::iterator iter = newSupplementLines->begin();
       iter != newSupplementLines->end();
       iter++) {
    ArMapFileLineGroup &group = *iter;
    strncpy(buf, group.getParentLine()->getLineText(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    ArLog::log(ArLog::Normal,
               "Parsing supplement line: %s",
               buf);
    
    isSuccess = parser.parseLine(buf) && isSuccess;
    if (!isSuccess) {

      ArLog::log(ArLog::Normal,
                 "Error parsing summary line: %s",
                 group.getParentLine()->getLineText());
    }

  } // end for each new supplement line

  myWorkingMap->remFromFileParser(&parser);

  return isSuccess;

} // end method applySupplementChanges


bool ArMapChanger::applyObjectChanges(ArMapChangeDetails *changeDetails)
{
  if ((changeDetails == NULL) || (myWorkingMap == NULL)) {
    return false;
  }

  ArMapFileLineSet *deletedLineSet = changeDetails->getChangedObjectLines
                                        (ArMapChangeDetails::DELETIONS);
  ArMapFileLineSet *addedLineSet = changeDetails->getChangedObjectLines
                                        (ArMapChangeDetails::ADDITIONS);

  if ((deletedLineSet == NULL) || (addedLineSet == NULL)) {
    // TODO Log error, really shouldn't happen
    return false;
  }

  if (myWorkingMap->getMapObjects() == NULL) {
    return false;
  }

  bool isSuccess = true;

  std::list<ArMapObject *> *mapObjectList = new std::list<ArMapObject *>
                                                    (*(myWorkingMap->getMapObjects()));


  for (ArMapFileLineSet::iterator dIter = deletedLineSet->begin();
       dIter != deletedLineSet->end();
       dIter++) {

    ArMapFileLineGroup &group = *dIter;

    ArArgumentBuilder *objectArg = new ArArgumentBuilder(512, '\0', false, true);
    objectArg->addPlain(group.getParentLine()->getLineText());
    objectArg->removeArg(0, true); // To get rid of the Cairn

    ArMapObject *objectToDelete = ArMapObject::createMapObject(objectArg);

    if (objectToDelete == NULL) {
      ArLog::log(ArLog::Normal,
                 "ArMapChanger::applyObjectChanges() error creating object to delete");
      isSuccess = false;
      continue;
      // TODO Return false?
    }

    // TODO Check name doesn't already exist?

    std::list<ArMapObject *>::iterator mIter = mapObjectList->begin();
    for (;
         mIter != mapObjectList->end();
         mIter++) {
      if (isMatchingObjects(objectToDelete, *mIter)) { // , changeDetails)) {
        break;
      }
    } // end for each map object 

    if (mIter != mapObjectList->end()) {
      mapObjectList->erase(mIter);
    }
    else {
      ArLog::log(ArLog::Normal, 
                 "Could not find matching map object to delete");
      isSuccess = false;
      objectToDelete->log();
    }

    delete objectToDelete;

  } // end for each object to delete


  for (ArMapFileLineSet::iterator aIter = addedLineSet->begin();
       aIter != addedLineSet->end();
       aIter++) {

    ArMapFileLineGroup &group = *aIter;

    ArArgumentBuilder *objectArg = new ArArgumentBuilder(512, '\0', false, true);
    objectArg->addPlain(group.getParentLine()->getLineText());
    objectArg->removeArg(0, true); // To get rid of the Cairn

    ArMapObject *objectToAdd = ArMapObject::createMapObject(objectArg);

    if (objectToAdd == NULL) {
      ArLog::log(ArLog::Normal,
                 "ArMapChanger::applyObjectChanges() error creating object to add");
      isSuccess = false;
      continue;
      // TODO Return false?
    }

    // TODO Check name doesn't already exist?
    // TODO Alright to mangle order?  Or should we reinsert at "right" line?

    mapObjectList->push_back(objectToAdd);

  } // end for each object to add

  myWorkingMap->setMapObjects(mapObjectList);

  ArUtil::deleteSet(mapObjectList->begin(), mapObjectList->end());
  delete mapObjectList;

  return isSuccess;

} // end method applyObjectChanges
  
bool ArMapChanger::isMatchingObjects(ArMapObject *obj1, 
                                     ArMapObject *obj2)
{
  // TODO Make this an ArMapObject method

  if ((obj1 == NULL) || (obj2 == NULL)) {
    return false;
  }

  if (!ArUtil::isStrEmpty(obj1->getName())) {
    if (!ArUtil::isStrEmpty(obj2->getName())) {
      if (ArUtil::strcasecmp(obj1->getName(), obj2->getName()) == 0) {
        return true;
      }
      else { // names don't match
        return false;
      } // end else names don't match
    }
    else { // second object has null name, no match
      return false;
    }
  } // end if object 1 has name

  // Otherwise, objects don't have names...
  
  if (obj1->hasFromTo() != obj2->hasFromTo()) {
    return false;
  }
  if (ArUtil::strcasecmp(obj1->getType(), obj2->getType()) != 0) {
    return false;
  }
  if (obj1->getPose() != obj2->getPose()) {
    return false;
  }
  if (obj1->hasFromTo()) {
    if (obj1->getFromPose() != obj2->getFromPose()) {
      return false;
    }
    if (obj1->getToPose() != obj2->getToPose()) {
      return false;
    }
  } // end if has from to

  if (ArUtil::strcasecmp(obj1->getDescription(), obj2->getDescription()) != 0) {
    return false;
  }
  if (ArUtil::strcasecmp(obj1->getIconName(), obj2->getIconName()) != 0) {
    return false;
  }

  std::list<ArLineSegment> obj1Lines = obj1->getFromToSegments();
  std::list<ArLineSegment> obj2Lines = obj2->getFromToSegments();

  if (obj1Lines.size() != obj2Lines.size()) {
    return false;
  }
    
  for (std::list<ArLineSegment>::iterator iter1 = obj1Lines.begin(), 
                                          iter2 = obj2Lines.begin();
        ((iter1 != obj1Lines.end()) && (iter2 != obj2Lines.end()));
        iter1++, iter2++) {
    ArLineSegment &seg1 = *iter1;
    ArLineSegment &seg2 = *iter2;

    if (seg1 != seg2) {
      return false;
    }
  } // end for each line segment

  return true;

} // end method isMatchingObjects


bool ArMapChanger::applyInfoChanges(ArMapChangeDetails *changeDetails)
{
  if ((changeDetails == NULL) || (myWorkingMap == NULL)) {
    return false;
  }
  bool isSuccess = true;

  std::list<std::string> changedInfoNames = changeDetails->findChangedInfoNames();


  for (std::list<std::string>::iterator tIter = changedInfoNames.begin();
       tIter != changedInfoNames.end();
       tIter++) {

    const char *infoName = (*tIter).c_str();
    
    IFDEBUG(ArLog::log(ArLog::Normal,
                       "ArMapChanger::applyInfoChanges() applying changes to %s",
                       infoName));

    ArMapFileLineSet *deletedLineSet = changeDetails->getChangedInfoLines
                                          (infoName,
                                           ArMapChangeDetails::DELETIONS);
    ArMapFileLineSet *addedLineSet = changeDetails->getChangedInfoLines
                                          (infoName,
                                           ArMapChangeDetails::ADDITIONS);
 

    if ((deletedLineSet == NULL) || (addedLineSet == NULL)) {
      // TODO Log error, really shouldn't happen
      continue;
    }

    std::list<ArArgumentBuilder *> *infoList = myWorkingMap->getInfo(infoName);

    if (infoList == NULL) {
      ArLog::log(ArLog::Normal,
                 "ArMapChanger::applyInfoChanges() null info list for %s",
                 infoName);

      isSuccess = false;
      continue;
    }
    
    std::list<ArArgumentBuilder *> tempInfoList;

    int lineNum = 0;
    bool isParentDeleted = false;

    for (std::list<ArArgumentBuilder *>::iterator iter1 = infoList->begin();
         iter1 != infoList->end();
         iter1++) {

      ArArgumentBuilder *arg = *iter1;
      lineNum++;

      if (!changeDetails->isChildArg(infoName, arg)) {
        isParentDeleted = false;

        std::string namePlusArg = infoName; // myWorkingMap->getInfoName(infoType);
        namePlusArg += " ";
        namePlusArg += arg->getFullString();

        ArMapFileLineSet::iterator delLineIter = 
                                  deletedLineSet->find(ArMapFileLine(lineNum, 
                                                                  namePlusArg.c_str()));
        
        if (delLineIter != deletedLineSet->end()) {

          // line has been deleted, do not re-add
          isParentDeleted = true;

          ArLog::log(ArLog::Normal,
                     "Deleted:  %s",
                     arg->getFullString());
          ArLog::log(ArLog::Normal,
                     "Based on:  %s",
                     namePlusArg.c_str());

          continue;
        }
 
      }
      else if (isParentDeleted) {
        continue;
      }
      
      tempInfoList.push_back(new ArArgumentBuilder(*arg));
      
    } // end for each original info line

    if (!addedLineSet->empty()) {
    
      std::list<ArArgumentBuilder *> newInfoList;
      
      int lineNum = 0;
      bool isDone = false;
      ArMapFileLineSet::iterator addIter = addedLineSet->begin();
      std::list<ArArgumentBuilder *>::iterator origIter = tempInfoList.begin();

      while (!isDone) {

        lineNum++;

        if (addIter != addedLineSet->end()) {

          ArMapFileLineGroup &lineGroup = *addIter;
          if (lineGroup.getParentLine()->getLineNum() <= lineNum) {

            // Add arg for parent
            ArArgumentBuilder *newArg = new ArArgumentBuilder(512, '\0', false, true);
            newArg->addPlain(lineGroup.getParentLine()->getLineText());
            newArg->removeArg(0, true); // To get rid of info name

            newArg->compressQuoted();

            newInfoList.push_back(newArg);
    
            IFDEBUG(ArLog::log(ArLog::Normal,
                               "ArMapChanger::applyInfoChanges() adding %s %s",
                               infoName,
                               newArg->getFullString()));

            for (std::vector<ArMapFileLine>::iterator cIter = lineGroup.getChildLines()->begin();
                 cIter != lineGroup.getChildLines()->end();
                 cIter++) {

              lineNum++;
              ArMapFileLine &line = *cIter;
              ArArgumentBuilder *childArg = new ArArgumentBuilder(512, '\0', false, true);
              childArg->addPlain(line.getLineText());
              childArg->removeArg(0, true); // To get rid of info name

              childArg->compressQuoted();

              newInfoList.push_back(childArg);
            
              IFDEBUG(ArLog::log(ArLog::Normal,
                                 "ArMapChanger::applyInfoChanges() adding child %s %s",
                                 infoName,
                                 childArg->getFullString()));

            } // end for each child line

            addIter++;
            
            continue;

          } // end if line added
        } // end if 
        
        if (origIter != tempInfoList.end()) {
          ArArgumentBuilder *arg = *origIter;
          newInfoList.push_back(arg);
          origIter++;
        }

        if ((addIter == addedLineSet->end()) &&
            (origIter == tempInfoList.end())) {
          isDone = true;
        }

      } // end while not done

      myWorkingMap->setInfo(infoName, &newInfoList);
      // Since setInfoList makes a copy, delete the allocated args.
      ArUtil::deleteSet(newInfoList.begin(), newInfoList.end());
    }
    else { // no lines added

      myWorkingMap->setInfo(infoName, &tempInfoList);
      // Since setInfoList makes a copy, delete the allocated args.
      ArUtil::deleteSet(tempInfoList.begin(), tempInfoList.end());

    } // end else no lines added

  } // end for each changed info type

  return isSuccess;

} // end method applyInfoChanges

// -----------------------------------------------------------------------------

// TODO: Not really sure whether we want to accumulate the packet list... might
// make more sense just to send...

bool ArMapChanger::convertChangeDetailsToPacketList
                          (ArMapChangeDetails *changeDetails,
                           std::list<ArNetPacket *> *packetListOut,
                           bool isRelay)
{
  if (packetListOut == NULL) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::convertChangeDetailsToPacketList() packet list");
    return false;
  }
  packetListOut->clear();

  myChangeDetails = changeDetails;

  bool isSuccess = true;
 
  ArMapId newMapId;
  changeDetails->getNewMapId(&newMapId);

  std::list<std::string> *scanTypeList = changeDetails->getScanTypes();

  // TODO: Not entirely sure that we want to do this here, or in the map 
  // producer...
  if (!isRelay && !ArUtil::isStrEmpty(newMapId.getSourceName())) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::convertChangeDetailsToPacketList() removing timestamp source = %s",
               newMapId.getSourceName());
    // This map was obtained from a remote source...  Rip out the timestamp so 
    // that the originator can set it to the correct value.
    newMapId.setTimestamp(-1);
    changeDetails->setNewMapId(newMapId);
  } // end if remote map

  ArNetPacket *startPacket = new ArNetPacket();
  addHeaderToPacket(START_CHANGES, 
                    NO_CHANGE, // unused
                    ArMapChangeDetails::ADDITIONS,  // unused
                    NULL,
                    startPacket);
  packetListOut->push_back(startPacket);

  for (int i = 0; 
       (isSuccess && (i < ArMapChangeDetails::CHANGE_TYPE_COUNT)); 
       i++) {

    ArMapChangeDetails::MapLineChangeType changeType = 
                            (ArMapChangeDetails::MapLineChangeType) i;

    std::list<std::string>::iterator iter = scanTypeList->end();

    for (iter = scanTypeList->begin(); iter != scanTypeList->end(); iter++) {
      const char *scanType = (*iter).c_str();
      isSuccess = isSuccess &&
                    addFileLineSetPackets
                        (SUMMARY_DATA,
                         changeType,
                         scanType,
                         NULL,
                         changeDetails->getChangedSummaryLines(changeType, scanType),
                         packetListOut);
    } // end for each scan type

    isSuccess = isSuccess &&
                    addFileLineSetPackets
                        (SUPPLEMENT_DATA, 
                         changeType,
                         NULL,
                         NULL,
                         changeDetails->getChangedSupplementLines(changeType),
                         packetListOut);


    //for (int infoType = 0; infoType < myInfoCount; infoType++) {
    for (std::list<std::string>::iterator iIter = myInfoNames.begin();
          iIter != myInfoNames.end();
          iIter++) {
      const char *infoName = (*iIter).c_str();

      isSuccess = isSuccess &&
                      addFileLineSetPackets
                          (INFO_DATA, 
                           changeType,
                           NULL,
                           infoName,
                           changeDetails->getChangedInfoLines(infoName, changeType),
                           packetListOut);
    } // end for each info type

    isSuccess = isSuccess &&
                    addFileLineSetPackets
                        (OBJECTS_DATA, 
                         changeType,
                         NULL,
                         NULL,
                         changeDetails->getChangedObjectLines(changeType),
                         packetListOut);

    for (iter = scanTypeList->begin(); iter != scanTypeList->end(); iter++) {
      const char *scanType = (*iter).c_str();
      isSuccess = isSuccess &&
                    addPointsPackets
                        (changeType,
                         scanType,
                         changeDetails->getChangedPoints(changeType, scanType),
                         packetListOut);
    } // end for each scan type

    for (iter = scanTypeList->begin(); iter != scanTypeList->end(); iter++) {
      const char *scanType = (*iter).c_str();
      isSuccess = isSuccess &&
                      addLinesPackets
                          (changeType,
                           scanType,
                           changeDetails->getChangedLineSegments(changeType, scanType),
                           packetListOut);
    } // end for each scan type

  } // end for each change type
  
  ArNetPacket *endPacket = new ArNetPacket();
  addHeaderToPacket(FINISH_CHANGES, 
                    NO_CHANGE, // unused
                    ArMapChangeDetails::ADDITIONS,  // unused
                    NULL,
                    endPacket);
  packetListOut->push_back(endPacket);

  myChangeDetails = NULL;

  return isSuccess;

} // end method convertChangeDetailsToPacketList



bool ArMapChanger::addFileLineSetPackets
                       (MapChangeDataType dataType, 
                        ArMapChangeDetails::MapLineChangeType changeType,
                        const char *scanType,
                        const char *extra,
                        ArMapFileLineSet *fileLineSet,
                        std::list<ArNetPacket *> *packetListOut)
{
  if (packetListOut == NULL) {
    return false;
  }
  if ((fileLineSet == NULL) || (fileLineSet->empty())) {
    return true;
  }

  // TODO Could probably make this work similar to the addGroup and addFileLine...
  // i.e. append to previous packet if there is room.

  ArNetPacket *packet = new ArNetPacket();

  addHeaderToPacket(CONTINUE_CHANGES, dataType, changeType, scanType, packet);

  // TODO Someday we should create a new packet defn that sends a string instead
  // of the info name index... (i.e. Keep this in mind if any other changes to 
  // the packet are necessary.)
  int extraIndex = -1;

  if ((dataType == INFO_DATA) && (!ArUtil::isStrEmpty(extra))) {

    int i = 0;
    for (std::list<std::string>::iterator iter = myInfoNames.begin();
         iter != myInfoNames.end();
         iter++, i++) {
      if (strcasecmp(extra, (*iter).c_str()) == 0) {
        extraIndex = i;
        break;
      }
    } // end for each info name  
  } // end if 

  packet->byte4ToBuf(extraIndex);

  packet->byte4ToBuf(fileLineSet->size());

  packetListOut->push_back(packet);

  bool isAddSuccess = true;

  for (ArMapFileLineSet::iterator iter2 = fileLineSet->begin();
       (isAddSuccess && (iter2 != fileLineSet->end()));
       iter2++) {

    ArMapFileLineGroup &group = *iter2;

    isAddSuccess = addGroupToPacketList(dataType, 
                                        changeType, 
                                        scanType,
                                        group, 
                                        packetListOut);
    
  } // end for each group

  return isAddSuccess;

} // end method addFileLineSetPackets


void ArMapChanger::addHeaderToPacket
                       (MapChangeCommand command,
                        MapChangeDataType dataType, 
                        ArMapChangeDetails::MapLineChangeType changeType,
                        const char *scanType,
                        ArNetPacket *packet)
{

  if (myChangeDetails == NULL) {
    ArLog::log(ArLog::Normal,
              "ArMapChanger::addHeaderToPacket() cannot create packet because no change details");
    return;
  }
  ArLog::log(ArLog::Normal,
             "ArMapChanger::addHeaderToPacket() creating packet for data type %i change type %i",
             dataType, changeType);

  packet->uByte2ToBuf(command);

  ArMapId origMapId;
  ArMapId newMapId;
  myChangeDetails->getOrigMapId(&origMapId);
  myChangeDetails->getNewMapId(&newMapId);

  bool isSuccess = true;

  isSuccess = ArMapId::toPacket(origMapId, packet) && isSuccess;
  isSuccess = ArMapId::toPacket(newMapId, packet) && isSuccess;

  if ((command != CONTINUE_CHANGES)) {
    return;
  }

  packet->uByteToBuf(dataType);
  packet->uByteToBuf(changeType);
  if (scanType != NULL) {
    packet->strToBuf(scanType);
  }
  else {
    packet->strToBuf("");
  }

} // end method addHeaderToPacket


bool ArMapChanger::addGroupToPacketList(MapChangeDataType dataType, 
                                        ArMapChangeDetails::MapLineChangeType changeType,
                                        const char *scanType,
                                        ArMapFileLineGroup &group,
                                        std::list<ArNetPacket *> *packetListOut)
{
  int packetPadding = 1000;

  ArNetPacket *packet = packetListOut->back();

  for (int i = 0; i < 2; i++) {
    if (packet->getLength() 
                    + strlen(group.getParentLine()->getLineText()) + 1
                    + 4  // line number
                    + 4  // child count
                    + packetPadding > packet->getMaxLength()) {
        
      if (i == 0) {

        packet = new ArNetPacket();
        addHeaderToPacket(CONTINUE_CHANGES, dataType, changeType, scanType, packet);

        packet->byte4ToBuf(-1); // for continuation
        packet->byte4ToBuf(-1); //     ""
        
        packetListOut->push_back(packet);
      }
      else {
        // TODO Log error
        return false;
      }
    }
    else {
      break;
    } // end if line is too long for packet
  } // end for each try

  packet->byte4ToBuf(group.getChildLines()->size());

  packet->byte4ToBuf(group.getParentLine()->getLineNum());
  packet->strToBuf(group.getParentLine()->getLineText());

  bool isAddSuccess = true;

  for (std::vector<ArMapFileLine>::iterator iter = group.getChildLines()->begin();
       (isAddSuccess && (iter != group.getChildLines()->end()));
       iter++) {
    const ArMapFileLine &fileLine = *iter;

    isAddSuccess = addFileLineToPacketList(dataType, 
                                           changeType, 
                                           scanType,
                                           fileLine, 
                                           packetListOut);

  }

  return isAddSuccess;

} // end method addGroupToPacketList


bool ArMapChanger::addFileLineToPacketList(MapChangeDataType dataType, 
                                           ArMapChangeDetails::MapLineChangeType changeType,
                                           const char *scanType,
                                           const ArMapFileLine &fileLine,
                                           std::list<ArNetPacket *> *packetListOut)
{
  int packetPadding = 1000;

  ArNetPacket *packet = packetListOut->back();

  for (int i = 0; i < 2; i++) {
    if (packet->getLength() 
                    + strlen(fileLine.getLineText()) + 1
                    + 4  // line number
                    + packetPadding > packet->getMaxLength()) {
        
      if (i == 0) {

        packet = new ArNetPacket();
        addHeaderToPacket(CONTINUE_CHANGES, dataType, changeType, scanType, packet);
        packet->byte4ToBuf(-1); // for continuation
        packet->byte4ToBuf(-1); //      ""
       
        packetListOut->push_back(packet);
      }
      else {

        ArLog::log(ArLog::Normal,
                   "ArMapChanger::addFileLineToPacketList() line #%i is too long to add",
                   fileLine.getLineNum());
        return false;
      }
    } // end if line is too long for packet
  } // end for each try

  packet->byte4ToBuf(fileLine.getLineNum());
  packet->strToBuf(fileLine.getLineText());

  return true;

} // end method addFileLineToPacketList


bool ArMapChanger::addPointsPackets
                       (ArMapChangeDetails::MapLineChangeType changeType,
                        const char *scanType,
                        std::vector<ArPose> *pointList,
                        std::list<ArNetPacket *> *packetListOut)
{
  if (packetListOut == NULL) {
    return false;
  }
  if ((pointList == NULL) || (pointList->empty())) {
    return true;
  }

  // TODO Could probably make this work similar to the addGroup and addFileLine...
  // i.e. append to previous packet if there is room.

  ArNetPacket *packet = new ArNetPacket();
  addHeaderToPacket(CONTINUE_CHANGES, POINTS_DATA, changeType, scanType, packet);
  packet->byte4ToBuf(pointList->size());

  packetListOut->push_back(packet);

  int currentCount = 0;

  for (std::vector<ArPose>::iterator iter = pointList->begin();
       iter != pointList->end();
       iter++) {
    
    if (currentCount >= MAX_POINTS_IN_PACKET) {

      ArNetPacket *packet = new ArNetPacket();

      addHeaderToPacket(CONTINUE_CHANGES, POINTS_DATA, changeType, scanType, packet);

      packet->byte4ToBuf(-1); // for a continuation...

      packetListOut->push_back(packet);

      currentCount = 0;
    } 
    currentCount++;
    /***
    ArLog::log(ArLog::Normal,
                "Packed: %li %li",
                (long int) (*iter).getX(),
                (long int) (*iter).getY());
    ***/

    packet->byte4ToBuf((long int) (*iter).getX());
    packet->byte4ToBuf((long int) (*iter).getY());

 } // end for each point

  return true;

} // end method addPointsPackets



bool ArMapChanger::addLinesPackets
                      (ArMapChangeDetails::MapLineChangeType changeType,
                       const char *scanType,
                       std::vector<ArLineSegment> *lineSegmentList,
                       std::list<ArNetPacket *> *packetListOut)
{
  if (packetListOut == NULL) {
    return false;
  }
  if ((lineSegmentList == NULL) || (lineSegmentList->empty())) {
    return true;
  }

  // TODO Could probably make this work similar to the addGroup and addFileLine...
  // i.e. append to previous packet if there is room.

  ArNetPacket *packet = new ArNetPacket();
  addHeaderToPacket(CONTINUE_CHANGES, LINES_DATA, changeType, scanType, packet);

  packet->byte4ToBuf(lineSegmentList->size());

  packetListOut->push_back(packet);

  int currentCount = 0;

  for (std::vector<ArLineSegment>::iterator iter = lineSegmentList->begin();
       iter != lineSegmentList->end();
       iter++) {
    
    if (currentCount >= MAX_LINES_IN_PACKET) {

      ArNetPacket *packet = new ArNetPacket();

      addHeaderToPacket(CONTINUE_CHANGES, LINES_DATA, changeType, scanType, packet);
      packet->byte4ToBuf(-1); // for a continuation...

      packetListOut->push_back(packet);

      currentCount = 0;
    } 
    currentCount++;

    packet->byte4ToBuf((long int) (*iter).getX1());
    packet->byte4ToBuf((long int) (*iter).getY1());
    packet->byte4ToBuf((long int) (*iter).getX2());
    packet->byte4ToBuf((long int) (*iter).getY2());

  } // end for each point

  return true;

} // end method addLinesPackets

// -----------------------------------------------------------------------------

bool ArMapChanger::convertPacketListToChangeDetails
                          (std::list<ArNetPacket *> &packetList,
                           ArMapChangeDetails *changeDetailsOut)
{

  if (changeDetailsOut == NULL) {
    return false;
  }

  bool isSuccess = true;
  int numPoints = 0;
  int numLines = 0;
  int numGroups = 0;
  int numChildren = 0;

  for (std::list<ArNetPacket *>::iterator iter = packetList.begin();
       (isSuccess && iter != packetList.end());
       iter++) {
    ArNetPacket *packet = *iter;
    if (packet == NULL) {
      continue;
    }

    MapChangeCommand command = CONTINUE_CHANGES;
    ArMapId origMapId;
    ArMapId newMapId;
    MapChangeDataType dataType = NO_CHANGE;
    ArMapChangeDetails::MapLineChangeType changeType =ArMapChangeDetails::ADDITIONS;
    std::string scanType;

    bool isSuccess = unpackHeader(packet,
                                  &command,
                                  &origMapId,
                                  &newMapId,
                                  &dataType,
                                  &changeType, 
                                  &scanType);

    if (command == START_CHANGES) {
      changeDetailsOut->setOrigMapId(origMapId);
      changeDetailsOut->setNewMapId(newMapId);
    }

    if (command != CONTINUE_CHANGES) {
      continue;
    }

    switch (dataType) {

    case NO_CHANGE:
      break;

    case SUMMARY_DATA:
    case INFO_DATA:
    case OBJECTS_DATA:
    case SUPPLEMENT_DATA:
      {
        isSuccess = unpackFileLineSet(packet,
                                      dataType,
                                      changeType,
                                      scanType.c_str(),
                                      &numGroups,
                                      &numChildren,
                                      changeDetailsOut);
      }
      break;

    case POINTS_DATA:
      {
        isSuccess = unpackPoints(packet,
                                 changeType,
                                 scanType.c_str(),
                                 &numPoints, 
                                 changeDetailsOut);
      }
      break;
    case LINES_DATA:
      {
        isSuccess = unpackLines(packet,
                                changeType,
                                scanType.c_str(),
                                &numLines, 
                                changeDetailsOut);
      }
      break;
    } // end switch dataType

  } // end for each packet

  return isSuccess; 

} // end method convertPacketListToChangeDetails


bool ArMapChanger::unpackHeader(ArNetPacket *packet,
                                MapChangeCommand *commandOut,
                                ArMapId *origMapIdOut,
                                ArMapId *newMapIdOut,
                                MapChangeDataType *dataTypeOut,
                                ArMapChangeDetails::MapLineChangeType *changeTypeOut,
                                std::string *scanTypeOut)
{
  if (packet == NULL) {
    return false;
  }
  if ((commandOut == NULL) || (origMapIdOut == NULL)) {
    return false;
  }
  ArTypes::UByte2 commandVal = packet->bufToUByte2();

  if ((commandVal > LAST_CHANGE_COMMAND)) {
    return false;
  }

  *commandOut = (MapChangeCommand) commandVal;

  bool isSuccess = true;

  isSuccess = ArMapId::fromPacket(packet, origMapIdOut) && isSuccess;

  if (newMapIdOut == NULL) {
    return isSuccess;
  }

  isSuccess = ArMapId::fromPacket(packet, newMapIdOut) && isSuccess;

  if ((commandVal != CONTINUE_CHANGES)) {
    return isSuccess;
  }

  if (dataTypeOut == NULL) {
    return isSuccess;
  }

  int dataTypeVal = packet->bufToUByte();
  if ((dataTypeVal < 0) || (dataTypeVal > LAST_CHANGE_DATA_TYPE)) {
    ArLog::log(ArLog::Normal,
                "ArMapChanger::convertPacketListToChangeDetails() data type error (%i)",
                dataTypeVal);
    return false;
  }
  *dataTypeOut = (MapChangeDataType) dataTypeVal;


  if (changeTypeOut == NULL) {
    return isSuccess;
  }

  int changeTypeVal = packet->bufToUByte();
  if ((changeTypeVal < 0) || (changeTypeVal > ArMapChangeDetails::LAST_CHANGE_TYPE)) {
    ArLog::log(ArLog::Normal,
                "ArMapChanger::convertPacketListToChangeDetails() change type error (%i)",
                changeTypeVal);
    return false;
  }

  *changeTypeOut = (ArMapChangeDetails::MapLineChangeType) changeTypeVal;

  if (scanTypeOut == NULL) {
    return isSuccess;
  }

  char buf[512];
  buf[0] = '\0';
  packet->bufToStr(buf, sizeof(buf));
  *scanTypeOut = buf;

  return isSuccess;

} // end method unpackHeader

bool ArMapChanger::unpackFileLineSet(ArNetPacket *packet,
                                     MapChangeDataType dataType, 
                                     ArMapChangeDetails::MapLineChangeType changeType,
                                     const char *scanType,
                                     int *numGroups,
                                     int *numChildren,
                                     ArMapChangeDetails *changeDetails)
{

  if ((packet == NULL) || (numGroups == NULL) || 
      (numChildren == NULL) || (changeDetails == NULL)) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::unpackFileLineSet() param error");
    return false;
  }

  
  // TODO Get info type...
  int extra = packet->bufToByte4();


  int tempGroups = packet->bufToByte4();
  if (tempGroups >= 0) {
   IFDEBUG(ArLog::log(ArLog::Normal,
              "ArMapChanger::unpackFileLineSet() setting numGroups to %i", tempGroups));
   *numGroups = tempGroups;
  }
  else {
    IFDEBUG(ArLog::log(ArLog::Normal,
               "ArMapChanger::unpackFileLineSet() continuing from previous (numGroups = %i)", 
               *numGroups));
  
  } // continuing from previous...

  if (*numGroups <= 0) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::unpackFileLineSet() invalid group count %i",
               *numGroups);
    return false; 
  }


  ArMapFileLineSet *fileLineSet = NULL;
  
  switch (dataType) {
  case SUMMARY_DATA:
    fileLineSet = changeDetails->getChangedSummaryLines(changeType, scanType);
    break;
  case INFO_DATA:
    {
    const char *infoName = NULL;
    if ((extra >= 0) && (extra < (int) myInfoNames.size())) {
      int i = 0;
      for (std::list<std::string>::iterator iter = myInfoNames.begin();
           iter != myInfoNames.end();
           iter++, i++) {
        if (i == extra) {
          infoName = (*iter).c_str();
          break;
        }
      } // end for
    } // end if 
    if (infoName != NULL) {
      fileLineSet = changeDetails->getChangedInfoLines(infoName, changeType);
    }
    }
    break;
  case OBJECTS_DATA:
    fileLineSet = changeDetails->getChangedObjectLines(changeType);
    break;
  default:
    ArLog::log(ArLog::Normal,
               "ArMapChanger::unpackFileLineSet() unexpected data type %i",
               dataType);
    return false;
    break;
  }

  if (fileLineSet == NULL) {
    ArLog::log(ArLog::Normal,
               "ArMapChanger::unpackFileLineSet() null file line set for %i",
               dataType);
    return false;
  }
  IFDEBUG(ArLog::log(ArLog::Normal,
              "ArMapChanger::unpackFileLineSet() unpacking packet for data type %i",
              dataType));

  char buf[10000];

  while ((*numGroups > 0) &&
         (packet->getReadLength() < packet->getLength()) &&
         (packet->isValid())) {

    IFDEBUG(ArLog::log(ArLog::Normal,
	                     "ArMapChanger::unpackFileLineSet() begin loop, numGroups = %i",
                       *numGroups));

    int tempChildren = packet->bufToByte4();
    if (tempChildren >= 0) {
      *numChildren = tempChildren;
      IFDEBUG(ArLog::log(ArLog::Normal,
                "ArMapChanger::unpackFileLineSet() setting numChildren to %i", tempChildren));
    }
    else {    
       IFDEBUG(ArLog::log(ArLog::Normal,
                "ArMapChanger::unpackFileLineSet() continuing previous children %i", *numChildren));
    } // continuing from previous...

    // Note that it's alright to have 0 children
    if (*numChildren < 0) {
      ArLog::log(ArLog::Normal,
                "ArMapChanger::unpackFileLineSet() invalid child count %i",
                *numChildren);
      return false; 
    }
  
    if (tempChildren >= 0) {

      // this is the parent
      int lineNum = packet->bufToByte4();
      packet->bufToStr(buf, sizeof(buf));

      if (!packet->isValid()) {
        ArLog::log(ArLog::Normal,
                  "ArMapChanger::unpackFileLineSet() packet not valid");
        return false;
      }

      IFDEBUG(ArLog::log(ArLog::Normal,
                 "ArMapChanger::unpackFileLineSet() creating new group: #%i %s",
                 lineNum, buf));

      ArMapFileLineGroup newGroup(ArMapFileLine(lineNum, buf));
      fileLineSet->push_back(newGroup);


    }
    if (*numChildren > 0) {

      if (fileLineSet->empty()) {
       ArLog::log(ArLog::Normal,
                  "ArMapChanger::unpackFileLineSet() no parent for children");
       return false;
      }
      ArMapFileLineGroup &group = fileLineSet->back();

      while ((*numChildren > 0) &&
             (packet->getReadLength() < packet->getLength()) &&
             (packet->isValid())) {

        int childLineNum = packet->bufToByte4();
        packet->bufToStr(buf, sizeof(buf));

        if (!packet->isValid()) {
          ArLog::log(ArLog::Normal,
		     "ArMapChanger::unpackFileLineSet() invalid packet in child read");
          return false;
        }

        IFDEBUG(ArLog::log(ArLog::Normal,
                  "ArMapChanger::unpackFileLineSet() creating new child (of #%i): #%i %s",
                  group.getParentLine()->getLineNum(),
	                childLineNum, 
                  buf));
        group.getChildLines()->push_back(ArMapFileLine(childLineNum, buf));

        *numChildren = *numChildren - 1;

      } // end while more children to read

    IFDEBUG(
	  if (*numChildren <= 0) {
	    ArLog::log(ArLog::Normal,
		      "ArMapChanger::unpackFileLineSet() exited inner loop because numChildren = %i",
		      *numChildren);

	  }
	  else if (packet->getReadLength() >= packet->getLength()) {
	    ArLog::log(ArLog::Normal,
		      "ArMapChanger::unpackFileLineSet() exited inner loop because readLength (%i) >= length (%i)",
		      packet->getReadLength(), packet->getLength());

	  }
	  else if (!packet->isValid()) {
	    ArLog::log(ArLog::Normal,
		      "ArMapChanger::unpackFileLineSet() exited inner loop because packet not valid");

	  }
    );



    } // end if there were children to read

    if (*numChildren == 0) {
      IFDEBUG(ArLog::log(ArLog::Normal,
                "ArMapChanger::unpackFileLineSet() done reading group, old num = %i",
                *numGroups));


      *numGroups = *numGroups - 1;
      
      IFDEBUG(ArLog::log(ArLog::Normal,
                "ArMapChanger::unpackFileLineSet() done reading group, new num = %i",
                *numGroups));
    }

  } // end while more to read from packet

  IFDEBUG(
  if (*numGroups <= 0) {
    ArLog::log(ArLog::Normal,
              "ArMapChanger::unpackFileLineSet() exited loop because numGroups = %i",
              *numGroups);

  }
  else if (packet->getReadLength() >= packet->getLength()) {
    ArLog::log(ArLog::Normal,
              "ArMapChanger::unpackFileLineSet() exited loop because readLength (%i) >= length (%i)",
              packet->getReadLength(), packet->getLength());

  }
  else if (!packet->isValid()) {
    ArLog::log(ArLog::Normal,
              "ArMapChanger::unpackFileLineSet() exited loop because packet not valid");

  }
  ); 

  return true;

} // end method unpackFileLineSet


bool ArMapChanger::unpackPoints(ArNetPacket *packet,
                                ArMapChangeDetails::MapLineChangeType changeType,
                                const char *scanType,
                                int *numPoints,
                                ArMapChangeDetails *changeDetails)
{
  if ((packet == NULL) || (numPoints == NULL) || (changeDetails == NULL)) {
    return false;
  }

  int tempPoints = packet->bufToByte4();
  if (tempPoints >= 0) {
    *numPoints = tempPoints;
    ArLog::log(ArLog::Normal,
               "Set numPoints = %i", tempPoints);
  }
  else {} // continuing from previous...

  if (*numPoints <= 0) {
    return false; 
  }

  std::vector<ArPose> *pointList = changeDetails->getChangedPoints(changeType, scanType);

  for (int i = 0; i < MAX_POINTS_IN_PACKET; i++) {

    long int x = packet->bufToByte4();
    long int y = packet->bufToByte4();

    ArPose p(x, y);
    pointList->push_back(p);

    /***
    ArLog::log(ArLog::Normal,
      "Unpacked: %li %li",
      (long int) p.getX(),
      (long int) p.getY());
    ***/

    *numPoints = *numPoints - 1;
    if (*numPoints == 0) {
      // TODO: Make sure packet is empty?
      return true;
    }
  } // end for each point in packet

  // TODO: Make sure packet is empty?

  return true;

} // end method unpackPoints

bool ArMapChanger::unpackLines(ArNetPacket *packet,
                               ArMapChangeDetails::MapLineChangeType changeType,
                               const char *scanType,
                               int *numLines,
                               ArMapChangeDetails *changeDetails)
{
  if ((packet == NULL) || (numLines == NULL) || (changeDetails == NULL)) {
    return false;
  }

  int tempLines = packet->bufToByte4();
  if (tempLines >= 0) {
    *numLines = tempLines;
  }
  else {} // continuing from previous...

  if (*numLines <= 0) {
    return false; 
  }

  std::vector<ArLineSegment> *lineSegmentList = 
                                  changeDetails->getChangedLineSegments(changeType, scanType);

  for (int i = 0; i < MAX_LINES_IN_PACKET; i++) {

    long int x1 = packet->bufToByte4();
    long int y1 = packet->bufToByte4();
    long int x2 = packet->bufToByte4();
    long int y2 = packet->bufToByte4();

    lineSegmentList->push_back(ArLineSegment(x1, y1, x2, y2));
    *numLines = *numLines - 1;
    if (*numLines == 0) {
      // TODO: Make sure packet is empty?
      return true;
    }
  } // end for each point in packet

  // TODO: Make sure packet is empty?

  return true;

} // end method unpackLines


AREXPORT bool ArMapChanger::addChangeCB(ArFunctor2<ArServerClient *, 
                                        std::list<ArNetPacket *> *> *functor)
{
  if (functor == NULL) {
    return false;
  }
  myChangeCBList.push_back(functor);
  return true;
}

AREXPORT bool ArMapChanger::remChangeCB
                          (ArFunctor2<ArServerClient *, 
                                      std::list<ArNetPacket *> *> *functor)
{
  if (functor == NULL) {
    return false;
  }
  // TODO Improve return val
  myChangeCBList.remove(functor); 
  return true;

}
  
AREXPORT bool ArMapChanger::addRobotChangeReplyCB
                          (ArFunctor2<ArServerClient *, ArNetPacket *>  *functor)
{
  if (functor == NULL) {
    return false;
  }
  myRobotChangeReplyCBList.push_back(functor);
  return true;

} // end method addRobotChangeReplyCB


AREXPORT bool ArMapChanger::remRobotChangeReplyCB
                          (ArFunctor2<ArServerClient *, ArNetPacket *>  *functor)
{
  if (functor == NULL) {
    return false;
  }
  // TODO Improve return val
  myRobotChangeReplyCBList.remove(functor); 
  return true;

} // end method remRobotChangeReplyCB

// -----------------------------------------------------------------------------

ArMapChanger::ClientChangeInfo::ClientChangeInfo(ArServerClient *client) :
  myClient(client),
  myForwarder(NULL),
  myStartTime(),
  myLastActivityTime(),
  myPacketList()
{
}

ArMapChanger::ClientChangeInfo::ClientChangeInfo(ArCentralForwarder *forwarder) :
  myClient(NULL),
  myForwarder(forwarder),
  myStartTime(),
  myLastActivityTime(),
  myPacketList()
{
}


ArMapChanger::ClientChangeInfo::~ClientChangeInfo()
{
  ArUtil::deleteSet(myPacketList.begin(), myPacketList.end());
  myPacketList.clear();
}
 
void ArMapChanger::ClientChangeInfo::addPacket(ArNetPacket *packet)
{
  if (packet == NULL) {
    return;
  }
  ArNetPacket *packetCopy = new ArNetPacket();
  packetCopy->duplicatePacket(packet);
  packetCopy->resetRead();

  myPacketList.push_back(packetCopy);

} // end method addPacket


AREXPORT void ArMapChanger::addPreWriteFileCB(ArFunctor *functor,
                                              ArListPos::Pos position)
{
  addToCallbackList(functor, position, &myPreWriteCBList);

} // end method addPreWriteFileCB

AREXPORT void ArMapChanger::remPreWriteFileCB(ArFunctor *functor)
{
  remFromCallbackList(functor, &myPreWriteCBList);

} // end method remPreWriteFileCB

AREXPORT void ArMapChanger::addPostWriteFileCB(ArFunctor *functor,
                                               ArListPos::Pos position)
{
  addToCallbackList(functor, position, &myPostWriteCBList);

} // end method addPostWriteFileCB

AREXPORT void ArMapChanger::remPostWriteFileCB(ArFunctor *functor)
{
  remFromCallbackList(functor, &myPostWriteCBList);

} // end method remPostWriteFileCB


AREXPORT void ArMapChanger::addToCallbackList(ArFunctor *functor,
                                              ArListPos::Pos position,
                                              std::list<ArFunctor*> *cbList)
{
  if (functor == NULL) {
    ArLog::log(ArLog::Terse,
               "ArMapChanger::addToCallbackList cannot add null functor");
    return;
  }
  if (cbList == NULL) {
    ArLog::log(ArLog::Terse,
               "ArMapChanger::addToCallbackList cannot add functor to null list");
    return;
  }

  switch (position) {
  case ArListPos::FIRST:
    cbList->push_front(functor);
    break;
  case ArListPos::LAST:
    cbList->push_back(functor);
    break;
  default:
    ArLog::log(ArLog::Terse,
               "ArMapChanger::addToCallbackList invalid position (%i)",
               position);
  } // end switch
} // end method addToCallbackList

AREXPORT void ArMapChanger::remFromCallbackList(ArFunctor *functor,
                                                std::list<ArFunctor*> *cbList)
{
  if (functor == NULL) {
    ArLog::log(ArLog::Terse,
               "ArMapChanger::remFromCallbackList cannot remove null functor");
    return;
  }
  if (cbList == NULL) {
    ArLog::log(ArLog::Terse,
               "ArMapChanger::addToCallbackList cannot remove functor to null list");
    return;
  }
  cbList->remove(functor);

} // end method remFromCallbackList


AREXPORT void ArMapChanger::handleRobotChangeReplyPacket(ArServerClient *client, 
                                                         ArNetPacket *packet)
{
  IFDEBUG(ArLog::log(ArLog::Normal,
                     "ArMapChanger::handleRobotChangeReplyPacket()"));
 
  myInterleaveMutex.lock();
  if (!myIsWaitingForReturn)
  {
    myInterleaveMutex.unlock();
    return;
  }
  
  char robotName[512];
  packet->bufToStr(robotName, sizeof(robotName)); 
  
  int ret = packet->bufToUByte2();

  // packet->bufToStr(fileName, sizeof(fileName));
  // if (myInterleaved && ret == 10)
  if (ret == CHANGE_SUCCESS)
  {
    myReadyForNextPacket = true;
    myIsWaitingForReturn = false;
    ArLog::log(ArLog::Normal,
               "ArMapChanger::handleRobotChangeReplyPacket() set myReadyForNextPacket = true && myIsWaitingForReturn = false");
  }
  else
  {
    myIsWaitingForReturn = false;
    ArLog::log(ArLog::Normal,
               "ArMapChanger::handleRobotChangeReplyPacket() set myIsWaitingForReturn = false");
  }
  
  myInterleaveMutex.unlock();
          
  for (std::list< ArFunctor2<ArServerClient *, ArNetPacket *> *>::iterator cbIter = 
                                                                          myRobotChangeReplyCBList.begin();
       cbIter != myRobotChangeReplyCBList.end();
       cbIter++) {

    packet->resetRead();

    ArFunctor2<ArServerClient *, ArNetPacket *> *functor = *cbIter;

    if (functor != NULL) {
      functor->invoke(client, packet);
    }
  } 
} // end method handleRobotChangeReplyPacket


