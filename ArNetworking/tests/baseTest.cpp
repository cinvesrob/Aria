#include "Aria.h"
#include "ArNetworking.h"




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

  client.runAsync();

  while (client.getRunningWithLock())
  {
    keyHandler.checkKeys();
    ArUtil::sleep(100);
  }

  Aria::shutdown();
  return 0;
}
