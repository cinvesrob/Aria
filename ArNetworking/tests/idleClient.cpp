#include "Aria.h"
#include "ArNetworking.h"


void idleProcessingPending(ArNetPacket *packet)
{
  if (packet->bufToByte() == 1)
    printf("Idle processing pending\n");
  else
    printf("No idle processing pending\n");
}

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

  ArGlobalFunctor1<ArNetPacket *> idleProcessingPendingCB(
	  &idleProcessingPending);
  client.addHandler("idleProcessingPending", &idleProcessingPendingCB);
  client.request("idleProcessingPending", -1);

  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}




