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

int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArClientBase client;

  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);
  parser.loadDefaultArguments();

  if (!clientConnector.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    clientConnector.logOptions();
    exit(0);
  }

  if (parser.getArgc() < 4 || parser.getArgc() > 6)
  {
    printf("usage: %s <x> <y> <th> <optional:xyspread> <optional:thspread>", argv[0]);
    exit(1);
  }

  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 
  client.runAsync();

  ArNetPacket sending;
  // put in the arguments (you can see what they are from doing -lcl on clientDemo)
  sending.byte4ToBuf(atoi(parser.getArg(1)));
  sending.byte4ToBuf(atoi(parser.getArg(2)));
  sending.byte4ToBuf(atoi(parser.getArg(3)));
  if (parser.getArgc() > 4)
    sending.uByte4ToBuf(atoi(parser.getArg(4)));
  if (parser.getArgc() > 5)
    sending.uByte4ToBuf(atoi(parser.getArg(5)));
  // send the packet
  client.requestOnce("localizeToPose", &sending);

  // you have to give the client some time to send the command
  ArUtil::sleep(500);
  

  Aria::shutdown();
  return 0;
}

