#include "Aria.h"
#include "ArNetworking.h"

int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;
  
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArConfig *config;
  config = Aria::getConfig();

  ArRobotP3DX dx;
  //dx.writeFile("dx.txt");
  *Aria::getConfig() = dx;
  //Aria::getConfig()->writeFile("dxcopy.txt");

  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  ArServerHandlerConfig configHandler(&server, Aria::getConfig());
  server.run();
  
  Aria::shutdown();

  return 0;
}
