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
#ifndef ARSERVERMAP_H
#define ARSERVERMAP_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArConfig.h"


class ArServerClient;

class ArMapInterface;

/**  @brief Service providing the contents of a map file to the client.
 *
 * A map can be provided from an ArMap object, or else the 
 * filename can be given in the global ArConfig object (see Aria::getConfig())
 * as "Map" in the "Files" section.  This handler can also be configured to 
 * only send "point" data, "line" data, or both kinds of data from the map.
 *
 * This service accepts the following data requests:
 * <ul>
 *  <li><code>getMapId</code>
 *  <li><code>getMapName</code>
 *  <li><code>getMap</code>
 *  <li><code>getGoals</code>
 *  <li><code>getMapBinary</code>
 *  <li><code>getMapMultiScans</code>
 * </ul>
 *
 * The following data types will also be broadcast to all clients to indicate
 * certain events:
 * <ul>
 *  <li><code>mapUpdated</code>
 *  <li><code>goalsUpdated</code>
 * </ul>
 *
 * The <code>getMapId</code> request replies with: a map
 * source identifier (NULL-terminated string); a filename for the map
 * (NULL-terminated string); a checksum value preceded by a 4-byte unsigned
 * integer providing the length (in bytes) of that checksum value; the total
 * data size of the map contents (4-byte unsigned integer); and a timestamp
 * for the map file (last modified, 4-byte signed integer, UNIX time).
 * 
 * The <code>getMapName</code> request replies with a packet containing a 
 * NULL-terminated string containing the filename of the map, or an empty string
 * ("") if there is no map.
 *
 * The <code>getMap</code> request replies with a series of packets each
 * containing one line from the map file (as a null-terminated string), 
 * followed by an empty packet signifying
 * the end of the packet. 
 * (see the documentation for ArMap for the map file
 * format). This data may be written (with newlines added) to a map file,
 * or may be parsed directly by an empty ArMap object using ArMap::parseLine()
 * and ArMap::parsingComplete(). See tests/mapClient.cpp for an example of usage.
 *
 * The <code>getGoals</code> request replies with a packet containing a series of NULL-terminated
 * strings containing the names of the Goal objects in the map.
 *
 * The <code>getMapBinary</code> request replies with the map headers and
 * objects list (see ArMap for map file format) as text, but supplies point
 * "DATA" and/or "LINES" as an undelimited sequence of 2-byte integers. (In
 * the case of point data, each pair of integers is a point; for lines, each
 * sequence of four integers defines a line).  This binary representation of data
 * is more compact than the ASCII text representation. This request results in a
 * series of packets, with an empty packet signifying the end of the series.
 *
 * The <code>getMapMultiScans</code> request is similar to getMapBinary,
 * but it includes a list of the scan sources, along with the point and lines 
 * for each scan source in binary format.
 *
 * The <code>mapUpdated</code> packet is sent to all connected clients whenever
 * a new map is loaded or the map is changed.  The packet contains no data; the 
 * new map can be downloaded using one of the above requests.
 *
 * The <code>goalsUpdated</code> packet is sent to all connected clients
 * whenever the list of Goal objects changes in the map or a new map is loaded.
 * The packet contains no data; the new list of goals can be downloaded using 
 * <code>getGoals</code>, or by downloading the entire map.
 *
 */
class ArServerHandlerMap
{
public:
  enum DataToSend { LINES = 1, POINTS = 2, BOTH = 3 };
  /// Constructor
  AREXPORT ArServerHandlerMap(ArServerBase *server, 
                              ArMapInterface *arMap = NULL, 
			                        DataToSend dataToSend = BOTH);
  /// Destructor
  AREXPORT virtual ~ArServerHandlerMap();
  /// Loads the map from a file
  AREXPORT bool loadMap(const char *mapFile);
  /// Uses a map already loaded
  AREXPORT void useMap(ArMapInterface *mapObj, 
                       bool takeOwnershipOfMap = false);
  /// Gets the map object this is using
  AREXPORT ArMapInterface *getMap(void);

  /// Handles the request for the map ID.
  AREXPORT void serverGetMapId(ArServerClient *client,
                               ArNetPacket *packet);
  /// The command that gets the map name
  AREXPORT void serverGetMapName(ArServerClient *client,
				                         ArNetPacket *packet);
  /// The command that'll get the map itself
  AREXPORT void serverGetMap(ArServerClient *client,
			     ArNetPacket *packet);
  /// The command that gets the map, with the data in binary format for improved performance
  AREXPORT void serverGetMapBinary(ArServerClient *client,
				                           ArNetPacket *packet);

  /// Requests that the server send the map, including scan data for multiple sources if available.
  AREXPORT void serverGetMapMultiScans(ArServerClient *client,
				                               ArNetPacket *packet);

  /// Requests that the server send the map with the specified maximum features.
  AREXPORT void serverGetMapWithMaxCategory(ArServerClient *client,
				                                    ArNetPacket *packet);

  /// The command that'll get the goals
  AREXPORT void serverGetGoals(ArServerClient *client,
			                         ArNetPacket *packet);

  /// Sets which kind of data we send 
  AREXPORT void setDataToSend(DataToSend dataToSend) 
    { myDataToSend = dataToSend; }
  /// Gets which kind of data we send
  DataToSend getDataToSend(void) { return myDataToSend; }

protected:
  AREXPORT void handleCheckMap(ArServerClient *client, 
                               ArNetPacket *packet);

  AREXPORT void sendMapWithMaxCategory(ArServerClient *client,
				                               const char *maxCategory);

  AREXPORT bool processFile(void);
  AREXPORT void mapChanged(void);
  // internal function that is used to toss the map to the client
  AREXPORT void writeMapToClient(const char *line, ArServerClient *client);
 

  AREXPORT void writePointsToClient(int pointCount,
				    std::vector<ArPose> *points,
				    ArServerClient *client);

  AREXPORT void writeLinesToClient(int lineCount,
				    std::vector<ArLineSegment> *points,
				    ArServerClient *client);

  ArServerBase *myServer;
  bool myOwnMap;
  DataToSend myDataToSend;
  ArMapInterface *myMap;
  std::string myMapName;
  ArServerHandlerMap *myServerHandlerMap;

  bool myAlreadyLoaded;
  char myMapFileName[512];
  char myLastMapFile[1024];
  struct stat myLastMapFileStat;

  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetMapIdCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetMapNameCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetMapCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetMapBinaryCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetMapMultiScansCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetMapMaxCategoryCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myGetGoalsCB;
  ArFunctor2C<ArServerHandlerMap, 
      ArServerClient *, ArNetPacket *> myCheckMapCB;
  ArRetFunctorC<bool, ArServerHandlerMap> myProcessFileCB;
  ArFunctorC<ArServerHandlerMap> myMapChangedCB;
};

#endif

