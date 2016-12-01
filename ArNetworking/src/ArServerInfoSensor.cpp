#include "Aria.h"
#include "ArExport.h"
#include "ArServerInfoSensor.h"

AREXPORT ArServerInfoSensor::ArServerInfoSensor(ArServerBase *server, ArRobot *robot) :
  myGetSensorListCB(this, &ArServerInfoSensor::getSensorList),
  myGetSensorCurrentCB(this, &ArServerInfoSensor::getSensorCurrent),
  myGetSensorCumulativeCB(this, &ArServerInfoSensor::getSensorCumulative)
  
{
  myRobot = robot;
  myServer = server;
  
  if (myServer != NULL)
  {
    /*
      These are set as RETURN_COMPLEX since they won't work with the
      connection forwarding stuff (since that can't deal with requests
      set up like this)
    */
    myServer->addData("getSensorList", 
		      "gets a list of sensors attached to the robot",
		      &myGetSensorListCB, 
		      "none", 
		      "byte2: numSensors,  repeating for numSensors: string: sensorName", 
		      "SensorInfo", "RETURN_COMPLEX");

    myServer->addData("getSensorCurrent", 
		      "gets the current sensor readings for requested sensors",
		      &myGetSensorCurrentCB,
		      "string: sensorName",
		      "byte2: numReadings string: sensorName, repeating for numReadings times: byte4: x byte4: y.... if numReadings is -1 it means no sensor by that name",
		      "SensorInfo", "RETURN_COMPLEX");

    myServer->addData("getSensorCumulative", 
		      "gets the cumulative sensor readings for requested sensors",
		      &myGetSensorCumulativeCB,
		      "string: sensorName",
		      "byte2: numReadings string: sensorName, repeating for numReadings times: byte4: x byte4: y.... if numReadings is -1 it means no sensor by that name",
		      "SensorInfo", "RETURN_COMPLEX");
  }
}

AREXPORT ArServerInfoSensor::~ArServerInfoSensor() 
{

}

AREXPORT void ArServerInfoSensor::getSensorList(ArServerClient *client, 
					  ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  std::list<ArRangeDevice *> *devList;
  std::list<ArRangeDevice *>::iterator it;

  myRobot->lock();
  devList = myRobot->getRangeDeviceList();
  
  if (devList == NULL)
  {
    myRobot->unlock();
    client->sendPacketUdp(&sendPacket);
    return;
  }
  
  sendPacket.byte2ToBuf(devList->size());

  for (it = devList->begin(); it != devList->end(); it++)
  {
    sendPacket.strToBuf((*it)->getName());
  }
  myRobot->unlock();
  client->sendPacketUdp(&sendPacket);
}


AREXPORT void ArServerInfoSensor::getSensorCurrent(ArServerClient *client, 
					ArNetPacket *packet)
{
  ArRangeDevice *dev;
  char sensor[512];
  std::list<ArPoseWithTime *> *readings;
  std::list<ArPoseWithTime *>::iterator it;

  while (packet->getDataLength() > packet->getDataReadLength())
  {
    ArNetPacket sendPacket;

    // find out the sensor they want
    packet->bufToStr(sensor, sizeof(sensor));
    myRobot->lock();
    if ((dev = myRobot->findRangeDevice(sensor)) == NULL)
    {
      myRobot->unlock();
      ArLog::log(ArLog::Verbose, "ArServerInfoSensor::getSensorCurrent: No sensor %s", sensor);
      sendPacket.byte2ToBuf(-1);
      sendPacket.strToBuf(sensor);
      client->sendPacketUdp(&sendPacket);
      continue;
    }
    
    myRobot->unlock();
    dev->lockDevice();
    readings = dev->getCurrentBuffer();
    if (readings == NULL)
    {
      dev->unlockDevice();
      ArLog::log(ArLog::Verbose, "ArServerInfoSensor::getSensorCurrent: No current buffer for %s", sensor);
      sendPacket.byte2ToBuf(0);
      sendPacket.strToBuf(sensor);
      client->sendPacketUdp(&sendPacket);
      continue;
    } 
    
    sendPacket.byte2ToBuf(readings->size());
    sendPacket.strToBuf(sensor);
    for (it = readings->begin(); it != readings->end(); it++)
    {
      sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getX()));
      sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getY()));
    }
    dev->unlockDevice();
    client->sendPacketUdp(&sendPacket);
  }
  

}

AREXPORT void ArServerInfoSensor::getSensorCumulative(ArServerClient *client, 
					ArNetPacket *packet)
{
  ArRangeDevice *dev;
  char sensor[512];
  std::list<ArPoseWithTime *> *readings;
  std::list<ArPoseWithTime *>::iterator it;

  while (packet->getDataLength() > packet->getDataReadLength())
  {
    ArNetPacket sendPacket;  
    // find out the sensor they want
    packet->bufToStr(sensor, sizeof(sensor));
    myRobot->lock();
    if ((dev = myRobot->findRangeDevice(sensor)) == NULL)
    {
      myRobot->unlock();
      ArLog::log(ArLog::Verbose, "ArServerInfoSensor::getSensorCumulative: No sensor %s", sensor);
      sendPacket.byte2ToBuf(-1);
      sendPacket.strToBuf(sensor);
      client->sendPacketUdp(&sendPacket);
      continue;
    }
    
    myRobot->unlock();
    dev->lockDevice();
    readings = dev->getCumulativeBuffer();
    if (readings == NULL)
    {
      dev->unlockDevice();
      ArLog::log(ArLog::Verbose, "ArServerInfoSensor::getSensorCumulative: No current buffer for %s", sensor);
      sendPacket.byte2ToBuf(0);
      sendPacket.strToBuf(sensor);
      client->sendPacketUdp(&sendPacket);
      continue;
    } 
    
    sendPacket.byte2ToBuf(readings->size());
    sendPacket.strToBuf(sensor);
    for (it = readings->begin(); it != readings->end(); it++)
    {
      sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getX()));
      sendPacket.byte4ToBuf(ArMath::roundInt((*it)->getY()));
    }
    dev->unlockDevice();
    client->sendPacketUdp(&sendPacket);
  }

}
