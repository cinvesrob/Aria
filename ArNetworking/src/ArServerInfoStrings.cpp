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
#include "ArExport.h"
#include "ArServerInfoStrings.h"

AREXPORT ArServerInfoStrings::ArServerInfoStrings(ArServerBase *server) :
  myAddStringFunctor(this, &ArServerInfoStrings::addString),
  myNetGetStringsInfoCB(this, &ArServerInfoStrings::netGetStringsInfo),
  myNetGetStringsCB(this, &ArServerInfoStrings::netGetStrings)
{
  myStringsMutex.setLogName("ArServerInfoStrings::myStringsMutex");
  myServer = server;
  if (myServer != NULL)
  {
    myServer->addData("getStringsInfo", 
		      "Gets the information about the 'getStrings' command",
		      &myNetGetStringsInfoCB,
		      "none", 
  "uByte2: count; repeating count times  string: name, uByte2: maxlength", 
		      "NavigationInfo", "RETURN_SINGLE");
    myServer->addData("getStrings", 
	      "Gets the strings, for info about them see getStringsInfo",
		      &myNetGetStringsCB,
		      "none",
		      "repeating: string", 
		      "NavigationInfo", "RETURN_SINGLE");
    
  }
  myMaxMaxLength = 512;
}

AREXPORT ArServerInfoStrings::~ArServerInfoStrings()
{

}

AREXPORT void ArServerInfoStrings::buildStringsInfoPacket(void)
{
  myStringsMutex.lock();
  std::list<ArStringInfoHolder *>::iterator it;
  ArStringInfoHolder *info;
  
  myStringInfoPacket.empty();
  ArTypes::UByte2 count;
  count = myStrings.size();
  myStringInfoPacket.uByte2ToBuf(count);
  for (it = myStrings.begin(); it != myStrings.end(); it++)
  {
    info = (*it);
    myStringInfoPacket.strToBuf(info->getName());
    myStringInfoPacket.uByte2ToBuf(info->getMaxLength());
  }
  myStringsMutex.unlock();
}

AREXPORT void ArServerInfoStrings::buildStringsPacket(void)
{
  myStringsMutex.lock();

  /* This part didn't work since the last string packet build was
   * never set, so just commenting it since it caused problems when
   * the value wrapped
  if (myLastStringPacketBuild.mSecSince() < 100)
  {
    myStringsMutex.unlock();
    return;
  }
  */

  myStringPacket.empty();
  std::list<ArStringInfoHolder *>::iterator it;
  ArStringInfoHolder *info;

  char *buf;
  buf = new char[myMaxMaxLength+1];

  for (it = myStrings.begin(); it != myStrings.end(); it++)
  {
    info = (*it);
    info->getFunctor()->invoke(buf, info->getMaxLength());
    myStringPacket.strToBuf(buf);
  }

  delete[] buf;

  myStringsMutex.unlock();
}

AREXPORT void ArServerInfoStrings::netGetStringsInfo(ArServerClient *client,
							ArNetPacket *packet)
{
  buildStringsInfoPacket();
  client->sendPacketTcp(&myStringInfoPacket);
}

AREXPORT void ArServerInfoStrings::netGetStrings(ArServerClient *client, 
						    ArNetPacket *packet)
{
  buildStringsPacket();
  client->sendPacketTcp(&myStringPacket);
}


AREXPORT void ArServerInfoStrings::addString(
	const char *name, ArTypes::UByte2 maxLength,
	ArFunctor2<char *, ArTypes::UByte2> *functor)
{
  myStringsMutex.lock();
  if (myMaxMaxLength < maxLength)
    myMaxMaxLength = maxLength;
  myStrings.push_back(new ArStringInfoHolder(name, maxLength, functor));
  myStringsMutex.unlock();
  buildStringsInfoPacket();
  myServer->broadcastPacketTcp(&myStringInfoPacket, "getStringsInfo");
}


AREXPORT ArStringInfoHolder *ArServerInfoStrings::internalGetStringInfoHolder(
	const char *name)
{
  myStringsMutex.lock();
  std::list<ArStringInfoHolder *>::iterator it;
  ArStringInfoHolder *info;
  
  for (it = myStrings.begin(); it != myStrings.end(); it++)
  {
    info = (*it);
    if (ArUtil::strcasecmp(info->getName(), name) == 0)
    {
      myStringsMutex.unlock();
      return info;
    }
    myStringInfoPacket.strToBuf(info->getName());
    myStringInfoPacket.uByte2ToBuf(info->getMaxLength());
  }
    
  myStringsMutex.unlock();
  return NULL;
}
