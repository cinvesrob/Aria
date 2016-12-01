#ifndef ARCENTRALFORWARDER_H
#define ARCENTRALFORWARDER_H

#include "Aria.h"
#include "ArServerBase.h"
#include "ArClientBase.h"

/**
   Class for forwarding...  You sould use the commands on this instead
   of the ones on the server and client it holds.  
  
   requestStop is missing since that'd only cause problems.

   request doesn't take a packet since that'd cause problems with the
   proxying, other than that everything should be like the client.

   Talk to MattL if there's questions.
**/
class ArCentralForwarder 
{
public:
  /// Normal constructor
  AREXPORT ArCentralForwarder(
	  ArServerBase *mainServer, ArSocket *socket,
	  const char *robotName, int startingPort, 
	  std::map<int, ArTime *> *usedPorts,
	  ArFunctor2<ArCentralForwarder *,
		     ArServerClient *> *notifyServerClientRemovedCB,
	  const char *enforceProtocolVersion,
	  ArServerCommands::Type enforceType
      );
  /// Constructor for those that are going to inherit
  AREXPORT ArCentralForwarder();
  /// Destructor
  AREXPORT virtual ~ArCentralForwarder();

  /// Gets the robot name
  virtual const char *getRobotName(void) { return myRobotName.c_str(); }

  /// Adds a functor for some particular data
  AREXPORT virtual bool addHandler(const char *name, 
			    ArFunctor1 <ArNetPacket *> *functor);

  /// Removes a functor for some particular data by name
  AREXPORT virtual bool remHandler(const char *name, ArFunctor1<ArNetPacket *> *functor);

  /// Request some data every @a mSec milliseconds
  AREXPORT virtual bool request(const char *name, long mSec);		

  /// Request some data (or send a command) just once
  AREXPORT virtual bool requestOnce(const char *name, 
			                      ArNetPacket *packet = NULL,
                            bool quiet = false);

  /// Request some data (or send a command) just once by UDP 
  AREXPORT virtual bool requestOnceUdp(const char *name, 
			       ArNetPacket *packet = NULL, 
			       bool quiet = false);

  /// Request some data (or send a command) just once with a string as argument
  AREXPORT virtual bool requestOnceWithString(const char *name, const char *str);

  /// Sees if this data exists
  AREXPORT virtual bool dataExists(const char *name);

  /// Gets if this forwarder is connected
  virtual bool isConnected(void) { return myState == STATE_CONNECTED; }
  
  /// Gets the server (shouldn't need to be used by anyone)
  virtual ArServerBase *getServer(void) { return myServer; }
  /// Gets the client (shouldn't need to be used by anyone)
  virtual ArClientBase *getClient(void) { return myClient; }

  /// the call that actually does all the work (not virtual since this
  /// may be different per implementation)
  AREXPORT bool callOnce(double heartbeatTimeout, double udpHeartbeatTimeout,
			 double robotBackupTimeout, double clientBackupTimeout);

  /// Gets the port (shouldn't need to be used by anyone)
  int getPort(void) { return myPort; }
  void willReplace(void) { myBeingReplaced = true; }
protected:
  AREXPORT void netCentralHeartbeat(ArNetPacket *packet);

  void robotServerClientRemoved(ArServerClient *client);
  void clientServerClientRemoved(ArServerClient *client);
  void receiveData(ArNetPacket *packet);
  void internalRequestChanged(long interval, unsigned int command);
  bool internalRequestOnce(ArServerClient *client, ArNetPacket *packet,
			   bool tcp);

  AREXPORT bool startingCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool connectingCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool gatheringCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);
  AREXPORT bool connectedCallOnce(
	  double heartbeatTimeout, double udpHeartbeatTimeout,
	  double robotBackupTimeout, double clientBackupTimeout);

  std::string myEnforceProtocolVersion;
  ArServerCommands::Type myEnforceType;

  ArServerBase *myMainServer;
  ArSocket *mySocket;
  std::string myRobotName;
  std::string myPrefix;
  int myStartingPort;
  std::map<int, ArTime *> *myUsedPorts;
  ArFunctor2<ArCentralForwarder *,
	     ArServerClient *> *myForwarderServerClientRemovedCB;

  enum State
  {
    STATE_STARTING,
    STATE_CONNECTING,
    STATE_GATHERING,
    STATE_CONNECTED
  };
  bool myBeingReplaced;


  ArServerBase *myServer;
  ArClientBase *myClient;
  State myState;
  int myPort;
  ArServerBase *server;
  ArClientBase *client;

  bool myRobotHasCentralServerHeartbeat;
  ArTime myLastSentCentralServerHeartbeat;

  enum ReturnType
  {
    RETURN_NONE,
    RETURN_SINGLE,
    RETURN_VIDEO,
    RETURN_UNTIL_EMPTY,
    RETURN_COMPLEX,
    RETURN_VIDEO_OPTIM,
  };

  std::map<unsigned int, ReturnType> myReturnTypes;
  std::map<unsigned int, std::list<ArServerClient *> *> myRequestOnces;
  std::map<unsigned int, ArTime *> myLastRequest;
  std::map<unsigned int, ArTime *> myLastBroadcast;

  ReturnType getReturnType(int command);
  void checkRequestOnces(unsigned int command);
  void setLastRequest(unsigned int command);
  void setLastBroadcast(unsigned int command);

  ArTime myLastTcpHeartbeat;
  ArTime myLastUdpHeartbeat;

  ArFunctor1C<ArCentralForwarder, ArNetPacket *> myReceiveDataFunctor;
  ArFunctor2C<ArCentralForwarder, 
	      long, unsigned int> myInternalRequestChangedFunctor;
  ArRetFunctor3C<bool, ArCentralForwarder, ArServerClient *, 
	      ArNetPacket *, bool> myInternalRequestOnceFunctor;
  ArFunctor1C<ArCentralForwarder, 
      ArServerClient *> myRobotServerClientRemovedCB;
  ArFunctor1C<ArCentralForwarder, 
      ArNetPacket *> myNetCentralHeartbeatCB;
  ArFunctor1C<ArCentralForwarder, 
	      ArServerClient *> myClientServerClientRemovedCB;
  
};


#endif // ARSERVERSWITCHFORWARDER
