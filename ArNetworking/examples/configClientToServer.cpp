#include "Aria.h"
#include "ArNetworking.h"

/*
  Pass this a file to send to the server (should be made by configClient then modified).

  Takes a file to send, and can take the host to send to too

 */

ArClientBase *client;
ArClientHandlerConfig *configHandler;

char *file;

void saveConfigSucceeded(void)
{
  printf("HERE: Save config succeeded\n");
}

void saveConfigFailed(const char *str)
{
  printf("HERE: Save config failed: %s\n", str);
}

void gotConfig(void)
{
  char errorBuffer[1024];
  ArConfig *newConfig;
  if (!configHandler->getConfig()->parseFile(file, false, false, errorBuffer,
					     sizeof(errorBuffer)))
    printf("Error loading file: %s\n", errorBuffer);
  
  configHandler->saveConfigToServer();
  client->loopOnce();
  ArUtil::sleep(1000);
  client->loopOnce();
  ArUtil::sleep(1000);
  client->disconnect();
  Aria::shutdown();
  exit(0);
}

int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArGlobalFunctor gotConfigCB(&gotConfig);
  ArGlobalFunctor saveConfigSucceededCB(&saveConfigSucceeded);
  ArGlobalFunctor1<const char *> saveConfigFailedCB(&saveConfigFailed);
  std::string hostname;

  client = new ArClientBase;
  configHandler = new ArClientHandlerConfig(client, true);

  configHandler->addGotConfigCB(&gotConfigCB);
  configHandler->addSaveConfigSucceededCB(&saveConfigSucceededCB);
  configHandler->addSaveConfigFailedCB(&saveConfigFailedCB);
	
  if (argc == 1)
  {
    printf("Usage: %s <file> <host>\n", argv[0]);
    exit(1);
  }
  file = argv[1];
  if (argc == 2)
    hostname = "localhost";
  else
    hostname = argv[2];

  
  if (!client->blockingConnect(hostname.c_str(), 7272))
  {
    printf("Could not connect to server, exiting\n");
    exit(1);
  }
  //client->requestOnce("setConfig");
  configHandler->requestConfigFromServer();
  //client->requestOnce("setConfig");
  client->run();
  return 0;
}
