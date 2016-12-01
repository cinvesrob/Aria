#include "Aria.h"
#include "ArNetworking.h"

ArClientBase client;

// suppress the output of drawings until we get the whole list in and then for a few seconds beyond that
bool getDrawingListDone = true;
ArTime getDrawingListDoneTime;

void drawingData(ArNetPacket *packet)
{
  if (getDrawingListDone && getDrawingListDoneTime.secSince() < 5)
    return;

  int x, y;
  int numReadings;
  int i;

  numReadings = packet->bufToByte4();

  if (numReadings == 0)
  {
    printf("No readings for sensor %s\n\n", client.getName(packet));
    return;
  }

  printf("Readings (%d) for %s:", numReadings, client.getName(packet));
  for (i = 0; i < numReadings; i++)
  {
    x = packet->bufToByte4();
    y = packet->bufToByte4();
    printf(" (%d %d)", x, y);
  }
  printf("\n\n");
}

ArGlobalFunctor1<ArNetPacket *> drawingDataCB(&drawingData);

void getDrawingList(ArNetPacket *packet)
{
  int numDrawings;
  int i;
  char name[512];
  char shape[512];
  char visibility[512];
  long primary, size, layer, secondary;
  unsigned long refresh;

  if (packet->getDataReadLength() == packet->getDataLength())
  {
    ArLog::log(ArLog::Normal, "");
    ArLog::log(ArLog::Normal, "Done with getDrawingList, will begin logging drawing data in 5 seconds");
    getDrawingListDone = true;
    getDrawingListDoneTime.setToNow();
    return;
  }

  packet->bufToStr(name, sizeof(name));
  packet->bufToStr(shape, sizeof(shape));
  primary = packet->bufToByte4();
  size = packet->bufToByte4();
  layer = packet->bufToByte4();
  refresh = packet->bufToByte4();
  secondary = packet->bufToByte4();
  packet->bufToStr(visibility, sizeof(visibility));  

  ArLog::log(ArLog::Normal, "name %-40s shape %20s", name, shape);
  ArLog::log(ArLog::Normal, 
	     "\tprimary %08x size %2d layer %2d refresh %4u secondary %08x",
	     primary, size, layer, refresh, secondary);

  if (strcasecmp(visibility, "On") == 0 || 
      strcasecmp(visibility, "DefaultOn") == 0)
  {
    client.addHandler(name, &drawingDataCB);
    client.request(name, refresh);
    ArLog::log(ArLog::Normal, "\tRequesting %s since visibilty %s", name, visibility);
  }
  else
  {
    ArLog::log(ArLog::Normal, "\tNot requesting %s since visibilty %s", name, visibility);
  }
  
}

int main(int argc, char **argv)
{

  std::string hostname;
  ArGlobalFunctor1<ArNetPacket *> getDrawingListCB(&getDrawingList);
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
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  printf("Connected to server.\n");

  client.addHandler("getDrawingList", &getDrawingListCB);
  client.requestOnce("getDrawingList");
  
  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}
