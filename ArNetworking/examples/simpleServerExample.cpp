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


/** @example simpleServerExample.cpp This is a simple example of an ArNetworking server.
 * This is the simplest possible ArNetworking server.  It creates the server but
 * does not try to connect to the robot or any other
 * devices, and provides no services.
 *
 * For more complete servers that connect to the robot, see
 * simpleRobotServerExample.cpp and  serverDemo.cpp.
 */

int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);

  // The base server object, manages all connections to clients.
  ArServerBase server;

  // This object simplifies configuration and opening of the ArServerBase
  // object.
  ArServerSimpleOpener simpleOpener(&parser);

  // parse the command line. fail and print the help if the parsing fails
  // or if the help was requested with -help
  parser.loadDefaultArguments();
  if (!simpleOpener.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {    
    simpleOpener.logOptions();
    Aria::exit(1);
  }

  // Use the ArSimpleOpener to open the server port
  if (!simpleOpener.open(&server))
  {
    ArLog::log(ArLog::Terse, "Error: Could not open server on port %d", simpleOpener.getPort());
    exit(1);
  }



  // The simple opener might have information to display right before starting 
  // the server thread:
  simpleOpener.checkAndLog();

  // now let the server base run in a new thread, accepting client connections.
  server.runAsync();

  ArLog::log(ArLog::Normal, "Server is now running... Press Ctrl-C to exit.");


  while(true)
    ArUtil::sleep(500);
  Aria::exit(0);
}


