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
#ifndef ARCLIENTSWITCH_H
#define ARCLIENTSWITCH_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArClientBase.h"

/**
   The serverInfoFile takes the form of a config file roughly, there
   are 3 things you can put in it now. 'user <i>user</i>', 'password
   <i>password</i>' and 'serverKey <i>serverKey</i>'.  Note that it loads these
   files sequentially so if you pass it 5 files it'll read them in the
   order they were passed in.  If you give it just the keyword but not
   the value (ie 'user') then it'll clear out that value.

   Some program command line options can be used to configure this class:
   @verbinclude ArClientSwitchManager_options
**/
class ArClientSwitchManager : public ArASyncTask
{
public:
  AREXPORT ArClientSwitchManager(
	  ArServerBase *serverBase, 
	  ArArgumentParser *parser, 
	  const char *serverDescription = "Central Server",
	  const char *clientSoftwareDescription = "Software");
  AREXPORT virtual ~ArClientSwitchManager();
  /// Returns if we're connected or not
  AREXPORT bool isConnected(void);
  /// Function to parse the arguments given in the constructor
  AREXPORT bool parseArgs(void); 
  /// Log the options the simple connector has
  AREXPORT void logOptions(void) const;
  /// Gets the hostname we're using for the central server (NULL means we're not trying to sue the central server)
  AREXPORT const char *getCentralServerHostName(void);
  /// Gets the identifier we're using
  AREXPORT const char *getIdentifier(void);
  /// Sets the identifier we're using
  void setIdentifier(const char *identifier)
	{ myIdentifier = identifier; }

  /// Enforces the that the server is using this protocol version
  AREXPORT void enforceProtocolVersion(const char *protocolVersion);
  /// Enforces that the robots that connect are this type
  AREXPORT void enforceType(ArServerCommands::Type type);

  /// Gets the config display hint items dependent on the central
  /// server should use (still check getCentralServerHostName to see
  /// if it's being used)
  const char *getConfigDisplayHint(void) 
    { return myConfigDisplayHint.c_str(); }

  /// The handler for the response to the switch command
  AREXPORT void clientSwitch(ArNetPacket *packet);
  /// The handler for the packet to let the server know we're still talking to it
  AREXPORT void netCentralHeartbeat(ArServerClient *client, 
				    ArNetPacket *packet);
  /// The handler for the packet that comes from the server so we know
  /// we're getting these
  AREXPORT void netCentralServerHeartbeat(ArServerClient *client, 
					  ArNetPacket *packet);

  /// Parses the file for holding the user, password, and server key
  AREXPORT bool parseFile(const char *fileName);
  AREXPORT virtual void *runThread(void *arg);  

  /// Sets debug logging 
  AREXPORT void setDebugLogging(bool debugLogging = false) 
    { myDebugLogging = debugLogging; }
  /// Gets if this is using debug logging 
  AREXPORT bool getDebugLogging(void) { return myDebugLogging; }
  
  /// Gets the server client the forwarder is using (internal)
  /**
     @internal
  **/
  ArServerClient* getServerClient(void) 
    { return myServerClient; }    

  /// Adds central server or identifier not passed into the config
  AREXPORT void addToConfig(const char *configSection,
			    const char *connectName, const char *connectDesc, 
			    const char *addressName, const char *addressDesc);

  /// Adds a callback when we fail to connect
  AREXPORT void addFailedConnectCB(
	  ArFunctor1<const char *> *functor, int position = 50) 
    { myFailedConnectCBList.addCallback(functor, position); }
  /// Removes a callback when we fail to connect
  AREXPORT void remFailedConnectCB(ArFunctor1<const char *> *functor)
    { myFailedConnectCBList.remCallback(functor); }

  /// Adds a callback when we connect
  AREXPORT void addConnectedCB(
	  ArFunctor1<const char *> *functor, int position = 50) 
    { myConnectedCBList.addCallback(functor, position); }
  /// Removes a callback when we connect
  AREXPORT void remConnectedCB(ArFunctor1<const char *> *functor)
    { myConnectedCBList.remCallback(functor); }


protected:
  AREXPORT void socketClosed(void);
  ArServerBase *myServer;  
  ArArgumentParser *myParser;
  std::string myServerDesc;
  std::string myClientSoftwareDesc;
  
  ArServerClient *myServerClient;
  ArTime myLastTcpHeartbeat;
  ArTime myLastUdpHeartbeat;

  ArFileParser myFileParser;

  bool myServerHasHeartbeat;
  double myServerHeartbeatTimeout;
  double myServerUdpHeartbeatTimeout;
  double myServerBackupTimeout;

  bool fileUserCallback(ArArgumentBuilder *arg);
  bool filePasswordCallback(ArArgumentBuilder *arg);
  bool fileServerKeyCallback(ArArgumentBuilder *arg);

  enum State 
  {
    IDLE, ///< Don't want to connect
    TRYING_CONNECTION, ///< If we're trying to connect
    CONNECTING, ///< If we're waiting for the response from the server
    CONNECTED, ///< If we're connected
    LOST_CONNECTION ///< If we lost a connection... wait a bit and try again
  };
  State myState;
  ArTime myStartedState;
  ArTime myLastConnectionAttempt;
  //bool myGaveTimeWarning;

  bool processFile(void);
  AREXPORT void switchState(State state);
  
  ArMutex myDataMutex;

  bool myTryConnection;
  ArClientBase *myClient;

  std::string myUser;
  std::string myPassword;
  std::string myServerKey;

  std::string myCentralServer;
  int myCentralServerPort;  
  std::string myIdentifier;

  std::string myEnforceProtocolVersion;
  ArServerCommands::Type myEnforceType;

  bool myConfigFirstProcess;
  bool myConfigConnectToCentralServer;
  char myConfigCentralServer[1024];
  char myConfigIdentifier[1024];
  std::string myConfigDisplayHint;

  ArCallbackList1<const char *> myFailedConnectCBList;
  ArCallbackList1<const char *> myConnectedCBList;

  bool myDebugLogging;

  ArRetFunctorC<bool, ArClientSwitchManager> myParseArgsCB;
  ArConstFunctorC<ArClientSwitchManager> myLogOptionsCB;
  ArFunctorC<ArClientSwitchManager> mySocketClosedCB;
  ArFunctor1C<ArClientSwitchManager, ArNetPacket *> mySwitchCB;
  ArFunctor2C<ArClientSwitchManager, ArServerClient *, 
      ArNetPacket *> myNetCentralHeartbeatCB;
  ArFunctor2C<ArClientSwitchManager, ArServerClient *, 
      ArNetPacket *> myNetCentralServerHeartbeatCB;
  ArRetFunctor1C<bool, ArClientSwitchManager, 
      ArArgumentBuilder *> myFileUserCB;
  ArRetFunctor1C<bool, ArClientSwitchManager, 
      ArArgumentBuilder *> myFilePasswordCB;
  ArRetFunctor1C<bool, ArClientSwitchManager, 
      ArArgumentBuilder *> myFileServerKeyCB;
  ArRetFunctorC<bool, ArClientSwitchManager> myProcessFileCB;

};


#endif // ARCLIENTSWITCH_H

