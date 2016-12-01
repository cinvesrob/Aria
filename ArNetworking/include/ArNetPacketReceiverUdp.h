#ifndef NLNETPACKETRECEIVERUDP_H
#define NLNETPACKETRECEIVERUDP_H

#include "ArNetPacket.h"

/**
   This is the receiver for UDP packets.
**/

class ArNetPacketReceiverUdp
{
public:
  AREXPORT ArNetPacketReceiverUdp();
  AREXPORT ~ArNetPacketReceiverUdp();
  
  /// Sets the socket this receiver uses
  AREXPORT void setSocket(ArSocket *socket);
  /// Gets the socket this receiver uses
  AREXPORT ArSocket *getSocket(void);

  /// Sets the callback for use when a packet is received
  AREXPORT void setProcessPacketCB(ArFunctor2<ArNetPacket *, 
				   struct sockaddr_in *> *functor);

  /// Gets the callback used when a packet is received
  AREXPORT ArFunctor2<ArNetPacket *, struct sockaddr_in *> *
                                                              getProcessPacketCB(void);
  
  /// Reads in all the data available calling the processPacketCB
  AREXPORT bool readData(void);

protected:
  ArFunctor2<ArNetPacket *, struct sockaddr_in *> *myProcessPacketCB;
  ArSocket *mySocket;
  ArTime myLastPacket;
  ArNetPacket myPacket;
  char myBuff[ArNetPacket::MAX_LENGTH+20];

};

#endif // NLNETPACKETRECEIVERUDP_H
