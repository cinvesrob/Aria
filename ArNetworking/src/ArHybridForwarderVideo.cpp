#include "Aria.h"
#include "ArExport.h"
#include "ArHybridForwarderVideo.h"

/**
   @param server this class will addData the video data to the server

   @param client IF the client is connected to something this class
   will get video data from the client and send it out when requested
 **/
AREXPORT ArHybridForwarderVideo::ArHybridForwarderVideo(ArServerBase *server, 
							                                          ArClientBase *client)
{
  myMutex.setLogName("ArHybridForwarderVideo::myMutex");
  myServer = server;
  myClient = client;
  finishConstructor();
}

/**
   This will try to connect a client to the hostname and port given,
   IF client connects to something this class will get video data from
   the client and send it out when requested

   @param server a base server object
   @param hostname the host to connect a client to
   @param port the port to connect a client to 
**/
AREXPORT ArHybridForwarderVideo::ArHybridForwarderVideo(ArServerBase *server,
							                                          const char *hostname,
							                                          int port)
{
  myServer = server;
  myClient = new ArClientBase();
  myClient->setConnectTimeoutTime(1);
  myClient->blockingConnect(hostname, port, false);
  myClient->runAsync();
  finishConstructor();
}

AREXPORT ArHybridForwarderVideo::~ArHybridForwarderVideo()
{

}

AREXPORT void ArHybridForwarderVideo::finishConstructor(void)
{
  myClient->lock();
  mySendVideoSizeCB = new ArFunctor2C<ArHybridForwarderVideo, ArServerClient*, 
  ArNetPacket *>(this, &ArHybridForwarderVideo::sendVideoSize);

  mySendVideoCB = new ArFunctor2C<ArHybridForwarderVideo, ArServerClient*, 
  ArNetPacket *>(this, &ArHybridForwarderVideo::sendVideo);
  
  myReceiveVideoSizeCB = new ArFunctor1C<ArHybridForwarderVideo, 
  ArNetPacket *>(this, &ArHybridForwarderVideo::receiveVideoSize);

  myReceiveVideoCB = new ArFunctor1C<ArHybridForwarderVideo, 
  ArNetPacket *>(this, &ArHybridForwarderVideo::receiveVideo);

  myClientCycleCB = new ArFunctorC<ArHybridForwarderVideo>(
	  this, &ArHybridForwarderVideo::clientCycleCallback);

  myReqSent = false;
  myLastReqSent.setToNow();
  myLastReceivedVideo.setToNow();
  myVideoRequestTime = 100;
  myForwardingVideo = false;

  if (myClient != NULL && myServer != NULL && myClient->isConnected())
  {
    myClient->addCycleCallback(myClientCycleCB);
    if (myClient->dataExists("videoSize"))
    {
      myServer->addData("videoSize", 
			"gets the width and height of the video data", 
			mySendVideoSizeCB, 
			"none", "uByte2: width, uByte2: height",
			"Video", "RETURN_SINGLE");
      myClient->addHandler("videoSize", myReceiveVideoSizeCB);
      myClient->requestOnce("videoSize");
    }
    if (myClient->dataExists("sendVideo"))
    {
      ArLog::log(ArLog::Normal, "Forwarding video.");
      myForwardingVideo = true;
      myClient->addHandler("sendVideo", myReceiveVideoCB);
      ArNetPacket packet;
      packet.uByteToBuf(90);
      myClient->requestOnce("sendVideo", &packet);

      myIsSendVideoAvailable = 
        myServer->addData("sendVideo", "gets video from the robot's camera (you should requestOnce this, you shouldn't request it, since you could easily fill the bandwidth that way)",
			                    mySendVideoCB, "uByte: quality (0 - 100)", 
			  "out: uByte2: width, uByte2: height, (len - readLen)*uByte: jpegData", "Video", "RETURN_VIDEO");
    }
  }
  myClient->unlock();
}


AREXPORT const char *ArHybridForwarderVideo::getCameraName()
{
  return myCameraName.c_str();
}

AREXPORT void ArHybridForwarderVideo::setCameraName(const char *cameraName)
{
  if (cameraName != NULL) {
    myCameraName = cameraName;
  }
  else {
    myCameraName = "";
  }

} // end method setCameraName


AREXPORT void ArHybridForwarderVideo::addToCameraCollection
                                          (ArCameraCollection &collection)
{
  if (myCameraName.empty()) {
    return;
  } // end if no camera name

  const char *cameraName = myCameraName.c_str();
  if (!collection.exists(cameraName)) {

    return;
  } // end if camera not yet created

  bool isSuccess = true;
  
  if (myForwardingVideo) {

    std::string packetName = "getPicture" + myCameraName;

    // This is a little unusual to add the handler here, but this method
    // should only be called once, so hopefully it's no problem...
    isSuccess = myServer->addData(packetName.c_str(), 
                                  "Gets video from the robot's camera",
			                            mySendVideoCB, "uByte: quality (0 - 100)", 
			                            "out: uByte2: width, uByte2: height, (len - readLen)*uByte: jpegData", "Video");

    if (isSuccess) {
      isSuccess = collection.addCameraCommand
                              (cameraName,
                              ArCameraCommands::GET_PICTURE,
                              packetName.c_str(),
                              myVideoRequestTime); // TODO ? 
    }

    if (!isSuccess) {
      ArLog::log(ArLog::Verbose,
                "Video forwarder could not add camera %s command %s",
                cameraName,
                packetName.c_str());
    } // end if error occurred

  } // end if forwarding video

  // Left the original addData("sendVideo") in the finishConstructor()
  // method - just to try to avoid any backwards compatibility issues.
  if (myIsSendVideoAvailable) {

    isSuccess = collection.addCameraCommand
                            (cameraName,
                            ArCameraCommands::GET_VIDEO,
                            "sendVideo",
                            myVideoRequestTime); // TODO ? 
  }

} // end method addToCameraCollection



AREXPORT void ArHybridForwarderVideo::sendVideoSize(ArServerClient *client, 
						    ArNetPacket *packet)
{
  myMutex.lock();
  myReceivedVideoSize.setCommand(packet->getCommand());
  client->sendPacketTcp(&myReceivedVideoSize);
  myMutex.unlock();
}

AREXPORT void ArHybridForwarderVideo::sendVideo(ArServerClient *client, 
						ArNetPacket *packet)
{
  //printf("Sending video\n");
  myMutex.lock();
  mySendVideoArgument.duplicatePacket(packet);
  mySendVideoArgument.resetRead();
  // MPL fixed the code so that this next line worked, but should
  // probably let the base figure out the command (since it does it
  // for everything else)
  //myReceivedVideo.setCommand(packet->getCommand());
  myReceivedVideo.setCommand(0);
  client->sendPacketUdp(&myReceivedVideo);
  //printf("Sent video %d\n", myReceivedVideo.getLength());
  myMutex.unlock();
}

AREXPORT void ArHybridForwarderVideo::receiveVideoSize(ArNetPacket *packet)
{
  //printf("Receiving video size\n");
  myMutex.lock();
  myReceivedVideoSize.duplicatePacket(packet);
  mySendVideoArgument.resetRead();
  myMutex.unlock();
}

AREXPORT void ArHybridForwarderVideo::receiveVideo(ArNetPacket *packet)
{
  //printf("Receiving video\n");
  myMutex.lock();
  myReceivedVideo.duplicatePacket(packet);
  myReceivedVideo.resetRead();
  myLastReceivedVideo.setToNow();
  myReqSent = false;
  myMutex.unlock();
}

AREXPORT void ArHybridForwarderVideo::clientCycleCallback(void)
{
  myMutex.lock();
  if ((myLastReceivedVideo.mSecSince() > myVideoRequestTime && !myReqSent) ||
      (myReqSent && myLastReqSent.mSecSince() > 3000))
  {
    //printf("Requesting video\n");
    myClient->requestOnce("sendVideo", &mySendVideoArgument);
    myLastReqSent.setToNow();
    myReqSent = true;
  } 
  myMutex.unlock();
}

AREXPORT bool ArHybridForwarderVideo::isForwardingVideo(void) const
{
  return myForwardingVideo;
}
