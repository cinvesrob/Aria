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

ArMap arMap;
ArClientBase client;
ArTime start;

void handleGetMapName(ArNetPacket *packet)
{
  char buffer[512];

  packet->bufToStr(buffer, sizeof(buffer));
  printf("MapFile: %s\n", buffer);
}

void handleGetMap(ArNetPacket *packet)
{
  char buffer[10000];

  if (packet->getDataReadLength() == packet->getDataLength())
  {
    printf("Empty packet signifying end of map (for central forward)\n");
    return;
  }
  
  packet->bufToStr(buffer, sizeof(buffer));
  // if we got an end of line char instead of a line it means the map is over
  if (buffer[0] == '\0')
  {
    printf("Map took %g seconds\n", start.mSecSince() / 1000.0);
    arMap.parsingComplete();
    arMap.writeFile("mapExample.map");
    //client.disconnect();
    //exit(0);
  }
  else
  {
    //printf("line '%s'\n", buffer);
    arMap.parseLine(buffer);
  }

}

int main(int argc, char **argv)
{

  ArGlobalFunctor1<ArNetPacket *> getMapNameCB(handleGetMapName);
  ArGlobalFunctor1<ArNetPacket *> getMapCB(handleGetMap);

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
    exit(1);
  } 

  client.addHandler("getMap", &getMapCB);
  client.addHandler("getMapName", &getMapNameCB);
  client.requestOnce("getMapName");
  start.setToNow();
  client.requestOnce("getMap");

  client.run();

}
