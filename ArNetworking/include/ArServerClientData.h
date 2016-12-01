#ifndef ARSERVERCLIENTDATA_H
#define ARSERVERCLIENTDATA_H

#include "Aria.h"

class ArServerClientData
{
public:
  ArServerClientData(ArServerData *serverData, long mSecInterval, 
		     ArNetPacket *packet)
    {
      myServerData = serverData; 
      myMSecInterval = mSecInterval;
      myPacket.duplicatePacket(packet);
      myReadLength = myPacket.getReadLength();
      myLastSent.setToNow();
    }
  virtual ~ArServerClientData() {}
  ArServerData *getServerData(void) { return myServerData; }
  long getMSec(void) { return myMSecInterval; }
  ArNetPacket *getPacket(void) 
    { myPacket.setReadLength(myReadLength); return &myPacket; }
  ArTime getLastSent (void) { return myLastSent; }
  void setLastSentToNow(void) { myLastSent.setToNow(); }
  void setMSec(long mSec) { myMSecInterval = mSec; }
  void setPacket(ArNetPacket *packet) 
    { 
      myPacket.duplicatePacket(packet); 
      myReadLength = myPacket.getReadLength();
    }
protected:
  ArServerData *myServerData;
  long myMSecInterval;
  ArNetPacket myPacket;
  unsigned int myReadLength;
  ArTime myLastSent;

};

#endif // ARSERVERCLIENTDATA_H
