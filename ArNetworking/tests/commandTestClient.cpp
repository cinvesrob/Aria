#include "Aria.h"
#include "ArNetworking.h"

ArClientBase client;

void commandList(ArNetPacket *packet)
{
  char name[512];
  char description[512];
  int numCommands;
  int i;

  numCommands = packet->bufToByte2();
  printf("%d commands\n", numCommands);
  for (i = 0; i < numCommands; i++)
  {
    packet->bufToStr(name, sizeof(name));
    packet->bufToStr(description, sizeof(description));
    printf("%-20s%s\n", name, description);
    client.requestOnce(name);
  }

}

void stringCommandList(ArNetPacket *packet)
{
  char name[512];
  char description[512];
  int numCommands;
  int i;
  char buf[512];

  numCommands = packet->bufToByte2();
  printf("%d string commands\n", numCommands);
  for (i = 0; i < numCommands; i++)
  {
    packet->bufToStr(name, sizeof(name));
    packet->bufToStr(description, sizeof(description));
    printf("%-20s%s\n", name, description);
    sprintf(buf, "%ld some random fun string %ld", ArMath::random(),
	    ArMath::random());
    client.requestOnceWithString(name, buf);
  }
  
}



int main(int argc, char **argv)
{

  std::string hostname;
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);

  if (argc == 1)
    hostname = "localhost";
  else
    hostname = argv[1];
  if (!client.blockingConnect(hostname.c_str(), 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  } 
  ArGlobalFunctor1<ArNetPacket *> commandCB(&commandList);
  ArGlobalFunctor1<ArNetPacket *> stringCommandCB(&stringCommandList);
  client.addHandler("listCommands", &commandCB);
  client.addHandler("listStringCommands", &stringCommandCB);
  client.requestOnce("listCommands");
  client.requestOnce("listStringCommands");
  /*
  client.requestOnce("Function1");
  client.requestOnce("Function2");
  client.requestOnce("Function3");
  client.requestOnce("Function1");
  client.requestOnceWithString("StringFunction4", "Some string!");
  client.requestOnceWithString("StringFunction5", "funfunfun");
  client.requestOnceWithString("StringFunction6", "six");
  client.requestOnceWithString("StringFunction4", "Some other string!");
  */
  client.runAsync();
  while (client.getRunningWithLock())
  {
    ArUtil::sleep(1);
    //printf("%d ms since last data\n", client.getLastPacketReceived().mSecSince());
  }
  Aria::shutdown();
  return 0;

}




