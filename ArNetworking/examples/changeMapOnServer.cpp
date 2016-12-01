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

ArClientBase *client;
ArClientHandlerConfig *configHandler;
bool done = false;

std::string newmapname;

void gotConfig(void)
{
  configHandler->lock();
  ArConfig *newConfig = configHandler->getConfig();
  printf("Got configuration from server. Changing map to %s\n", newmapname.c_str());
  ArConfigSection *s = newConfig->findSection("Files");
  if(!s) return;
  ArConfigArg *arg = s->findParam("Map");
  if(!arg) return;
  arg->setString(newmapname.c_str());
  //char error[512];
  //newConfig->callProcessFileCallbacks(false, error, 512);
  configHandler->unlock();
  puts("Saving...");
  configHandler->saveConfigToServer();
  ArUtil::sleep(10000); // hack todo wait for map changed notification or error.
  done = true;
}

int main(int argc, char **argv)
{
  Aria::init();
  ArGlobalFunctor gotConfigCB(&gotConfig);
  std::string hostname;

  client = new ArClientBase;
  configHandler = new ArClientHandlerConfig(client);

  configHandler->addGotConfigCB(&gotConfigCB);

  ArArgumentParser parser(&argc, argv);
	
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed(1))
  {
    Aria::logOptions();
    exit(0);
  }

  char *val = parser.checkParameterArgument("-map");
  if(val)
  {
    newmapname = val;
    printf("Will set new map name to %s on server.\n", newmapname.c_str());
  }
  else
  {
    puts("Error: must specify new map file name with -map argument.\nExample:\n\t./changeMapOnServer -host 10.0.126.32 -map columbia.map");
    Aria::exit(-5);
    return -5;
  }


  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(client))
  {
    puts("Error conncting to server. Use -host argument to specify host address.");
    Aria::exit(-4);
    return -4;
  } 

  puts("Connected to server.");

  client->setRobotName(client->getHost()); // include server hostname in log messages


  puts("Requesting current configuration...");
  configHandler->requestConfigFromServer();
  client->runAsync();

  while (!done)
    ArUtil::sleep(100);
  
  Aria::exit(0);
}
