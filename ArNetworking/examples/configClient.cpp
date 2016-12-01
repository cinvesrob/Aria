#include "Aria.h"
#include "ArNetworking.h"

ArClientBase *client;
ArClientHandlerConfig *configHandler;
bool done = false;

void gotConfig(void)
{
  ArConfig *newConfig;
  done = true;
  configHandler->getConfig()->writeFile("configClient.txt");
  newConfig = new ArConfig(*(configHandler->getConfig()));
  newConfig->writeFile("configClientNew.txt");
}

int main(int argc, char **argv)
{
  Aria::init();
  ArGlobalFunctor gotConfigCB(&gotConfig);
  std::string hostname;

  client = new ArClientBase;
  configHandler = new ArClientHandlerConfig(client);

  configHandler->addGotConfigCB(&gotConfigCB);

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
  if (!clientConnector.connectClient(client))
  {
    exit(1);
  } 

  client->setRobotName(client->getHost()); // include server hostname in log messages

  configHandler->requestConfigFromServer();
  client->runAsync();

  while (!done)
    ArUtil::sleep(100);
  
  if (configHandler->canRequestDefaults())
  {
    configHandler->requestDefaultConfigFromServer();
    while (!configHandler->haveGottenDefaults())
      ArUtil::sleep(100);
    printf("%d\n", configHandler->haveGottenDefaults());
    configHandler->getDefaultConfig()->writeFile("configClientDefaults.txt");
    printf("wrote defaults\n");
  }


  Aria::exit(0);
}
