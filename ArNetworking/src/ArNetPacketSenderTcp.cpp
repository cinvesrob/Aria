#include "Aria.h"
#include "ArExport.h"
#include "ArNetPacketSenderTcp.h"

AREXPORT ArNetPacketSenderTcp::ArNetPacketSenderTcp() :
  mySocket(NULL),
  myPacketList(),
  myPacket(NULL),
  myAlreadySent(false),
  myBuf(NULL),
  myLength(0)
{
  myDataMutex.setLogName("ArNetPacketSenderTcp::myDataMutex");
  setDebugLogging(false);
  myBackupTimeout = -1;
  myLastGoodSend.setToNow();
}

AREXPORT ArNetPacketSenderTcp::~ArNetPacketSenderTcp()
{
  ArNetPacket *packet;
  int i = 0;
  long bytes = 0;
  while (myPacketList.begin() != myPacketList.end())
  {
    i++;
    packet = myPacketList.front();
    bytes += packet->getLength();
    myPacketList.pop_front();
    delete packet;
  }
  if (i > 0)
    ArLog::log(ArLog::Normal, "Deleted %d packets of %d bytes", i, bytes);
}

/**
   Sets the socket that this receiver will use, note that it does not
   transfer ownership of the socket.  

   @param socket the socket to use for receiving data
**/
AREXPORT void ArNetPacketSenderTcp::setSocket(ArSocket *socket)
{
  myDataMutex.lock();
  mySocket = socket;
  myDataMutex.unlock();
}

AREXPORT void ArNetPacketSenderTcp::setBackupTimeout(
	double connectionTimeoutInMins)
{
  myBackupTimeout = connectionTimeoutInMins;
}

AREXPORT void ArNetPacketSenderTcp::setLoggingPrefix(
	const char *loggingPrefix)
{
  if (loggingPrefix != NULL && loggingPrefix[0] != '\0')
    myLoggingPrefix = loggingPrefix;
  else
    myLoggingPrefix = "";
}

AREXPORT void ArNetPacketSenderTcp::setDebugLogging(bool debugLogging)
{
  myDebugLogging = debugLogging; 
  if (myDebugLogging) 
    myVerboseLogLevel = ArLog::Normal;
  else 
    myVerboseLogLevel = ArLog::Verbose; 
}

/**
   Gets the socket that the receiver is using, note that it does not
   have ownership of this socket and that whatever created it should.
**/
AREXPORT ArSocket *ArNetPacketSenderTcp::getSocket(void)
{
  return mySocket;
}

AREXPORT void ArNetPacketSenderTcp::sendPacket(ArNetPacket *packet,
					       const char *loggingString)
{
  ArNetPacket *sendPacket;
  sendPacket = new ArNetPacket(packet->getLength() + 5);
  sendPacket->duplicatePacket(packet);
  if (myDebugLogging && sendPacket->getCommand() <= 255 && 
      loggingString != NULL && loggingString[0] != '\0')
    sendPacket->setArbitraryString(loggingString);
  myDataMutex.lock();
  myPacketList.push_back(sendPacket);
  /* this shouldn't really ever be in doubt
  if (myDebugLogging && sendPacket->getCommand() <= 255 && 
      loggingString != NULL && loggingString[0] != '\0')
    ArLog::log(ArLog::Normal, "%s Sender put command %d in list", 
	       myLoggingPrefix.c_str(), 
	       loggingString, sendPacket->getCommand());
  */
  myDataMutex.unlock();
}

AREXPORT bool ArNetPacketSenderTcp::sendData(void)
{
  int ret;
  ArTime start;
  start.setToNow();
  //printf("sendData %g\n", start.mSecSince() / 1000.0);
  myDataMutex.lock();
  // if we have no data to send count it as a good send
  if (myPacketList.begin() == myPacketList.end() && myPacket == NULL)
    myLastGoodSend.setToNow();

  while (myPacketList.begin() != myPacketList.end() || myPacket != NULL)
  {
    if (myPacket == NULL)
    {
      //printf("!startedSending %g\n", start.mSecSince() / 1000.0);
      myPacket = myPacketList.front();
      myPacketList.pop_front();
      myAlreadySent = 0;
      myBuf = myPacket->getBuf();
      myLength = myPacket->getLength();
      if (myDebugLogging && myPacket->getCommand() <= 255)
	ArLog::log(ArLog::Normal, "%s %s Starting sending tcp command %d",
		   myLoggingPrefix.c_str(), 
		   myPacket->getArbitraryString(), myPacket->getCommand());
      if (myPacket->getCommand() == 0)// || myPacket->getCommand() > 1000)
      {
	ArLog::log(ArLog::Normal, "%sgetCommand is %d when it probably shouldn't be", myLoggingPrefix.c_str(), myPacket->getCommand());
      }
    }
    if (myLength < 0 || myLength > ArNetPacket::MAX_LENGTH)
    {
      ArLog::log(ArLog::Terse, "%sArNetPacketSenderTcp: getLength for command %d packet is bad at %d", 
		 myLoggingPrefix.c_str(), myPacket->getCommand(), myLength);
      delete myPacket;
      myPacket = NULL;
      continue;
    }
    if (myLength - myAlreadySent == 0)
      ArLog::log(ArLog::Normal, "%sHave no data to send... but ...",
		 myLoggingPrefix.c_str());
    ret = mySocket->write(&myBuf[myAlreadySent], myLength - myAlreadySent);
    if (ret < 0)
    {
      // we didn't send any data so make sure we've sent some recently enough
      if (myBackupTimeout >= -.0000001 && myLastGoodSend.secSince() >= 5 &&
	  myLastGoodSend.secSince() / 60.0 >= myBackupTimeout)
      {
	ArLog::log(ArLog::Normal, "%sConnection to %s backed up for %g minutes and is being closed",
		   myLoggingPrefix.c_str(), mySocket->getIPString(), 
		   myBackupTimeout);
	myDataMutex.unlock();
	return false;
      }
      
#ifdef WIN32
      if (WSAGetLastError() == WSAEWOULDBLOCK)
      {
	myDataMutex.unlock();
	if (myDebugLogging && myPacket->getCommand() <= 255)
	  ArLog::log(ArLog::Normal, 
	     "%s%sContinue sending tcp command %d, no data could be sent",
		     myLoggingPrefix.c_str(), myPacket->getArbitraryString(), 
		     myPacket->getCommand());
	return true;
      }
      else 
      {
	ArLog::log(ArLog::Normal, "%sWindows failed write with error %d on packet %d with length of %d", myLoggingPrefix.c_str(), 
		   WSAGetLastError(), myPacket->getCommand(), myLength);
	myDataMutex.unlock();
	return false;
      }

#else
      if (errno == EAGAIN)/* || errno == EINTR)*/
      {
	myDataMutex.unlock();
	if (myDebugLogging && myPacket->getCommand() <= 255)
	  ArLog::log(ArLog::Normal, 
	     "%s%sContinue sending tcp command %d, no data could be sent",
		     myLoggingPrefix.c_str(), 
		     myPacket->getArbitraryString(), myPacket->getCommand());
	return true;
      }
      else 
      {
	ArLog::log(ArLog::Normal, "%sLinux failed write with error %d on packet %d with length of %d", 
		   myLoggingPrefix.c_str(), errno, myPacket->getCommand(), 
		   myLength);
	myDataMutex.unlock();
	return false;
      }
#endif 
    }
    else if (ret == 0)
    {
      // we didn't send any data so make sure we've sent some recently enough
      if (myBackupTimeout >= -.0000001 && myLastGoodSend.secSince() >= 5 &&
	  myLastGoodSend.secSince() / 60.0 >= myBackupTimeout)
      {
	ArLog::log(ArLog::Normal, "%sConnection to %s backed up for %g minutes and is being closed",
		   myLoggingPrefix.c_str(), mySocket->getIPString(), 
		   myBackupTimeout);
	myDataMutex.unlock();
	return false;
      }
      // If network connectivity goes down for a length of time, and the
      // socket's buffer fills, we will have this error. An alternative action
      // would be to  retain myAlreadySent at its current value and keep trying to 
      // write data to the socket, in the hope that the network will come back.
      ArLog::log(myVerboseLogLevel, "%s Couldn't send packet with command %d and length %d: No data could be sent through socket (%d bytes sent until now).", 
		 myLoggingPrefix.c_str(), myPacket->getCommand(), myLength, 
		 myAlreadySent);
      myDataMutex.unlock();
      return true;
    }
    else if (ret > 0)
    {
      // we sent some data, count it as a good send
      myLastGoodSend.setToNow();
      myAlreadySent += ret;
      if (myAlreadySent == myLength)
      {
	if (myDebugLogging && myPacket->getCommand() <= 255)
	  ArLog::log(ArLog::Normal, "%s%sFinished sending tcp command %d",
		     myLoggingPrefix.c_str(), myPacket->getArbitraryString(), 
		     myPacket->getCommand());
	//printf("sent one %g\n", start.mSecSince() / 1000.0);
	delete myPacket;
	myPacket = NULL;
	continue;
      }
      else if (myDebugLogging && myPacket->getCommand() <= 255)
	ArLog::log(ArLog::Normal, 
		   "%s%sContinue sending tcp command %d, sent %d",
		   myLoggingPrefix.c_str(), myPacket->getArbitraryString(), 
		   myPacket->getCommand(), ret);

    }
    else
    {
      ArLog::log(ArLog::Terse, "%sBad case in sArNetPacketSenderTcp::sendData",
		 myLoggingPrefix.c_str());      
    }
  }
  myDataMutex.unlock();
  return true;
}
