#include "Aria.h"
#include "ArNetworking.h"

ArMap arMap;
ArClientBase client;
ArTime start;

void handleGetMapName(ArNetPacket *packet)
{
  char buffer[512];

  packet->bufToStr(buffer, sizeof(buffer));
  printf("MapFile: %s\n", buffer);
}

void handleGetMap(ArNetPacket *packet)
{
  char buffer[10000];

  if (packet->getDataReadLength() == packet->getDataLength())
  {
    printf("Empty packet signifying end of map (for central forward)\n");
    return;
  }
  
  packet->bufToStr(buffer, sizeof(buffer));
  // if we got an end of line char instead of a line it means the map is over
  if (buffer[0] == '\0')
  {
    printf("Map took %g seconds\n", start.mSecSince() / 1000.0);
    arMap.parsingComplete();
    arMap.writeFile("mapExample.map");
    //client.disconnect();
    //exit(0);
  }
  else
  {
    //printf("line '%s'\n", buffer);
    arMap.parseLine(buffer);
  }

}

int main(int argc, char **argv)
{

  ArGlobalFunctor1<ArNetPacket *> getMapNameCB(handleGetMapName);
  ArGlobalFunctor1<ArNetPacket *> getMapCB(handleGetMap);

  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  ArArgumentParser parser(&argc, argv);
	
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
    exit(1);
  } 

  client.addHandler("getMap", &getMapCB);
  client.addHandler("getMapName", &getMapNameCB);
  client.requestOnce("getMapName");
  start.setToNow();
  client.requestOnce("getMap");

  client.run();

}
