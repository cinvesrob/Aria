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
