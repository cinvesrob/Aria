#ifndef ARNETPACKETSENDERTCP_H
#define ARNETPACKETSENDERTCP_H

#include "Aria.h"
#include "ArNetPacket.h"

class ArNetPacketSenderTcp
{
public:
  /// Constructor
  AREXPORT ArNetPacketSenderTcp();
  /// Destructor
  AREXPORT ~ArNetPacketSenderTcp();
  
  /// Sets the socket this sender uses
  AREXPORT void setSocket(ArSocket *socket);
  /// Gets the socket this sender uses
  AREXPORT ArSocket *getSocket(void);

  /// Sets the connection timeout
  AREXPORT void setBackupTimeout(double connectionTimeoutInMins);

  /// Sets debug logging
  AREXPORT void setDebugLogging(bool debugLogging);

  /// Sets the logging prefix
  AREXPORT void setLoggingPrefix(const char *prefix);

  /// Sends a packet
  AREXPORT void sendPacket(ArNetPacket *packet, 
			   const char *loggingString = "");

  /// Tries to send the data there is to be sent
  AREXPORT bool sendData(void);
protected:
  ArMutex myDataMutex;
  bool myDebugLogging;
  std::string myLoggingPrefix;
  ArLog::LogLevel myVerboseLogLevel;
  ArSocket *mySocket;
  std::list<ArNetPacket *> myPacketList;
  ArNetPacket *myPacket;
  int myAlreadySent;
  const char *myBuf;
  int myLength;
  double myBackupTimeout;
  ArTime myLastGoodSend;

};

#endif 
