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
