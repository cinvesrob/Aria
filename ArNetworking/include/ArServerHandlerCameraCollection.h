#ifndef ARSERVERHANDLERCAMERACOLLECTION_H
#define ARSERVERHANDLERCAMERACOLLECTION_H

#include <Aria.h>
#include <ArExport.h>
#include <ArFunctor.h>
#include <ArCameraCollection.h>

#include "ArServerBase.h"
#include "ArServerClient.h"

/// Provides information to a client about different cameras and how to send requests to their different ArServerHandlerCamera and video image server objects.
/** 
 * ArServerHandlerCameraCollection defines the network packet handlers to 
 * transmit information regarding the cameras and their video streams that are installed on the robot.
 *
 * This class handles the following requests:
 *  <ul>
 *    <li>getCameraList: Replies with a packet containing a 2-byte integer 
 *      indicating the number of cameras. Then, for each camera:
 *      <ol>
 *        <li>camera name (string)</li>
 *        <li>camera type (string)</li>
 *        <li>camera name for user display (string)</li>
 *        <li>camera type for user display (string)</li>
 *        <li>list of requests that can be sent for this camera, beginning with the 2-byte number of commands, then, for
 *        each command:
 *        <ol>
 *          <li>the generic descriptive name (string)</li>
 *          <li>the specific command request name (string)</li>
 *          <li>an appropriate frequency for making this request (4-byte integer)</li>
 *        </ol>
 *        </li>
 *        <li>list of parameters, beginning with the 2-byte number of parameters, then,
 *        for each parameter, the parameter definition/value (see ArClientArg for format).
 *        </li>
 *      </ol>
 *    </li>
 * 
 *    <li>setCameraParams: Set some camera parameters. Begins with camera name
 *    string, then for each parameter, the parameter name and value (see 
 *    ArClientArg for the format).  An empty parameter name string terminates 
 *    the list.  In reply, this handler sends a packet confirming the changes 
 *    (see cameraParamUpdated below).</li>
 *  </ul>
 *
 *  When a camera is added or a parameter changes, the server broadcasts the
 *  following packets to each client:
 *  <ul>
 *    <li>cameraListUpdate: no arguments, use getCameraList to get the new list.</li>
 *
 *    <li>cameraParamUpdated: starts with camera name, then a list of parameter
 *    name and value (see ArClientArg for format). An empty parameter name
 *    string terminates the list.</li>
 *  </ul>
 *  Note that the cameraParamUpdated is currently only sent in response to a 
 *  setCameraParams request.  That is, it does not monitor the actual parameter  
 *  values in the ArCameraCollection. 
 *  <p>
 *  In order to manipulate a particular camera using the request names and other
 *  information provided by ArServerHandlerCameraCollection, see ArServerHandlerCamera and
 *  ArVideoServer.
 */
class ArServerHandlerCameraCollection
{
protected:
  /// Name of the network packet to get the camera list.
  static const char *GET_COLLECTION_PACKET_NAME;
  /// Name of the network packet to set a particular camera parameter.
  static const char *SET_PARAMS_PACKET_NAME;

  /// Name of the network packet broadcast when the collection is modified.
  static const char *COLLECTION_UPDATED_PACKET_NAME;
  /// Name of the network packet broadcast when a parameter value is modified.
  static const char *PARAMS_UPDATED_PACKET_NAME;

  /// Command group for these packets.
  static const char *COMMAND_GROUP;

public:
  /// Constructor.
  /**
   * @param server the ArServerBase * used to send/receive requests
   * @param cameraCollection the ArCameraCollection * to be transmitted
  **/
  AREXPORT ArServerHandlerCameraCollection(ArServerBase *server, 
										                       ArCameraCollection *cameraCollection);
	/// Destructor.						  
  AREXPORT virtual ~ArServerHandlerCameraCollection();

  // --------------------------------------------------------------------------
  // Network Packet Handlers
  // --------------------------------------------------------------------------

  /// Handles the request to get the camera list / collection.
  /**
   * @param client the ArServerClient * that is making the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void getCameraList(ArServerClient *client, ArNetPacket *packet);

  /// Handles the request to edit a camera's parameters.
  /**
   * @param client the ArServerClient * that is making the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void setParams(ArServerClient *client, ArNetPacket *packet);

  // --------------------------------------------------------------------------
  // ArCameraCollection Modified Callback
  // --------------------------------------------------------------------------

  /// Callback executed when the associated camera collection is modified.
  AREXPORT void handleCameraCollectionModified();


private:

  /// Disabled copy ctor
  ArServerHandlerCameraCollection(const ArServerHandlerCameraCollection &);
  /// Disabled assignment operator
  ArServerHandlerCameraCollection &operator=(const ArServerHandlerCameraCollection &);

protected:

  /// The server used to send/receive requests
  ArServerBase *myServer;
  /// The associated camera collection 
  ArCameraCollection *myCameraCollection;
  
  /// Server callback for the get camera list request
  ArFunctor2<ArServerClient *, ArNetPacket *> *myGetCameraListCB;
  /// Server callback for the set parameter request
  ArFunctor2<ArServerClient *, ArNetPacket *> *mySetParamCB;

  /// Notification callback when the camera collection has been modified 
  ArFunctor *myCollectionModifiedCB;

}; // end class ArServerHandlerCameraCollection


#endif // ARSERVERHANDLERCAMERACOLLECTION_H
