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
