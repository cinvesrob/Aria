#include "Aria.h"
#include "ArNetworking.h"

void testFunction(ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sending;
  printf("responding to a packet of command %d\n", packet->getCommand());
  client->sendPacketTcp(&sending);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArGlobalFunctor2<ArServerClient *, ArNetPacket *> testCB(&testFunction);
  ArServerBase server;
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArNetPacket packet;

  server.addData("test", "some wierd test", &testCB, "none", "none");
  server.addData("test2", "another wierd test", &testCB, "none", "none");
  server.addData("test3", "yet another wierd test", &testCB, "none", "none");
  if (!server.open(7273))
  {
    printf("Could not open server port\n");
    exit(1);
  }
  server.runAsync();
  while (server.getRunningWithLock())
  {
    ArUtil::sleep(1000);
    server.broadcastPacketTcp(&packet, "test3");
  }
  Aria::shutdown();
  return 0;
}
