#include "Aria.h"
#include "ArNetworking.h"


void getModeDataList(ArNetPacket *packet)
{
  char mode[512];
  char data[512];
  int numData;
  int i;

  numData = packet->bufToByte4();
  printf("%d data in modes\n", numData);
  for (i = 0; i < numData; i++)
  {
    packet->bufToStr(mode, sizeof(mode));
    packet->bufToStr(data, sizeof(data));
    printf("%-20s%s\n", mode, data);
  }
}

void getModeInfo(ArNetPacket *packet)
{
  char mode[512];
  unsigned char locked;
  unsigned char willUnlock;
  
  packet->bufToStr(mode, sizeof(mode));
  locked = packet->bufToUByte();
  willUnlock = packet->bufToUByte();
  
  printf("Mode: %s locked: %d willUnlockIfRequested: %d\n", 
	 mode, locked, willUnlock);
  
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

  ArGlobalFunctor1<ArNetPacket *> getModeDataListCB(&getModeDataList);
  ArGlobalFunctor1<ArNetPacket *> getModeInfoCB(&getModeInfo);
  client.addHandler("getModeDataList", &getModeDataListCB);
  client.requestOnce("getModeDataList");
  client.addHandler("getModeInfo", &getModeInfoCB);
  client.request("getModeInfo", -1);

  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}




