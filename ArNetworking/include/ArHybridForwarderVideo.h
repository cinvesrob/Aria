#ifndef ARHYBRIDFORWARDERVIDEO_H
#define ARHYBRIDFORWARDERVIDEO_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArClientBase.h"

/// This class takes video another source and serves it back up
class ArHybridForwarderVideo : public ArCameraCollectionItem
{
public:
  /// Constructor that takes a client
  AREXPORT ArHybridForwarderVideo(ArServerBase *server, ArClientBase *client);
  /// Constructor that just takes a host and port and makes its own client
  AREXPORT ArHybridForwarderVideo(ArServerBase *server, 
				  const char *hostname = "localhost", 
				  int port = 7070);
  /// Destructor
  AREXPORT virtual ~ArHybridForwarderVideo();

  /// Returns if we're forwarding video or not
  AREXPORT bool isForwardingVideo(void) const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // ArCameraCollectionItem methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /// Returns the name of the camera handled by this item.
  AREXPORT virtual const char *getCameraName();

  /// Sets the name of the camera handled by this item.
  /**
   * This method must be called before addToCameraCollection().
  **/
  AREXPORT virtual void setCameraName(const char *cameraName);

  /// Adds this item to the given camera collection.
  AREXPORT virtual void addToCameraCollection(ArCameraCollection &collection);


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // Packet methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

  /// Sends the last received video size (just gets this once)
  AREXPORT void sendVideoSize(ArServerClient *client, ArNetPacket *packet);
  /// Sends the last received video
  AREXPORT void sendVideo(ArServerClient *client, ArNetPacket *packet);
  /// Receives the video size (just gets this once)
  AREXPORT void receiveVideoSize(ArNetPacket *packet);
  /// Receives the video (and sets a timer to wait a bit and get it again)
  AREXPORT void receiveVideo(ArNetPacket *packet);
  /// Our callback that requests more video
  AREXPORT void clientCycleCallback(void);
  /// Sets how often after getting video we ask for it again
  void setVideoRequestTime(int ms) { myVideoRequestTime = ms; }
  /// Gets how often after getting video we ask for it again
  int setVideoRequestTime(void) const { return myVideoRequestTime; }
protected:
  AREXPORT void finishConstructor(void);

  std::string myCameraName;

  ArMutex myMutex;
  bool myForwardingVideo;
  ArNetPacket myReceivedVideoSize;
  ArNetPacket myReceivedVideo;
  ArNetPacket mySendVideoArgument;
  ArTime myLastReceivedVideo;
  ArTime myLastReqSent;
  bool myReqSent;
  int myVideoRequestTime;
  ArServerBase *myServer;
  ArClientBase *myClient;
  ArFunctor2C<ArHybridForwarderVideo, ArServerClient*, 
      ArNetPacket *> *mySendVideoSizeCB;
  ArFunctor2C<ArHybridForwarderVideo, ArServerClient*, 
      ArNetPacket *> *mySendVideoCB;
  ArFunctor1C<ArHybridForwarderVideo, ArNetPacket *> *myReceiveVideoSizeCB;
  ArFunctor1C<ArHybridForwarderVideo, ArNetPacket *> *myReceiveVideoCB;
  ArFunctorC<ArHybridForwarderVideo> *myClientCycleCB;

  bool myIsSendVideoAvailable;
};


#endif 
