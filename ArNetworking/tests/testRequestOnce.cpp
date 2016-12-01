#include "Aria.h"
#include "ArNetworking.h"

void handleUpdate(ArNetPacket *packet)
{
  if (packet->getPacketSource() == ArNetPacket::TCP)
    printf("Tcp ... \n");
  else
    printf("Udp ... \n");
}

int main(int argc, char **argv)
{
  ArClientBase client;
  ArGlobalFunctor1<ArNetPacket *> handleUpdateCB(&handleUpdate);

  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArTime startTime;
  startTime.setToNow();

  /* Aria components use this to get options off the command line: */
  ArArgumentParser parser(&argc, argv);

  /* This will be used to connect our client to the server, including
   * various bits of handshaking (e.g. sending a password, retrieving a list
   * of data requests and commands...)
   * It will get the hostname from the -host command line argument: */
  ArClientSimpleConnector clientConnector(&parser);

  parser.loadDefaultArguments();

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    Aria::logOptions();
    exit(0);
  }

  
  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Took %ld msec to connect\n", startTime.mSecSince());
  
  client.runAsync();
  
  client.lock();
  client.addHandler("update", &handleUpdateCB);
  
  client.requestOnce("update");
  client.unlock();
  ArUtil::sleep(1000);

  client.lock();
  client.requestOnceUdp("update");
  client.unlock();
  ArUtil::sleep(1000);
  client.lock();
  client.disconnect();
  client.unlock();
  ArUtil::sleep(50);
  exit(0);
}
