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
#include "ArClientBase.h"
#include "ArServerCommands.h"
#include "ArClientCommands.h"
#include "md5.h"

//#define ARDEBUG_CLIENTBASE

#if (defined(_DEBUG) && defined(ARDEBUG_CLIENTBASE))
#define IFDEBUG(code) {code;}
#else
#define IFDEBUG(code)
#endif 

AREXPORT ArClientBase::ArClientBase() :
  myLogPrefix(""),
  myProcessPacketCB(this, &ArClientBase::processPacket, NULL, true),
  myProcessPacketUdpCB(this, &ArClientBase::processPacketUdp),
  myHost(""), myPort(-1)
{
  myDataMutex.setLogName("ArClientBase::myDataMutex");
  myClientMutex.setLogName("ArClientBase::myClientMutex");
  myMapsMutex.setLogName("ArClientBase::myMapsMutex");
  myStateMutex.setLogName("ArClientBase::myStateMutex");
  myCallbackMutex.setLogName("ArClientBase::myCallbackMutex");
  myCycleCallbackMutex.setLogName("ArClientBase::myCycleCallbackMutex");
  myPacketTrackingMutex.setLogName("ArClientBase::myPacketTrackingMutex");

  setRobotName("ArClientBase");
  setThreadName("ArClientBase");
  myTcpSender.setSocket(&myTcpSocket);
  myTcpReceiver.setSocket(&myTcpSocket);
  myTcpReceiver.setProcessPacketCB(&myProcessPacketCB);
  myUdpReceiver.setSocket(&myUdpSocket);
  myUdpReceiver.setProcessPacketCB(&myProcessPacketUdpCB);

  myDebugLogging = false;
  myVerboseLogLevel = ArLog::Verbose;
  myEnforceType = ArServerCommands::TYPE_UNSPECIFIED;

  myNonBlockingConnectState = NON_BLOCKING_STATE_NONE;
  clear();

  myTcpSender.setDebugLogging(myDebugLogging);
}

AREXPORT ArClientBase::~ArClientBase()
{
  clear();
}

/**
 * The robot name is used solely for log messages.
 * @param name an optional char * name of the connected robot to be used in
 * the log messages
 * @param debugLogging a bool set to true if log messages should be written
 * at the Normal level, if false, then they are written as Verbose
 * @param robotId an optional int which can be appended to the robot name
 * in the log messages; if 0, then only the robot name is logged
**/
AREXPORT void ArClientBase::setRobotName(const char *name, 
                                         bool debugLogging,
                                         int robotId)
{
  myDebugLogging = debugLogging;
  myRobotName = ((name != NULL) ? name : "");

  if (myDebugLogging)
    myVerboseLogLevel = ArLog::Normal;
  else 
    myVerboseLogLevel = ArLog::Verbose;

  myLogPrefix = "";
  if (!myRobotName.empty() && (robotId != 0)) {
    char buf[512];
    snprintf(buf, sizeof(buf),
              "%s[%i]: ", myRobotName.c_str(), robotId);
    myLogPrefix = buf;
  }
  if (!myRobotName.empty() && (robotId == 0)) {
    myLogPrefix = myRobotName + ": ";
  }
  myTcpSender.setLoggingPrefix(myLogPrefix.c_str());
  myTcpReceiver.setLoggingPrefix(myLogPrefix.c_str());
}

AREXPORT const char *ArClientBase::getRobotName() const
{
  return myRobotName.c_str();
}
  
AREXPORT const char *ArClientBase::getLogPrefix() const
{
  return myLogPrefix.c_str();
}

AREXPORT bool ArClientBase::getDebugLogging(void)
{
  return myDebugLogging;
}


void ArClientBase::clear(void)
{
  internalSwitchState(STATE_NO_CONNECTION);

  myServerReportedUdpPort = 0;
  myUdpConfirmedFrom = false;
  myUdpConfirmedTo = false;
  myTcpOnlyTo = false;
  myTcpOnlyFrom = false;
  myTimeoutTime = 10;
  myQuiet = false;
  myLastPacketReceived.setToNow();

	myIsRunningAsync = false;
  myDisconnectSoon = false;
	myIsStartedDisconnect = false;
  myIsStopped = false;

  myRejected = 0;
  myRejectedString[0] = '\0';

  myReceivedDataList = false;
  myReceivedArgRetList = false;
  myReceivedGroupAndFlagsList = false;

  //resetTracking();

  myNameIntMap.clear();
  ArUtil::deleteSetPairs(myIntDataMap.begin(), myIntDataMap.end());
  myIntDataMap.clear();
  
  /// MPL adding this since these look leaked
  ArUtil::deleteSetPairs(myTrackingSentMap.begin(), 
			 myTrackingSentMap.end());
  myTrackingSentMap.clear();

  ArUtil::deleteSetPairs(myTrackingReceivedMap.begin(), 
			 myTrackingReceivedMap.end());
  myTrackingReceivedMap.clear();

}

/**
   cycleCallbacks are called every cycle, and are in no particular
   order, since all they should do is send packets, never any
   processing

   You cannot add a cycle callback from within a cycle or it will deadlock
   @param functor callback to add

**/
AREXPORT void ArClientBase::addCycleCallback(ArFunctor *functor)
{
  myCycleCallbackMutex.lock();
  myCycleCallbacks.push_front(functor);
  myCycleCallbackMutex.unlock();
}

/**
   cycleCallbacks are called every cycle, and are in no particular
   order, since all they should do is send packets, never any
   processing
   You cannot remove a cycle callback from within a cycle or it will deadlock
   @param functor callback to remove
**/
AREXPORT void ArClientBase::remCycleCallback(ArFunctor *functor)
{
  myCycleCallbackMutex.lock();
  myCycleCallbacks.remove(functor);
  myCycleCallbackMutex.unlock();
}


/**
   @param sec This value controls how long the client will give to
   connect, it is the argument in number of seconds... if <= 0 then no
   timeout will take place.
**/
AREXPORT void ArClientBase::setConnectTimeoutTime(int sec)
{
  myDataMutex.lock();
  if (sec < 0)
    myTimeoutTime = 0;
  else
    myTimeoutTime = sec;
  myDataMutex.unlock();
}

/**
   @return This value controls how long the client will give to
   connect, it is the argument in number of seconds... if <= 0 then no
   timeout will take place.
**/
AREXPORT int ArClientBase::getConnectTimeoutTime(void)
{
  return myTimeoutTime;
}

/**
   @param host the host to connect to

   @param port the port to connect on
   
   @param log whether to log our connection information using ArLog or not.
   Keeping the default value of true is recommended (disabling logging
   is usually only for internal and unusual uses).
   This setting also applies to logging disconnections and some other I/O events
   as well.

   @param user  user name, or NULL for none.
   @param password password (cleartext), or NULL for none.
   @param openOnIP address of the local network interface to use for the outgoing connection, or NULL for any/all.
 **/
AREXPORT bool ArClientBase::blockingConnect(const char *host, int port, 
					    bool log, const char *user,
					    const char *password,
					    const char *openOnIP)
{
  return internalBlockingConnect(host, port, log, user, password, NULL, 
				 openOnIP);
}

AREXPORT bool ArClientBase::internalBlockingConnect(
	const char *host, int port, bool log, const char *user,
	const char *password, ArSocket *tcpSocket, const char *openOnIP)
{
  NonBlockingConnectReturn ret;

  if ((ret = internalNonBlockingConnectStart(
	       host, port, log, user, password, tcpSocket, openOnIP)) != 
      NON_BLOCKING_CONTINUE)
    return false;
  
  while ((ret = internalNonBlockingConnectContinue()) == NON_BLOCKING_CONTINUE)
    ArUtil::sleep(1);
  
  if (ret == NON_BLOCKING_CONNECTED)
    return true;
  else
    return false;
}

AREXPORT ArClientBase::NonBlockingConnectReturn 
ArClientBase::internalNonBlockingConnectStart(
	const char *host, int port, bool log, const char *user,
	const char *password, ArSocket *tcpSocket, const char *openOnIP)
{
  ArNetPacket packet;
  
  myDataMutex.lock();
  if (myState == STATE_LOST_CONNECTION)
  {
    ArLog::log(ArLog::Normal, 
	       "%sTrying a connection on a client that'd lost connection.", 
	       myLogPrefix.c_str());
    clear();
  }
  
  myDataMutex.unlock();
  
  if (myState != STATE_NO_CONNECTION && 
      myState != STATE_FAILED_CONNECTION && 
      myState != STATE_REJECTED)
  {
    if (!myQuiet || myDebugLogging)
      ArLog::log(ArLog::Terse, 
		 "%sConnection already established or being connected.", 
		 myLogPrefix.c_str());
    return NON_BLOCKING_FAILED;
  }
  
  if (myDebugLogging)
    ArLog::log(ArLog::Normal, "%s0", myLogPrefix.c_str());
  myDataMutex.lock();
  // MPL added this here since the no connection state is gotten to by
  // a normal disconnect, and won't have had its data cleared
  clear();
  myStartedConnection.setToNow();
  
  myHost = host;
  myPort = port;
  myQuiet = !log;
  myTcpReceiver.setQuiet(myQuiet);
  if (user != NULL)
    myUser = user;
  else
    myUser = "";
  if (password != NULL)
    myPassword = password;
  else
    myPassword = "";
  
  myDataMutex.unlock();
  if (tcpSocket != NULL)
  {
    myDataMutex.lock();
    myTcpSocket.transfer(tcpSocket);
    internalStartUdp();
    myDataMutex.unlock();
    internalSwitchState(STATE_OPENED_SOCKET);
    
  }
  else
  {
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s1", myLogPrefix.c_str());
    loopOnce();
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s1.1", myLogPrefix.c_str());
    myDataMutex.lock();
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s1.2", myLogPrefix.c_str());
    if (internalConnect(host, port, log, openOnIP))
    {
      myDataMutex.unlock();
      if (myDebugLogging)
	ArLog::log(ArLog::Normal, "%s2", myLogPrefix.c_str());
      //break;
    }
    else
    {
      myTcpSocket.close();
      myUdpSocket.close();
      internalSwitchState(STATE_FAILED_CONNECTION);
      myDataMutex.unlock();
      if (myDebugLogging)
	ArLog::log(ArLog::Normal, "%s3", myLogPrefix.c_str());
      
      return NON_BLOCKING_FAILED;
    }
    //}
  }
  myNonBlockingConnectState = NON_BLOCKING_STATE_TCP;
  return NON_BLOCKING_CONTINUE;
}


AREXPORT ArClientBase::NonBlockingConnectReturn 
ArClientBase::internalNonBlockingConnectContinue(void)
{
  ArNetPacket packet;

  if (myNonBlockingConnectState == NON_BLOCKING_STATE_TCP)
  {
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s5", myLogPrefix.c_str());
    loopOnce();

    if (myState == STATE_REJECTED || myState == STATE_NO_CONNECTION || 
	myState == STATE_LOST_CONNECTION)
    {
      myTcpSocket.close();
      myUdpSocket.close();
      myNonBlockingConnectState = NON_BLOCKING_STATE_NONE;
      return NON_BLOCKING_FAILED;
    }
    if (myState == STATE_CONNECTED)
    {
      myStartedUdpConnection.setToNow();
      myNonBlockingConnectState = NON_BLOCKING_STATE_UDP;
      return internalNonBlockingConnectContinue();
    }

    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s6 (%d)", myLogPrefix.c_str(), myState);
    if (myTimeoutTime > 0 && 
	myStartedConnection.mSecSince() > myTimeoutTime * 1000.0)
    {
      if (myDebugLogging)
	ArLog::log(ArLog::Normal, "%s7", myLogPrefix.c_str());
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Terse, 
		   "%sConnection timed out.", myLogPrefix.c_str());
      internalSwitchState(STATE_FAILED_CONNECTION);
      //myState = STATE_NO_CONNECTION;
      myTcpSocket.close();
      myUdpSocket.close();
      myNonBlockingConnectState = NON_BLOCKING_STATE_NONE;
      return NON_BLOCKING_FAILED;
    }
    return NON_BLOCKING_CONTINUE;
  }
  else if (myNonBlockingConnectState == NON_BLOCKING_STATE_UDP)
  {
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s8", myLogPrefix.c_str());
    loopOnce();

    if (myState != STATE_CONNECTED)
    {
      if (myDebugLogging)
	ArLog::log(ArLog::Normal, "%s9", myLogPrefix.c_str());
      myTcpSocket.close();
      myUdpSocket.close();
      myNonBlockingConnectState = NON_BLOCKING_STATE_NONE;
      return NON_BLOCKING_FAILED;
    }

    if ((myUdpConfirmedFrom || myTcpOnlyFrom) && 
	(myUdpConfirmedTo || myTcpOnlyTo))
    {
      if (myDebugLogging)
	ArLog::log(ArLog::Normal, "%s10", myLogPrefix.c_str());
      myNonBlockingConnectState = NON_BLOCKING_STATE_NONE;
      return NON_BLOCKING_CONNECTED;
    }

    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%s11", myLogPrefix.c_str());
    // see if we didn't get udp from
    if (myStartedUdpConnection.mSecSince() > myTimeoutTime * .66 * 1000.0 &&
	!myUdpConfirmedFrom && !myTcpOnlyFrom)
    {
      packet.empty();
      packet.setCommand(ArClientCommands::TCP_ONLY);
      sendPacketTcp(&packet);
      myDataMutex.lock();
      myTcpOnlyFrom = true;
      myDataMutex.unlock();
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Normal, 
		   "%sDidn't get confirmation of UDP from server, switching to TCP.", 
		   myLogPrefix.c_str());
    }
    // see if we haven't confirmed udp to
    if (myStartedUdpConnection.mSecSince() > myTimeoutTime * .66 * 1000.0 &&
	!myUdpConfirmedTo && !myTcpOnlyTo)
    {
      packet.empty();
      myDataMutex.lock();
      myTcpOnlyTo = true;
      myDataMutex.unlock();
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Normal, 
		   "%sDidn't get confirmation of UDP to server, switching to TCP.", 
		   myLogPrefix.c_str());
    }
    if (myTimeoutTime > 0 && 
	myStartedUdpConnection.mSecSince() > myTimeoutTime * 1000.0)
    {
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Terse, "%sConnection timed out (2).", 
		   myLogPrefix.c_str());
      ArLog::log(
	      myVerboseLogLevel, "%sTimeout %d startedMSecSince %d startedUdpWaitMSecSince %d udpFrom %d udpTo %d tcpOnlyFrom %d tcpOnlyTo %d", 
	      myLogPrefix.c_str(),
	      myTimeoutTime, myStartedConnection.mSecSince(), 
	      myStartedUdpConnection.mSecSince(), myUdpConfirmedFrom, 
	      myUdpConfirmedTo, myTcpOnlyFrom, myTcpOnlyTo);
      //myState = STATE_NO_CONNECTION;
      internalSwitchState(STATE_FAILED_CONNECTION);
    }
    return NON_BLOCKING_CONTINUE;
  }
  else if (myNonBlockingConnectState == NON_BLOCKING_STATE_NONE)
  {
    ArLog::log(ArLog::Normal, "Non blocking connect called when it wasn't started");
    return NON_BLOCKING_FAILED;
  }
  else
  {
    ArLog::log(ArLog::Normal, "Non blocking connect called when in unknown state");
    return NON_BLOCKING_FAILED;
  }
}

bool ArClientBase::internalConnect(const char *host, int port, bool obsolete,
				   const char *openOnIP)
{
  if (myDebugLogging)
    ArLog::log(ArLog::Normal, "%sa", myLogPrefix.c_str());
  // if we connect tcp then connect up the udp
  if (myTcpSocket.connect(host, port, ArSocket::TCP, openOnIP))
  {
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%sb", myLogPrefix.c_str());
    if (!myTcpSocket.setNonBlock())
    {
      ArLog::log(ArLog::Terse,
		 "%sFailed to set TCP socket nonblock, could not connect",
		 myLogPrefix.c_str());
      return false;
    }
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%sc", myLogPrefix.c_str());
    internalStartUdp();
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%sd", myLogPrefix.c_str());
    if (!myQuiet || myDebugLogging)
      ArLog::log(myVerboseLogLevel, 
 "%sOpened client connection to %s on tcp port %d with my udp port of %d",
		myLogPrefix.c_str(), host, port, myUdpPort);
    // we can't tell if udp connected or not (because its udp) so just
    // say we're good
    internalSwitchState(STATE_OPENED_SOCKET);
    if (myDebugLogging)
      ArLog::log(ArLog::Normal, "%se", myLogPrefix.c_str());
    return true;
  }
  else
  {
    if (!myQuiet || myDebugLogging)
      ArLog::log(ArLog::Terse,
		 "%sconnect: Failed to open TCP socket", 
		 myLogPrefix.c_str());
    internalSwitchState(STATE_FAILED_CONNECTION);
    return false;
  }
}

void ArClientBase::internalStartUdp(void)
{
  // fire up the udp
  if (!myUdpSocket.create(ArSocket::UDP) ||
      !myUdpSocket.findValidPort(10000) ||
      !myUdpSocket.setNonBlock())
  {
    myUdpSocket.setLinger(0);
    if (!myQuiet || myDebugLogging)
      ArLog::log(ArLog::Terse, 
		 "%sconnect: Failed to open udp socket, forcing TCP",
		 myLogPrefix.c_str());
    //internalSwitchState(STATE_FAILED_CONNECTION);
    //return false;		 
    myTcpOnlyTo = true;
    myTcpOnlyFrom = true;
  }
  myUdpSinValid = false;
  myUdpSin.sin_family=AF_INET;
  myUdpSin.sin_addr = *myTcpSocket.inAddr();
  myUdpPort = myUdpSocket.netToHostOrder(myUdpSocket.inPort());
}

AREXPORT void ArClientBase::disconnectSoon(void)
{
  myDisconnectSoon = true;
}

AREXPORT bool ArClientBase::disconnect(void)
{
  bool ret = startNonBlockingDisconnect();

  ArUtil::sleep(10);

  finishNonBlockingDisconnect();

  return ret;
}

AREXPORT bool ArClientBase::startNonBlockingDisconnect()
{
  if (!myQuiet || myDebugLogging)
    ArLog::log(ArLog::Normal, "%sDisconnected from server.", 
	       myLogPrefix.c_str()); 

  bool ret = false;
  ArNetPacket packet;

  packet.setCommand(ArClientCommands::SHUTDOWN);
  ret = sendPacketTcp(&packet);
  myTcpSender.sendData();

  return ret;
}

AREXPORT void ArClientBase::finishNonBlockingDisconnect()
{
  myTcpSocket.close();
  myUdpSocket.close();
  myState = STATE_NO_CONNECTION;
}



AREXPORT  bool ArClientBase::setupPacket(ArNetPacket *packet)
{
  if (myState == STATE_NO_CONNECTION || myState == STATE_FAILED_CONNECTION || 
      myState == STATE_LOST_CONNECTION || myState == STATE_REJECTED)
    return false;

  if (packet->getLength() > ArNetPacket::MAX_LENGTH)
  {
    ArLog::log(ArLog::Terse, 
	       "%s: setupPacket packet for command %s packet is bad at %d", 
	       myLogPrefix.c_str(), getName(packet->getCommand()), 
	       packet->getLength());
    return false;
  }

  packet->finalizePacket();
  return true;
} 

AREXPORT bool ArClientBase::sendPacketTcp(ArNetPacket *packet)
{
  if (!setupPacket(packet))
  {
    return false;
  }
  else
  {
    trackPacketSent(packet, true);
    myTcpSender.sendPacket(packet, myLogPrefix.c_str());
    return true;
  }
}

AREXPORT bool ArClientBase::sendPacketUdp(ArNetPacket *packet)
{
  if (!setupPacket(packet))
  {
    return false;
  }
  else
  {
    trackPacketSent(packet, false);
    if (myUdpSocket.sendTo(packet->getBuf(), packet->getLength(), &myUdpSin) 
	== packet->getLength())
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

AREXPORT void ArClientBase::loopOnce(void)
{
  std::list<ArFunctor *>::iterator it;
  if (myState != STATE_NO_CONNECTION && myState != STATE_FAILED_CONNECTION &&
      myState != STATE_LOST_CONNECTION && myState != STATE_REJECTED)
  {
    if (myDisconnectSoon)
    {
			if (myIsRunningAsync) {
				startNonBlockingDisconnect();
	      
				ArUtil::sleep(100);
	      
				finishNonBlockingDisconnect();
			}
			else {
				if (!myIsStartedDisconnect) {

					myIsStartedDisconnect = true;
					myStartedShutdown.setToNow();
					startNonBlockingDisconnect();
				}
				else if (myStartedShutdown.mSecSince() > 100) {
					finishNonBlockingDisconnect();
				}
			}
      return;
    }
    // if we couldn't receive tcp data fail
    if (!myTcpReceiver.readData())
    {
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Normal, 
		   "%sLost connection to server (couldn't recv).", 
		   myLogPrefix.c_str());
      myTcpSocket.close();
      myUdpSocket.close();
      internalSwitchState(STATE_LOST_CONNECTION);
      stopRunning();
      myCallbackMutex.lock();
      for (it = myDisconnectOnErrorCBList.begin(); 
	   it != myDisconnectOnErrorCBList.end(); 
	   ++it)
	(*it)->invoke();
      myCallbackMutex.unlock();
  
    }
    // if we can't receive UDP just make an error log
    else if (!myTcpOnlyFrom && !myUdpReceiver.readData())
    {
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Normal, "%sUDP Troubles, continuing", 
		   myLogPrefix.c_str());
    }
    if (!myTcpSender.sendData())
    {
      if (!myQuiet || myDebugLogging)
	ArLog::log(ArLog::Normal, 
		   "%sLost connection to server (couldn't send).", 
		   myLogPrefix.c_str());
      myTcpSocket.close();
      myUdpSocket.close();
      internalSwitchState(STATE_LOST_CONNECTION);
      stopRunning();
      myCallbackMutex.lock();
      for (it = myDisconnectOnErrorCBList.begin(); 
	   it != myDisconnectOnErrorCBList.end(); 
	   ++it)
	(*it)->invoke();
      myCallbackMutex.unlock();
    }
  }
  myCycleCallbackMutex.lock();
  for (it = myCycleCallbacks.begin(); it != myCycleCallbacks.end(); ++it)
    (*it)->invoke();
  myCycleCallbackMutex.unlock();

}

AREXPORT void ArClientBase::run(void)
{
  runInThisThread();
}

AREXPORT void ArClientBase::runAsync(void)
{
	myIsRunningAsync = true;
  create(true, false);
}

AREXPORT void *ArClientBase::runThread(void *arg)
{
  threadStarted();
  while (myRunning)
  {
    loopOnce();
    ArUtil::sleep(1);
  }
  threadFinished();
  return NULL;
}

AREXPORT void ArClientBase::stopRunning(void)
{
  ArASyncTask::stopRunning();
  myIsStopped = true;
}

AREXPORT bool ArClientBase::isStopped()
{
  return myIsStopped;
}


AREXPORT void ArClientBase::processPacket(ArNetPacket *packet, bool tcp)
{
  ArNetPacket retPacket;
  std::list<ArFunctor *>::iterator it;

  myDataMutex.lock();
  myLastPacketReceived.setToNow();
  myDataMutex.unlock();

  if (myDebugLogging && packet->getCommand() <= 255)
  {
    ArLog::log(ArLog::Normal, "%sGot command %d", 
	       myLogPrefix.c_str(), packet->getCommand());
  }

  // See if we should close our connection
  if (packet->getCommand() == ArServerCommands::SHUTDOWN)
  {
    if (!myQuiet || myDebugLogging)
      ArLog::log(ArLog::Normal, 
								 "%sServer shutting down and closed connection.", 
								 myLogPrefix.c_str());
    stopRunning();
    disconnect();
    internalSwitchState(STATE_LOST_CONNECTION);

    myCallbackMutex.lock();
    for (it = myServerShutdownCBList.begin(); 
				 it != myServerShutdownCBList.end(); 
				 ++it)
      (*it)->invoke();
    myCallbackMutex.unlock();
  }
	else if (myState == STATE_CONNECTED) {

		// Some of the following cases are duplicated in the remainder of the 
		// if-else-if stmt below. (These are mostly error conditions.)  This 
		// is a result of moving the connected state to the top of the big long 
		// if-else-if stmt since it is the most common scenario.  Should probably
		// re-examine the redundancies and improve their handling someday....
		switch (packet->getCommand()) {
		case ArServerCommands::INTRODUCTION:
		{
			if (!myQuiet || myDebugLogging)
				ArLog::log(ArLog::Terse, 
									"%sIntroduction received when not in STATE_OPENED_SOCKET", 
									myLogPrefix.c_str());
			return;
		}
		case ArServerCommands::CONNECTED:
		{
			ArLog::log(ArLog::Terse, 
								"%sConnected packet received not during STATE_EXCHANGED_INTROS",
								myLogPrefix.c_str());
			return;
		}
		case ArServerCommands::REJECTED:
		{
			ArLog::log(ArLog::Terse, 
								"%sRejected packet received after connected", 
								myLogPrefix.c_str());
			return;
		}

		case ArServerCommands::LIST:
		case ArServerCommands::LISTSINGLE:
		case ArServerCommands::LISTARGRET:
		case ArServerCommands::LISTARGRETSINGLE:
		case ArServerCommands::LISTGROUPANDFLAGS:
		case ArServerCommands::LISTGROUPANDFLAGSSINGLE:
		{
			myMapsMutex.lock();
			buildList(packet);
			myMapsMutex.unlock();
			return;
		}
		case ArServerCommands::UDP_CONFIRMATION:
		{
			if (!myQuiet || myDebugLogging)
				ArLog::log(myVerboseLogLevel, 
									"%sClients udp connection to server confirmed.", 
									myLogPrefix.c_str());
			myDataMutex.lock();
			myUdpConfirmedTo = true;
			myDataMutex.unlock();
			return;
		}
		case ArServerCommands::UDP_INTRODUCTION:
		{
		  ArLog::log(ArLog::Terse,
			     "%sClient got udp introduction over tcp...", 
			     myLogPrefix.c_str());
		  return;
		}
		case ArServerCommands::TCP_ONLY:
		{
			if (!myQuiet || myDebugLogging)
				ArLog::log(ArLog::Normal, "%sClient told to only use tcp.", 
									myLogPrefix.c_str());
			myDataMutex.lock();
			myTcpOnlyTo = true;
			myDataMutex.unlock();
			return;
		}
		default:
		{
			std::map<unsigned int, ArClientData *>::iterator it;
			ArClientData *clientData = NULL;
			// see if we have the command
			myMapsMutex.lock();
			if ((it = myIntDataMap.find(packet->getCommand())) != myIntDataMap.end()) {
				clientData = (*it).second;
				myMapsMutex.unlock();
			}
			else { // command not found

				myMapsMutex.unlock();
				ArLog::log(ArLog::Terse, 
									"%sGot packet for %d which doesn't exist, packet was %d long", 
									myLogPrefix.c_str(), packet->getCommand(), packet->getLength());
				packet->log();

				ArLog::log(ArLog::Terse, "%sPacket: ", myLogPrefix.c_str());
				for (int ijk = 0; ijk < packet->getLength(); ijk++)
					ArLog::log(ArLog::Terse, " %d ", 
										(unsigned char) packet->getBuf()[ijk]);
			}

			if (clientData == NULL)
			{
				ArLog::log(ArLog::Terse,
									"%sNull client data for command %d", 
									myLogPrefix.c_str(), packet->getCommand());    
				return;
			}
	    
			clientData->lockFunctorList();
			if (clientData->getFunctorList()->begin() == 
							clientData->getFunctorList()->end())
			{
				ArLog::log(myVerboseLogLevel, 
									"%sNo functor to handle command %d", 
									myLogPrefix.c_str(), packet->getCommand());
				clientData->unlockFunctorList();
				return;
			}
			else
			{
				std::list<ArFunctor1<ArNetPacket *> *>::const_iterator it;
				ArFunctor1<ArNetPacket *> *functor = NULL;

				// PS (with KMC help) - make a copy so that we can remove from the list
				std::list<ArFunctor1<ArNetPacket *> *> copiedList = *(clientData->getFunctorList());
				
				for (it = copiedList.begin(); 
						it != copiedList.end();
						it++)
				//for (it = clientData->getFunctorList()->begin(); 
				//		it != clientData->getFunctorList()->end();
				//		it++)
				{
					packet->resetRead();
					functor = (*it);
					functor->invoke(packet);
				}
				clientData->unlockFunctorList();
				trackPacketReceived(packet, tcp);
		
			}
	    
		} // end default
		return;

		} // end switch command
	}
  /// see if we've opened the socket and this is an intro
  else if (myState == STATE_OPENED_SOCKET && 
           packet->getCommand() == ArServerCommands::INTRODUCTION)
  {

    char buf[512];
    char passwordKey[2048];
    char enforceProtocolVersion[256];

    myDataMutex.lock();
    // first read the data from the packet
    packet->bufToStr(buf, 512);
    myServerReportedUdpPort = packet->bufToUByte2();
    
    if (!myQuiet || myDebugLogging)
    {
      if (myServerReportedUdpPort == 0)
				ArLog::log(myVerboseLogLevel, "%sConnection to version %s",
									 myLogPrefix.c_str(),
									 buf);
      else
				ArLog::log(myVerboseLogLevel, 
									 "%sConnection to version %s with udp port %u", 
									 myLogPrefix.c_str(),
									 buf, myServerReportedUdpPort);
    }
    if (myServerReportedUdpPort == 0)
    {
      myTcpOnlyTo = true;
      myTcpOnlyFrom = true;
    }
    // set our expected port (its okay if its otherwise)
    myUdpSin.sin_port = ArSocket::hostToNetOrder(myServerReportedUdpPort);
    // get the keys
    myAuthKey = packet->bufToUByte4();
    myIntroKey = packet->bufToUByte4();
    packet->bufToStr(passwordKey, sizeof(passwordKey));
    packet->bufToStr(enforceProtocolVersion, sizeof(enforceProtocolVersion));
    if (!myEnforceProtocolVersion.empty() && 
	strcasecmp(enforceProtocolVersion, 
		   myEnforceProtocolVersion.c_str()) != 0)
    {
      myRejected = 3;
      strcpy(myRejectedString, "Client rejecting server, since server protocol version does not match");
      ArLog::log(ArLog::Normal, 
		 "%sClient rejecting server, since server protocol version does not match",
		 myLogPrefix.c_str());
      internalSwitchState(STATE_REJECTED);
      return;
    }

    // introduce ourself normally via tcp
    retPacket.empty();
    retPacket.setCommand(ArClientCommands::INTRODUCTION);
    retPacket.uByte2ToBuf(myUdpPort);
    retPacket.strToBuf(myUser.c_str());
    // old simple cleartext way
    //retPacket.strToBuf(myPassword.c_str());
    md5_state_t md5State;
    unsigned char md5Digest[16];
    
    md5_init(&md5State);
    md5_append(&md5State, (unsigned char *)myServerKey.c_str(), 
							 myServerKey.size());
    md5_append(&md5State, (unsigned char *)passwordKey, strlen(passwordKey));
    md5_append(&md5State, (unsigned char *)myPassword.c_str(), 
					     myPassword.size());
    md5_finish(&md5State, md5Digest);

    retPacket.dataToBuf((const char *)md5Digest, 16);
    retPacket.strToBuf(myEnforceProtocolVersion.c_str());
    retPacket.byteToBuf(myEnforceType);

    sendPacketTcp(&retPacket);

    myPassword = "%s";
    bool tcpOnlyFrom = myTcpOnlyFrom;
    myDataMutex.unlock();
    if (tcpOnlyFrom)
    {
      retPacket.empty();
      retPacket.setCommand(ArClientCommands::TCP_ONLY);
      sendPacketTcp(&retPacket);
    }
    internalSwitchState(STATE_EXCHANGED_INTROS);
  }
  // if we're not at opened socket and got an intro somethings horribly wrong
  else if (myState != STATE_OPENED_SOCKET && 
					 packet->getCommand() == ArServerCommands::INTRODUCTION)
  {
    if (!myQuiet || myDebugLogging)
			ArLog::log(ArLog::Terse, 
								 "%sIntroduction received when not in STATE_OPENED_SOCKET", 
								 myLogPrefix.c_str());
    return;
  }
  // if we're at opened socket and received something other than intro
  // then somethings horribly wrong
  else if (myState == STATE_OPENED_SOCKET && 
					 packet->getCommand() != ArServerCommands::INTRODUCTION)
  {
    if (!myQuiet || myDebugLogging)
			ArLog::log(ArLog::Terse, 
								 "%sPacket other than introduction received when in STATE_OPENED_SOCKET", 
								 myLogPrefix.c_str());
    return;
  }
  // if we've exchanged intros and are waiting for connection or rejection
  else if (myState == STATE_EXCHANGED_INTROS && 
					 packet->getCommand() == ArServerCommands::CONNECTED)
  {
    if (!myQuiet || myDebugLogging)
    {
      if (myTcpOnlyTo && myTcpOnlyFrom)
				ArLog::log(ArLog::Terse, 
									 "%sClient now connected to server (tcp only).", 
									 myLogPrefix.c_str());
      else
				ArLog::log(ArLog::Terse, 
									 "%sClient now connected to server.", myLogPrefix.c_str());
    }
    internalSwitchState(STATE_WAITING_LIST);
    // introduce ourself via udp
    retPacket.empty();
    retPacket.setCommand(ArClientCommands::UDP_INTRODUCTION);
    myDataMutex.lock();
    retPacket.uByte4ToBuf(myAuthKey);
    myDataMutex.unlock();
    sendPacketUdp(&retPacket);
    return;
  }
  // if we've received a connected and aren't at exchanging intros
  else if (packet->getCommand() == ArServerCommands::CONNECTED)
  {
    ArLog::log(ArLog::Terse, 
							 "%sConnected packet received not during STATE_EXCHANGED_INTROS",
							 myLogPrefix.c_str());
    return;
  }
  // if we're exchanging intros and were rejected
  else if (myState != STATE_CONNECTED &&
					 packet->getCommand() == ArServerCommands::REJECTED)
  {
    myRejected = packet->bufToByte2();
    packet->bufToStr(myRejectedString, sizeof(myRejectedString));
    if (!myQuiet || myDebugLogging)
    {
      if (myRejected == 1)
				ArLog::log(ArLog::Normal, 
									 "%sServer rejected connection because of bad user/password", 
									 myLogPrefix.c_str());
      else if (myRejected == 2)
				ArLog::log(ArLog::Normal, 
									 "%sServer rejected connection since it is using the centralserver at %s", 
									 myLogPrefix.c_str(), myRejectedString);
      else if (myRejected == 3)
				ArLog::log(ArLog::Normal, 
									 "%sServer rejected for bad reason (3 should be from client side... it reported '%s')", 
									 myLogPrefix.c_str(), myRejectedString);
      else if (myRejected == 4)
				ArLog::log(ArLog::Normal, 
									 "%sServer rejected client because client using wrong protocol version.", 
									 myLogPrefix.c_str());
      
      else
				ArLog::log(ArLog::Normal, 
									 "%sServer rejected connection for unknown reason %d '%s'", 
									 myLogPrefix.c_str(), myRejected, myRejectedString);
    }
      
    internalSwitchState(STATE_REJECTED);
    return;
  }
  // if we received rejected and aren't at exchanging intros
  else if (packet->getCommand() == ArServerCommands::REJECTED)
  {
    ArLog::log(ArLog::Terse, 
							 "%sRejected packet received after connected", 
							 myLogPrefix.c_str());
    return;
  }
  // if we're waiting for our list and get it we're connected
  else if (myState == STATE_WAITING_LIST &&
					 packet->getCommand() == ArServerCommands::LIST)
  {
    internalSwitchState(STATE_CONNECTED);
    myDataMutex.lock();
    md5_state_t md5State;
    
    md5_init(&md5State);
    md5_append(&md5State, (unsigned char *)&myAuthKey, sizeof(myAuthKey));
    md5_append(&md5State, (unsigned char *)&myIntroKey, sizeof(myIntroKey));
    long rand = ArMath::random();
    md5_append(&md5State, (unsigned char *)&rand, sizeof(rand));
    unsigned int time = ArUtil::getTime();
    md5_append(&md5State, (unsigned char *)&time, sizeof(time));

    md5_finish(&md5State, myClientKey);
    myDataMutex.unlock();
    myMapsMutex.lock();
    buildList(packet);
    myMapsMutex.unlock();
    return;
  }
  // see if we got any lists when not waiting for them
  else if (packet->getCommand() == ArServerCommands::LIST ||
					 packet->getCommand() == ArServerCommands::LISTSINGLE || 
					 packet->getCommand() == ArServerCommands::LISTARGRET ||
					 packet->getCommand() == ArServerCommands::LISTARGRETSINGLE ||
					 packet->getCommand() == ArServerCommands::LISTGROUPANDFLAGS ||
					 packet->getCommand() == ArServerCommands::LISTGROUPANDFLAGSSINGLE)
  {
    myMapsMutex.lock();
    buildList(packet);
    myMapsMutex.unlock();
    return;
  }
  // if we're in exhanged intros and receive a packet not for that state
  else if (myState == STATE_EXCHANGED_INTROS && 
					 packet->getCommand() != ArServerCommands::CONNECTED && 
					 packet->getCommand() != ArServerCommands::REJECTED)
  {
    ArLog::log(ArLog::Terse, 
							 "%sIn STATE_EXCHANGE_INTROS and received something other than connected or rejected (%d).", myLogPrefix.c_str(), packet->getCommand());
    return;
  }

	/*** KMC Moved into switch statement above
  // if we're connected and got a udp confirmed packet then everything is good
  else if (myState == STATE_CONNECTED && 
					 packet->getCommand() == ArServerCommands::UDP_CONFIRMATION)
  {
    if (!myQuiet || myDebugLogging)
      ArLog::log(myVerboseLogLevel, 
							   "%sClients udp connection to server confirmed.", 
								 myLogPrefix.c_str());
    myDataMutex.lock();
    myUdpConfirmedTo = true;
    myDataMutex.unlock();
    return;
  }
	***/

  // if we're not connected and got a udp confirmation it means
  // something is wrong
  else if (packet->getCommand() == ArServerCommands::UDP_CONFIRMATION)
  {
    ArLog::log(ArLog::Terse, 
							 "ArClientBase: Udp Confirmation received when not in STATE_CONNECTED");
    return;
  }
  else if (packet->getCommand() == ArServerCommands::UDP_INTRODUCTION)
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientBase: Udp Introduction received over tcp... when not in STATE_CONNECTED");
    return;
  }
	/*** KMC Moved into switch statement above
  // if we're connected and got a tcp only packet then everything is good
  else if (myState == STATE_CONNECTED && 
					 packet->getCommand() == ArServerCommands::TCP_ONLY)
  {
    if (!myQuiet || myDebugLogging)
      ArLog::log(ArLog::Normal, "%sClient told to only use tcp.", 
								 myLogPrefix.c_str());
    myDataMutex.lock();
    myTcpOnlyTo = true;
    myDataMutex.unlock();
    return;
  }
	**/

	/*** KMC Moved into switch statement above
  // if none of the above were triggered its just a normal packet
  else if (myState == STATE_CONNECTED)
  {
    std::map<unsigned int, ArClientData *>::iterator it;
    ArClientData *clientData = NULL;
    // see if we have the command
    myMapsMutex.lock();
		if ((it = myIntDataMap.find(packet->getCommand())) != myIntDataMap.end()) {
      clientData = (*it).second;
      myMapsMutex.unlock();
		}
		else { // command not found

      myMapsMutex.unlock();
      ArLog::log(ArLog::Terse, 
								 "%sGot packet for %d which doesn't exist, packet was %d long", 
								 myLogPrefix.c_str(), packet->getCommand(), packet->getLength());
      packet->log();

			ArLog::log(ArLog::Terse, "%sPacket: ", myLogPrefix.c_str());
      for (int ijk = 0; ijk < packet->getLength(); ijk++)
				ArLog::log(ArLog::Terse, " %d ", 
									 (unsigned char) packet->getBuf()[ijk]);
    }

    if (clientData == NULL)
    {
      ArLog::log(ArLog::Terse,
								 "%sNull client data for command %d", 
								 myLogPrefix.c_str(), packet->getCommand());    
      return;
    }
    
    clientData->lockFunctorList();
    if (clientData->getFunctorList()->begin() == 
						clientData->getFunctorList()->end())
    {
      ArLog::log(myVerboseLogLevel, 
								 "%sNo functor to handle command %d", 
								 myLogPrefix.c_str(), packet->getCommand());
      clientData->unlockFunctorList();
      return;
    }
    else
    {
      std::list<ArFunctor1<ArNetPacket *> *>::const_iterator it;
      ArFunctor1<ArNetPacket *> *functor = NULL;

      for (it = clientData->getFunctorList()->begin(); 
					 it != clientData->getFunctorList()->end();
					 it++)
      {
				packet->resetRead();
				functor = (*it);
				functor->invoke(packet);
      }
      clientData->unlockFunctorList();
      trackPacketReceived(packet, tcp);
	
    }
  }
	****/

  else
  {
	  //packet->bufToStr(&str);
    //packet->printHex();
    ArLog::log(myVerboseLogLevel, 
	       "%sBogus packet of %u command %d long (probably after close)", 
	       myLogPrefix.c_str(), packet->getCommand(),  
	       packet->getLength());
  }
  //sendPacketTcp(packet);
  
}

void ArClientBase::buildList(ArNetPacket *packet)
{
  ArClientData *clientData;
  unsigned int listLen;
  unsigned int i;
  unsigned int command;
  char name[512];
  char description[512];
  char argDesc[1024];
  char retDesc[1024];
  char commandGroup[512];
  char dataFlags[512];

  // if its a single list snag it and run
  if (packet->getCommand() == ArServerCommands::LISTSINGLE)
  {
    command = packet->bufToUByte2();
    packet->bufToStr(name, sizeof(name));
    packet->bufToStr(description, sizeof(description));
    clientData = new ArClientData(name, description, command, NULL);
     ArLog::log(myVerboseLogLevel, 
	       "%sNew entry number %d for data %s with description %s", 
	       myLogPrefix.c_str(), command, name, description);
    if (myIntDataMap.find(command) != myIntDataMap.end())
      ArLog::log(ArLog::Normal, 
	     "%sIs already an entry for number %d as data %d\n, overwriting", 
		 myLogPrefix.c_str(), command, 
		 myIntDataMap[command]->getName());
    myNameIntMap[name] = command;
    myIntDataMap[command] = clientData;
    return;
  }
  else if (packet->getCommand() == ArServerCommands::LIST)
  {
    myReceivedDataList = true;
    listLen = packet->bufToUByte2();
    // otherwise loop and read them all
    for (i = 0; i < listLen; i++)
    {
      command = packet->bufToUByte2();
      packet->bufToStr(name, sizeof(name));
      packet->bufToStr(description, sizeof(description));
      clientData = new ArClientData(name, description, command, NULL);
      ArLog::log(myVerboseLogLevel, 
       "%sNew entry number %d for data %s with description %s", 
		 myLogPrefix.c_str(), command, name, description);
      if (myIntDataMap.find(command) != myIntDataMap.end() && 
	  strcmp(myIntDataMap[command]->getName(), name))
	ArLog::log(ArLog::Normal, 
	      "%sIs already an entry for number %d as data %d\n, overwriting", 
		   myLogPrefix.c_str(), command, 
		   myIntDataMap[command]->getName());
      myNameIntMap[name] = command;
      myIntDataMap[command] = clientData;
    }
    return;
  }
  // if its a single list snag it and run
  else if (packet->getCommand() == ArServerCommands::LISTARGRETSINGLE)
  {
    command = packet->bufToUByte2();
    clientData = myIntDataMap[command];
    if (clientData == NULL)
    {
      ArLog::log(ArLog::Normal, 
		 "%sbuildList: Unknown command %s %d",
		 myLogPrefix.c_str(), getName(packet->getCommand(), true), 
		 packet->getCommand());
      return;
    }
    packet->bufToStr(argDesc, sizeof(argDesc));
    packet->bufToStr(retDesc, sizeof(retDesc));
    clientData->setArgRetDescs(argDesc, retDesc);
    return;
  }
  else if (packet->getCommand() == ArServerCommands::LISTARGRET)
  {
    myReceivedArgRetList = true;
    listLen = packet->bufToUByte2();
    // otherwise loop and read them all
    for (i = 0; i < listLen; i++)
    {
      command = packet->bufToUByte2();
      clientData = myIntDataMap[command];
      if (clientData == NULL)
      {
	ArLog::log(ArLog::Normal, 
		   "%sbuildList: Unknown command %s %d", myLogPrefix.c_str(), 
		   getName(packet->getCommand(), true), packet->getCommand());
  // KMC Need to do something here because otherwise it blows up on 
  // the setArgRetDescs call below...
       return;
      }
      packet->bufToStr(argDesc, sizeof(argDesc));
      packet->bufToStr(retDesc, sizeof(retDesc));
      clientData->setArgRetDescs(argDesc, retDesc);
    }
    return;
  }
  // if its a single list snag it and run
  else if (packet->getCommand() == ArServerCommands::LISTGROUPANDFLAGSSINGLE)
  {
    command = packet->bufToUByte2();
    clientData = myIntDataMap[command];
    if (clientData == NULL)
    {
      ArLog::log(ArLog::Normal, 
		 "%sbuildList: Unknown command %s %d", 
		 myLogPrefix.c_str(), getName(packet->getCommand(), true), 
		 packet->getCommand());
      return;
    }
    packet->bufToStr(commandGroup, sizeof(commandGroup));
    packet->bufToStr(dataFlags, sizeof(dataFlags));
    clientData->setCommandGroup(commandGroup);
    clientData->addDataFlags(dataFlags);
    return;
  }
  else if (packet->getCommand() == ArServerCommands::LISTGROUPANDFLAGS)
  {
    myReceivedGroupAndFlagsList = true;
    listLen = packet->bufToUByte2();
    // otherwise loop and read them all
    for (i = 0; i < listLen; i++)
    {
      command = packet->bufToUByte2();
      clientData = myIntDataMap[command];
      if (clientData == NULL)
      {
	ArLog::log(ArLog::Normal, 
		   "%sbuildList: Unknown command %s %d", myLogPrefix.c_str(), 
		   getName(packet->getCommand(), true), packet->getCommand());
  // KMC Need to do something here because otherwise it blows up on 
  // the setArgRetDescs call below...
       return;
      }
      packet->bufToStr(commandGroup, sizeof(commandGroup));
      packet->bufToStr(dataFlags, sizeof(dataFlags));
      clientData->setCommandGroup(commandGroup);
      clientData->addDataFlags(dataFlags);
    }
    return;
  }
  else
  {
    ArLog::log(ArLog::Terse, 
	       "%sbuildList: Unhandled packet type %s %d", 
	       myLogPrefix.c_str(), getName(packet->getCommand(), true), 
	       packet->getCommand());
  }
}

AREXPORT void ArClientBase::processPacketUdp(ArNetPacket *packet,
				 struct sockaddr_in *sin)
{
  unsigned char *bytes = (unsigned char *)&sin->sin_addr.s_addr;
  long introKey;
  ArNetPacket retPacket;

  myDataMutex.lock();
  myLastPacketReceived.setToNow();
  // see if its an intro packet and we're connected (or could be but
  // not yet know it)
  if ((myState == STATE_CONNECTED || myState == STATE_EXCHANGED_INTROS ||
      myState == STATE_WAITING_LIST) && 
      packet->getCommand() == ArServerCommands::UDP_INTRODUCTION)
  {
    // see if we've already confirmed the UDP
    if (myUdpConfirmedFrom)
    {
      myDataMutex.unlock();
      return;
    }
    introKey = packet->bufToByte4();
    if (myIntroKey != introKey)
    {
      ArLog::log(ArLog::Terse, 
		 "%sUdp introduction packet received with wrong introKey", 
		 myLogPrefix.c_str());
      myDataMutex.unlock();
      return;
    }
    if (myServerReportedUdpPort != ArSocket::netToHostOrder(sin->sin_port))
      ArLog::log(myVerboseLogLevel,
		 "%sPorts don't match, said from server %d and given was %d", 
		 myLogPrefix.c_str(), myServerReportedUdpPort, 
		 ArSocket::netToHostOrder(sin->sin_port));
    myUdpSin.sin_port = sin->sin_port;
    myUdpConfirmedFrom = true;
    // now confirm to the server we got the UDP packet
    retPacket.empty();
    retPacket.setCommand(ArClientCommands::UDP_CONFIRMATION);
    if (!myQuiet || myDebugLogging)
      ArLog::log(ArLog::Normal, "%sServer connected to us on udp port %d", 
		 myLogPrefix.c_str(), 
		 ArSocket::netToHostOrder(myUdpSin.sin_port));
    myDataMutex.unlock();
    sendPacketTcp(&retPacket);
    return;
  }
  // if we receive an intro packet but aren't connected
  else if (packet->getCommand() == ArServerCommands::UDP_INTRODUCTION)
  {
    ArLog::log(ArLog::Terse, 
	       "%sUdp introduction packet received while not connected", 
	       myLogPrefix.c_str());
    myDataMutex.unlock();
    return;
  }
  // if its not an intro, make sure it matchs our server
  else if (myUdpSin.sin_port == sin->sin_port &&
	myUdpSin.sin_addr.s_addr == sin->sin_addr.s_addr)
  {
    // TODO make this reject them if they're not in the user area
    myDataMutex.unlock();
    processPacket(packet, false);
    myDataMutex.lock();
  }
  // if it doesn't warn about it
  else
  {
    ArLog::log(ArLog::Normal, "%sBogus UDP packet from %d.%d.%d.%d %d", 
	       myLogPrefix.c_str(), bytes[0], bytes[1], bytes[2], bytes[3], 
	       ArSocket::netToHostOrder(sin->sin_port));
  }
  myDataMutex.unlock();
}

void ArClientBase::internalSwitchState(ClientState state)
{
  myState = state;
  myStateStarted.setToNow();
}

/**
   This adds a functor that will be called with a packet whenever a
   packet for that name arrives 

   Note that if you try and add a handler for a specific piece of data
   from within a handler for that specific piece of data you'll
   deadlock.

   @param name the name of the data to use with this functor

   @param functor the functor to call with the packet when data with
   this name arrives
   
   @return false is returned if this name doesn't exist in the data

   @pynote Pass a function taking one argument for @arg functor
   @javanote Use a subclass of ArFunctor_NetPacket for the @arg functor object.
**/
AREXPORT bool ArClientBase::addHandler(const char *name, 
				       ArFunctor1 <ArNetPacket *> *functor)
{
  if (name == NULL) {
   ArLog::log(ArLog::Normal, "%saddHandler: Cannot add NULL name", 
	      myLogPrefix.c_str());
   return false;
  }
  ArClientData *clientData;
  // see if we have this data
  myMapsMutex.lock();
  if (myNameIntMap.find(name) == myNameIntMap.end())
  {
    ArLog::log(ArLog::Normal, 
	       "%saddHandler: There is no data by the name \"%s\" to handle", 
	       myLogPrefix.c_str(), name);
    myMapsMutex.unlock();
    return false;
  }
  // since we have the data get the pointer to it
  clientData = myIntDataMap[myNameIntMap[name]];
  myMapsMutex.unlock();

  // make sure nothings gone wrong
  if (clientData == NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "%saddHandler: There was no clientData for data \"%s\"", 
	       myLogPrefix.c_str(), name);
    return false;
  }
  clientData->lockFunctorList();
  // see if it already has a functor
  if (clientData->getFunctorList()->begin() != 
      clientData->getFunctorList()->end())
  {
    ArLog::log(myVerboseLogLevel, 
   "%saddHandler: There is already a functor for data \"%s\", adding anyways", 
	       myLogPrefix.c_str(), name);
    //return false;
  }
  // addthe functor
  clientData->addFunctor(functor);
  clientData->unlockFunctorList();
  return true;
}

/**
   This removes a functor from being called with a packet whenever a
   packet for that name arrives 

   Note that if you try and remove a handler for a specific piece of
   data from within a handler for that specific piece of data you'll
   deadlock.

   @param name the name of the data to use with this functor

   @param functor the functor to call with the packet when data with
   this name arrives
   
   @return false is returned if this name doesn't exist in the data 
**/

AREXPORT bool ArClientBase::remHandler(const char *name, 
				       ArFunctor1<ArNetPacket *> *functor)
{
  ArClientData *clientData;
  myMapsMutex.lock();
  // see if we have this client data
  if (myNameIntMap.find(name) == myNameIntMap.end())
  {
    ArLog::log(ArLog::Normal, 
	       "%sremHandler: There is no data \"%s\"", 
	       myLogPrefix.c_str(), name);
    myMapsMutex.unlock();
    return false;
  }
  if ((clientData = myIntDataMap[myNameIntMap[name]]) == NULL)
  {
    ArLog::log(ArLog::Normal, "%sremHandler: There was no client data for data \"%s\"", myLogPrefix.c_str(), name);
    myMapsMutex.unlock();
    return false;
  }
  myMapsMutex.unlock();
  // set the functor to NULL
  clientData->lockFunctorList();
  clientData->remFunctor(functor);
  clientData->unlockFunctorList();
  return true;
}


/**
   @param name the name of the data to find the command of

   @return 0 if there is no command of that name, the command number
   otherwise
**/
AREXPORT unsigned int ArClientBase::findCommandFromName(const char *name)
{
  std::map<std::string, unsigned int>::iterator it;
  unsigned int ret;

  myMapsMutex.lock();  
  if ((it = myNameIntMap.find(name)) == myNameIntMap.end())
  {
    ArLog::log(ArLog::Normal, 
       "%sFinding command for \"%s\" but no data with that name exists", 
	       myLogPrefix.c_str(), name);
    myMapsMutex.unlock();  
    return 0;
  }
  ret = (*it).second;
  myMapsMutex.unlock();  
  return ret;
}

/**
   This requests data from the server for the given name, at a given
   interval, optionally passing along data with the request.

   @param name the name of the data to request from the server

   @param mSec request interval: the number of milliseconds we want a refresh on this
   data, if this number is 0 then the server will send the information
   as often as it is available; if -1, then the server will send the
   information only when it changes (for broadcast type packets)

   @param packet a packet to use for the request, may contain data to use as
parameters or arguments along with the request.
 **/
AREXPORT bool ArClientBase::request(const char *name, long mSec, 
				    ArNetPacket *packet)
{
  unsigned int command = findCommandFromName(name);

  if (command == 0)
  {
    ArLog::log(ArLog::Normal, 
	       "%sRequesting data for \"%s\", but it doesn't exist", 
	       myLogPrefix.c_str(), name);
    return false;
  }
  
  ArLog::log(myVerboseLogLevel, 
             "%sRequesting data for \"%s\"", 
             myLogPrefix.c_str(), name);
  return requestByCommand(command, mSec, packet);
}

/**
   This requests data from the server for the given name, at mSec
   milliseconds interval, optionally passing along data for the
   request.

   @param command the command number of the data to request

   @param mSec the number of milliseconds we want a refresh on this
   data, if this number is 0 then the server will send the information
   as often as it is available; if -1, then the server will send the
   information only when it changes (for broadcast type packets)

   @param packet the packet that contains the data to use as argument
 **/
AREXPORT bool ArClientBase::requestByCommand(unsigned int command, 
					     long mSec, ArNetPacket *packet)
{
  ArNetPacket sending;

  sending.setCommand(ArClientCommands::REQUEST);
  sending.uByte2ToBuf(command);
  sending.byte4ToBuf(mSec);
  if (packet != NULL)
  {
    packet->resetRead();
    sending.dataToBuf(&packet->getBuf()[packet->getReadLength()],
		      packet->getLength() - packet->getReadLength());
  }
  return sendPacketTcp(&sending);
}


/**
   @param name the name to stop sending
**/
AREXPORT bool ArClientBase::requestStop(const char *name)
{
  ArLog::log(myVerboseLogLevel, 
             "%sRequesting stop data for \"%s\"", 
             myLogPrefix.c_str(), name);
  return requestStopByCommand(findCommandFromName(name));
}

/**
   @param command the command number to stop
**/
AREXPORT bool ArClientBase::requestStopByCommand(unsigned int command)
{
  ArNetPacket sending;
  sending.setCommand(ArClientCommands::REQUESTSTOP);
  sending.uByte2ToBuf(command);
  return sendPacketTcp(&sending);
}


/**
   This requests the data from the server, but only once... this is
   useful for things like video data that you don't want to clog up
   the bandwidth with... its also useful for things like sending
   commands

   @param name the name of the data to request
   @param packet the packet that contains the data to use as argument
   @param quiet a bool set to true to override the verbose logging of this 
   method (generally for repeating requests); default is false to log.
**/
AREXPORT bool ArClientBase::requestOnce(const char *name, 
                                        ArNetPacket *packet,
                                        bool quiet)
{
  if (!quiet) 
    ArLog::log(myVerboseLogLevel, 
               "%sRequesting data once for \"%s\"", myLogPrefix.c_str(), name);
  return requestOnceByCommand(findCommandFromName(name), packet);
}

/**
   Send one request over UDP.  For example, use this method for 
   certain kinds of data where constant streaming would be undesirable 
   (e.g. video images that might saturate the network), or for sending
   a simple command.
   Note that since the request is sent using UDP, you cannot
   assume it will get there or that you will get a response.

   @param name the name of the data to request
   @param packet the packet that contains the data to use as argument
   @param quiet If true, suppress verbose-level log message
**/
AREXPORT bool ArClientBase::requestOnceUdp(
	const char *name, ArNetPacket *packet, bool quiet)
{
  if (!quiet) 
    ArLog::log(myVerboseLogLevel, 
	       "%sRequesting data once (UDP) for \"%s\"", 
	       myLogPrefix.c_str(), name);
  return requestOnceByCommandUdp(findCommandFromName(name), packet);
}

/**
   This requests the data from the server, but only once... this is
   useful for things like video data that you don't want to clog up
   the bandwidth with... its also useful for things like sending
   commands

   @param command the command number to request
   @param packet the packet that contains the data to use as argument
**/
AREXPORT bool ArClientBase::requestOnceByCommand(unsigned int command, 
						 ArNetPacket *packet)
{
  if (packet != NULL)
  {
    packet->setCommand(command);
    return sendPacketTcp(packet);
  }
  else
  {
    ArNetPacket tempPacket;
    tempPacket.setCommand(command);
    return sendPacketTcp(&tempPacket);
  }
}

/**
   This requests the data from the server, but only once... this is
   useful for things like video data that you don't want to clog up
   the bandwidth with... its also useful for things like sending
   commands

   @param command the command number to request
   @param packet the packet that contains the data to use as argument
**/
AREXPORT bool ArClientBase::requestOnceByCommandUdp(unsigned int command, 
						    ArNetPacket *packet)
{
  if (packet != NULL)
  {
    packet->setCommand(command);
    if (!myTcpOnlyTo)
      return sendPacketUdp(packet);
    else
      return sendPacketTcp(packet);
  }
  else
  {
    ArNetPacket tempPacket;
    tempPacket.setCommand(command);
    if (!myTcpOnlyTo)
      return sendPacketUdp(&tempPacket);
    else
      return sendPacketTcp(&tempPacket);
  }
}

/**
   This requests the data from the server, but only once... this is
   useful for things like video data that you don't want to clog up
   the bandwidth with... its also useful for things like sending
   commands

   @param name the name of the data to request
   @param str a string to send as the packet argument
**/
AREXPORT bool ArClientBase::requestOnceWithString(const char *name, 
						  const char *str)
{
  unsigned int command = findCommandFromName(name);
  if(command == 0)
  {
    ArLog::log(ArLog::Normal, 
       "%sRequesting data for \"%s\" but no data with that name exists", 
	       myLogPrefix.c_str(), name);
    //myMapsMutex.unlock();
    return false;
  }

  ArLog::log(myVerboseLogLevel, 
             "%sRequesting data once for \"%s\" [%s]", 
             myLogPrefix.c_str(), name, str);
  
  ArNetPacket tempPacket;
  tempPacket.strToBuf(str);
  tempPacket.setCommand(command);
  return sendPacketTcp(&tempPacket);
}

AREXPORT bool ArClientBase::requestOnceWithInt16(const char *name, ArTypes::Byte2 val)
{
  unsigned int command = findCommandFromName(name);
  if(command == 0)
  {
    ArLog::log(ArLog::Normal, 
       "%sRequesting data for \"%s\" but no data with that name exists", 
	       myLogPrefix.c_str(), name);
    return false;
  }
  ArLog::log(myVerboseLogLevel, 
             "%sRequesting data once for \"%s\" [%d]", 
             myLogPrefix.c_str(), name, val);
  ArNetPacket p;
  p.byte2ToBuf(val);
  p.setCommand(command);
  return sendPacketTcp(&p);
}

AREXPORT bool ArClientBase::requestOnceWithInt32(const char *name, ArTypes::Byte4 val)
{
  unsigned int command = findCommandFromName(name);
  if(command == 0)
  {
    ArLog::log(ArLog::Normal, 
       "%sRequesting data for \"%s\" but no data with that name exists", 
	       myLogPrefix.c_str(), name);
    return false;
  }
  ArLog::log(myVerboseLogLevel, 
             "%sRequesting data once for \"%s\" [%d]", 
             myLogPrefix.c_str(), name, val);
  ArNetPacket p;
  p.byte4ToBuf(val);
  p.setCommand(command);
  return sendPacketTcp(&p);
}

AREXPORT bool ArClientBase::requestOnceWithDouble(const char *name, double val)
{
  unsigned int command = findCommandFromName(name);
  if(command == 0)
  {
    ArLog::log(ArLog::Normal, 
       "%sRequesting data for \"%s\" but no data with that name exists", 
	       myLogPrefix.c_str(), name);
    return false;
  }
  ArLog::log(myVerboseLogLevel, 
             "%sRequesting data once for \"%s\" [%f]", 
             myLogPrefix.c_str(), name, val);
  ArNetPacket p;
  p.doubleToBuf(val);
  p.setCommand(command);
  return sendPacketTcp(&p);
}

/**
   @param name the name to stop sending
**/
AREXPORT bool ArClientBase::dataExists(const char *name)
{
  bool ret;
  myMapsMutex.lock();
  ret = (myNameIntMap.find(name) != myNameIntMap.end());
  myMapsMutex.unlock();
  return ret;
}

AREXPORT void ArClientBase::logDataList(void)
{
  std::map<unsigned int, ArClientData *>::iterator it;
  ArClientData *clientData;

  myMapsMutex.lock();
  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "%sAvailable data:", myLogPrefix.c_str());
  for (it = myIntDataMap.begin(); it != myIntDataMap.end(); it++)
  {
    clientData = (*it).second;
    ArLog::log(ArLog::Terse, "");
    ArLog::log(myVerboseLogLevel, "Number %d, %d functors", 
	       clientData->getCommand(),
	       clientData->getFunctorList()->size());
    ArLog::log(ArLog::Terse, "Data: %s", clientData->getName());
    ArLog::log(ArLog::Terse, "\tDescription: %s",
	       clientData->getDescription());
    ArLog::log(ArLog::Terse, "\tArgument: %s",
	       clientData->getArgumentDescription());
    ArLog::log(ArLog::Terse, "\tReturn: %s",
	       clientData->getReturnDescription());
    ArLog::log(ArLog::Terse, "\tCommandGroup: %s",
	       clientData->getCommandGroup());
    ArLog::log(ArLog::Terse, "\tDataFlags: %s",
	       clientData->getDataFlagsString());

  }
  myMapsMutex.unlock();
}

/**
    @param functor functor created from ArFunctorC which refers to the 
    function to call.
    @param position whether to place the functor first or last
    @see remDisconnectOnErrorCB
 **/

AREXPORT void ArClientBase::addDisconnectOnErrorCB(ArFunctor *functor, 
						   ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myDisconnectOnErrorCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    myDisconnectOnErrorCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "%saddDisconnectOnErrorCB: Invalid position", 
	       myLogPrefix.c_str());
  myCallbackMutex.unlock();
}

/** 
    @param functor the functor to remove from the list of connect callbacks
    @see addDisconnectOnErrorCB
**/
AREXPORT void ArClientBase::remDisconnectOnErrorCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  myDisconnectOnErrorCBList.remove(functor);
  myCallbackMutex.unlock();
}

/**
    @param functor functor created from ArFunctorC which refers to the 
    function to call.
    @param position whether to place the functor first or last
    @see remServerShutdownCB
 **/

AREXPORT void ArClientBase::addServerShutdownCB(ArFunctor *functor, 
						   ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myServerShutdownCBList.push_front(functor);
  else if (position == ArListPos::LAST)
    myServerShutdownCBList.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
	       "%saddServerShutdownCB: Invalid position", myLogPrefix.c_str());
  myCallbackMutex.unlock();
}

/** 
    @param functor the functor to remove from the list of connect callbacks
    @see addServerShutdownCB
**/
AREXPORT void ArClientBase::remServerShutdownCB(ArFunctor *functor)
{
  myCallbackMutex.lock();
  myServerShutdownCBList.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientBase::setTcpOnlyFromServer(void)
{
  ArNetPacket packet;

  packet.setCommand(ArClientCommands::TCP_ONLY);
  sendPacketTcp(&packet);
  myDataMutex.lock();
  myTcpOnlyFrom = true;
  myDataMutex.unlock();
}

AREXPORT void ArClientBase::setTcpOnlyToServer(void)
{
  myDataMutex.lock();
  myTcpOnlyTo = true;
  myDataMutex.unlock();
}

AREXPORT bool ArClientBase::isTcpOnlyFromServer(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myTcpOnlyFrom;
  myDataMutex.unlock();
  return ret;
}
  
AREXPORT bool ArClientBase::isTcpOnlyToServer(void)
{
  bool ret;
  myDataMutex.lock();
  ret = myTcpOnlyTo;
  myDataMutex.unlock();
  return ret;
}


AREXPORT ArTime ArClientBase::getLastPacketReceived(void) 
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastPacketReceived;
  myDataMutex.unlock();
  return ret;
}

/**
   Sets up the backup timeout, if there are packets to send to the
   server and they haven't been sent for longer than this then the
   connection is closed.  Less than 0 means this won't happen.  If
   this is positive but less than 5 seconds then 5 seconds is used.
**/
AREXPORT void ArClientBase::setBackupTimeout(double timeoutInMins)
{
  myBackupTimeout = timeoutInMins;
  myTcpSender.setBackupTimeout(myBackupTimeout);
}

AREXPORT const char *ArClientBase::getName(ArNetPacket *packet, 
					   bool internalCall)
{
  return getName(packet->getCommand(), internalCall);
}

AREXPORT const char *ArClientBase::getName(unsigned int command,
					   bool internalCall)
{
  const char *ret;
  if (!internalCall)
    myMapsMutex.lock();
  std::map<unsigned int, ArClientData *>::iterator it;  
  if ((it = myIntDataMap.find(command)) == myIntDataMap.end())
    ret = NULL;
  else
    ret = (*it).second->getName(); 
  if (!internalCall)
    myMapsMutex.unlock();
  return ret;
}

AREXPORT void ArClientBase::setServerKey(const char *serverKey, bool log)
{
  myDataMutex.lock();
  myServerKey = serverKey;
  myDataMutex.unlock();
  if (log)
    ArLog::log(ArLog::Normal, "%sNew server key set", myLogPrefix.c_str());
}


AREXPORT void ArClientBase::enforceProtocolVersion(const char *protocolVersion,
						   bool log)
{
  myDataMutex.lock();
  if (protocolVersion != NULL)
    myEnforceProtocolVersion = protocolVersion;
  else
    myEnforceProtocolVersion = "";
  myDataMutex.unlock();
  if (log)
    ArLog::log(ArLog::Normal, "%sNew enforceProtocolVersionSet", myLogPrefix.c_str());
}

AREXPORT void ArClientBase::enforceType(ArServerCommands::Type type,
    bool log)
{
  myDataMutex.lock();
  myEnforceType = type;
  myDataMutex.unlock();
  if (log)
    ArLog::log(ArLog::Normal, "%sNew enforce type: %s", 
	       myLogPrefix.c_str(), ArServerCommands::toString(type));
	     
}

/**
   NOTE that this is only valid AFTER the client is connected
**/
AREXPORT void ArClientBase::getClientKey(unsigned char key[CLIENT_KEY_LENGTH])
{
  memcpy(key, myClientKey, CLIENT_KEY_LENGTH);
}

AREXPORT const char *ArClientBase::getHost(void)
{
  const char *ret;
  myDataMutex.lock();
  ret = myHost.c_str();
  myDataMutex.unlock();
  return ret;
}

void ArClientBase::trackPacketSent(ArNetPacket *packet, bool tcp)
{
  myPacketTrackingMutex.lock();
  if (myTrackingSentMap.find(packet->getCommand()) == myTrackingSentMap.end())
    myTrackingSentMap[packet->getCommand()] = new Tracker;

  if (tcp)
  {
    myTrackingSentMap[packet->getCommand()]->myPacketsTcp++;
    myTrackingSentMap[packet->getCommand()]->myBytesTcp += packet->getLength();
  }
  else
  {
    myTrackingSentMap[packet->getCommand()]->myPacketsUdp++;
    myTrackingSentMap[packet->getCommand()]->myBytesUdp += packet->getLength();
  }
  myPacketTrackingMutex.unlock();
}

void ArClientBase::trackPacketReceived(ArNetPacket *packet, 
						  bool tcp)
{
  myPacketTrackingMutex.lock();
  if (myTrackingReceivedMap.find(packet->getCommand()) == 
      myTrackingReceivedMap.end())
    myTrackingReceivedMap[packet->getCommand()] = new Tracker;

  if (tcp)
  {
    myTrackingReceivedMap[packet->getCommand()]->myPacketsTcp++;
    myTrackingReceivedMap[packet->getCommand()]->myBytesTcp += 
                                                  packet->getLength();
  }
  else
  {
    myTrackingReceivedMap[packet->getCommand()]->myPacketsUdp++;
    myTrackingReceivedMap[packet->getCommand()]->myBytesUdp += 
                                                  packet->getLength();
  }
  myPacketTrackingMutex.unlock();
}

AREXPORT void ArClientBase::logTracking(bool terse)
{
  myDataMutex.lock();
  myPacketTrackingMutex.lock();
  std::map<unsigned int, Tracker *>::iterator it;

  unsigned int command;
  Tracker *tracker = NULL;
  long seconds;

  seconds = myTrackingStarted.secSince();
  if (seconds == 0)
    seconds = 1;

  const char *namePtr;
  char name[512];

  long packetsReceivedTcp = 0;
  long bytesReceivedTcp = 0;
  long packetsReceivedUdp = 0;
  long bytesReceivedUdp = 0;

  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "%sReceived tracking (active %d seconds):", 
	     myLogPrefix.c_str(), seconds);
  for (it = myTrackingReceivedMap.begin(); it != myTrackingReceivedMap.end(); it++)
  {
    command = (*it).first;
    tracker = (*it).second;

    packetsReceivedTcp += tracker->myPacketsTcp;
    bytesReceivedTcp += tracker->myBytesTcp;
    packetsReceivedUdp += tracker->myPacketsUdp;
    bytesReceivedUdp += tracker->myBytesUdp;

    if ((namePtr = getName(command, true)) != NULL && namePtr[0] != '\0')
      snprintf(name, sizeof(name), "%s", namePtr);
    // if we're command 255 or less and there's no name its probably
    // one of the server commands we don't really need to track
    else if (command <= 255)
      continue;
    // we should know what the name of everything other then the
    // server command is, but print if we don't, just in case
    else
      snprintf(name, sizeof(name), "#%d", command);      

    if (terse)
    {
      ArLog::log(ArLog::Terse, 
		 "%35s %7ld pkts %10ld B %7ld B/sec", 
		 name, tracker->myPacketsTcp + tracker->myPacketsUdp, 
		 tracker->myBytesTcp + tracker->myBytesUdp,
		 ((tracker->myBytesTcp + tracker->myBytesUdp)/
		  seconds));
    }
    else
    {
      ArLog::log(ArLog::Terse, 
	 "%35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/s ", 
		 name, tracker->myPacketsTcp, tracker->myBytesTcp, 
		 tracker->myBytesTcp/seconds,
		 tracker->myPacketsUdp, tracker->myBytesUdp,
		 tracker->myBytesUdp/seconds);
    }
  }
  
  ArLog::log(ArLog::Terse, "");
  if (terse)
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld pkts %10ld B %7ld B/sec", 
	       "Total Received", packetsReceivedTcp + packetsReceivedUdp, 
	       bytesReceivedTcp + bytesReceivedUdp,
	       (bytesReceivedTcp + bytesReceivedUdp) / seconds);
  }
  else
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",  
	       "Total Received", packetsReceivedTcp, bytesReceivedTcp, 
	       bytesReceivedTcp/seconds, packetsReceivedUdp, bytesReceivedUdp, 
	       bytesReceivedUdp/seconds);
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp rcvs %10ld tcp B %7ld tcp B/S %7ld udp rcvs %10ld udp B %7ld udp B/sec",
	       "Low level Sockets Received", myTcpSocket.getRecvs(), 
	       myTcpSocket.getBytesRecvd(), 
	       myTcpSocket.getBytesRecvd()/seconds, myUdpSocket.getRecvs(), 
	       myUdpSocket.getBytesRecvd(), 
	       myUdpSocket.getBytesRecvd()/seconds);
  }

  long packetsSentTcp = 0;
  long bytesSentTcp = 0;
  long packetsSentUdp = 0;
  long bytesSentUdp = 0;

  ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "%sSent tracking (active %d seconds):", 
	     myLogPrefix.c_str(), seconds);
  for (it = myTrackingSentMap.begin(); it != myTrackingSentMap.end(); it++)
  {
    command = (*it).first;
    tracker = (*it).second;

    packetsSentTcp += tracker->myPacketsTcp;
    bytesSentTcp += tracker->myBytesTcp;
    packetsSentUdp += tracker->myPacketsUdp;
    bytesSentUdp += tracker->myBytesUdp;

    if ((namePtr = getName(command, true)) != NULL && namePtr[0] != '\0')
      snprintf(name, sizeof(name), "%s", namePtr);
    // if we're command 255 or less and there's no name its probably
    // one of the server commands we don't really need to track
    else if (command <= 255)
      continue;
    // we should know what the name of everything other then the
    // server command is, but print if we don't, just in case
    else
      snprintf(name, sizeof(name), "#%d", command);      

    if (terse)
    {
      ArLog::log(ArLog::Terse, 
		 "%35s %7ld pkts %10ld B %7ld B/sec", 
		 name, tracker->myPacketsTcp + tracker->myPacketsUdp, 
		 tracker->myBytesTcp + tracker->myBytesUdp,
		 ((tracker->myBytesTcp + tracker->myBytesUdp)/
		  seconds));
    }
    else
    {
      ArLog::log(ArLog::Terse, 
	 "%35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/s ", 
		 name, tracker->myPacketsTcp, tracker->myBytesTcp, 
		 tracker->myBytesTcp/seconds,
		 tracker->myPacketsUdp, tracker->myBytesUdp,
		 tracker->myBytesUdp/seconds);
    }
  }

  ArLog::log(ArLog::Terse, "");
  if (terse)
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld pkts %10ld B %7ld B/sec", 
	       "Total Sent", packetsSentTcp + packetsSentUdp, 
	       bytesSentTcp + bytesSentUdp,
	       (bytesSentTcp + bytesSentUdp) / seconds);    
    ArLog::log(ArLog::Terse, "");
    ArLog::log(ArLog::Terse, "%-35s %7ld pkts %10ld B %7ld B/sec", 
	       "Total Sent and Received", 
	       (packetsSentTcp + packetsSentUdp + 
		packetsReceivedTcp + packetsReceivedUdp),
	       (bytesSentTcp + bytesSentUdp + 
		bytesReceivedTcp + bytesReceivedUdp),
	       (bytesSentTcp + bytesSentUdp + 
		bytesReceivedTcp + bytesReceivedUdp) / seconds);
  }
  else
  {
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",  
	       "Total Sent", packetsSentTcp, bytesSentTcp, bytesSentTcp / seconds,
	       packetsSentUdp, bytesSentUdp, bytesSentUdp / seconds);
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp snds %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",
	       "Low level Sockets Sent", myTcpSocket.getSends(), 
	       myTcpSocket.getBytesSent(), 
	       myTcpSocket.getBytesSent() / seconds, myUdpSocket.getSends(), 
	       myUdpSocket.getBytesSent(), 
	       myUdpSocket.getBytesSent() / seconds);

    ArLog::log(ArLog::Terse, "");
    ArLog::log(ArLog::Terse, "%-35s %7ld tcp pkts %10ld tcp B %7ld tcp B/S %7ld udp pkts %10ld udp B %7ld udp B/sec",  
	       "Total Sent and Received", packetsSentTcp = packetsReceivedTcp, 
	       bytesSentTcp + bytesReceivedTcp, 
	       (bytesSentTcp + bytesReceivedTcp) / seconds,
	       packetsSentUdp + packetsReceivedUdp, 
	       bytesSentUdp + bytesReceivedUdp, 
	       (bytesSentUdp + bytesReceivedUdp) / seconds);
  }
  ArLog::log(ArLog::Terse, "");
  myPacketTrackingMutex.unlock();
  myDataMutex.unlock();
}


AREXPORT void ArClientBase::resetTracking(void)
{
  myPacketTrackingMutex.lock();
  
  std::map<unsigned int, Tracker *>::iterator it;

  myTrackingStarted.setToNow();

  for (it = myTrackingSentMap.begin(); it != myTrackingSentMap.end(); it++)
    (*it).second->reset();

  for (it = myTrackingReceivedMap.begin(); 
       it != myTrackingReceivedMap.end(); 
       it++)
    (*it).second->reset();

  myTcpSocket.resetTracking();
  myUdpSocket.resetTracking();
  myPacketTrackingMutex.unlock();
}
