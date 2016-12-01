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
#ifndef ARSERVERSENSORINFO_H
#define ARSERVERSENSORINFO_H

#include "Aria.h"
#include "ArServerBase.h"

class ArServerClient;

/** Service providing clients with data from range sensors.
 * This service accepts the following data requests: 
 * <ul>
 *  <li><code>getSensorList</code> to get a list of all robot sensors</li>
 *  <li><code>getSensorCurrent</code> to get one range sensor's set of current  readings</li>
 *  <li><code>getSensorCumulative</code> to get one range sensor's set of cumualtive  readings</li>
 * </ul>
 *
 * The <code>getSensorList</code> request replies with the following data packet:
 * <ol>
 *  <li>Number of sensors (2-byte integer)
 *  <li>For each sensor:
 *    <ol><li>sensor name (Null-terminated string)</li></ol>
 *  </li>
 * </ol>
 *
 * The <code>getSensorCurrent</code> and <code>getSensorCumulative</code>
 * requests must include the following data:
 * <ol>
 *  <li>Sensor name (Null-terminated string)</li>
 * </ol>
 *
 * The <code>getSensorCurrent</code> and <code>getSensorCumulative</code>
 * requests reply with the following data packets:
 * <ol>
 *  <li>Number of readings, or -1 for invalid sensor name error (2-byte integer)</li>
 *  <li>Sensor name (null-terminated string)</li>
 *  <li>For each reading:
 *    <ol>
 *      <li>X coordinate of reading (4-byte integer)</li>
 *      <li>Y coordinate of reading (4-byte integer)</li>
 *    </ol>
 *  </li>
 * </ol>
 *
 * This service's requests are all in the <code>SensorInfo</code> group.
 */
class ArServerInfoSensor
{
public:
  AREXPORT ArServerInfoSensor(ArServerBase *server, ArRobot *robot);
  AREXPORT virtual ~ArServerInfoSensor();
  AREXPORT void getSensorList(ArServerClient *client, ArNetPacket *packet);
  AREXPORT void getSensorCurrent(ArServerClient *client, ArNetPacket *packet);
  AREXPORT void getSensorCumulative(ArServerClient *client, 
				    ArNetPacket *packet);
protected:
  ArRobot *myRobot;
  ArServerBase *myServer;
  ArFunctor2C<ArServerInfoSensor, ArServerClient *, ArNetPacket *> myGetSensorListCB;
  ArFunctor2C<ArServerInfoSensor, ArServerClient *, ArNetPacket *> myGetSensorCurrentCB;
  ArFunctor2C<ArServerInfoSensor, ArServerClient *, ArNetPacket *> myGetSensorCumulativeCB;
  
};


#endif
