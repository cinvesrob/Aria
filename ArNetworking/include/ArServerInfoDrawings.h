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
#ifndef ARSERVERINFODRAWINGS_H
#define ARSERVERINFODRAWINGS_H

#include "Aria.h"
#include "ArServerBase.h"

class ArServerClient;

/** Service to provide clients with information about graphical figures to be
 displayed with the map.  

 Use addDrawing() to add a figure, or use addRangeDevice() or 
 addRobotsRangeDevice() to automatically create standard drawings for 
 range devices.

 Clients may use the <code>listDrawings</code> data
 request to receive a list of figures and metadata about those figures.
 The reply packet to the <code>listDrawings</code> request is as follows:
 <ol>
  <li>Number of figures (4-byte integer)Then, for each figure, the following values are given
  <li>For each figure:
    <ol>
      <li>Figure name (null-terminated string)</li>
      <li>Shape ID (null-terminated string)</li>
      <li>Primary color: <ol>
          <li>Unused (byte)</li>
          <li>Red (byte)</li>
          <li>Green (byte)</li>
          <li>Blue (byte)</li>
      </ol></li>
      <li>Shape size (4-byte integer)</li>
      <li>Layer number (4-byte integer)</li>
      <li>Suggested refresh time (4-byte integer)</li>
      <li>Secordary color: <ol>
          <li>Unused (byte)</li>
          <li>Red (byte)</li>
          <li>Green (byte)</li>
          <li>Blue (byte)</li>
      </ol></li>
    </li>
   </ol>
  </ol>

 This command is in the <code>SensorInfo</code> permission group for users.
*/
class ArServerInfoDrawings
{
public:
  /// Constructor
  AREXPORT ArServerInfoDrawings(ArServerBase *server);
  /// Destructor
  AREXPORT virtual ~ArServerInfoDrawings();
  /// Adds a shape to the set of figures
  AREXPORT bool addDrawing(ArDrawingData *drawingData, const char *name,
		   ArFunctor2<ArServerClient *, ArNetPacket *> *functor);
  /// Adds a specific range device to be drawn (using its default shape)
  AREXPORT bool addRangeDevice(ArRangeDevice *rangeDevice);
  /// Adds all of the robot's range devices (using their default shape)
  AREXPORT bool addRobotsRangeDevices(ArRobot *robot);
  /// Client callback: Puts the list of shapes that can be drawn and their metadata into a reply packet (internal use mostly)
  AREXPORT void netListDrawings(ArServerClient *client, ArNetPacket *packet);

  AREXPORT void netGetDrawingList(ArServerClient *client, ArNetPacket *packet);

  /// Client callback utility: Puts the current data for the given range device into a reply packet (internal use mostly)
  AREXPORT void netRangeDeviceCurrent(ArServerClient *client, 
				      ArNetPacket *packet,
				      ArRangeDevice *device);
  /// Client callback utilit: Puts the cumulative buffer of the given range device into a reply packet (internal use mostly)
  AREXPORT void netRangeDeviceCumulative(ArServerClient *client, 
					 ArNetPacket *packet,
					 ArRangeDevice *device);
  /// internal function that gets the drawing data for a drawing
  /// @internal
  AREXPORT ArDrawingData *internalGetDrawingData(const char *name);
  /// internal function that gets the functor for a drawing
  /// @internal
  AREXPORT ArFunctor2<ArServerClient *, ArNetPacket *> *internalGetDrawingCallback(const char *name);
protected:
  ArServerBase *myServer;
  std::map<std::string, ArDrawingData *, ArStrCaseCmpOp> myDrawingDatas;
  std::map<std::string, ArFunctor2<ArServerClient *, ArNetPacket *> *, ArStrCaseCmpOp> myDrawingCallbacks;
  ArFunctor2C<ArServerInfoDrawings, ArServerClient *, ArNetPacket *> myNetListDrawingsCB;  
  ArFunctor2C<ArServerInfoDrawings, ArServerClient *, ArNetPacket *> myNetGetDrawingListCB;  
};


#endif
