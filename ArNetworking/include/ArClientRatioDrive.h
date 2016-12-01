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

#ifndef ARCLIENTRATIODRIVE_H
#define ARCLIENTRATIODRIVE_H

#include "Aria.h"
#include "ArNetworking.h"


/** Send continuous "ratio drive" requests to server, which spcifies robot velocities relative to user-configured maximums.
 *  The user can set maximum translation and rotational velocities via server
 *  ArConfig.  Remote clients can request robot speeds expressed as part (percentage)
 *  of those maximums. Remote clients can also enable or disable "safe" mode in
 *  which robot motion is limited by sensed and mapped obstacles.  The
 *  ratioDrive service is provided by ArServerModeRatioDrive on the server. You can
 *  test whether a running server supports ratioDrive by calling
 *  <code>client.dataExists("ratioDrive");</code>
 *  ArClientRatioDrive uses a robot task callback to periodically send the
 *  desired velocities set by calling setTransVelRatio() and setRotVelRatio().  

 * 
 * Once any velocity ratio is set, this class will continously keep sending
 * ratioDrive request packets to the server with the set ratios (these are sent
 * from the ArNetworking communications cycle using a cycle callback).  
 *  Calling stop() causes ArClientRatioDrive to stop sending requests, and to request "stop" mode (Provided on
 *  the server by ArServerModeStop).  
 *
 * To access configuration data on the
 *  server from software, use ArClientHandlerConfig.  (Users can access
 *  configuration using MobileEyes.)
 */

class ArClientRatioDrive
{
public: 
  AREXPORT ArClientRatioDrive(ArClientBase *client);
  AREXPORT virtual ~ArClientRatioDrive();

  /// Select a translational (forward/back) velocity amount (percentage of server's maximum), and
  /// start sending continuous ratioDrive requests in the background. 
  /// Use 100 for full velocity, 50 for half velocity, 0 for none, etc.
  AREXPORT void setTransVelRatio(double r);

  /// Select a rotational velocity amount (percentage of server's maximum), and
  /// start sending continuous ratioDrive requests in the background. 
  /// Use 100 for full velocity, 50 for half velocity, 0 for none, etc.
  AREXPORT void setRotVelRatio(double r);

  /// Select a lateral velocity amount (percentage of server's maximum), and
  /// start sending continuous ratioDrive requests in the background. Only some
  /// robots (Seekur) support lateral movement.
  /// Use 100 for full velocity, 50 for half velocity, 0 for none, etc.
  AREXPORT void setLatVelRatio(double r);

  /// This is an additional modifier applied to each of the velocities
  /// when received by the server.  It is 100% by default but can be reduced
  /// here to provide an overall control of all robot speed.
  void setThrottle(double t) { myThrottle = t; }

  /// Request stop mode on the server (sends 'stop' request), and stop sending
  /// ratioDrive requests until a new ratio value is set using setTransVelRatio(),
  /// setRotVelRatio(), or setLatVelRatio().  
  /// Note: You may need to wait a few seconds
  /// for the robot to fully stop before driving again -- stop mode will report a status of "Stopping"
  /// while slowing down, then "Stopped" when robot is fully stopped, and it may
  /// not be possible to interrupt the Stopping stage.
  AREXPORT void stop();

  /// Send a request to enable "safe drive" mode on the server
  AREXPORT void safeDrive();

  /// Send a request to disable "safe drive" mode on the server (disables
  /// sensing etc.)
  AREXPORT void unsafeDrive();

  void setDebugPrint(bool p = true) { myPrinting = p; }

protected:
  void task();
  void sendInput();
  ArClientBase *myClient;
  ArKeyHandler *myKeyHandler;
  bool myPrinting;
  /// Current translation value (a percentage of the  maximum speed)
  double myTransRatio;
  /// Current rotation ration value (a percentage of the maximum rotational velocity)
  double myRotRatio;
  /// Current rotation ration value (a percentage of the maximum rotational velocity)
  double myLatRatio;
  double myThrottle;
  bool myStop;
  ArFunctorC<ArClientRatioDrive> myCycleCB;
};

#endif
