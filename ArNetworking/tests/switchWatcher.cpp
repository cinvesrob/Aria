#include "Aria.h"
#include "ArNetworking.h"


void clientListHandler(ArNetPacket *packet)
{
  int numClients;
  int i;
  char hostName[512];
  int port;
  char robotName[512];
  char flags[512];
  char robotIP[512];
  

  numClients = packet->bufToUByte2();
  printf("%d clients connected to server\n", numClients);
  for (i = 0; i < numClients; i++)
  {
    packet->bufToStr(hostName, sizeof(hostName));
    port = packet->bufToUByte2();
    packet->bufToStr(robotName, sizeof(robotName));
    packet->bufToStr(flags, sizeof(flags));
    packet->bufToStr(robotIP, sizeof(robotIP));
    printf("\tHost: '%s' Port: %d ID: '%s' Flags: %s IP: %s\n", hostName, port, robotName, flags, robotIP);
  }
  printf("\n");
}


void clientRemovedHandler(ArNetPacket *packet)
{
  char hostName[512];
  int port;
  char robotName[512];
  char flags[512];
  char robotIP[512];

  packet->bufToStr(hostName, sizeof(hostName));
  port = packet->bufToUByte2();
  packet->bufToStr(robotName, sizeof(robotName));
  packet->bufToStr(flags, sizeof(flags));
  packet->bufToStr(robotIP, sizeof(robotIP));
  printf("Removed Host: '%s' Port: %d ID: '%s' Flags: %s IP: %s\n", hostName, port, robotName, flags, robotIP);
}

void clientAddedHandler(ArNetPacket *packet)
{
  char hostName[512];
  int port;
  char robotName[512];
  char flags[512];
  char robotIP[512];

  packet->bufToStr(hostName, sizeof(hostName));
  port = packet->bufToUByte2();
  packet->bufToStr(robotName, sizeof(robotName));
  packet->bufToStr(flags, sizeof(flags));
  packet->bufToStr(robotIP, sizeof(robotIP));
  printf("Added Host: '%s' Port: %d ID: '%s' Flags: %s IP: %s\n", hostName, port, robotName, flags, robotIP);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArGlobalFunctor1<ArNetPacket *> clientListHandlerCB(&clientListHandler);
  ArGlobalFunctor1<ArNetPacket *> clientAddedHandlerCB(&clientAddedHandler);
  ArGlobalFunctor1<ArNetPacket *> clientRemovedHandlerCB(&clientRemovedHandler);
  ArNetPacket packet;

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

  client.addHandler("clientList", &clientListHandlerCB);
  client.requestOnce("clientList");
  client.addHandler("clientRemoved", &clientRemovedHandlerCB);
  client.request("clientRemoved", -1);
  client.addHandler("clientAdded", &clientAddedHandlerCB);
  client.request("clientAdded", -1);
  client.run();
  Aria::shutdown();
  return 0;
}
