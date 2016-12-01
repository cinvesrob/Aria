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
#ifndef NLNETPACKETRECEIVERTCP_H
#define NLNETPACKETRECEIVERTCP_H

#include "Aria.h"
#include "ArNetPacket.h"

/**
   This class receives TCP packets from a socket, you need to have an
   open socket and give it to the socket with setSocket, then you need
   to set up a callback to process packets with setProcessPacketCB,
   finally call readData which will read in all the data and call the
   processPacketCB.
**/
class ArNetPacketReceiverTcp
{
public:
  /// Constructor
  AREXPORT ArNetPacketReceiverTcp();
  /// Destructor
  AREXPORT ~ArNetPacketReceiverTcp();
  
  /// Sets the socket this receiver uses
  AREXPORT void setSocket(ArSocket *socket);
  /// Gets the socket this receiver uses
  AREXPORT ArSocket *getSocket(void);

  /// Sets the callback for use when a packet is received
  AREXPORT void setProcessPacketCB(ArFunctor1<ArNetPacket *> *functor);

  /// Gets the callback used when a packet is received
  AREXPORT ArFunctor1<ArNetPacket *> *getProcessPacketCB(void);

  /// Sets the logging prefix
  AREXPORT void setLoggingPrefix(const char *loggingPrefix);

  /// Reads in all the data available calling the processPacketCB
  AREXPORT bool readData(void);
  
  /// Sets whether we're quiet about errors or not
  void setQuiet(bool quiet) { myQuiet = quiet; }
  /// Gets whether we're quiet about errors or not
  bool getQuiet(void) { return myQuiet; }
protected:
  enum Ret { 
    RET_CONN_CLOSED, // the connection was closed (in a good manner)
    RET_CONN_ERROR, // the connection was has an error (so close it)
    RET_GOT_PACKET, // we got a good packet
    RET_BAD_PACKET, // we got a bad packet (checksum wrong)
    RET_FAILED_READ, // our read failed (no data)
    RET_TIMED_OUT}; // we were reading and timed out
  /// Reads in a single packet, returns NULL if not one
  AREXPORT Ret readPacket(int msWait);


  enum State { STATE_SYNC1, STATE_SYNC2, STATE_LENGTH1,
	       STATE_LENGTH2, STATE_ACQUIRE_DATA };

  enum {
    TOTAL_PACKET_LENGTH = ArNetPacket::MAX_LENGTH+ArNetPacket::HEADER_LENGTH+ArNetPacket::FOOTER_LENGTH
  };

  State myState;
  ArFunctor1<ArNetPacket *> *myProcessPacketCB;
  bool myQuiet;
  ArSocket *mySocket;
  ArTime myLastPacket;
  ArNetPacket myPacket;


  char myReadBuff[TOTAL_PACKET_LENGTH];
  int myReadCount;
  int myReadLength;
  int myReadCommand;
  unsigned char mySync1;
  unsigned char mySync2;
  std::string myLoggingPrefix;
};

#endif // NLNETPACKETRECEIVERTCP
