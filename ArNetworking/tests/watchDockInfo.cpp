#include "Aria.h"
#include "ArNetworking.h"

void dockInfoChanged(ArNetPacket *packet)
{
  int state = packet->bufToUByte();
  int forcedDock = packet->bufToUByte();
  int secondsToShutdown = packet->bufToUByte2();

  std::string stateStr;
  std::string forcedStr;

  if (state == 0)
    stateStr = "  Undocked";
  else if (state == 1)
    stateStr = "   Docking";
  else if (state == 2)
    stateStr = "   Docked";
  else if (state == 3)
    stateStr = "Undocking";
  else
    stateStr = "  Unknown";
  
  if (forcedDock == 0)
    forcedStr = "false";
  else if (forcedDock == 1)
    forcedStr = " true";
  else
    forcedStr = "unknown";

  if (secondsToShutdown == 0)
    ArLog::log(ArLog::Normal, "State: %s Forced: %s Shutdown: never", 
	       stateStr.c_str(), forcedStr.c_str());
  else
    ArLog::log(ArLog::Normal, "State: %s Forced: %s Shutdown: %d", 
	       stateStr.c_str(), forcedStr.c_str(), secondsToShutdown);
  
}



/* Key handler for the escape key: shutdown all of Aria. */
void escape(void)
{
  printf("esc pressed, shutting down aria\n");
  Aria::shutdown();
}

int main(int argc, char **argv)
{
  Aria::init();

  ArClientBase client;

  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Connected to server.\n");


  /* Create a key handler and also tell Aria about it */
  ArKeyHandler keyHandler;
  Aria::setKeyHandler(&keyHandler);

  /* Global escape-key handler to shut everythnig down */
  ArGlobalFunctor escapeCB(&escape);
  keyHandler.addKeyHandler(ArKeyHandler::ESCAPE, &escapeCB);

  client.addHandler("dockInfoChanged", new ArGlobalFunctor1<ArNetPacket *>(&dockInfoChanged));
  client.requestOnce("dockInfoChanged");
  client.request("dockInfoChanged", -1);

  client.runAsync();

  while (client.getRunningWithLock())
  {
    keyHandler.checkKeys();
    ArUtil::sleep(100);
  }

  /* The client stopped running, due to disconnection from the server, general
   * Aria shutdown, or some other reason. */
  Aria::shutdown();
  return 0;
}
