#include "Aria.h"
#include "ArNetworking.h"

void sendEmpty(ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendingPacket;
  sendingPacket.byteToBuf(0);
  client->sendPacketTcp(&sendingPacket);
}

int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;
  ArGlobalFunctor2<ArServerClient *, ArNetPacket *> sendEmptyCB(&sendEmpty);
  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  ArServerInfoDrawings drawing(&server);

  ArDrawingData arrows("polyarrow", ArColor(0, 0, 255), 5, 50);
  ArDrawingData dots("polydots", ArColor(0, 255, 0), 12, 50);
  drawing.addDrawing(&arrows, "arrows", &sendEmptyCB);
  drawing.addDrawing(&dots, "dots", &sendEmptyCB);
  server.run();

}
