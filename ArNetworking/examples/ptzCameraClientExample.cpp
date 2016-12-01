#include "Aria.h"
#include "ArNetworking.h"
#include <math.h>


/** @example ptzCameraClientExample.cpp Example client showing how to control a Pan/Tilt/Zoom
 * camera remotely. 
 *
 * To use, serverDemo, arnlServer from ARNL, sonarnlServer from SONARNL, or
 * your own server program that includes an ArServerHandlerCamera object.
 * Then from another host, run ptzCameraClientExample with the -host argument.  
 * For example, if the hostname of the robot's onboard computer is "robot", run:
 *   ptzCameraClientExample -host robot
 * Or use an IP address, for example, if the address is 10.0.126.32:
 *   ptzCameraClientExample -host 10.0.126.32
 *
 * This program connects to the server, gets a list of cameras if neccesary, and
 * for each camera on the robot, it receives and prints out one information
 * packet, then a continuous stream of data packets, and then sends requests
 * to cycle through each of its pan and tilt limits.
 *
 * This program does not get any video images from the server. To do so, use
 * a video server (e.g. SAV server or ACTS), and see getVideoExample for an
 * example client.
 */


/** A request packet that is able to send a copy of itself, with data packed in,
 * to a server.
 */
class ArNetCameraRequest : public ArNetPacket
{
  ArClientBase *myClient;
  std::string myAbsReqName;
public:
  ArNetCameraRequest(ArClientBase *client, const char *cameraName = "") : myClient(client), myAbsReqName(std::string("setCameraAbs")+cameraName)
  {}
  void setCameraName(const char *name) {
    myAbsReqName = std::string("setCameraAbs") + name;
  }
  bool requestPanTiltZoomAbs(double pan, double tilt, double zoom);
  bool requestPanTiltAbs(double pan, double tilt);
};


/** Includes callbacks for receiving info and data packets from a
 * camera server and storing the values parsed from them.
 */
class ArClientHandlerCamera {
public:
  std::string name;
  std::string type;
  std::string displayName;
  std::string displayType;
  double minPan, maxPan, minTilt, maxTilt, minZoom, maxZoom;
  bool haveZoom;
  double pan, tilt, zoom;

  void handleCameraInfoReply(ArNetPacket *packet);
  void handleCameraDataReply(ArNetPacket *packet);

  ArClientBase *myClient;
  ArNetCameraRequest request;
  ArMutex myMutex;

  ArFunctor1C<ArClientHandlerCamera, ArNetPacket*> myCameraInfoReplyFunc;
  ArFunctor1C<ArClientHandlerCamera, ArNetPacket*> myCameraDataReplyFunc;

  ArClientHandlerCamera(ArClientBase *client, const char *cameraName) : 
    name(cameraName), myClient(client), request(client, cameraName) ,
    myCameraInfoReplyFunc(this, &ArClientHandlerCamera::handleCameraInfoReply),
    myCameraDataReplyFunc(this, &ArClientHandlerCamera::handleCameraDataReply)
  {
    myClient->addHandler((std::string("getCameraInfo")+name).c_str(), &myCameraInfoReplyFunc);
    myClient->addHandler((std::string("getCameraData")+name).c_str(), &myCameraDataReplyFunc);
  }

  void requestUpdates(int dataRequestFreq) {
    myClient->request((std::string("getCameraInfo")+name).c_str(), -1);
    myClient->request((std::string("getCameraData")+name).c_str(), 100);
  }

  void lock() { myMutex.lock(); }
  void unlock() { myMutex.unlock(); }
};


/** Requests a list of cameras if neccesary, maintains a list of cameras, and implements example pan/tilt movements */
class PtzCameraExample
{
  ArClientBase *myClient;

  std::set<ArClientHandlerCamera*> myCameras;
  ArMutex mutex;

  ArFunctor1C<PtzCameraExample, ArNetPacket*> myCameraListReplyFunc;

  void handleCameraListReply(ArNetPacket *packet);


public:
  PtzCameraExample(ArClientBase *client) : 
    myClient(client), 
    myCameraListReplyFunc(this, &PtzCameraExample::handleCameraListReply)
  {
  }

  bool init();
  void run();
};



int main(int argc, char **argv)
{
  Aria::init();
  ArClientBase client;

  ArArgumentParser parser(&argc, argv);
  ArClientSimpleConnector clientConnector(&parser);
  parser.loadDefaultArguments();
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    Aria::logOptions();
    return 1;
  }

  if (!clientConnector.connectClient(&client))
  {
    if(client.wasRejected())
      ArLog::log(ArLog::Terse, "Error, server '%s' rejected connection. Exiting.", client.getHost());
    else
      ArLog::log(ArLog::Terse, "Error, could not connect to server '%s'. Exiting.", client.getHost());
    return 2;
  }

  client.setRobotName(client.getHost());  // include server hostname in log messages

  client.runAsync();

  PtzCameraExample example(&client);
  if(!example.init())
    return 1;

  example.run();


  Aria::shutdown();
  return 0;
}


bool PtzCameraExample::init()
{
  // If the server has the "getCameraList" request, then it's using
  // ArServerHandlerCameraCollection, and migth have multiple PTZs/cameras each with
  // its own set of requests. So send a "getCameraList" request, and when its
  // reply is received, the handler will send "getCameraInfo" requests for each.
  // If the server does not have "getCameraList", it only has one PTZ camera, just
  // send "getCameraInfo". The handler for that will send various control
  // commands.
  // If the server does not have "getCameraInfo", then it doesn't provide any
  // access to PTZ cameras.
  if(myClient->dataExists("getCameraList"))
  {
    ArLog::log(ArLog::Normal, "Server may have multiple cameras. Requesting list.");
    myClient->addHandler("getCameraList", &myCameraListReplyFunc);
    myClient->requestOnce("getCameraList");
  }
  else if(myClient->dataExists("getCameraInfo"))
  {
    ArLog::log(ArLog::Normal, "Server does not support multiple cameras. Requesting info for its camera.");
    ArClientHandlerCamera *camClient = new ArClientHandlerCamera(myClient, "");
    camClient->requestUpdates(100);
    mutex.lock();
    myCameras.insert(camClient);
    mutex.unlock();
  }
  else
  {
    ArLog::log(ArLog::Terse, "Error, server does not have any camera control requests. (Was the server run with video features enabled or video forwarding active?)");
    return false;
  }
  return true;
}

void PtzCameraExample::handleCameraListReply(ArNetPacket *pkt)
{
  ArTypes::Byte2 numCams = pkt->bufToByte2();
  ArLog::log(ArLog::Normal, "%d cameras in list.", numCams);
  char camName[128];
  char camType[128];
  char displayName[128];
  char displayType[128];
  char cmdDesc[128];
  char cmdName[128];
  ArTypes::Byte4 cmdFreq;
  int dataReqFreq = 100;
  for(ArTypes::Byte2 i = 0; i < numCams; ++i)
  {
    memset(camName, 0, 128);
    memset(camType, 0, 128);
    memset(displayName, 0, 128);
    memset(displayType, 0, 128);
    pkt->bufToStr(camName, 128); // name

    ArClientHandlerCamera *cam = new ArClientHandlerCamera(myClient, camName);

    pkt->bufToStr(camType, 128); // type
    cam->type = camType;
    pkt->bufToStr(displayName, 128); // description
    cam->displayName = displayName;
    pkt->bufToStr(displayType, 128); // description
    cam->displayType = displayType;
    ArTypes::Byte2 numCmds = pkt->bufToByte2();
    ArLog::log(ArLog::Normal, "%d commands for camera \'%s\' (%s) / \'%s\' (%s)", numCmds, camName, camType, displayName, displayType);
    for(ArTypes::Byte2 c = 0; c < numCmds; ++c)
    {
      memset(cmdDesc, 0, 128);
      memset(cmdName, 0, 128);
      char cmdDesc[128];
      char cmdName[128];
      pkt->bufToStr(cmdDesc, 128); // description
      pkt->bufToStr(cmdName, 128); // request name
      cmdFreq = pkt->bufToByte4(); // recommended request frequency
      ArLog::log(ArLog::Normal, "Camera %s has %s command named %s with recommended request frequency %d.", camName, cmdDesc, cmdName, cmdFreq);

      if(strcmp(cmdDesc, "getCameraData") == 0)
        dataReqFreq = cmdFreq;
    }
    ArTypes::Byte2 numParams = pkt->bufToByte2();
    ArLog::log(ArLog::Normal, "Camera %s has %d parameters.", camName, numParams);
    for(ArTypes::Byte2 p = 0; p < numParams; ++p)
    {
      ArClientArg carg;
      ArConfigArg arg;
      if(!carg.bufToArgValue(pkt, arg))
        ArLog::log(ArLog::Normal, "Hmm, error getting ArClientArg for camera %s's parameter #%d.", camName, p);
    }

    cam->requestUpdates(dataReqFreq);
    mutex.lock();
    myCameras.insert(cam);
    mutex.unlock();
  }
}

void ArClientHandlerCamera::handleCameraInfoReply(ArNetPacket *pkt)
{
  lock();
  minPan = pkt->bufToByte2() / 1000.0;
  maxPan = pkt->bufToByte2() / 1000.0;
  minTilt = pkt->bufToByte2() / 1000.0;
  maxTilt = pkt->bufToByte2() / 1000.0;
  minZoom = pkt->bufToByte2() / 1000.0;
  maxZoom = pkt->bufToByte2() / 1000.0;
  haveZoom = pkt->bufToByte();
  ArLog::log(ArLog::Normal, "Got getCameraInfo reply with pan range (%f, %f), tilt range (%f, %f), zoom range (%f, %f), zoom valid %d.", minPan, maxPan, minTilt, maxTilt, minZoom, maxZoom, haveZoom);
  unlock();
}

void ArClientHandlerCamera::handleCameraDataReply(ArNetPacket *pkt)
{
  lock();
  pan = pkt->bufToByte2() / 1000.0;
  tilt = pkt->bufToByte2() / 1000.0;
  zoom = pkt->bufToByte2() /1000.0;
  ArLog::log(ArLog::Normal, "Got camera data from camera %s with current pan=%f, tilt=%f, zoom=%f.", name.c_str(), pan, tilt, zoom);
  unlock();
}

bool ArNetCameraRequest::requestPanTiltAbs(double pan, double tilt)
{
  empty();
  byte2ToBuf((ArTypes::Byte2)(pan * 1000.0));
  byte2ToBuf((ArTypes::Byte2)(tilt * 1000.0));
  finalizePacket();
  return myClient->requestOnce(myAbsReqName.c_str(), this);
}

bool ArNetCameraRequest::requestPanTiltZoomAbs(double pan, double tilt, double zoom)
{
  empty();
  byte2ToBuf((ArTypes::Byte2)(pan * 1000.0));
  byte2ToBuf((ArTypes::Byte2)(tilt * 1000.0));
  byte2ToBuf((ArTypes::Byte2)(zoom * 1000.0));
  finalizePacket();
  return myClient->requestOnce(myAbsReqName.c_str(), this);
}


void PtzCameraExample::run()
{
  enum { MinPan, MaxPan, Center1, MinTilt, MaxTilt, Center2} stage;
  stage = MinPan;
  while(myClient->isConnected()) 
  {
    mutex.lock();
    for(std::set<ArClientHandlerCamera*>::const_iterator i = myCameras.begin(); i != myCameras.end(); ++i)
    {
      ArClientHandlerCamera* c = (*i);
      c->lock();
      switch(stage)
      {
        case MinPan:
          c->request.requestPanTiltAbs(c->minPan, 0);
          stage = MaxPan;
          break;
        case MaxPan:
          c->request.requestPanTiltAbs(c->maxPan, 0);
          stage = Center1;
          break;
        case Center1:
          c->request.requestPanTiltAbs(0, 0);
          stage = MinTilt;
          break;
        case MinTilt:
          c->request.requestPanTiltAbs(0, c->minTilt);
          stage = MaxTilt;
          break;
        case MaxTilt:
          c->request.requestPanTiltAbs(0, c->maxTilt);
          stage = Center2;
          break;
        case Center2:
          c->request.requestPanTiltAbs(0, 0);
          stage = MinPan;
      }
      c->unlock();
    }
    mutex.unlock();
    ArUtil::sleep(3000);
  }
}
