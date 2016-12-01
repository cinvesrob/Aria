#include "Aria.h"
#include "ArNetworking.h"

int main(int argc, char **argv)
{
  Aria::init();
  //ArLog::init(ArLog::StdOut, ArLog::Verbose);
  ArClientBase client;

  ArArgumentParser parser(&argc, argv);

  ArClientSimpleConnector clientConnector(&parser);
  parser.loadDefaultArguments();

  if (!clientConnector.parseArgs() || !parser.checkHelpAndWarnUnparsed())
  {
    clientConnector.logOptions();
    exit(0);
  }
  
  if (!clientConnector.connectClient(&client))
  {
    if (client.wasRejected())
      printf("Server '%s' rejected connection, exiting\n", client.getHost());
    else
      printf("Could not connect to server '%s', exiting\n", client.getHost());
    exit(1);
  } 
  client.runAsync();

  ArNetPacket sending;
  // We have to tell it what section we're sending
  sending.strToBuf("Section");
  // The map is in the files section
  sending.strToBuf("Files");
  // The parameter name
  sending.strToBuf("Map");
  // The value of the parameter
  sending.strToBuf("entire2.map");

  // you could put in however many of these you want... 

  client.requestOnce("setConfig", &sending);

  // you have to give the client some time to send the command
  ArUtil::sleep(500);
  

  Aria::shutdown();
  return 0;
}

