#include "Aria.h"
#include "ArNetworking.h"

void test(ArNetPacket *packet)
{
  printf("command %d\n", packet->getCommand());
}

int main(int argc, char **argv)
{
  ArClientBase client;
  ArGlobalFunctor1<ArNetPacket *> testCB(&test);

  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArTime startTime;
  startTime.setToNow();
  if (!client.blockingConnect("localhost", 7273))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  }    
  printf("Took %ld msec to connect\n", startTime.mSecSince());
  
  client.runAsync();
  
  client.lock();
  client.addHandler("test", &testCB);
  client.addHandler("test2", &testCB);
  client.addHandler("test3", &testCB);
  client.logDataList();
  
  client.requestOnce("test");
  client.request("test2", 100);
  client.request("test3", -1);
  client.unlock();
  ArUtil::sleep(1000);
  printf("Changing speed\n");
  client.lock();
  client.request("test2", 300);
  client.unlock();
  ArUtil::sleep(1000);
  client.lock();
  client.requestStop("test2");
  client.unlock();
  
  ArUtil::sleep(1000);
  client.lock();
  client.disconnect();
  client.unlock();
  ArUtil::sleep(50);
  exit(0);
}
