#include "Aria.h"
#include "ArServerBase.h"


int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;
  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }
  server.run();
  return 0;
}
