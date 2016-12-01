#include "Aria.h"
#include "ArNetworking.h"


int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArServerBase server;
  ArMap arMap;

  if (argc < 2)
  {
    printf("Usage: %s <mapToServe>\n", argv[0]);
    exit(2);
  }
  arMap.readFile(argv[1]);
  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  ArServerHandlerMap netMap(&server, &arMap);
  server.run();

}
