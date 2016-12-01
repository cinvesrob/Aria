
#include "Aria.h"
#include "ArExport.h"
#include "ArClientHandlerRobotUpdate.h"
#include "ArNetPacket.h"
#include "ArClientBase.h"

AREXPORT ArClientHandlerRobotUpdate::ArClientHandlerRobotUpdate(ArClientBase *client) : 
	myClient(client),
	myHandleUpdateOldCB(this, &ArClientHandlerRobotUpdate::handleUpdateOld),
	myHandleUpdateNumbersCB(this, &ArClientHandlerRobotUpdate::handleUpdateNumbers),
	myHandleUpdateStringsCB(this, &ArClientHandlerRobotUpdate::handleUpdateStrings)
{
	myHandleUpdateOldCB.setName("ArClientHandlerRobotUpdate::myHandelUpdateOldCB");
	myHandleUpdateStringsCB.setName("ArClientHandlerRobotUpdate::myHandelUpdateStringsCB");
	myHandleUpdateNumbersCB.setName("ArClientHandlerRobotUpdate::myHandelUpdateNumbersCB");
	myStatusChangedCBList.setName("ArClientHandlerRobotUpdate::myStatusChangedCBList");
	myModeChangedCBList.setName("ArClientHandlerRobotUpdate::myModeChangedCBList");
	myUpdateCBList.setName("ArClientHandlerRobotUpdate::myUpdateCBList");
}



AREXPORT ArClientHandlerRobotUpdate::~ArClientHandlerRobotUpdate()
{
	if(myClient)
	{
		stopUpdates();
	}
}

AREXPORT void ArClientHandlerRobotUpdate::stopUpdates()
{
		myClient->remHandler("update", &myHandleUpdateOldCB);
		myClient->requestStop("update");
		myClient->remHandler("updateNumbers", &myHandleUpdateNumbersCB);
		myClient->requestStop("updateNumbers");
		myClient->remHandler("updateStrings", &myHandleUpdateStringsCB);
		myClient->requestStop("updateStrings");
}

AREXPORT void ArClientHandlerRobotUpdate::requestUpdates(int freq)
{
    //if(!myClient->isConnected()) ArLog::log(ArLog::Normal, "warning client not connected");
	if(myClient->dataExists("updateStrings") && myClient->dataExists("updateNumbers"))
	{
		myClient->addHandler("updateStrings", &myHandleUpdateStringsCB);
		myClient->request("updateStrings", -1);
		myClient->addHandler("updateNumbers", &myHandleUpdateNumbersCB);
		myClient->request("updateNumbers", freq);
	}
	else
	{
		ArLog::log(ArLog::Verbose, "ArClientHandlerRobotUpdate: warning: using legacy \"update\" request");
		myClient->addHandler("update", &myHandleUpdateOldCB);
		myClient->request("update", freq);
	}
}


void ArClientHandlerRobotUpdate::handleUpdateOld(ArNetPacket *packet)
{
  /* Extract the data from the update packet. Its format is status and
   * mode (null-terminated strings), then 6 doubles for battery voltage, 
   * x position, y position and orientation (theta) (from odometry), current
   * translational velocity, and current rotational velocity. Translation is
   * always milimeters, rotation in degrees.
   */
  char s[256];
  char m[256];
  bool statusChanged = false;
  bool modeChanged = false;
  memset(s, 0, 256);
  memset(m, 0, 256);

  packet->bufToStr(s, 256);
  packet->bufToStr(m, 256);


  lock();
  if(myStatus != s)
  {
	  statusChanged = true;
	  myStatus = s;
  }
  if(myMode != m)
  {
	  modeChanged = true;
      myMode = m;
  }
  parseData(packet);
  RobotData data = myData; // copy data
  unlock();

  if(modeChanged)
	  myModeChangedCBList.invoke(m);
  if(statusChanged)
	  myStatusChangedCBList.invoke(s);

  myUpdateCBList.invoke(data);
}

void ArClientHandlerRobotUpdate::handleUpdateNumbers(ArNetPacket *packet)
{
  /* Extract the data from the updateNumbers packet. Its format is 6
   * doubles for battery voltage, x position, y position and
   * orientation (theta) (from odometry), current translational
   * velocity, and current rotational velocity. Translation is always
   * milimeters, rotation in degrees.
   */
  lock();
  parseData(packet);
  RobotData data = myData; // copy data
  unlock();

  myUpdateCBList.invoke(data);
}

void ArClientHandlerRobotUpdate::handleUpdateStrings(ArNetPacket *packet)
{
  /* Extract the data from the updateStrings packet. Its format is
   * status and mode (null-terminated strings).
   */
  char s[256];
  char m[256];
  bool statusChanged = false;
  bool modeChanged = false;
  memset(s, 0, 256);
  memset(m, 0, 256);
  packet->bufToStr(s, 256);
  packet->bufToStr(m, 256);

  lock();
  if(myStatus != s)
  {
	  statusChanged = true;
	  //ArLog::log(ArLog::Normal, "status %s\n", s);
	  myStatus = s;
  }
  if(myMode != m)
  {
	  modeChanged = true;
	  //ArLog::log(ArLog::Normal, "mode %s\n");
      myMode = m;
  }
  unlock();

  if(modeChanged)
	  myModeChangedCBList.invoke(m);
  if(statusChanged)
	  myStatusChangedCBList.invoke(s);
}


void ArClientHandlerRobotUpdate::parseData(ArNetPacket *packet)
{
  myData.voltage = ( (double) packet->bufToByte2() )/10.0;
  myData.haveVoltage  = true;
  // TODO determine if this is actually state of charge from batteryInfo request
  myData.pose.setX( (double) packet->bufToByte4() );
  myData.pose.setY( (double) packet->bufToByte4() );
  myData.pose.setTh( (double) packet->bufToByte2() );
  myData.vel = (double) packet->bufToByte2();
  myData.rotVel = (double) packet->bufToByte2();
  myData.latVel = (double) packet->bufToByte2();
  myData.haveTemperature = true;
  myData.temperature = (double) packet->bufToByte();
}