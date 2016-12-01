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

/**
  @example configClientToServe.cpp Upload configuration to a server, read from a
file
   
  Usage: ./configClientToServer <file> [server]
  Where <file> is the filename to load and [server] is the address or hostname
  of the ARNL, MOGS or ArNetworking server to send the new configuration to 
  (localhost is used if omitted.)
  
  Note: arnlServer will receive the new configuration data but will not apply it
  until it is in an "idle" mode, such as Stop mode.

 */

ArClientBase *client;
ArClientHandlerConfig *configHandler;

char *file;

void saveConfigSucceeded(void)
{
  printf("Save config succeeded\n");
}

void saveConfigFailed(const char *str)
{
  printf("Save config failed: %s\n", str);
}

void gotConfig(void)
{
  char errorBuffer[1024];
  ArConfig *newConfig;
  if (!configHandler->getConfig()->parseFile(file, false, false, errorBuffer,
					     sizeof(errorBuffer)))
    printf("Error loading file: %s\n", errorBuffer);
  
  configHandler->saveConfigToServer();
  client->loopOnce();
  ArUtil::sleep(1000);
  client->loopOnce();
  ArUtil::sleep(1000);
  client->disconnect();
  Aria::shutdown();
  exit(0);
}

int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArGlobalFunctor gotConfigCB(&gotConfig);
  ArGlobalFunctor saveConfigSucceededCB(&saveConfigSucceeded);
  ArGlobalFunctor1<const char *> saveConfigFailedCB(&saveConfigFailed);
  std::string hostname;

  client = new ArClientBase;
  configHandler = new ArClientHandlerConfig(client, true);

  configHandler->addGotConfigCB(&gotConfigCB);
  configHandler->addSaveConfigSucceededCB(&saveConfigSucceededCB);
  configHandler->addSaveConfigFailedCB(&saveConfigFailedCB);
	
  if (argc == 1)
  {
    printf("Usage: %s <file> [host]\n", argv[0]);
    exit(1);
  }
  file = argv[1];
  if (argc == 2)
    hostname = "localhost";
  else
    hostname = argv[2];

  
  if (!client->blockingConnect(hostname.c_str(), 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  }
  //client->requestOnce("setConfig");
  configHandler->requestConfigFromServer();
  //client->requestOnce("setConfig");
  client->run();
  return 0;
}
