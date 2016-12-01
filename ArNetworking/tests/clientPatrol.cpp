#include "Aria.h"
#include "ArNetworking.h"

int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
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
  client.runAsync();

  ArNetPacket sending;
  // We have to tell it what route we want to patrol
  sending.strToBuf("hallways");
  // tell it how much many times we want to patrol (<= 0 == forever)
  sending.byte4ToBuf(0);

  client.requestOnce("patrolRouteCount", &sending);

  // note that there's also another call (patrol) that just has it always
  // patrol forever but this gives you more options and is only
  // slightly more complicated (just give it that byte4)

  // you have to give the client some time to send the command
  ArUtil::sleep(500);
  

  Aria::shutdown();
  return 0;
}

