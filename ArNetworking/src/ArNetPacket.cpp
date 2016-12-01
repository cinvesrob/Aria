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
#include "ArNetPacket.h"

AREXPORT ArNetPacket::ArNetPacket(ArTypes::UByte2 bufferSize) :
  ArBasePacket(bufferSize, ArNetPacket::HEADER_LENGTH, NULL, 
	       ArNetPacket::FOOTER_LENGTH),
  myPacketSource(TCP),
  myAddedFooter(false),
  myArbitraryString(),
  myCommand(0)
{
  insertHeader();
}  

AREXPORT ArNetPacket::ArNetPacket(const ArNetPacket &other) :
  ArBasePacket(other),
  myPacketSource(other.myPacketSource),
  myAddedFooter(other.myAddedFooter),
  myArbitraryString(other.myArbitraryString),
  myCommand(other.myCommand)
{
}

AREXPORT ArNetPacket &ArNetPacket::operator=(const ArNetPacket &other) 
{
  if (this != &other) {
    ArBasePacket::operator=(other);
    myPacketSource = other.myPacketSource;
    myAddedFooter  = other.myAddedFooter;
    myArbitraryString = other.myArbitraryString;
    myCommand      = other.myCommand;
  }
  return *this;
}


AREXPORT ArNetPacket::~ArNetPacket()
{

}

AREXPORT void ArNetPacket::doubleToBuf(double val)
{
  char buf[256];
  if (val == -HUGE_VAL)
	sprintf(buf, "-INF");
  else if (val == HUGE_VAL)
	sprintf(buf, "INF");
  else
	sprintf(buf, "%g", val);
  strToBuf(buf);
}

AREXPORT double ArNetPacket::bufToDouble(void)
{
  char buf[256];
  char *endPtr;
  double ret;

  bufToStr(buf, sizeof(buf));
  if (strncmp(buf, "-INF", sizeof(buf)) == 0)
  {
	ret = -HUGE_VAL;
	return ret;
  }
  else if (strncmp(buf, "INF", sizeof(buf)) == 0)
  {
	ret = HUGE_VAL;
	return ret;
  }
  else
  {
	ret = strtod(buf, &endPtr);
	if (endPtr[0] == '\0' && endPtr != buf)
	  return ret;
	else
      return 0;
  }
}

AREXPORT void ArNetPacket::empty(void)
{
  myCommand = 0;
  myLength = myHeaderLength;
  myAddedFooter = false;
  resetValid();
}


void ArNetPacket::insertHeader()
{
  int length = myLength;
  myLength = 0;
  uByteToBuf(0xF);         // 1
  uByteToBuf(0xC);         // 2

  if (myAddedFooter)
    uByte2ToBuf(length);   // 3 & 4
  else
    uByte2ToBuf(length+2); // 3 & 4

  uByte2ToBuf(myCommand);  // 5 & 6

  if (myAddedFooter)
    myLength = length - 2;
  else
    myLength = length;

} // end method insertHeader


AREXPORT void ArNetPacket::finalizePacket(void)
{

  insertHeader();

  int chkSum = calcCheckSum();
  byteToBuf((chkSum >> 8) & 0xff );
  byteToBuf(chkSum & 0xff );
  myAddedFooter = true;
  //log();
  //printf("%d %d %d\n", myLength ,myCommand, chkSum);
}

AREXPORT void ArNetPacket::resetRead(void)
{
  myReadLength = 4;
  myCommand = bufToUByte2();
  myReadLength = myHeaderLength;
  resetValid();

}

AREXPORT void ArNetPacket::setCommand(ArTypes::UByte2 command)
{
  myCommand = command;
}

AREXPORT ArTypes::UByte2 ArNetPacket::getCommand(void)
{
  return myCommand;
}

AREXPORT void ArNetPacket::duplicatePacket(ArNetPacket *packet)
{
  myLength = packet->myLength;

  // if (myMaxLength < myLength)
  //  setMaxLength(packet->myLength);
  if (myMaxLength < myLength + packet->myFooterLength)
    setMaxLength(packet->myLength + packet->myFooterLength);

  myReadLength = packet->myReadLength;
  myHeaderLength = packet->myHeaderLength;
  myFooterLength = packet->myFooterLength;
  myCommand = packet->myCommand;
  myAddedFooter = packet->myAddedFooter;
  memcpy(myBuf, packet->getBuf(), packet->myLength + packet->myFooterLength);
  myArbitraryString = packet->myArbitraryString;
}

AREXPORT ArTypes::Byte2 ArNetPacket::calcCheckSum(void)
{
  int c = 0;

  //printf("%d\n", myLength); 
  //log();
  int i = 3;
  int n = myLength - 2;

  while (n > 3) {

    //printf("n %d i %d c %d c1 %d c2 %d\n", n, i, c, myBuf[i], myBuf[i+1]);
    c += ((unsigned char)myBuf[i]<<8) | (unsigned char)myBuf[i+1];
    c = c & 0xffff;
    n -= 2;
    i += 2;
  }
  //printf("aft n %d i %d c %d\n", n, i, c);
  if (n > 0) 
    c = c ^ (int)((unsigned char) myBuf[i]);
  //printf("%d\n", c);
  return c;
}

AREXPORT bool ArNetPacket::verifyCheckSum(void) 
{
  ArTypes::Byte2 chksum;
  ArTypes::Byte2 calcedChksum;
  unsigned char c1, c2;
  int length;

  if (myLength - 2 < myHeaderLength)
    return false;

  c2 = myBuf[myLength-2];
  c1 = myBuf[myLength-1];
  chksum = (c1 & 0xff) | (c2 << 8);
  length = myLength;
  myLength = myLength - 2;
  calcedChksum = calcCheckSum();
  myLength = length;

  //printf("%d %d\n", chksum, calcedChksum);
  if (chksum == calcedChksum) {
    return true;
  } else {
    return false;
  }
  
}
