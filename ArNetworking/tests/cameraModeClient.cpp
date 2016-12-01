#include "Aria.h"
#include "ArNetworking.h"

void getCameraModeList(ArNetPacket *packet)
{
  ArTypes::UByte2 numModes;
  
  numModes = packet->bufToUByte2();

  printf("%d camera modes:\n", numModes);
  char modeName[512];
  for (int i = 0; i < numModes; i++)
  {
    packet->bufToStr(modeName, sizeof(modeName));
    printf("\t%s\n", modeName);
  }
}

void cameraModeUpdated(ArNetPacket *packet)
{
  char modeName[512];
  packet->bufToStr(modeName, sizeof(modeName));
  printf("Switched to mode %s\n", modeName);
}

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


  client.addHandler("getCameraModeListCam1", 
		    new ArGlobalFunctor1<ArNetPacket *>(&getCameraModeList));
  
  client.requestOnce("getCameraModeListCam1");

  ArNetPacket sending;
  
  // This does the look at goal mode 
  sending.empty();
  sending.strToBuf("lookAtGoal");
  client.requestOnce("setCameraModeCam1", &sending);

  // This does the look at point mode (at 0 0);

  sending.empty();
  sending.strToBuf("lookAtPoint");
  sending.byte4ToBuf(1157);
  sending.byte4ToBuf(-15786);
  client.requestOnce("setCameraModeCam1", &sending);

  while (Aria::getRunning() && client.isConnected())
  {
    client.loopOnce();
    ArUtil::sleep(1000);
  }
  Aria::shutdown();
  return 0;

};
