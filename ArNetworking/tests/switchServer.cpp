#include "Aria.h"
#include "ArNetworking.h"


void forwarderAdded(ArCentralForwarder *forwarder)
{
  printf("@ Forwarder added for %s\n", 
	 forwarder->getRobotName());
}

void forwarderRemoved(ArCentralForwarder *forwarder)
{
  printf("@ Forwarder removed for %s\n", 
	 forwarder->getRobotName());
}

int main(int argc, char **argv)
{
  Aria::init();
  
  ArServerBase robotServer;
  if (!robotServer.open(5000))
  {
    printf("Could not open robot server port\n");
    Aria::exit(1);
  }

  ArServerBase clientServer;
  if (!clientServer.open(7272))
  {
    printf("Could not open robot server port\n");
    Aria::exit(1);
  }

  ArCentralManager switchManager(&robotServer, &clientServer);

  switchManager.addForwarderAddedCallback(
	  new ArGlobalFunctor1<ArCentralForwarder *>(&forwarderAdded), 
	  100);
  switchManager.addForwarderRemovedCallback(
	  new ArGlobalFunctor1<ArCentralForwarder *>(&forwarderRemoved), 
	  100);

  // Start a small handler to monitor communication between the server and 
  // client.
  ArServerHandlerCommMonitor commMonitor(&clientServer);

  ArServerHandlerCommands commands(&clientServer);
  commands.setPrefix("CentralServer");

  ArServerSimpleServerCommands serverCommands(&commands, &clientServer, 
					      false);
  commands.addCommand(
	  "NetworkLogConnections", "Logs the connections to the central server, and to all the forwarded connections",
	  new ArFunctorC<ArCentralManager>
	  (&switchManager, &ArCentralManager::logConnections));

  clientServer.runAsync();
  robotServer.run();
  Aria::exit(0);
}
