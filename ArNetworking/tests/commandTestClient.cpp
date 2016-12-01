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

void commandList(ArNetPacket *packet)
{
  char name[512];
  char description[512];
  int numCommands;
  int i;

  numCommands = packet->bufToByte2();
  printf("%d commands\n", numCommands);
  for (i = 0; i < numCommands; i++)
  {
    packet->bufToStr(name, sizeof(name));
    packet->bufToStr(description, sizeof(description));
    printf("%-20s%s\n", name, description);
    client.requestOnce(name);
  }

}

void stringCommandList(ArNetPacket *packet)
{
  char name[512];
  char description[512];
  int numCommands;
  int i;
  char buf[512];

  numCommands = packet->bufToByte2();
  printf("%d string commands\n", numCommands);
  for (i = 0; i < numCommands; i++)
  {
    packet->bufToStr(name, sizeof(name));
    packet->bufToStr(description, sizeof(description));
    printf("%-20s%s\n", name, description);
    sprintf(buf, "%ld some random fun string %ld", ArMath::random(),
	    ArMath::random());
    client.requestOnceWithString(name, buf);
  }
  
}



int main(int argc, char **argv)
{

  std::string hostname;
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  if (argc == 1)
    hostname = "localhost";
  else
    hostname = argv[1];
  if (!client.blockingConnect(hostname.c_str(), 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  } 
  ArGlobalFunctor1<ArNetPacket *> commandCB(&commandList);
  ArGlobalFunctor1<ArNetPacket *> stringCommandCB(&stringCommandList);
  client.addHandler("listCommands", &commandCB);
  client.addHandler("listStringCommands", &stringCommandCB);
  client.requestOnce("listCommands");
  client.requestOnce("listStringCommands");
  /*
  client.requestOnce("Function1");
  client.requestOnce("Function2");
  client.requestOnce("Function3");
  client.requestOnce("Function1");
  client.requestOnceWithString("StringFunction4", "Some string!");
  client.requestOnceWithString("StringFunction5", "funfunfun");
  client.requestOnceWithString("StringFunction6", "six");
  client.requestOnceWithString("StringFunction4", "Some other string!");
  */
  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}




