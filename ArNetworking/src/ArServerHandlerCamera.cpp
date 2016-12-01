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
#include "ArServerHandlerCamera.h"

const char *ArServerHandlerCamera::CONTROL_COMMAND_GROUP = "CameraControl";
const char *ArServerHandlerCamera::INFO_COMMAND_GROUP = "CameraInfo";
const char *ArServerHandlerCamera::NO_ARGS = "None";

//#define DEBUG_ARSERVERHANDLERCAMERA

#if (defined(_DEBUG) && defined(DEBUG_ARSERVERHANDLERCAMERA))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

/**
   @param robot The robot we're attached to (mostly used for locking the camera before sending commands)
   @param server The server we're putting commands into
   @param camera The camera we're going to command and give information from... it can be NULL in which case no commands are added
 **/
AREXPORT ArServerHandlerCamera::ArServerHandlerCamera(ArServerBase *server,
						                                          ArRobot *robot, 
						                                          ArPTZ *camera) : 
  ArCameraCollectionItem(),
  myRobot(robot),
  myServer(server),
  myCamera(camera),
  myCameraName(),
  myCameraCollection(NULL),
  myCommandToPacketNameMap(),
  myCommandToIntervalMap(),
  myCommandToCBMap(),

  myUserTaskCB(this, &ArServerHandlerCamera::userTask),
  myCameraCB(this, &ArServerHandlerCamera::camera),
  myCameraAbsCB(this, &ArServerHandlerCamera::cameraAbs),
  myCameraUpdateCB(this, &ArServerHandlerCamera::cameraUpdate),
  myCameraInfoCB(this, &ArServerHandlerCamera::cameraInfo)
{
  init();
}

AREXPORT ArServerHandlerCamera::ArServerHandlerCamera
                                    (const char *cameraName,
                                     ArServerBase *server,
                                     ArRobot *robot,
                                     ArPTZ *camera,
                                     ArCameraCollection *collection) :
  ArCameraCollectionItem(),
  myRobot(robot),
  myServer(server),
  myCamera(camera),
  myCameraName(cameraName),
  myCameraCollection(collection),
  myCommandToPacketNameMap(),
  myCommandToIntervalMap(),
  myCommandToCBMap(),

  myModeMutex(),
  myCameraMode(CAMERA_MODE_POSITION),
  myCameraModeNameMap(),
  myCameraModePacket(),
  myLookAtPoint(),
  myPointResetZoom(false),
  myGoal(),
  myGoalAchieved(true),
  myGoalAchievedLast(false),
  myGoalResetZoom(false),

  myUserTaskCB(this, &ArServerHandlerCamera::userTask),
  myCameraCB(this, &ArServerHandlerCamera::camera),
  myCameraAbsCB(this, &ArServerHandlerCamera::cameraAbs),
  myCameraUpdateCB(this, &ArServerHandlerCamera::cameraUpdate),
  myCameraInfoCB(this, &ArServerHandlerCamera::cameraInfo)
{
  init();

  if (myCameraCollection != NULL) {
    doAddToCameraCollection(*myCameraCollection);
  }
} // end ctor

AREXPORT ArServerHandlerCamera::~ArServerHandlerCamera()
{
  for (std::map<std::string, ArFunctor2<ArServerClient *, ArNetPacket *> *>::iterator iter =
          myCommandToCBMap.begin();
       iter != myCommandToCBMap.end();
       iter++) {

     // TODO: REALLY should remove the callback from myServer (but there's currently
     // no way to do so...

     delete iter->second;
     iter->second = NULL;

  } // end for each callback

  myCommandToCBMap.clear();

} // end dtor
  

void ArServerHandlerCamera::init()
{
  myModeMutex.setLogName("ArServerHandlerCamera::myModeMutex");

  createCommandNames();

  createCommandCBs();

  addAllCommandsToServer();

  myCameraMode = CAMERA_MODE_POSITION;
  myGoalAchieved = true;
  myUserTaskCB.setName("ArServerHandlerCamera");
  if (myRobot != NULL)
    myRobot->addUserTask("ArServerHandlerCamera", 50, &myUserTaskCB);

  myCameraModeNameMap[CAMERA_MODE_POSITION] = "Position";
  myCameraModeNameMap[CAMERA_MODE_LOOK_AT_GOAL] = "LookAtGoal";
  myCameraModeNameMap[CAMERA_MODE_LOOK_AT_POINT] = "LookAtPoint";

} // end method init


AREXPORT const char *ArServerHandlerCamera::getCameraName()
{
  return myCameraName.c_str();
}

void ArServerHandlerCamera::createCommandNames()
{
  myCommandToPacketNameMap[ArCameraCommands::GET_CAMERA_DATA] = "getCameraData";
  myCommandToPacketNameMap[ArCameraCommands::GET_CAMERA_INFO] = "getCameraInfo";
  myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_ABS]  = "setCameraAbs";
  myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_PCT]  = "setCameraPct";
  myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_REL]  = "setCameraRel";

  myCommandToPacketNameMap[ArCameraCommands::GET_CAMERA_MODE_LIST]  = "getCameraModeList";
  myCommandToPacketNameMap[ArCameraCommands::CAMERA_MODE_UPDATED]  = "cameraModeUpdated";
  myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_MODE]  = "setCameraMode";
  myCommandToPacketNameMap[ArCameraCommands::RESET_CAMERA]  = "resetCamera";
	
  if (!myCameraName.empty()) {
  
    for (std::map<std::string, std::string>::iterator iter = myCommandToPacketNameMap.begin();
         iter != myCommandToPacketNameMap.end();
         iter++) {

      std::string baseCommandName = iter->second;
      iter->second = baseCommandName + myCameraName;

    } // end for each command
  } // end if camera name specified


  // Do these afterwards so that the name does NOT get mangled. (??)
  // This preserves some aspect of backwards compatibility...

 	myCommandToPacketNameMap[ArCameraCommands::GET_CAMERA_DATA_INT] = "cameraUpdate";
  myCommandToPacketNameMap[ArCameraCommands::GET_CAMERA_INFO_INT] = "cameraInfo";
	myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_ABS_INT] = "cameraAbs";
	myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_REL_INT] = "camera";
	myCommandToPacketNameMap[ArCameraCommands::SET_CAMERA_PCT_INT] = "cameraPct";

} // end method createCommandNames


void ArServerHandlerCamera::createCommandCBs()
{

  myCommandToCBMap[ArCameraCommands::GET_CAMERA_DATA] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleGetCameraData);
  // Default interval for requesting camera info
  myCommandToIntervalMap[ArCameraCommands::GET_CAMERA_DATA] = 100;

  myCommandToCBMap[ArCameraCommands::GET_CAMERA_INFO] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleGetCameraInfo);

	myCommandToCBMap[ArCameraCommands::SET_CAMERA_ABS] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleSetCameraAbs);

	myCommandToCBMap[ArCameraCommands::SET_CAMERA_PCT] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleSetCameraPct);

	myCommandToCBMap[ArCameraCommands::SET_CAMERA_REL] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleSetCameraRel);

 	myCommandToCBMap[ArCameraCommands::GET_CAMERA_DATA_INT] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::cameraUpdate);

  myCommandToCBMap[ArCameraCommands::GET_CAMERA_INFO_INT] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::cameraInfo);

	myCommandToCBMap[ArCameraCommands::SET_CAMERA_ABS_INT] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::cameraAbs);

	myCommandToCBMap[ArCameraCommands::SET_CAMERA_REL_INT] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::camera);

	myCommandToCBMap[ArCameraCommands::SET_CAMERA_PCT_INT] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::cameraPct);
	
  myCommandToCBMap[ArCameraCommands::GET_CAMERA_MODE_LIST] = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleGetCameraModeList);
  myCommandToCBMap[ArCameraCommands::CAMERA_MODE_UPDATED]  = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleCameraModeUpdated);
  myCommandToCBMap[ArCameraCommands::SET_CAMERA_MODE]  = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleSetCameraMode);
  myCommandToCBMap[ArCameraCommands::RESET_CAMERA]  = 
    new ArFunctor2C<ArServerHandlerCamera, ArServerClient *, ArNetPacket *>
          (this,
           &ArServerHandlerCamera::handleResetCamera);

} // end method createCommandCBs


void ArServerHandlerCamera::addAllCommandsToServer()
{
  if (myServer == NULL) {
    ArLog::log(ArLog::Normal, 
               "ArServerHandlerCamera::addAllCommandsToServer() cannot add to NULL server");
    return;
  }

  // 
  addCommandToServer
     (ArCameraCommands::GET_CAMERA_DATA, 
	    "Get \"double\" information about camera position. (factor == 100)",
	    NO_ARGS, 
	    "byte2: pan * factor, byte2: tilt * factor, byte2: zoomPercent * factor (optional)",
      INFO_COMMAND_GROUP);

  addCommandToServer
     (ArCameraCommands::GET_CAMERA_INFO, 
		  "Get (\"double\") information about the camera's pan, tilt, and (optional) zoom ranges. (factor == 100)",
		  NO_ARGS, 
		  "byte2: minPan * factor, byte2: maxPan * factor byte2: minTilt * factor, byte2: maxTilt * factor, byte2: minZoom * factor, byte2: maxZoom * factor, byte: isZoomAvailable",
      INFO_COMMAND_GROUP);

 addCommandToServer
     (ArCameraCommands::SET_CAMERA_ABS, 
      "Sends absolute (\"double\") pan, tilt, and zoom (optional) commands to the camera. (factor == 100)",
      NO_ARGS, 
      "byte2: pan * factor, byte2: tilt * factor, byte2: zoom * factor (optional)",
      CONTROL_COMMAND_GROUP);

 if ((myCamera != NULL) && (myCamera->canGetFOV())) {
    addCommandToServer
      (ArCameraCommands::SET_CAMERA_PCT, 
		    "Sends (\"double\") pan, tilt, and zoom (optional) commands to point camera, as percent of visible frame. (factor == 100)",
		    NO_ARGS,
		    "byte2: panOffset * factor, byte2: tiltOffset * factor, byte2: zoom * factor (optional)",
      CONTROL_COMMAND_GROUP);
 }
 else {
   removeCommand(ArCameraCommands::SET_CAMERA_PCT);
 }

 addCommandToServer
     (ArCameraCommands::SET_CAMERA_REL, 
		  "Sends (\"double\") pan, tilt, and zoom (optional) commands to the camera, relative to its current position. (factor == 100)",
		  NO_ARGS, 
		  "byte2: relPan * factor, byte2: relTilt * factor, byte2: relZoom * factor (optional)",
      CONTROL_COMMAND_GROUP);

 addCommandToServer(ArCameraCommands::GET_CAMERA_MODE_LIST,
		    "Gets the list of modes this camera supports",
		    NO_ARGS,
		    "ubyte2: numModes; <repeating numModes times> string: modeName",
        INFO_COMMAND_GROUP);
		    

 addCommandToServer(ArCameraCommands::CAMERA_MODE_UPDATED,
		    "Sent whenever the camera mode is updated",
		    NO_ARGS,
		    "string: modeName; <other data depends on mode>",
        INFO_COMMAND_GROUP);

 addCommandToServer(ArCameraCommands::SET_CAMERA_MODE,
		    "Sets the mode of the camera",
		    "string: modeName; <other data depends on mode>",
		    NO_ARGS,
        CONTROL_COMMAND_GROUP);		    

 addCommandToServer(ArCameraCommands::RESET_CAMERA,
		    "Resets the camera to 0 0 0 and may reinitialize it (depending on camera)",
		    NO_ARGS,
		    NO_ARGS,
        CONTROL_COMMAND_GROUP);		    
		    

  // Continue to support old-style integer commands for backward compatibility....

 addCommandToServer
   (ArCameraCommands::GET_CAMERA_DATA_INT, 
	  "Get (integer) information about camera position",
	  NO_ARGS, 
	  "byte2: pan, byte2: tilt, byte2: zoom (optional)",
    INFO_COMMAND_GROUP);

 addCommandToServer
   (ArCameraCommands::GET_CAMERA_INFO_INT, 
		"Get (integer) information about the camera's pan, tilt, and (optional) zoom ranges",
		NO_ARGS, 
		"byte2: minPan, byte2: maxPan byte2: minTilt, byte2: maxTilt, byte2: minZoom, byte2: maxZoom, byte: isZoomAvailable",
    INFO_COMMAND_GROUP);

 addCommandToServer
   (ArCameraCommands::SET_CAMERA_REL_INT, 
		"Sends (integer) pan, tilt, and zoom (optional) commands to the camera, relative to its current position",
		NO_ARGS, 
		"byte: relPan, byte: relTilt, byte: relZoom (optional)",
    CONTROL_COMMAND_GROUP);

 addCommandToServer
   (ArCameraCommands::SET_CAMERA_ABS_INT, 
    "Sends absolute (integer) pan, tilt, and zoom (optional) commands to the camera",
    NO_ARGS, 
    "byte: pan, byte: tilt, byte: zoom (optional)",
    CONTROL_COMMAND_GROUP);

 addCommandToServer
   (ArCameraCommands::SET_CAMERA_PCT_INT, 
    "Sends (integer) pan, tilt commands to the camera, as percent of visible frame",
    NO_ARGS, 
    "byte: pan, byte: tilt, byte: zoom (optional)",
    CONTROL_COMMAND_GROUP);

} // end method addToServer


void ArServerHandlerCamera::addCommandToServer(const char *command, 
                                               const char *description,
				                                       const char *argumentDescription,
				                                       const char *returnDescription,
                                               const char *commandGroup)
{
  if (command == NULL) {
    ArLog::log(ArLog::Normal, 
               "ArServerHandlerCamera::addCommandToServer() cannot add NULL command");
    return;
  }

  std::map<std::string, std::string>::iterator iter = 
    myCommandToPacketNameMap.find(command);

  if (iter == myCommandToPacketNameMap.end()) {
    ArLog::log(ArLog::Normal, 
               "ArServerHandlerCamera::addCommandToServer() cannot find packet name for command %s",
               command);
    return;
  }
  
  const char *packetName = iter->second.c_str();
  ArFunctor2<ArServerClient *, ArNetPacket *> *callback = NULL;

  std::map<std::string, ArFunctor2<ArServerClient *, ArNetPacket *> *>::iterator cbIter =
    myCommandToCBMap.find(command);
  if (cbIter != myCommandToCBMap.end()) {
    callback = cbIter->second;
  }
  if (callback == NULL) {
    ArLog::log(ArLog::Normal, 
               "ArServerHandlerCamera::addCommandToServer() cannot find callback for command %s",
               command);
  }

  bool isSuccess = true;

  /**
   * It'd be kind of nice if we could do something along the following lines
   * so that the PTZ controls do not show up in ME.  (Not entirely sure how
   * though because - at least in the case of VCC4 - there's a four second
   * timeout before the comm is deemed unidirectional.)
  if ((myCamera != NULL) &&
      (!myCamera->isBidirectionalComm()) &&
      (strcmp(commandGroup, CONTROL_COMMAND_GROUP) == 0)) {
    isSuccess = false;
  }
  **/

  if (isSuccess) {
    isSuccess = myServer->addData
                       (packetName, 
		                    description,
		                    callback,
		                    argumentDescription, 
		                    returnDescription,
                        commandGroup, "RETURN_SINGLE");
  }
  if (!isSuccess) {
    ArLog::log(ArLog::Normal, 
               "ArServerHandlerCamera::addCommandToServer() error adding server handler for packet %s",
               packetName);
    removeCommand(command);
  }

} // end method addCommandToServer


void ArServerHandlerCamera::doAddToCameraCollection(ArCameraCollection &collection)
{
  if (myCameraName.empty()) {
  
    return;
  } // end if no camera name

  const char *cameraName = myCameraName.c_str();
  if (!collection.exists(cameraName)) {

    return;
  } // end if camera not yet created

  for (std::map<std::string, std::string>::iterator iter = myCommandToPacketNameMap.begin();
        iter != myCommandToPacketNameMap.end();
        iter++) {

    const char *command = iter->first.c_str();
    const char *packetName = iter->second.c_str();

    int requestInterval = -1; 
    std::map<std::string, int>::iterator nIter = myCommandToIntervalMap.find(command);
    if (nIter != myCommandToIntervalMap.end()) {
      requestInterval = nIter->second;
    }

    bool isSuccess = collection.addCameraCommand(cameraName,
                                                 command,
                                                 packetName,
                                                 requestInterval);

    if (!isSuccess) {

    } // end if error occurred

  } // end for each command
} // end method doAddToCameraCollection


void ArServerHandlerCamera::removeCommand(const char *command)
{
  std::map<std::string, std::string>::iterator iter = myCommandToPacketNameMap.find(command);
  if (iter != myCommandToPacketNameMap.end()) {
    myCommandToPacketNameMap.erase(iter);
  }

  std::map<std::string, ArFunctor2<ArServerClient *, ArNetPacket *> *>::iterator cbIter =
    myCommandToCBMap.find(command);
  if (cbIter != myCommandToCBMap.end()) {
    delete cbIter->second;
    cbIter->second = NULL;
    myCommandToCBMap.erase(cbIter);
  }

} // end method removeCommand


AREXPORT void ArServerHandlerCamera::addToCameraCollection(ArCameraCollection &collection)
{
  doAddToCameraCollection(collection);
}

// -----------------------------------------------------------------------------
// Packet Handlers:
// -----------------------------------------------------------------------------

AREXPORT void ArServerHandlerCamera::handleGetCameraData(ArServerClient *client, 
                                                         ArNetPacket *packet)
{
  if ((client == NULL) || (myRobot == NULL) || (myCamera == NULL)) {
    return;
  }

  ArNetPacket sendPacket;

  myRobot->lock();
  double pan = myCamera->getPan();
  double tilt = myCamera->getTilt();
  double zoom = (getCurrentZoomRatio() * 100.0);

  myRobot->unlock();

  addDoubleToPacket(pan, sendPacket);
  addDoubleToPacket(tilt, sendPacket);
  addDoubleToPacket(zoom, sendPacket);

  client->sendPacketUdp(&sendPacket);

} // end method handleGetCameraData


AREXPORT void ArServerHandlerCamera::handleGetCameraInfo(ArServerClient *client, 
                                                         ArNetPacket *packet)
{
  if ((client == NULL) || (myRobot == NULL) || (myCamera == NULL)) {
    return;
  }

  ArNetPacket sendPacket;

  myRobot->lock();

  double minPan  = myCamera->getMaxNegPan();
  double maxPan  = myCamera->getMaxPosPan();
  double minTilt = myCamera->getMaxNegTilt();
  double maxTilt = myCamera->getMaxPosTilt();
  double minZoom = 0;
  double maxZoom = 100;
  bool isZoomAvailable = myCamera->canZoom();

  ArLog::log(ArLog::Verbose, 
             "ArServerHandlerCamera: client requested camera info (my camera name is %s), returning: minPan %f maxPan %f minTilt %f maxTilt %f minZoom %f maxZoom %f isZoomAvailable %d", 
             myCameraName.c_str(), minPan, maxPan, minTilt, maxTilt, minZoom, maxZoom, isZoomAvailable);

  myRobot->unlock();

  addDoubleToPacket(minPan, sendPacket);
  addDoubleToPacket(maxPan, sendPacket);
  addDoubleToPacket(minTilt, sendPacket);
  addDoubleToPacket(maxTilt, sendPacket);
  addDoubleToPacket(minZoom, sendPacket);
  addDoubleToPacket(maxZoom, sendPacket);

  sendPacket.byteToBuf(isZoomAvailable);

  client->sendPacketUdp(&sendPacket);

} // end method handleGetCameraInfo


AREXPORT void ArServerHandlerCamera::handleSetCameraAbs(ArServerClient *client, 
                                                        ArNetPacket *packet)
{
  if ((packet == NULL) || (myRobot == NULL) || (myCamera == NULL)) {
    return;
  }

  double pan =  getDoubleFromPacket(*packet);
  double tilt = getDoubleFromPacket(*packet);
  double zoom = getDoubleFromPacket(*packet);

  setCameraAbs(pan, tilt, zoom);

} // end method handleSetCameraAbs


AREXPORT void ArServerHandlerCamera::handleSetCameraRel(ArServerClient *client, 
                                                        ArNetPacket *packet)
{
  if (packet == NULL) {
    return;
  }

  //double invAtZoom = 1;

  double pan =  getDoubleFromPacket(*packet);
  double tilt = getDoubleFromPacket(*packet);
  double zoom = getDoubleFromPacket(*packet);

  setCameraRel(pan, tilt, zoom);

} // end method handleSetCameraRel


AREXPORT void ArServerHandlerCamera::handleSetCameraPct(ArServerClient *client, 
											                                  ArNetPacket *packet)
{

  if (!myCamera->canGetFOV())
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerCamera::cameraPct called when camera can't get the FOV");
    return;
  }
  // Values range -100 to 100.  0 indicates no change.
  //int panPct = packet->bufToByte();
  //int tiltPct = -packet->bufToByte();

  double panPct  = getDoubleFromPacket(*packet);
  double tiltPct = -(getDoubleFromPacket(*packet));

  setCameraPct(panPct, tiltPct);

} // end method handleSetCameraPct


// ----------------------------------------------------------------------------
// Old Packet Handlers (integers, for backwards compatibility)
// ----------------------------------------------------------------------------


AREXPORT void ArServerHandlerCamera::camera(ArServerClient *client, 
					                                  ArNetPacket *packet)
{

  //double invAtZoom = 1;

  int pan = packet->bufToByte();
  int tilt = packet->bufToByte();
  int zoom = packet->bufToByte();

  setCameraRel(pan, tilt, zoom);

} // end method camera


AREXPORT void ArServerHandlerCamera::cameraAbs(ArServerClient *client, 
				                                       ArNetPacket *packet)
{

  int pan = packet->bufToByte();
  int tilt = packet->bufToByte();
  int zoom = packet->bufToByte();

  setCameraAbs(pan, tilt, zoom);

} // end method cameraAbs


AREXPORT void ArServerHandlerCamera::cameraPct(ArServerClient *client, 
											                         ArNetPacket *packet)
{

  if (!myCamera->canGetFOV())
  {
    ArLog::log(ArLog::Normal, "ArServerHandlerCamera::cameraPct called when camera can't get the FOV");
    return;
  }
  // Values range -100 to 100.  0 indicates no change.
  int panPct = packet->bufToByte();
  int tiltPct = -packet->bufToByte();

  setCameraPct(panPct, tiltPct);

} // end method cameraPct


AREXPORT void ArServerHandlerCamera::cameraUpdate(ArServerClient *client, 
				                                          ArNetPacket *packet)
{
  ArNetPacket send;

  myRobot->lock();
  int pan  = ArMath::roundInt(myCamera->getPan());
  int tilt = ArMath::roundInt(myCamera->getTilt());
  int zoom = ArMath::roundInt(myCamera->getZoom()/((double)(myCamera->getMaxZoom() - myCamera->getMinZoom()) + myCamera->getMinZoom()) * 100.0);

  myRobot->unlock();

  send.byte2ToBuf(pan);
  send.byte2ToBuf(tilt);
  send.byte2ToBuf(zoom);
  client->sendPacketUdp(&send);

} // end method cameraUpdate


AREXPORT void ArServerHandlerCamera::cameraInfo(ArServerClient *client, 
												                        ArNetPacket *packet)
{
  ArNetPacket send;

  myRobot->lock();

  int minPan = ArMath::roundInt(myCamera->getMaxNegPan());
  int maxPan = ArMath::roundInt(myCamera->getMaxPosPan());
  int minTilt = ArMath::roundInt(myCamera->getMaxNegTilt());
  int maxTilt = ArMath::roundInt(myCamera->getMaxPosTilt());
  int minZoom = 0;
  int maxZoom = 100;
  bool isZoomAvailable = myCamera->canZoom();
  //ArLog::log(ArLog::Normal, "minPan %d maxPan %d minTilt %d maxTilt %d minZoom %d maxZoom %d isZoomAvailable %d", minPan, maxPan, minTilt, maxTilt, minZoom, maxZoom, isZoomAvailable);

  myRobot->unlock();

  send.byte2ToBuf(minPan);
  send.byte2ToBuf(maxPan);
  send.byte2ToBuf(minTilt);
  send.byte2ToBuf(maxTilt);
  send.byte2ToBuf(minZoom);
  send.byte2ToBuf(maxZoom);
  send.byteToBuf(isZoomAvailable);

  client->sendPacketUdp(&send);

} // end method cameraInfo


/**
   @param pan the degrees to pan to (absolute)
   @param tilt the degrees to tilt to (absolute)
   @param zoom the percent (number between 0 and 100) to pan to (absolute)
   @param lockRobot whether to lock the robot or not
**/

AREXPORT void ArServerHandlerCamera::setCameraAbs(
	double pan, double tilt, double zoom, bool lockRobot)
{
 if (zoom > 100)
    zoom = 100;
 else if (zoom < 0)
   zoom = 0;
 
 if (lockRobot)
   myRobot->lock();
 
 cameraModePosition();
 
 myCamera->panTilt(pan, tilt);
 
 double absZoom = myCamera->getMinZoom() + ((zoom / 100.0) * getZoomRange());
 
 IFDEBUG(ArLog::log(ArLog::Normal,
		    "ArServerHandlerCamera::handleSetCameraAbs() p = %f, t = %f, z = %f, calc absZoom = %f",
		    pan, tilt, zoom, absZoom));
 
 myCamera->zoom(ArMath::roundInt(absZoom));
 
 if (lockRobot)
   myRobot->unlock();
 
} // end method doSetCameraAbs


/**
   @param pan the degrees to pan to (relative)
   @param tilt the degrees to tilt to (relative)
   @param zoom the percent (number between 0 and 100) to change pan by (relative)
   @param lockRobot whether to lock the robot or not
**/

AREXPORT void ArServerHandlerCamera::setCameraRel(
	double pan, double tilt, double zoom, bool lockRobot)
{
  if ((myRobot == NULL) || (myCamera == NULL)) {
    return;
  }

  IFDEBUG(ArLog::log(ArLog::Normal,
                    "ArServerHandlerCamera::doSetCameraRel() p = %f, t = %f, z = %f",
                    pan, tilt, zoom));

  // The camera mode is set to position only if the pan or tilt changed.

  double invAtZoom = 1;

  if (myCamera->getMaxZoom() - myCamera->getMinZoom() != 0)
  {
    // see what zoom we're at so we can modify total pan and tilt amounts by it
    invAtZoom = 1 - getCurrentZoomRatio();

    //totalPanAmount /= atZoom / (100 / (double) totalPanAmount);
    //totalTiltAmount /= atZoom / (100 / (double) totalTiltAmount);
    if (invAtZoom < .01)
      invAtZoom = .01;
    if (invAtZoom > 1.01)
      invAtZoom = 1;
  }
  else
  {
    invAtZoom = 1;
  }

  if (pan > 0)
    pan = ceil(pan * invAtZoom);
  else
    pan = floor(pan * invAtZoom);

  if (tilt > 0)
    tilt = ceil(tilt * invAtZoom);
  else
    tilt = floor(tilt * invAtZoom);

  if (lockRobot)
    myRobot->lock();

  if (zoom != 0)
  {
    double newZoomPct = (getCurrentZoomRatio() * 100.0) + zoom;

    double newAbsZoom = myCamera->getMinZoom() + ((newZoomPct / 100.0) * getZoomRange());

    IFDEBUG(ArLog::log(ArLog::Normal,
                      "ArServerHandlerCamera::handleSetCameraRel() newZoomPct = %f, newAbsZoom = %f",
                       newZoomPct, newAbsZoom));

    myCamera->zoom(ArMath::roundInt(newAbsZoom));

  } // end if zoom change
  
  // Only want to pan...
  if ((pan == 0) && (tilt != 0)) 
  {
    cameraModePosition();
    myCamera->tiltRel(tilt);
  }
  // Only want to tilt...
  else if ((pan != 0) && (tilt == 0)) 
  {
    cameraModePosition();
    myCamera->panRel(pan);
  }
  else if (pan != 0 && tilt != 0) 
  { // pan and tilt...
    cameraModePosition();
    myCamera->panTiltRel(pan, tilt);
  }

  if (lockRobot)
    myRobot->unlock();

} // end method doSetCameraRel



/**
   @param panPct the double percent offset from the image frame center
   by which to pan the camera; values range -100 to 100.  0 indicates
   no change.  

   @param tiltPct the double percent offset from the image
   frame center by which to tilt the camera; values range -100 to 100.
   0 indicates no change.
   
   @param lockRobot whether to lock the robot or not
**/

AREXPORT void ArServerHandlerCamera::setCameraPct(
	double panPct, double tiltPct, bool lockRobot)
{
  cameraModePosition();

  double zoom = getCurrentZoomRatio();
  
  IFDEBUG(ArLog::log(ArLog::Normal,
                    "ArServerHandlerCamera::handleSetCameraPct() panPct = %f, tiltPct = %f curZoomRatio = %f",
                      panPct, tiltPct, zoom));

  /*
     So first we find the field of view we're at, this is interpolated
     from the max and min zooms... with an exponential factor (because
     empircally it seemed to be needed
  */

  double exponent = M_PI/2; // found this to be 1.6 so this seems a
			    // likely guess
  double fov = ((myCamera->getFOVAtMinZoom() - myCamera->getFOVAtMaxZoom()) * 
	 pow((1 - zoom), exponent)  + 
	          myCamera->getFOVAtMaxZoom());


  double oldTilt = myCamera->getTilt();
  double oldPan = myCamera->getPan();


  /*
    So this whole thing is done as a vector calculations... so first
    we find where our old x, y, z value was to start with then we add
    in the x, y, z change from the left/right component of the image
    click and from the up/down component of the image click to find
    our final x, y, z and then we figure out the pan/tilt that points
    us there... this is basically all assuming a unit sphere (ie
    length 1)

    One thing that could be done to make it more accurate is to remove
    some of the error (ie right now from going straight right in the
    image at 0 all we do is add x, if we removed a little y too to
    normalize it on the unit sphere it'd be more accurate).
  **/

  /*
   
    Derivation of these equations (for finding the x, y, z from
    current pan tilt)

	   |   ,
	   |  /|z 
	   | / |
	   |/t |y
	   -)--|-------
	  / \)p|  /
	 /  h\ | / x
	/     \|/
	

     Where pan and tilt are the values we're panned and tilted at now
     and x, y, and z are the coords in the unit sphere that we
     have... h is the hypoenuse of the line projected down into the
     x/y plane


     sin tilt = z / 1 (because its the unit sphere)   
!    sin tilt = z                -,                  
                                  |                  
     tan tilt = z / hyp           |                
     tan tilt = sin tilt / hyp  <-'
     hyp = sin tilt / tan tilt            -,   -,
                                           |    | 
     sin pan = x / hyp                     |    | 
     sin pan * hyp = x                     |    |
     x = hyp * sin pan                     |    |
!    x = (sin tilt/tan tilt) * sin pan   <-'    |
                                                |
     cos pan = y / hyp                          |          
     cos pan * hyp = y                          |
     y = hyp * sin pan                          | 
!    y = (sin tilt/tan tilt) * cos pan        <-'
     

   The old X here should be      sin(tilt)
                                 --------- * sin(pan)
			         tan(tilt) 

   The old Y here should be      sin(tilt)
                                 --------- * cos(pan)
			         tan(tilt) 

   The old z here should be      sin(tilt);

   The funkiness is because of divide by zeros and stuff, this is
   supposed to be the hypot though and so it can't really ever be 0 or
   infnity so just cap the thing
 **/
  double sinTiltOverTanTilt;
  if (fabs(oldTilt) < .01)
    sinTiltOverTanTilt = 1;
  else
    sinTiltOverTanTilt = ArMath::sin(oldTilt) / ArMath::tan(oldTilt);
  double oldX = sinTiltOverTanTilt * ArMath::sin(oldPan);
  double oldY = sinTiltOverTanTilt * ArMath::cos(oldPan);	 
  double oldZ = ArMath::sin(oldTilt);

  // figure out the angle of the left/right point in the image
  double lr = panPct / 100.0 * fov/2.0;

  /**
     Figure how much the x/y/z components of the left/right vector,
     this part is pretty straightforward since its just multiplying
     the left/right vector by the cos or sin to get the component in
     that direction (which is based on the pan)

     @verbatim 
     ------------------   y 
     |\)lr |
     | \   | sin lr
     |  \  | 
     |   \ |
     |    \|
     x
     @endverbatim

     That derives the sin lr for the length of the vector... for how
     much of that is in which direction

     @verbatim
         dx = sin lr * sin pan
     ------------------------- y
     |\)pan  |
     | \     |
     |  \slr |  dy = sin lr * cos pan
     |   \   |
     |    \  |
     |     \ |
     |      \|
     x
     @endverbatim
     
     Where slr is the sin lr derived above and pan is the pan angle
     (since we're doing vectors the position we calculate these
     components from doesn't matter so we just move it back to 0 to
     make life easier)... 
     
     The - on the sin is needed, I think its because the camera pans
     positive to the right and negative to the left instead of
     positive to the right.
     
    **/
  double lrDx = + ArMath::sin(lr) * ArMath::cos(oldPan);
  double lrDy =  - ArMath::sin(lr) * ArMath::sin(oldPan);
  double lrDz = 0;

  // the angle of the up/down point in the image
  double ud = tiltPct / 100.0 * fov/2.0 *.71;

  /**
     Figure how much the x/y/z components of the up/down vector, 

     So for x and y first we find how much is in the x/y plane
     (sin(ud) * sin(oldTilt)) and then we multiply that by the same
     sin or cos of the pan to see how much is in which coord...

     For z we just multiply the up/down vector by the cos to find what
     the movement in z is.
   **/
  double udDx =  - (ArMath::sin(ud) * ArMath::sin(oldTilt) * 
		     ArMath::sin(oldPan));
  double udDy =  - (ArMath::sin(ud) * ArMath::sin(oldTilt) * 
		     ArMath::cos(oldPan));
  double udDz = ArMath::sin(ud) * ArMath::cos(oldTilt);


  /**
     Now add the original vector, the vector from left/right and the
     vector from up/down together.
   **/
  double x = oldX + lrDx + udDx;
  double y = oldY + lrDy + udDy;
  double z = oldZ + lrDz + udDz;


  /** 
     Now find where we have to pan and tilt to... 

	   |   ,
	   |  /|z 
	   | / |
	   |/t |y
	   -)--|-------
	  / \)p|  /
	 /  h\ | / x
	/     \|/
	
     Where x, y, and z are the values we found above and tilt and pan
     are the tilt (angle of the line off of the x/y plane) and pan
     values (angle of the line within the x/y plain) we need to point
     at that point, h is the hypotenuse of the line projected into the
     x/y plane.
     
     

     tan pan = x / y
!    pan = atan (x / y)               

     cos pan = y / hyp
     hyp = y / cos pan

     sin pan = x / hyp
     hyp = x / sin pan

     tan tilt = z / hyp
     
!    tilt = atan (z / hyp)
     (substitute in one of the hyps)
  **/

  /**
     Pan is easy since its just the atan of x and y.
   **/
  double newPan = ArMath::atan2(x, y);
  double tiltDenom;
  /**
     This denom can be either y/cos(pan) or x / sin(pan) depending on
     which solution you use and we use them in a way that won't get us
     a divide by 0.
   **/ 
  if (fabs(oldPan) < 1)
    tiltDenom = y / ArMath::cos(newPan);  
  else if (fabs(oldPan - 90) < 1)
    tiltDenom = x / ArMath::sin(newPan);  
  else
    tiltDenom = (y / ArMath::cos(newPan) + x / ArMath::sin(newPan)) / 2;
  double newTilt = ArMath::atan2(z, tiltDenom);
  if (lockRobot)
    myRobot->lock();
  myCamera->panTilt(newPan, newTilt);
  if (lockRobot)
    myRobot->unlock();
  /*
  ArLog::log(ArLog::Normal, "w %f h %f zoom %f fov %g oldPan %g oldTilt %g oldx %g oldy %g oldz %g x %g y %g z %g newPan %g newTilt %g",
	     w, h, zoom, fov, oldPan, oldTilt, oldX, oldY, oldZ, x, y, z, newPan, newTilt);
  */

} // end method doSetCameraPct

// ----------------------------------------------------------------------------
// Helper Methods
// ----------------------------------------------------------------------------

AREXPORT void ArServerHandlerCamera::handleGetCameraModeList(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sending;

  myModeMutex.lock();
  sending.uByte2ToBuf(myCameraModeNameMap.size());

  std::map<CameraMode, std::string>::iterator it;
  for (it = myCameraModeNameMap.begin(); it != myCameraModeNameMap.end(); it++)
  {
    sending.strToBuf((*it).second.c_str());
  }
  myModeMutex.unlock();
  client->sendPacketTcp(&sending);
}

AREXPORT void ArServerHandlerCamera::handleCameraModeUpdated(
	ArServerClient *client, ArNetPacket *packet)
{
  myModeMutex.lock();
  client->sendPacketTcp(&myCameraModePacket);
  myModeMutex.unlock();
}

AREXPORT void ArServerHandlerCamera::handleSetCameraMode(
	ArServerClient *client, ArNetPacket *packet)
{
  char modeName[512];
  packet->bufToStr(modeName, sizeof(modeName));


  CameraMode newMode;
  std::map<CameraMode, std::string>::iterator it;
  ArPose lookPoint;

  for (it = myCameraModeNameMap.begin(); it != myCameraModeNameMap.end(); it++)
  {
    if (ArUtil::strcasecmp((*it).second, modeName) == 0)
    {
      newMode = (*it).first;
      if (newMode == CAMERA_MODE_POSITION)
      {
	      cameraModePosition();
      }
      else if (newMode == CAMERA_MODE_LOOK_AT_GOAL)
      {
	      cameraModeLookAtGoal();
      }
      else if (newMode == CAMERA_MODE_LOOK_AT_POINT)
      {
	      lookPoint.setX(packet->bufToByte4());
	      lookPoint.setY(packet->bufToByte4());
	      cameraModeLookAtPoint(lookPoint);
      }
      return;
    }
  }

  ArLog::log(ArLog::Normal, 
	     "ArServerHandlerCamera::%s: Could not find mode %s to switch to", 
	     myCameraName.c_str(), modeName);
}

void ArServerHandlerCamera::userTask(void)
{
  myModeMutex.lock();
  if (myCameraMode == CAMERA_MODE_LOOK_AT_GOAL)
  {
    if (!myGoalResetZoom)
    {
      myCamera->zoom(0);
      myGoalResetZoom = true;
    }
    if (!myGoalAchieved)
    {
      myCamera->panTilt(-myRobot->findDeltaHeadingTo(myGoal), 0);
    }
    else if (!myGoalAchievedLast)
    {
      myCamera->panTilt(0, 0);
      myGoalAchievedLast = true;
    }
  }
  if (myCameraMode == CAMERA_MODE_LOOK_AT_POINT)
  {
    if (!myPointResetZoom)
    {
      myCamera->zoom(0);
      myPointResetZoom = true;
    }
    myCamera->panTilt(-myRobot->findDeltaHeadingTo(myLookAtPoint), 0);
  }
  myModeMutex.unlock();
}

void ArServerHandlerCamera::buildModePacket(void)
{
  myCameraModePacket.empty();

  myCameraModePacket.strToBuf(myCameraModeNameMap[myCameraMode].c_str());
  if (myCameraMode == CAMERA_MODE_LOOK_AT_POINT)
  {
    myCameraModePacket.byte4ToBuf(
	    ArMath::roundInt(myLookAtPoint.getX()));
    myCameraModePacket.byte4ToBuf(
	    ArMath::roundInt(myLookAtPoint.getY()));
  }
}

AREXPORT void ArServerHandlerCamera::cameraModePosition(void)
{
  myModeMutex.lock();
  if (myCameraMode != CAMERA_MODE_POSITION)
  {
    ArLog::log(ArLog::Normal, "Looking by position");
    myCameraMode = CAMERA_MODE_POSITION;
    buildModePacket();
    myServer->broadcastPacketTcp(
	    &myCameraModePacket,
	    myCommandToPacketNameMap[
		    ArCameraCommands::CAMERA_MODE_UPDATED].c_str());
	    
  }
  myModeMutex.unlock();
}

AREXPORT void ArServerHandlerCamera::cameraModeLookAtGoal(void)
{
  myModeMutex.lock();
  if (myCameraMode != CAMERA_MODE_LOOK_AT_GOAL)
  {
    ArLog::log(ArLog::Normal, "Looking at goal");
    myCameraMode = CAMERA_MODE_LOOK_AT_GOAL;
    myGoalAchievedLast = myGoalAchieved;
    myGoalResetZoom = false;

    buildModePacket();
    myServer->broadcastPacketTcp(
	    &myCameraModePacket,
	    myCommandToPacketNameMap[
		    ArCameraCommands::CAMERA_MODE_UPDATED].c_str());
  }
  myModeMutex.unlock();
}

AREXPORT void ArServerHandlerCamera::cameraModeLookAtPoint(ArPose pose,
							   bool controlZoom)
{
  myModeMutex.lock();
  if (myCameraMode != CAMERA_MODE_LOOK_AT_POINT || 
      myLookAtPoint.findDistanceTo(pose) > 1)
  {
    ArLog::log(ArLog::Normal, "Looking at point %.0f %.0f", 
	       pose.getX(), pose.getY());
    if (myCameraMode != CAMERA_MODE_LOOK_AT_POINT && controlZoom)
      myPointResetZoom = false;
    myCameraMode = CAMERA_MODE_LOOK_AT_POINT;
    myLookAtPoint = pose;
    buildModePacket();
    myServer->broadcastPacketTcp(
	    &myCameraModePacket,
	    myCommandToPacketNameMap[
		    ArCameraCommands::CAMERA_MODE_UPDATED].c_str());
  }
  myModeMutex.unlock();
}

AREXPORT void ArServerHandlerCamera::cameraModeLookAtGoalSetGoal(ArPose pose)
{
  myModeMutex.lock();
  myGoal = pose;
  myGoalAchieved = false;
  myGoalAchievedLast = false;
  myGoalResetZoom = false;
  myModeMutex.unlock();
}

AREXPORT void ArServerHandlerCamera::cameraModeLookAtGoalClearGoal(void)
{
  myModeMutex.lock();
  myGoalAchieved = true;
  myModeMutex.unlock();
}

AREXPORT void ArServerHandlerCamera::handleResetCamera(
	ArServerClient *client, ArNetPacket *packet)
{
  resetCamera();
}

AREXPORT void ArServerHandlerCamera::resetCamera(bool lockRobot)
{
  if (lockRobot)
    myRobot->lock();
  myCamera->reset();
  if (lockRobot)
    myRobot->unlock();
  cameraModePosition();  
}

