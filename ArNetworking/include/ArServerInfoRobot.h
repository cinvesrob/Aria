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
#ifndef ARSERVERROBOTINFO_H
#define ARSERVERROBOTINFO_H

#include "Aria.h"
#include "ArServerBase.h"

class ArServerClient;

/** Server component providing basic robot status information to clients.
 * This service accepts the following data requests: 
 * <ul>
 *  <li><code>updateNumbers</code></li>
 *  <li><code>updateStrings</code></li>
 *  <li><code>update</code></li>
 *  <li><code>physicalInfo</code></li>
 *  <li><code>batteryInfo</code></li>
 *  <li><code>activityTimeInfo</code></li>
 * </ul>
 *
 *  For bandwidth savings you should use <code>updateNumbers</code>
 *  at a higher frequency 
 * and <code>updateStrings</code> at a lower frequency instead of
 * <code>update</code> at high frequency request rate.
 *
 * The <code>updateNumbers</code> request returns data regarding the current
 * state of the robot. 
 * It replies with the following data packet:
 * <ol>
 *  <li>Battery voltage (times 10) (2 Byte integer)</li>
 *  <li>X position (4 byte floating point number)</li>
 *  <li>Y position (4 byte floating point number)</li>
 *  <li>Theta (2-byte floating point number)</li>
 *  <li>Forward velocity (2-byte integer)</li>
 *  <li>Rotation velocity (2-byte integer)</li>
 * </ol>
 *
 * The <code>updateStrings</code> request broadcasts data regarding
 * the current state of the robot.  Request this at -1 to get the
 * broadcasts and an initial packet with the inital data.
 * It replies with the following data packet:
 * <ol>
 *  <li>Status (Null-terminated string)</li>
 *  <li>Mode (Null-terminated string)</li>
 *</ol>
 * 
 * The deprecated <code>update</code> request returns all data regarding the current
 * state of the robot. 
 * It replies with the following data packet:
 * <ol>
 *  <li>Status (Null-terminated string)</li>
 *  <li>Mode (Null-terminated string)</li>
 *  <li>Battery voltage (times 10) (2 Byte integer)</li>
 *  <li>X position (4 byte floating point number)</li>
 *  <li>Y position (4 byte floating point number)</li>
 *  <li>Theta (2-byte floating point number)</li>
 *  <li>Forward velocity (2-byte integer)</li>
 *  <li>Rotation velocity (2-byte integer)</li>
 * </ol>
 *
 *
 * The <code>physicalInfo</code> request returns data regarding the physical characteristics of
 * the robot.  This information does not change during the robot execution,
 * so only needs to be requested once or as needed. It replies with the 
 * following data packet:
 * <ol>
 *  <li>Robot type (Null-terminated string)</li>
 *  <li>Robot sub-type (Null-terminated string)</li>
 *  <li>Robot width in mm (2 byte integer)</li>
 *  <li>Robot front length - mm from center of rotation to front of robot (2 byte integer)</li>
 *  <li>Robot rear length - mm from center of rotation to back of robot (2 byte integer)</li>
 * </ol>
 *
 * The <code>batteryInfo</code> request replies with the following data packet:
 * <ol>
 *  <li>Warning voltage (double)</li>
 *  <li>Shutdown voltage (double)</li>
 * </ol>
 *
 * The <code>activityTimeInfo</code> request replies with the following data packet:
 * <ol>
 *   <li>Sec since - the ArServerMode::getActiveModeActivityTimeSecSince value (4-byte int)</li>
 * </ol>
 *
 * These requests are in the <code>RobotInfo</code> command group.
 */

class ArServerInfoRobot
{
public:
  /// Constructor
  AREXPORT ArServerInfoRobot(ArServerBase *server, ArRobot *robot);
  /// Destructor
  AREXPORT virtual ~ArServerInfoRobot();
  /// The function that sends updates about the robot off to the client
  AREXPORT void update(ArServerClient *client, ArNetPacket *packet);
  /// The function that sends updates about the robot off to the client
  AREXPORT void updateNumbers(ArServerClient *client, ArNetPacket *packet);
  /// The function that sends updates about the robot off to the client
  AREXPORT void updateStrings(ArServerClient *client, ArNetPacket *packet);
  /// The function that sends battery info about the robot off to the client
  AREXPORT void batteryInfo(ArServerClient *client, ArNetPacket *packet);
  /// The function that sends information about the physical robot 
  AREXPORT void physicalInfo(ArServerClient *client, ArNetPacket *packet);
  /// The function that sends information about the time that the server mode was last active 
  AREXPORT void activityTimeInfo(ArServerClient *client, ArNetPacket *packet);
protected:
  ArServerBase *myServer;
  ArRobot *myRobot;

  void userTask(void);
  
  std::string myStatus;
  std::string myExtendedStatus;
  std::string myMode;
  std::string myOldStatus;
  std::string myOldExtendedStatus;
  std::string myOldMode;

  
  ArFunctor2C<ArServerInfoRobot, ArServerClient *, ArNetPacket *> myUpdateCB;
  ArFunctor2C<ArServerInfoRobot, ArServerClient *, ArNetPacket *> myUpdateNumbersCB;
  ArFunctor2C<ArServerInfoRobot, ArServerClient *, ArNetPacket *> myUpdateStringsCB;
  ArFunctor2C<ArServerInfoRobot, ArServerClient *, ArNetPacket *> myBatteryInfoCB;
  ArFunctor2C<ArServerInfoRobot, ArServerClient *, ArNetPacket *> myPhysicalInfoCB;
  ArFunctor2C<ArServerInfoRobot, ArServerClient *, ArNetPacket *> myActivityTimeInfoCB;
  ArFunctorC<ArServerInfoRobot> myUserTaskCB;
};

#endif
