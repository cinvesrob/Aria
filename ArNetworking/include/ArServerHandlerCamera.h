#ifndef ARSERVERHANDLERCAMERA_H
#define ARSERVERHANDLERCAMERA_H

#include "Aria.h"

#include "ArNetworking.h"

/** Accepts and processes camera pan/tilt/zoom requests from the client, and 
    provides information about the camera (such as its current position).

    The following requests are accepted. Arguments and return values are stored 
    in the packet as signed 16-bit values. (They are converted to double-precision
    floating point values when received by dividing by 100; to store floating
    point values, multiply by 100 first: 
    <code>packet.byte2ToBuf((ArTypes::Byte2)(100.0 * doubleVal));</code> or 
    see also getDoubleFromPacket() and addDoubleToPacket().)
    
    Each request name is suffixed by the name of the camera to control.   If more than one 
    camera is used, each can have a different name (given in the constructor), and commands for the different cameras
    are differentiated by appending the camera name to the command requests listed below.  For example,
    an ArServerHandlerCamera  with camera name of "A" would receive "setCameraRelA". while the
    handler with camera name of "B" would receive "setCameraAbsB".   If no camera name is given 
    (typical if just one ArServerHandlerCamera object is created),
    just the base requests are used.  To provide information to the client about the multiple cameras and their names, use ArCameraCollection and an ArServerHandlerCameraCollection object.

    <table>
      <tr><td>setCameraRel</td> <td> Command to pan and tilt the camera from the 
                                     current position by the values given in the 
                                     first two arguments. If a third argument is 
                                     given, zoom from the current position.</td></tr>
      <tr><td>setCameraAbs</td> <td> Command to pan and tilt the camera to the 
                                     absolute positions given by the first two 
                                     arguments. If a third argument is given, set 
                                     the zoom to that absolute position.</td></tr>
      <tr><td>setCameraPct</td> <td> Command to pan and tilt the camera to the 
                                     given positions as percentage of their overall 
                                     ranges.</td></tr>
      <tr><td>getCameraData</td> <td>Data request to get current pan and tilt values. 
                                     Also get zoom value if available.</td></tr>
      <tr><td>getCameraInfo</td> <td>Data request to get pan and tilt ranges. Also 
                                     gets the zoom range if available.</td></tr>
				     
    </tr>

    In addition, some older camera requests are accepted for partial backwards 
    compatability with old clients: cameraUpdate, cameraInfo, cameraAbs, camera, 
    and cameraPct.  These requests take values as single bytes, rather than multiplied
    floats as 2-bytes as the above commands do. Note, these cannot be used in a multi-camera 
    configuration.

    These requests are in a user permission group called CameraControl.

    In addition to positioning the camera, ArServerHandlerCamera can support additional "modes" for camera control, if additional code is added to update ArServerHandlerCamera with the state of other systems such as navigation.  The additional modes are LookAtGoal and LookAtPoint.  An external system such as navigation (e.g. ARNL) must keep the camera handler object updated with its current goal by calling cameraModeLookAtGoalSetGoal() and cameraModeLookAtGoalClearGoal(), and ArServerHandlerCamera will continuously point the camera at that point or goal as the robot moves. The network requests associated with setting modes are:
    <ul>
      <li>getCameraModeList<CameraName>:   Gets the list of modes that this camera 
                                     supports  </li>
      <li>getCameraModeUpdated<CameraName>:   Sent when the mode is changed </li>
      <li>setCameraMode<CameraName>:   Command to change the camera mode (give mode name) </li>
    </ul>
*/
class ArServerHandlerCamera : public ArCameraCollectionItem
{
private:
  static const char *CONTROL_COMMAND_GROUP;
  static const char *INFO_COMMAND_GROUP;
  static const char *NO_ARGS;
public:

  /// Constructs a new handler for the specified camera.
  /**
   * @param cameraName the unique char * name of the associated camera
   * @param server the ArServerBase * used to send/receive requests
   * @param robot the associated ArRobot * 
   * @param camera the ArPTZ * that provides access to the actual camera
   * @param collection the ArCameraCollection * to which to add the 
   * camera command information; if NULL, then addToCameraCollection()
   * must be called manually.
  **/
  AREXPORT ArServerHandlerCamera(const char *cameraName,
                                 ArServerBase *server,
                                 ArRobot *robot,
                                 ArPTZ *camera,
                                 ArCameraCollection *collection);

  /// Constructor
  /**
   * This constructor is maintained for backwards compatibility.  It will
   * not work in a multi-camera configuration.
   * @deprecated
   * @param server the ArServerBase * used to send/receive requests
   * @param robot the associated ArRobot * 
   * @param camera the ArPTZ * that provides access to the actual camera
  **/
  AREXPORT ArServerHandlerCamera(ArServerBase *server, 
                                 ArRobot *robot, 
				                         ArPTZ *camera);

  /// Destructor
  AREXPORT virtual ~ArServerHandlerCamera();

  // -----------------------------------------------------------------------------
  // ArCameraCollectionItem Interface:
  // -----------------------------------------------------------------------------

  /// Sends the camera absolute commands to the camera (and puts it in position mode)
  AREXPORT void setCameraAbs(double pan, double tilt, double zoom, 
			       bool lockRobot = true);

  /// Sends the camera relative commands to the camera (and puts it in position mode)
  AREXPORT void setCameraRel(double pan, double tilt, double zoom, 
			     bool lockRobot = true);

  /// Sends the camera percent commands to the camera (and puts it in position mode)
  AREXPORT void setCameraPct(double panPct, double tiltPct, 
			     bool lockRobot = true);

  /// Resets the camera
  AREXPORT void resetCamera(bool lockRobot = true);

  /// Puts the camera into a mode where it will look at the goal
  AREXPORT void cameraModeLookAtGoal(void);

  /// Puts the camera into a mode where it will look at a point
  AREXPORT void cameraModeLookAtPoint(ArPose pose, bool controlZoom = true);

  /// Sets the goal for the camera mode that points at the goal
  AREXPORT void cameraModeLookAtGoalSetGoal(ArPose pose);
  /// Clears the goal for the camera mode that points at the goal
  AREXPORT void cameraModeLookAtGoalClearGoal(void);

  /// Puts the camera in position mode
  AREXPORT void cameraModePosition(void);






  /// Returns the name of the camera that is controlled by this handler.
  /**
   * @return char * the unique name of the associated camera (if specified in 
   * the constructor)
  **/
  AREXPORT const char *getCameraName();

  /// Adds the camera commands to the given collection.
  /**
   * @param collection the ArCameraCollection to which to add the camera commands
  **/
  AREXPORT virtual void addToCameraCollection(ArCameraCollection &collection);


  // -----------------------------------------------------------------------------
  // Packet Handlers:
  // -----------------------------------------------------------------------------

  // New methods that send/receive "double"s

  /// Handles the getCameraData network packet, returning information in double format.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleGetCameraData(ArServerClient *client, ArNetPacket *packet);

  /// Handles the getCameraInfo network packet, returning information in double format.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleGetCameraInfo(ArServerClient *client, ArNetPacket *packet);

  /// Handles the setCameraAbs network packet, with information in double format.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleSetCameraAbs(ArServerClient *client, ArNetPacket *packet);

  /// Handles the setCameraPct network packet, with information in double format.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleSetCameraPct(ArServerClient *client, ArNetPacket *packet);

  /// Handles the setCameraRel network packet, with information in double format.
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleSetCameraRel(ArServerClient *client, ArNetPacket *packet);

  /// Handles the mode list for the camera
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleGetCameraModeList(ArServerClient *client, ArNetPacket *packet);

  /// Handle the mode request, generally this is sent when it changes
  /**
   * Refer to handleSetCameraMode for a description of the supported modes
   * and the various packet structures.
   * 
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleCameraModeUpdated(ArServerClient *client, 
					                              ArNetPacket *packet);

  /// Handle the set mode request
  /**
   * The following modes are currently supported:
   *  - Position:    The user controls the camera position; no additional parameters
   *                 in the packet
   *  - LookAtGoal:  The robot automatically points the camera in the direction
   *                 of its destination goal; no additional parameters in the packet
   *  - LookAtPoint: The robot "looks" at a specified point in the map; the packet
   *                 contains: byte4, the x coordinate; byte4, the y coordinate
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleSetCameraMode(ArServerClient *client, 
				                            ArNetPacket *packet);

  /// Handle the reset
  /**
   * @param client the ArServerClient * that sent the request
   * @param packet the ArNetPacket * that contains the request
  **/
  AREXPORT void handleResetCamera(ArServerClient *client, 
				                          ArNetPacket *packet);
  // Old methods that send/receive ints

  /// Handles the camera network packet, with information stored as byte integers.
  AREXPORT void camera(ArServerClient *client, ArNetPacket *packet);
  /// Handles the cameraAbs network packet, with information stored as byte integers.
  AREXPORT void cameraAbs(ArServerClient *client, ArNetPacket *packet);
  /// Handles the cameraPct network packet, with information stored as byte integers.
  AREXPORT void cameraPct(ArServerClient *client, ArNetPacket *packet);
  /// Handles the cameraUpdate network packet, with information returned as byte2 integers.
  AREXPORT void cameraUpdate(ArServerClient *client, ArNetPacket *packet);
  /// Handles the cameraInfo network packet, with information returned as byte2 integers.
  AREXPORT void cameraInfo(ArServerClient *client, ArNetPacket *packet);

protected:


  // -----------------------------------------------------------------------------
  // Helper Methods:
  // -----------------------------------------------------------------------------

  enum {
    DOUBLE_FACTOR = 100 ///< Multiplier for putting/getting "double" values to/from a packet
  };

  /// Adds the given value as a "double" to the given network packet.
  /**
   * The "double" value is really a 2-byte integer multiplied by the DOUBLE_FACTOR.
  **/
  AREXPORT void addDoubleToPacket(double val,
                                  ArNetPacket &packet)
  {
    // TODO: Does this need to be rounded? Any error/overflow checking?
    packet.byte2ToBuf((ArTypes::Byte2) (val * DOUBLE_FACTOR));
  }
  

  /// Reads a "double" value from the given network packet.
  /**
   * The "double" value is really a 2-byte integer multiplied by the DOUBLE_FACTOR.
  **/
  AREXPORT double getDoubleFromPacket(ArNetPacket &packet)
  {
    int tempVal = packet.bufToByte2();
    return ((double) tempVal / (double) DOUBLE_FACTOR);
  }

  /// Gets the current camera zoom as a ratio of its zoom range.
  AREXPORT double getCurrentZoomRatio()
  {
    if (myCamera->getMaxZoom() != myCamera->getMinZoom()) {

     return ((double)(myCamera->getZoom() -  myCamera->getMinZoom()))
             / ((double)(myCamera->getMaxZoom() - myCamera->getMinZoom()));
    }
    else {
      return 0;
    }
  } // end method getCurrentZoomRatio
 

  /// Converts the given absolute zoom value to a ratio of the camera's zoom range.
  AREXPORT double getZoomRatio(double absZoom)
  {
    if (myCamera->getMaxZoom() != myCamera->getMinZoom()) {

      return ((double)(absZoom - myCamera->getMinZoom()))
              / ((double)(myCamera->getMaxZoom() - myCamera->getMinZoom()));
    }
    else {
      return 0;
    }
  } // end method getZoomRatio


  /// Gets the range of the camera's zoom.
  AREXPORT double getZoomRange()
  {
    return myCamera->getMaxZoom() - myCamera->getMinZoom();

  } // end method getZoomRange


private:
  
  /// Performs initialization common to all constructors.
  void init();

  /// Creates command (packet) names for the current camera.
  void createCommandNames();

  /// Creates the handlers for each of the supported commands. 
  void createCommandCBs();

  /// Adds all supported commands to the server.
  void addAllCommandsToServer();
  
  /// Adds the specified command to the server.
  void addCommandToServer(const char *command, 
                          const char *description,
				                  const char *argumentDescription,
				                  const char *returnDescription,
                          const char *commandGroup);

  /// Removes an unsupported command.
  void removeCommand(const char *command);

  /// Adds the information about all supported commands to the camera collection.
  void doAddToCameraCollection(ArCameraCollection &collection);

protected:

  /// Associated robot (primarily used for locking)
  ArRobot *myRobot;
  /// Server from which requests are received
  ArServerBase *myServer;
  /// Camera that is controlled
  ArPTZ *myCamera;

  /// Unique name of the associated camera
  std::string myCameraName;
  /// Pointer to the camera collection (if any) 
  ArCameraCollection *myCameraCollection;

  /// Map of ArCameraCommand names to a unique network packet name
  std::map<std::string, std::string> myCommandToPacketNameMap;
  /// Map of ArCameraCommand names to the preferred default request interval
  std::map<std::string, int> myCommandToIntervalMap;

  /// Map of ArCameraCommand names to the callback that handles the packet
  std::map<std::string, ArFunctor2<ArServerClient *, ArNetPacket *> *> myCommandToCBMap;

  enum CameraMode
  {
    CAMERA_MODE_POSITION,
    CAMERA_MODE_LOOK_AT_GOAL,
    CAMERA_MODE_LOOK_AT_POINT
  };
  
  ArMutex myModeMutex;
  CameraMode myCameraMode;
  std::map<CameraMode, std::string> myCameraModeNameMap;
  ArNetPacket myCameraModePacket;
  ArPose myLookAtPoint;
  bool myPointResetZoom;
  ArPose myGoal;
  bool myGoalAchieved;
  bool myGoalAchievedLast;
  bool myGoalResetZoom;


  void userTask(void);
  void buildModePacket(void);

  ArFunctorC<ArServerHandlerCamera> myUserTaskCB;

  // Leaving these just in case they are used by someone... 

  /// Old (integer-style) handler for the camera packet.
  /**
   * @deprecated
  **/
  ArFunctor2C<ArServerHandlerCamera, 
      ArServerClient *, ArNetPacket *> myCameraCB;
  /// Old (integer-style) handler for the cameraAbs packet.
  /**
   * @deprecated
  **/
  ArFunctor2C<ArServerHandlerCamera, 
      ArServerClient *, ArNetPacket *> myCameraAbsCB;
  /// Old (integer-style) handler for the cameraUpdate packet.
  /**
   * @deprecated
  **/
  ArFunctor2C<ArServerHandlerCamera, 
      ArServerClient *, ArNetPacket *> myCameraUpdateCB;
  /// Old (integer-style) handler for the cameraInfo packet.
  /**
   * @deprecated
  **/
  ArFunctor2C<ArServerHandlerCamera, 
      ArServerClient *, ArNetPacket *> myCameraInfoCB;

}; // end class ArServerHandlerCamera

#endif 
