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
#include "ArServerHandlerCameraCollection.h"


#include <ArConfigArg.h>
#include "ArClientArgUtils.h"

const char *ArServerHandlerCameraCollection::GET_COLLECTION_PACKET_NAME =
                        "getCameraList";
const char *ArServerHandlerCameraCollection::COLLECTION_UPDATED_PACKET_NAME = 
                        "cameraListUpdated";
const char *ArServerHandlerCameraCollection::SET_PARAMS_PACKET_NAME =
                        "setCameraParams";
const char *ArServerHandlerCameraCollection::PARAMS_UPDATED_PACKET_NAME = 
                        "cameraParamUpdated";

const char *ArServerHandlerCameraCollection::COMMAND_GROUP =
                        "CameraInfo"; // ??


AREXPORT ArServerHandlerCameraCollection::ArServerHandlerCameraCollection
											(ArServerBase *server, 
											 ArCameraCollection *cameraCollection) :
  myServer(server),
  myCameraCollection(cameraCollection),
  myGetCameraListCB(NULL),
  mySetParamCB(NULL),
  myCollectionModifiedCB(NULL)
{
  if (myServer != NULL)
  {
    myGetCameraListCB = new ArFunctor2C<ArServerHandlerCameraCollection,
                                        ArServerClient *, 
                                        ArNetPacket *>
                                  (this,
                                   &ArServerHandlerCameraCollection::getCameraList);

    myServer->addData(GET_COLLECTION_PACKET_NAME, 
		                  "Gets information about the robot's cameras.",
		                  myGetCameraListCB, 
		                  "none", 
                      "byte2: numCameras, repeating for numCameras: { string: cameraName, string: cameraType, string: displayName, string: displayType, byte2: numCommands, repeating for numCommands: {string: command, string: cameraCommandName byte4: requestInterval } byte2: numParams, repeating for numParams: {see ArConfigArg format}}",
		                  COMMAND_GROUP, "RETURN_SINGLE"); // ???


    mySetParamCB = new ArFunctor2C<ArServerHandlerCameraCollection,
                                        ArServerClient *, 
                                        ArNetPacket *>
                                  (this,
                                   &ArServerHandlerCameraCollection::setParams);

    myServer->addData(SET_PARAMS_PACKET_NAME, 
		                  "Sets the specified camera parameters.",
		                  mySetParamCB, 
                      "string: cameraName, repeating for each param: { string: paramName, <param value - see ArConfigArg format> }, empty paramName terminates list",
                      "string: cameraName, repeating for each modified param: { string: paramName, <param value - see ArConfigArg format> }, empty paramName terminates list",
		                  COMMAND_GROUP, "RETURN_SINGLE"); // ???


    myServer->addData(COLLECTION_UPDATED_PACKET_NAME, 
		                  "Single packet is sent when the camera collection has been modified.",
		                  NULL, 
		                  "none", 
		                  "none",
                      COMMAND_GROUP, "RETURN_SINGLE");

    myServer->addData(PARAMS_UPDATED_PACKET_NAME, 
		                  "Packet is sent when the camera parameters have been modified.",
		                  NULL, 
 		                  "none", 
                      "string: cameraName, repeating for each modified param: { string: paramName, <param value - see ArConfigArg format> }, empty paramName terminates list",
		                  COMMAND_GROUP, "RETURN_SINGLE"); // ???

  } // end if server 

  if (myCameraCollection != NULL) {

    myCollectionModifiedCB = new ArFunctorC<ArServerHandlerCameraCollection>
                                   (this,
                                    &ArServerHandlerCameraCollection::handleCameraCollectionModified);

    myCameraCollection->addModifiedCB(myCollectionModifiedCB);

  } // end if camera collection

} // end ctor
							  
AREXPORT ArServerHandlerCameraCollection::~ArServerHandlerCameraCollection()
{
  if ((myCameraCollection != NULL) &&
      (myCollectionModifiedCB != NULL)) {

    myCameraCollection->removeModifiedCB(myCollectionModifiedCB);
    delete myCollectionModifiedCB;
    myCollectionModifiedCB = NULL;

  } // end if camera collection

  // TODO remove and delete server CB

} // end dtor


AREXPORT void ArServerHandlerCameraCollection::getCameraList(ArServerClient *client, 
														                                 ArNetPacket *packet)
{
  if (client == NULL) {
    return; // Something very bad has happened...
  }

  ArNetPacket sendPacket;

  if (myCameraCollection == NULL) {
    sendPacket.byte2ToBuf(0);
    client->sendPacketTcp(&sendPacket);
  }

  // This lack of recursive locks is troublesome... Data might
  // change between calls...

  std::list<std::string> cameraNames;
  myCameraCollection->getCameraNames(cameraNames);
  
  sendPacket.byte2ToBuf(cameraNames.size());

  for (std::list<std::string>::iterator iter = cameraNames.begin();
       iter != cameraNames.end();
       iter++) {
  
    const char *curName = iter->c_str();

    sendPacket.strToBuf(curName);
     
    // TODO: ArNetPacket will NOT behave correctly if the given str is NULL
    // Fix this somehow...

    sendPacket.strToBuf(myCameraCollection->getCameraType(curName));
    sendPacket.strToBuf(myCameraCollection->getDisplayName(curName));
    sendPacket.strToBuf(myCameraCollection->getDisplayType(curName));

    // Send commands...

    std::list<std::string> commands;
    myCameraCollection->getCameraCommands(curName, commands);
  
    sendPacket.byte2ToBuf(commands.size());

    for (std::list<std::string>::iterator comIter = commands.begin();
         comIter != commands.end();
         comIter++) {
  
      const char *curCommand = comIter->c_str();
     
      sendPacket.strToBuf(curCommand);
      sendPacket.strToBuf(myCameraCollection->getCommandName(curName, curCommand));
      sendPacket.byte4ToBuf(myCameraCollection->getRequestInterval(curName, curCommand));

    } // end for each command
    
    // Send parameters...

    std::list<std::string> params;
    myCameraCollection->getParameterNames(curName, params);
  
    sendPacket.byte2ToBuf(params.size());

    ArConfigArg arg;
    ArClientArg clientArg;

    bool isSuccess = true;

    for (std::list<std::string>::iterator paramIter = params.begin();
         paramIter != params.end();
         paramIter++) {
  
      const char *paramName = paramIter->c_str();
      
      isSuccess = myCameraCollection->getParameter(curName,
                                                   paramName,
                                                   arg);
      if (!isSuccess) {
        ArLog::log(ArLog::Normal, 
                   "ArServerHandlerCameraCollection::getCameraList() could not find param %s", paramName);
        continue;
      }

      // Add the current parameter to the packet
      isSuccess = clientArg.createPacket(arg, &sendPacket);

    } // end for each parameter

  } // end for each camera

  client->sendPacketTcp(&sendPacket);

} // end method getCameraList
  


AREXPORT void ArServerHandlerCameraCollection::setParams(ArServerClient *client, 
                                                         ArNetPacket *packet)
{
  if ((packet == NULL) || (myCameraCollection == NULL)) {
    return;
  }
  char buffer[512];

  packet->bufToStr(buffer, sizeof(buffer));
  std::string cameraName = buffer;
  
  ArNetPacket paramUpdatedPacket;
  paramUpdatedPacket.strToBuf(cameraName.c_str());

  ArConfigArg arg;
  ArClientArg clientArg;
  bool isSuccess = true;
  bool isParamUpdated = false;

  bool isDone = false;

  while (!isDone) {

    packet->bufToStr(buffer, sizeof(buffer));
    std::string paramName = buffer;

    if (paramName.empty()) {
      isDone = true;
      break;
    }

    isSuccess = myCameraCollection->getParameter(cameraName.c_str(),
                                                 paramName.c_str(),
                                                 arg);

    if (!isSuccess) {
      ArLog::log(ArLog::Verbose, 
                 "ArServerHandlerCameraCollection::setParams() could not find camera %s, param %s", 
                 cameraName.c_str(), paramName.c_str());
      continue;
    }

    isSuccess = clientArg.bufToArgValue(packet, arg);

    if (!isSuccess) {
      continue;
    }

    isSuccess = myCameraCollection->setParameter(cameraName.c_str(), arg);

    if (!isSuccess) {
      continue;
    }

    paramUpdatedPacket.strToBuf(arg.getName());
    clientArg.argValueToBuf(arg, &paramUpdatedPacket);
    isParamUpdated = true;

  } // end for each param

  // TODO Send a confirmation back, or just use the broadcast packet?  (If a, then add
  // client to broadcast)

  // Add an empty string to indicate the last parameter...
  paramUpdatedPacket.strToBuf("");

  // Haven't quite decided whether to send the update packet, or a success/error notice...
  client->sendPacketTcp(&paramUpdatedPacket);


  if (isParamUpdated) {
    // NOT_EXCLUDING this used to not send it to the client, but no
    // longer has that behavior since it causes problems with the
    // central server
    myServer->broadcastPacketTcp(&paramUpdatedPacket, 
				 PARAMS_UPDATED_PACKET_NAME);
  }
   
  /**
  // TODO: Add error message?
  ArNetPacket retPacket;
  retPacket->strToBuf("");
  client->sendPacketTcp(&retPacket);
  **/

} // end method setParams


AREXPORT void ArServerHandlerCameraCollection::handleCameraCollectionModified()
{
  if (myServer == NULL) {
    return;
  }
  ArNetPacket emptyPacket;
  myServer->broadcastPacketTcp(&emptyPacket, COLLECTION_UPDATED_PACKET_NAME);

} // end method handleCameraCollectionModified



