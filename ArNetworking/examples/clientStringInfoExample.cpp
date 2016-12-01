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
/* This example shows how to get the detailed information available from server
 * application "string info group" table (shown as "custom details" or "details"
 * in MobileEyes). Exactly what data is available depends on what the server
 * application provides as part of the string info. Server applications can use
 * ArServerInfoStrings to publish the string info.
 */

#include "Aria.h"
#include "ArNetworking.h"
#include "ArClientHandlerRobotUpdate.h"
#include <string>
#include <vector>
#include <assert.h>

void handleStringsInfo(ArNetPacket*);
void handleStringsData(ArNetPacket*);

struct Item {
  std::string name;
  std::string value;
};
std::vector<struct Item> data;
ArMutex dataMutex;

ArClientBase client;

bool go = false;

int main(int argc, char **argv)
{
  Aria::init();
  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    Aria::exit(0);
  }

  
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    Aria::exit(1);
  } 

  printf("Connected to server.\n");

  client.setRobotName(client.getHost()); // include server name in log messages

  client.runAsync();

 
  ArClientHandlerRobotUpdate updates(&client);
  updates.requestUpdates();

  /* Use a functor object to receive replies to the "getStringsInfo" request.
   * See handleStringsInfo function below. */
  ArGlobalFunctor1<ArNetPacket*>  stringsInfoHandler(&handleStringsInfo);
  client.addHandler("getStringsInfo", &stringsInfoHandler);
  client.requestOnce("getStringsInfo");

  /* This will receive the data updates.  The updates are not requested here,
   * they are requested after we get the getStringsInfo reply. */
  ArGlobalFunctor1<ArNetPacket*> stringsDataHandler(&handleStringsData);
  client.addHandler("getStrings", &stringsDataHandler);

  printf("Mode  Status  Position  Velocity   Battery");
  while (client.getRunningWithLock())
  {
    if(!go) continue;
    updates.lock();
    printf("%s  %s  %.2f,%.2f,%.2f  %.2f,%.2f,%.2f  %.1f",
      updates.getMode(),
      updates.getStatus(),
      updates.getX(), updates.getY(), updates.getTh(),
      updates.getVel(), updates.getLatVel(), updates.getRotVel(),
      updates.getVoltage()
    );
    updates.unlock();
    dataMutex.lock();
    for(std::vector<struct Item>::const_iterator i = data.begin(); i != data.end(); ++i)
    {
      printf("   %s", i->value.c_str()); 
    }
    dataMutex.unlock();
    printf("\n");
    ArUtil::sleep(1000);
  }

  client.disconnect();
  Aria::exit(0);
  return 0;
}

/* Handler for new data */
void handleStringsData(ArNetPacket* pkt)
{
  dataMutex.lock();
  for(unsigned int i = 0; i < data.size(); ++i)
  {
    // todo check for unexpected end of packet
    std::string val = pkt->bufToString();
    data[i].value = val;
  }
  dataMutex.unlock();
}

/* Handler for list of data items. Print the rest of the table header and then
 * request the actual data updates with "getStrings" request. */
void handleStringsInfo(ArNetPacket *pkt)
{
  unsigned int n = pkt->bufToUByte2();
  dataMutex.lock();
  data.resize(n);
  for(unsigned int i = 0; i < n; ++i)
  {
    // todo check for unexpected end of packet
    data[i].name = pkt->bufToString();

    data[i].value = "?";
    unsigned int max = pkt->bufToUByte2();
    printf("   %s", data[i].name.c_str());
  }
  dataMutex.unlock();
  printf("\n");
  client.request("getStrings", 1000);
  go = true;
}
