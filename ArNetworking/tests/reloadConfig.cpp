#include "Aria.h"
#include "ArNetworking.h"

int main(int argc, char **argv)
{
  Aria::init();
  ArClientBase client;
  std::string host;
  

  ArArgumentParser parser(&argc, argv);
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!clientConnector.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    clientConnector.logOptions();
    exit(0);
  }

  
  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server rejected connection, exiting\n");
    else
      printf("Could not connect to server, exiting\n");
    exit(1);
  } 

  
  client.requestOnce("reloadConfig");


  ArTime start;
  start.setToNow();
  while (Aria::getRunning() && client.isConnected())
  {
    client.loopOnce();
    ArUtil::sleep(ArMath::random() % 10000);
    client.requestOnce("reloadConfig");
  }
  Aria::shutdown();
  return 0;
}
