#include "Aria.h"
#include "ArNetworking.h"

int main(int argc, char **argv)
{
  Aria::init();
  ArLog::init(ArLog::StdOut, ArLog::Normal);
  ArClientBase client;

  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);
  parser.loadDefaultArguments();

  if (!clientConnector.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    clientConnector.logOptions();
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
  client.setRobotName(client.getHost());  // include server hostname in log messages
  client.runAsync();
  ArUtil::sleep(500);
  client.logDataList();
  Aria::shutdown();
  return 0;
}

