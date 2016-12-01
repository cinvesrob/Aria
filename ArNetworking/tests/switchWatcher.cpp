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


void clientListHandler(ArNetPacket *packet)
{
  int numClients;
  int i;
  char hostName[512];
  int port;
  char robotName[512];
  char flags[512];
  char robotIP[512];
  

  numClients = packet->bufToUByte2();
  printf("%d clients connected to server\n", numClients);
  for (i = 0; i < numClients; i++)
  {
    packet->bufToStr(hostName, sizeof(hostName));
    port = packet->bufToUByte2();
    packet->bufToStr(robotName, sizeof(robotName));
    packet->bufToStr(flags, sizeof(flags));
    packet->bufToStr(robotIP, sizeof(robotIP));
    printf("\tHost: '%s' Port: %d ID: '%s' Flags: %s IP: %s\n", hostName, port, robotName, flags, robotIP);
  }
  printf("\n");
}


void clientRemovedHandler(ArNetPacket *packet)
{
  char hostName[512];
  int port;
  char robotName[512];
  char flags[512];
  char robotIP[512];

  packet->bufToStr(hostName, sizeof(hostName));
  port = packet->bufToUByte2();
  packet->bufToStr(robotName, sizeof(robotName));
  packet->bufToStr(flags, sizeof(flags));
  packet->bufToStr(robotIP, sizeof(robotIP));
  printf("Removed Host: '%s' Port: %d ID: '%s' Flags: %s IP: %s\n", hostName, port, robotName, flags, robotIP);
}

void clientAddedHandler(ArNetPacket *packet)
{
  char hostName[512];
  int port;
  char robotName[512];
  char flags[512];
  char robotIP[512];

  packet->bufToStr(hostName, sizeof(hostName));
  port = packet->bufToUByte2();
  packet->bufToStr(robotName, sizeof(robotName));
  packet->bufToStr(flags, sizeof(flags));
  packet->bufToStr(robotIP, sizeof(robotIP));
  printf("Added Host: '%s' Port: %d ID: '%s' Flags: %s IP: %s\n", hostName, port, robotName, flags, robotIP);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArGlobalFunctor1<ArNetPacket *> clientListHandlerCB(&clientListHandler);
  ArGlobalFunctor1<ArNetPacket *> clientAddedHandlerCB(&clientAddedHandler);
  ArGlobalFunctor1<ArNetPacket *> clientRemovedHandlerCB(&clientRemovedHandler);
  ArNetPacket packet;

  ArClientBase client;

  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Connected to server.\n");

  client.addHandler("clientList", &clientListHandlerCB);
  client.requestOnce("clientList");
  client.addHandler("clientRemoved", &clientRemovedHandlerCB);
  client.request("clientRemoved", -1);
  client.addHandler("clientAdded", &clientAddedHandlerCB);
  client.request("clientAdded", -1);
  client.run();
  Aria::shutdown();
  return 0;
}
