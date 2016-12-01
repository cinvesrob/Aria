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
#include "Aria.h"
#include "ArNetworking.h"

ArClientBase client;

// suppress the output of drawings until we get the whole list in and then for a few seconds beyond that
bool getDrawingListDone = true;
ArTime getDrawingListDoneTime;

void drawingData(ArNetPacket *packet)
{
  if (getDrawingListDone && getDrawingListDoneTime.secSince() < 5)
    return;

  int x, y;
  int numReadings;
  int i;

  numReadings = packet->bufToByte4();

  if (numReadings == 0)
  {
    printf("No readings for sensor %s\n\n", client.getName(packet));
    return;
  }

  printf("Readings (%d) for %s:", numReadings, client.getName(packet));
  for (i = 0; i < numReadings; i++)
  {
    x = packet->bufToByte4();
    y = packet->bufToByte4();
    printf(" (%d %d)", x, y);
  }
  printf("\n\n");
}

ArGlobalFunctor1<ArNetPacket *> drawingDataCB(&drawingData);

void getDrawingList(ArNetPacket *packet)
{
  int numDrawings;
  int i;
  char name[512];
  char shape[512];
  char visibility[512];
  long primary, size, layer, secondary;
  unsigned long refresh;

  if (packet->getDataReadLength() == packet->getDataLength())
  {
    ArLog::log(ArLog::Normal, "");
    ArLog::log(ArLog::Normal, "Done with getDrawingList, will begin logging drawing data in 5 seconds");
    getDrawingListDone = true;
    getDrawingListDoneTime.setToNow();
    return;
  }

  packet->bufToStr(name, sizeof(name));
  packet->bufToStr(shape, sizeof(shape));
  primary = packet->bufToByte4();
  size = packet->bufToByte4();
  layer = packet->bufToByte4();
  refresh = packet->bufToByte4();
  secondary = packet->bufToByte4();
  packet->bufToStr(visibility, sizeof(visibility));  

  ArLog::log(ArLog::Normal, "name %-40s shape %20s", name, shape);
  ArLog::log(ArLog::Normal, 
	     "\tprimary %08x size %2d layer %2d refresh %4u secondary %08x",
	     primary, size, layer, refresh, secondary);

  if (strcasecmp(visibility, "On") == 0 || 
      strcasecmp(visibility, "DefaultOn") == 0)
  {
    client.addHandler(name, &drawingDataCB);
    client.request(name, refresh);
    ArLog::log(ArLog::Normal, "\tRequesting %s since visibilty %s", name, visibility);
  }
  else
  {
    ArLog::log(ArLog::Normal, "\tNot requesting %s since visibilty %s", name, visibility);
  }
  
}

int main(int argc, char **argv)
{

  std::string hostname;
  ArGlobalFunctor1<ArNetPacket *> getDrawingListCB(&getDrawingList);
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);



  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);
  
  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Connected to server.\n");

  client.addHandler("getDrawingList", &getDrawingListCB);
  client.requestOnce("getDrawingList");
  
  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}
