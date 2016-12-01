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
#include "ArNetPacketReceiverUdp.h"
#ifndef WIN32
#include <errno.h>
#endif

AREXPORT ArNetPacketReceiverUdp::ArNetPacketReceiverUdp() :
  myProcessPacketCB(NULL),
  mySocket(NULL),
  myLastPacket(),
  myPacket(),
  myBuff()
{  
  memset(myBuff, 0, ArNetPacket::MAX_LENGTH+20);

  // one little odd note, this calls setBuf on the packet so that its
  // using our own buffer and not its own... this is the only place we
  // don't want it to own the buffer
  myPacket.setBuf(myBuff, sizeof(myBuff));
}

AREXPORT ArNetPacketReceiverUdp::~ArNetPacketReceiverUdp()
{

}

/**
   Sets the socket that this receiver will use, note that it does not
   transfer ownership of the socket.  

   @param socket the socket to use for receiving data
**/
AREXPORT void ArNetPacketReceiverUdp::setSocket(ArSocket *socket)
{
  mySocket = socket;
}

/**
   Gets the socket that the receiver is using, note that it does not
   have ownership of this socket and that whatever created it should.
**/
AREXPORT ArSocket *ArNetPacketReceiverUdp::getSocket(void)
{
  return mySocket;
}


/**
   @param functor the callback to use when a packet needs to be processed
**/
AREXPORT void ArNetPacketReceiverUdp::setProcessPacketCB(
	ArFunctor2<ArNetPacket *, struct sockaddr_in *> *functor)
{
  myProcessPacketCB = functor;
}

/**
   @return the callback used when a packet needs to be processed
**/
AREXPORT ArFunctor2<ArNetPacket *, struct sockaddr_in *> *ArNetPacketReceiverUdp::getProcessPacketCB(void)
{
  return myProcessPacketCB;
}

AREXPORT bool ArNetPacketReceiverUdp::readData(void)
{
  int ret;
  int packetLength;
  struct sockaddr_in sin;

  if (mySocket == NULL)
  {
    ArLog::log(ArLog::Verbose, "NULL Udp Socket");
    return false;
  }
  // while we can read a packet, do it
  while ((ret = mySocket->recvFrom(myBuff, sizeof(myBuff), &sin)) > 0)
  {
    packetLength = (myBuff[3] << 8) | myBuff[2] & 0xff;
    if (ret != packetLength)
      fprintf(stderr, "packet length %d not equal to udp packet length %d",
	      packetLength, ret);
    myPacket.setLength(packetLength);
    myPacket.setBuf(myBuff, sizeof(myBuff));
    if (myPacket.verifyCheckSum()) 
    {
      myPacket.resetRead();
      /* put this in if you want to see the packets received
	 //printf("Input ");
	 myPacket.log();
      */
      // you can also do this next line if you only care about type
      //printf("Input %x\n", myPacket.getCommand());
      //myPacket.log();
      myPacket.resetRead();
      if (myProcessPacketCB != NULL)
      {
	myPacket.setPacketSource(ArNetPacket::UDP);
	myProcessPacketCB->invoke(&myPacket, &sin);
      }
    }
    else 
    {
      myPacket.resetRead();
      ArLog::log(ArLog::Normal, 
		 "ArNetPacketReceiverUdp::receivePacket: bad packet, bad checksum on packet %d", myPacket.getCommand());
    }
    
  }


  
  // see why we ran out of packets, if its bad return false so that it
  // is known the socket is bad now
  if (ret < 0)
  {
#ifdef WIN32
    if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAECONNRESET)
      return true;
    else
      ArLog::log(ArLog::Terse, "Failed on read UDP, error %d", WSAGetLastError());
#endif
#ifndef WIN32
    if (errno == EAGAIN)
      return true;
    else
      ArLog::log(ArLog::Terse, "Failed on read UDP, error %d", errno);
#endif
    ArLog::log(ArLog::Terse, "Failed on the udp read");
    return false;
  }
  else if (ret == 0)
  {
    printf("Read 0 byte UDP packet\n");
    return true;
  }

  return true;
  
}
