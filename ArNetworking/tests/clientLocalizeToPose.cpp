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

  if (parser.getArgc() < 4 || parser.getArgc() > 6)
  {
    printf("usage: %s <x> <y> <th> <optional:xyspread> <optional:thspread>", argv[0]);
    exit(1);
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
  // put in the arguments (you can see what they are from doing -lcl on clientDemo)
  sending.byte4ToBuf(atoi(parser.getArg(1)));
  sending.byte4ToBuf(atoi(parser.getArg(2)));
  sending.byte4ToBuf(atoi(parser.getArg(3)));
  if (parser.getArgc() > 4)
    sending.uByte4ToBuf(atoi(parser.getArg(4)));
  if (parser.getArgc() > 5)
    sending.uByte4ToBuf(atoi(parser.getArg(5)));
  // send the packet
  client.requestOnce("localizeToPose", &sending);

  // you have to give the client some time to send the command
  ArUtil::sleep(500);
  

  Aria::shutdown();
  return 0;
}

