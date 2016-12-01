#include "Aria.h"
#include "ArNetworking.h"

int main(int argc, char **argv)
{
  Aria::init();
  ArServerBase server;

  if (!server.open(7272))
  {
    printf("Could not open server port\n");
    exit(1);
  }

  ArServerFileLister fileLister(&server, ".");
  ArServerFileToClient fileToClient(&server, ".");
  ArServerFileFromClient fileFromClient(&server, ".", "/tmp");
  ArServerDeleteFileOnServer fileDeleter(&server, ".");
  server.run();
}
