#include "Aria.h"
#include "ArNetworking.h"

ArClientFileLister *fileLister;
ArClientFileToClient *fileToClient;
ArClientFileFromClient *fileFromClient;
ArClientDeleteFileOnServer *deleteFileOnServer;

bool done = false;

void updated(int ret)
{
  done = true;
  if (ret == 0)
    fileLister->log(false);
  else
    printf("Bad update %d\n", ret);
}

void fileReceived(int ret)
{
  done = true;
  if (ret == 0)
    printf("Got file '%s'\n", fileToClient->getFileName());
  else
    printf("Didn't get file '%s' because %d\n", fileToClient->getFileName(), 
	   ret);
}

void fileSent(int ret)
{
  done = true;
  if (ret == 0)
    printf("Sent file '%s'\n", fileFromClient->getFileName());
  else
    printf("Didn't send file '%s' because %d\n", 
	   fileFromClient->getFileName(), 
	   ret);
}

void fileDeleted(int ret)
{
  done = true;
  if (ret == 0)
    printf("Deleted file '%s'\n", deleteFileOnServer->getFileName());
  else
    printf("Didn't delete file '%s' because %d\n", 
	   deleteFileOnServer->getFileName(), 
	   ret);
}

void waitForDone(void)
{
  while (!done && Aria::getRunning())
    ArUtil::sleep(1);
  done = false;
}

int main(int argc, char **argv)
{
  std::string hostname;
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArClientBase client;


  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);
  
  parser.loadDefaultArguments();

  ArClientFileFromClient::SendSpeed speed = ArClientFileFromClient::SPEED_AUTO;


  if (parser.checkArgument("-speed_fast"))
  {
    ArLog::log(ArLog::Normal, "Putting file with speed_fast");
    speed = ArClientFileFromClient::SPEED_FAST;
  }
  else if (parser.checkArgument("-speed_slow"))
  {
    ArLog::log(ArLog::Normal, "Putting file with speed_slow");
    speed = ArClientFileFromClient::SPEED_SLOW;
  }
  else
  {
    ArLog::log(ArLog::Normal, "Putting file with speed_auto");
  }

  bool pauseAfterSend;

  if (parser.checkArgument("-pauseAfterSend"))
  {
    ArLog::log(ArLog::Normal, "Will pause after send");
    pauseAfterSend = true;
  }
  else
  {
    ArLog::log(ArLog::Normal, "Will not pause after send");
    pauseAfterSend = false;
  }

  /* Check for -help, and unhandled arguments: */
  if (!Aria::parseArgs() || !parser.checkHelpAndWarnUnparsed(1) ||
      parser.getArgc() <= 1)
  {
    ArLog::log(ArLog::Normal, "%s <fileName> [-speed_fast] [-speed_slow] [-pauseAfterSend]",
	       argv[0]);
    ArLog::log(ArLog::Normal, "Default send speed is speed_auto");
    ArLog::log(ArLog::Normal, "");
    Aria::logOptions();
    exit(0);
  }

  const char *fileName = parser.getArg(1);

  

  /* Connect our client object to the remote server: */
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 

  fileFromClient = new ArClientFileFromClient(&client);
  fileFromClient->addFileSentCallback(
	  new ArGlobalFunctor1<int>(&fileSent));

  client.runAsync();

  done = false;
  if (!fileFromClient->putFileToDirectory(NULL, fileName, fileName,
					  speed))
  {
    printf("Error before sending file\n");
    Aria::exit(1);
  }

  waitForDone();

  if (pauseAfterSend)
    ArUtil::sleep(10000);

  Aria::exit(0);
  return 255;
}




