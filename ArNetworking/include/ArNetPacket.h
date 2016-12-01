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
#ifndef NLNETPACKET_H
#define NLNETPACKET_H

#include "Aria.h"

/// our packet for the network stuff
class ArNetPacket : public ArBasePacket
{
public:
  /// Constructor
  AREXPORT ArNetPacket(
	  ArTypes::UByte2 bufferSize = ArNetPacket::MAX_LENGTH + 5);

  /// Copy constructor
  AREXPORT ArNetPacket(const ArNetPacket &other);

  /// Assignment operator
  AREXPORT ArNetPacket &operator=(const ArNetPacket &other);

  /// Destructor
  AREXPORT virtual ~ArNetPacket();


  /// Sets the command this packet is
  AREXPORT void setCommand(ArTypes::UByte2 command);
  /// Gets the command this packet is
  AREXPORT ArTypes::UByte2 getCommand(void);
  enum { 
    SIZE_OF_LENGTH = 2,  ///< Number of bytes needed to store packet length value
    MAX_LENGTH = 32000,  ///< Suggested maximum total size of a packet (bytes)
    HEADER_LENGTH = 6,   ///< Bytes of packet data used for header
    FOOTER_LENGTH = 2,   ///< Byset of packet data used for footer 

    /** Suggested maximum size for data payload (this is the total suggested
        packet size minus headers and footers)
    */
    MAX_DATA_LENGTH = MAX_LENGTH - HEADER_LENGTH - FOOTER_LENGTH - SIZE_OF_LENGTH
  };
  
  /// Puts a double into the packet buffer
  AREXPORT virtual void doubleToBuf(double val);
  /// Gets a double from the packet buffer
  AREXPORT virtual double bufToDouble(void);
  AREXPORT virtual void empty(void);
  AREXPORT virtual void finalizePacket(void);
  AREXPORT virtual void resetRead(void);
  AREXPORT virtual void duplicatePacket(ArNetPacket *packet);
  
  /// returns true if the checksum matches what it should be
  AREXPORT bool verifyCheckSum(void);
  /// returns the checksum, probably used only internally
  AREXPORT ArTypes::Byte2 calcCheckSum(void);

  /// Iternal function that sets if we already added the footer(for forwarding)
  bool getAddedFooter(void) { return myAddedFooter; }
  /// Iternal function that sets if we already added the footer(for forwarding)
  void setAddedFooter(bool addedFooter) { myAddedFooter = addedFooter; }

  /// an enum for where the packet came from
  enum PacketSource
  {
    TCP, ///< Came in over tcp
    UDP ///< Came in over udp
  };
  PacketSource getPacketSource(void) { return myPacketSource; }
  void setPacketSource(PacketSource source) { myPacketSource = source; }
  void setArbitraryString(const char *string) { myArbitraryString = string; }
  const char *getArbitraryString(void) { return myArbitraryString.c_str(); }

private:

  /// Inserts the header information (command, length, etc.) into the buffer.
  void insertHeader();


protected:
  PacketSource myPacketSource;
  bool myAddedFooter;
  std::string myArbitraryString;
  ArTypes::UByte2 myCommand;
};


#endif
