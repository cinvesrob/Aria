#include "Aria.h"
#include "ArExport.h"
#include "ArClientSimpleConnector.h"

AREXPORT ArClientSimpleConnector::ArClientSimpleConnector(int *argc, char **argv) :
  myParseArgsCB(this, &ArClientSimpleConnector::parseArgs),
  myLogOptionsCB(this, &ArClientSimpleConnector::logOptions)
{
  reset();
  myParser = new ArArgumentParser(argc, argv);
  myOwnParser = true;
  myParseArgsCB.setName("ArClientSimpleConnector");
  Aria::addParseArgsCB(&myParseArgsCB, 75);
  myLogOptionsCB.setName("ArClientSimpleConnector");
  Aria::addLogOptionsCB(&myLogOptionsCB, 75);
}

AREXPORT ArClientSimpleConnector::ArClientSimpleConnector(ArArgumentBuilder *builder)  :
  myParseArgsCB(this, &ArClientSimpleConnector::parseArgs),
  myLogOptionsCB(this, &ArClientSimpleConnector::logOptions)
{
  reset();
  myParser = new ArArgumentParser(builder);
  myOwnParser = true;
  myParseArgsCB.setName("ArClientSimpleConnector");
  Aria::addParseArgsCB(&myParseArgsCB, 75);
  myLogOptionsCB.setName("ArClientSimpleConnector");
  Aria::addLogOptionsCB(&myLogOptionsCB, 75);
}

AREXPORT ArClientSimpleConnector::ArClientSimpleConnector(ArArgumentParser *parser) :
  myParseArgsCB(this, &ArClientSimpleConnector::parseArgs),
  myLogOptionsCB(this, &ArClientSimpleConnector::logOptions)
{
  reset();
  myParser = parser;
  myOwnParser = false;
  myParseArgsCB.setName("ArClientSimpleConnector");
  Aria::addParseArgsCB(&myParseArgsCB, 75);
  myLogOptionsCB.setName("ArClientSimpleConnector");
  Aria::addLogOptionsCB(&myLogOptionsCB, 75);
}

AREXPORT ArClientSimpleConnector::~ArClientSimpleConnector(void)
{

}

void ArClientSimpleConnector::reset(void)
{
  myHost = NULL;
  myUser = NULL;
  myPassword = NULL;
  myServerKey = NULL;
  myPort = 7272;
  myNoPassword = false;
  myLogDataList = false;
}

AREXPORT bool ArClientSimpleConnector::parseArgs(void)
{
  return parseArgs(myParser);
}

AREXPORT bool ArClientSimpleConnector::parseArgs(ArArgumentParser *parser)
{
  if (parser->checkArgument("-nopassword") || 
      parser->checkArgument("-np"))
    myNoPassword = true;

  if (parser->checkArgument("-logDataList") || 
      parser->checkArgument("-ldl"))
    myLogDataList = true;

  if (!parser->checkParameterArgumentString("-host", 
					     &myHost) ||
      !parser->checkParameterArgumentInteger("-port",
					     &myPort) ||
      !parser->checkParameterArgumentInteger("-p",
					     &myPort) ||
      !parser->checkParameterArgumentString("-user", 
					    &myUser) ||
      !parser->checkParameterArgumentString("-u", 
					    &myUser) ||
      !parser->checkParameterArgumentString("-password", 
					    &myPassword) ||
      !parser->checkParameterArgumentString("-pwd", 
					    &myPassword) ||
      !parser->checkParameterArgumentString("-setServerKey", 
					    &myServerKey) ||
      !parser->checkParameterArgumentString("-ssk", 
					    &myServerKey))

  {
    return false;
  }

  return true;
}

AREXPORT void ArClientSimpleConnector::logOptions(void) const
{
  ArLog::log(ArLog::Terse, "Options for ArClientSimpleConnector (see docs for more details):");
  ArLog::log(ArLog::Terse, "-host <hostName>");
  ArLog::log(ArLog::Terse, "-port <portNumber>");
  ArLog::log(ArLog::Terse, "-p <portNumber>");
  ArLog::log(ArLog::Terse, "-user <user>");
  ArLog::log(ArLog::Terse, "-u <user>");
  ArLog::log(ArLog::Terse, "-password <password>");
  ArLog::log(ArLog::Terse, "-pwd <password>");
  ArLog::log(ArLog::Terse, "-nopassword");
  ArLog::log(ArLog::Terse, "-np");
  ArLog::log(ArLog::Terse, "-logDataList");
  ArLog::log(ArLog::Terse, "-ldl");
}

AREXPORT bool ArClientSimpleConnector::connectClient(ArClientBase *client,
						     bool print)
{
  std::string host;
  std::string user;
  char password[512];
  password[0] = '\0';

  if (myServerKey != NULL && myServerKey[0] != '\0')
    client->setServerKey(myServerKey);
  
  if (myUser == NULL)
  {
    user = "";
  }
  else
  {
    user = myUser;
    if (myPassword != NULL)
    {
      strncpy(password, myPassword, sizeof(password) - 1);
      password[sizeof(password) - 1] = '\0';
    }
    else if (!myNoPassword)
    {
      printf("Enter password: ");
      fgets(password, sizeof(password) - 1, stdin);
      unsigned int i;
      unsigned int len;
      len = strlen(password);
      for (i = 0; i < len; i++)
      {
	if (password[i] == '\r' || password[i] == '\n')
	{
	  password[i] = '\0';
	  break;
	}
      }
    }
  }
  if (myHost != NULL)
    host = myHost;
  else
    host = "localhost";

  bool ret;
  ret = client->blockingConnect(host.c_str(), myPort, print, user.c_str(), 
				password);
  
  if (ret && myLogDataList)
    client->logDataList();

  return ret;
}
