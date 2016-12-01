#ifndef ARCENTRALMANAGER
#define ARCENTRALMANAGER

#include "Aria.h"
#include "ArServerBase.h"
#include "ArCentralForwarder.h"

class ArCentralManager : public ArASyncTask
{
public:
  /// Normal constructor
  AREXPORT ArCentralManager(ArServerBase *robotServer, ArServerBase *clientServer);
  /// Empty constructor for those that'll inherit and do other behavior
  AREXPORT ArCentralManager();
  /// Destructor
  AREXPORT virtual ~ArCentralManager();
  /// Enforces that everything is using this protocol version
  AREXPORT virtual void enforceProtocolVersion(const char *protocolVersion);
  /// Enforces that the robots that connect are this type
  AREXPORT virtual void enforceType(ArServerCommands::Type type);
  /// Adds a callback for when a new forwarder is added
  AREXPORT virtual void addForwarderAddedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor, int priority = 0);
  /// Removes a callback for when a new forwarder is added
  AREXPORT virtual void remForwarderAddedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor);
  /// Adds a callback for when a new forwarder is destroyed
  AREXPORT virtual void addForwarderRemovedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor, int priority = 0);
  /// Removes a callback for when a new forwarder is destroyed
  AREXPORT virtual void remForwarderRemovedCallback(
	  ArFunctor1<ArCentralForwarder *> *functor);	  
  /// Adds a cycle callback
  virtual void addCycleCallback(ArFunctor *functor,
			     int position = 50) 
    { myCycleCBList.addCallback(functor, position); }
  /// Removes a cycle callback
  virtual void remCycleCallback(ArFunctor *functor)
    { myCycleCBList.remCallback(functor); }


  /// Logs all the connection information
  AREXPORT void logConnections(void);
  /// Networking command to get the list of clients
  AREXPORT void netClientList(ArServerClient *client, ArNetPacket *packet);
  /// A callback so we can tell the main connection happened when a
  /// client is removed
  AREXPORT void forwarderServerClientRemovedCallback(
	  ArCentralForwarder *forwarder, ArServerClient *client);  
  /// A callback so we can close down other connetions when a main
  /// client loses connection
  AREXPORT void mainServerClientRemovedCallback(ArServerClient *client);  
  /// Networking command to switch the direction of a connection
  AREXPORT void netServerSwitch(ArServerClient *client, ArNetPacket *packet);
  AREXPORT virtual void *runThread(void *arg);


protected:
  void close(void);
  bool processFile(void);

  void forwarderAdded(ArCentralForwarder *forwarder);
  void forwarderRemoved(ArCentralForwarder *forwarder);
  
  bool removePendingDuplicateConnections(const char *robotName);

  ArServerBase *myRobotServer;
  ArServerBase *myClientServer;
  double myHeartbeatTimeout;
  double myUdpHeartbeatTimeout;
  double myRobotBackupTimeout;
  double myClientBackupTimeout;

  int myLoopMSecs;

  std::string myEnforceProtocolVersion;
  ArServerCommands::Type myEnforceType;

  int myMostForwarders;
  int myMostClients;

  ArTimeChecker myTimeChecker;
  ArCallbackList myCycleCBList;

  ArTypes::UByte4 myClosingConnectionID;
  std::list<ArSocket *> myClientSockets;
  std::list<std::string> myClientNames;
  std::list<ArCentralForwarder *> myForwarders;
  std::map<int, ArTime *> myUsedPorts;
  ArMutex myCallbackMutex;
  std::multimap<int, 
      ArFunctor1<ArCentralForwarder *> *> myForwarderAddedCBList;
  std::multimap<int, 
      ArFunctor1<ArCentralForwarder *> *> myForwarderRemovedCBList;
  ArMutex myDataMutex;
  int myOnSocket;
  ArFunctor2C<ArCentralManager, ArServerClient *, 
      ArNetPacket *> myNetSwitchCB;
  ArFunctor2C<ArCentralManager, ArServerClient *, 
      ArNetPacket *> myNetClientListCB;
  ArFunctorC<ArCentralManager> myAriaExitCB;
  ArRetFunctorC<bool, ArCentralManager> myProcessFileCB;
  ArFunctor2C<ArCentralManager, ArCentralForwarder *, 
	      ArServerClient *> myForwarderServerClientRemovedCB;
  ArFunctor1C<ArCentralManager, ArServerClient *> myMainServerClientRemovedCB;
};


#endif // ARSERVERSWITCHMANAGER
