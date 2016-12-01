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

void getCameraModeList(ArNetPacket *packet)
{
  ArTypes::UByte2 numModes;
  
  numModes = packet->bufToUByte2();

  printf("%d camera modes:\n", numModes);
  char modeName[512];
  for (int i = 0; i < numModes; i++)
  {
    packet->bufToStr(modeName, sizeof(modeName));
    printf("\t%s\n", modeName);
  }
}

void cameraModeUpdated(ArNetPacket *packet)
{
  char modeName[512];
  packet->bufToStr(modeName, sizeof(modeName));
  printf("Switched to mode %s\n", modeName);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArClientBase client;
  std::string host;
  

  ArArgumentParser parser(&argc, argv);
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!clientConnector.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    clientConnector.logOptions();
    exit(0);
  }

  
  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server rejected connection, exiting\n");
    else
      printf("Could not connect to server, exiting\n");
    exit(1);
  } 


  client.addHandler("getCameraModeListCam1", 
		    new ArGlobalFunctor1<ArNetPacket *>(&getCameraModeList));
  
  client.requestOnce("getCameraModeListCam1");

  ArNetPacket sending;
  
  // This does the look at goal mode 
  sending.empty();
  sending.strToBuf("lookAtGoal");
  client.requestOnce("setCameraModeCam1", &sending);

  // This does the look at point mode (at 0 0);

  sending.empty();
  sending.strToBuf("lookAtPoint");
  sending.byte4ToBuf(1157);
  sending.byte4ToBuf(-15786);
  client.requestOnce("setCameraModeCam1", &sending);

  while (Aria::getRunning() && client.isConnected())
  {
    client.loopOnce();
    ArUtil::sleep(1000);
  }
  Aria::shutdown();
  return 0;

};
