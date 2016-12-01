#include "Aria.h"
#include "ArExport.h"
#include "ArServerBase.h"
#include "ArServerSimpleOpener.h"


/**
   @param parser the parser to use for information
 **/
AREXPORT ArServerSimpleOpener::ArServerSimpleOpener(
	ArArgumentParser *parser, const char *prefix, 
	bool addAriaCallbacks) :
  myParseArgsCB(this, &ArServerSimpleOpener::parseArgs),
  myLogOptionsCB(this, &ArServerSimpleOpener::logOptions),
  myFileServerKeyCB(this, &ArServerSimpleOpener::fileServerKeyCallback)
{
  myParser = parser;
  myOwnParser = false;
  if (prefix != NULL)
    myPrefix = prefix;
  else
    myPrefix = "";

  myUserFile = NULL;
  myServerPort = 7272;
  myLogUserInfo = false;
  myLogCommandGroups = false;
  myServer = NULL;
  myOpenOnIP = NULL;
  myTcpOnly = false;

  std::string functorName;
  functorName = prefix;
  functorName += "ArServerSimpleOpener";

  if (addAriaCallbacks)
  {
    myParseArgsCB.setName(functorName.c_str());
    Aria::addParseArgsCB(&myParseArgsCB, 60);
    myLogOptionsCB.setName(functorName.c_str());
    Aria::addLogOptionsCB(&myLogOptionsCB, 60);
  }
    
  myFileServerKeyCB.setName("ArServerSimpleOpener::serverKey");
  myFileParser.addHandler("serverKey", &myFileServerKeyCB);
}

AREXPORT ArServerSimpleOpener::~ArServerSimpleOpener(void)
{

}

AREXPORT bool ArServerSimpleOpener::parseArgs(void)
{
  return parseArgs(myParser);
}

AREXPORT bool ArServerSimpleOpener::parseArgs(ArArgumentParser *parser)
{
  if (myPrefix.size() > 0)
    return parseArgsWithPrefix(parser);
  else
    return parseArgsWithOutPrefix(parser);
  
}

AREXPORT bool ArServerSimpleOpener::parseArgsWithOutPrefix(ArArgumentParser *parser)
{

  bool wasReallySetOnlyTrue = parser->getWasReallySetOnlyTrue();
  parser->setWasReallySetOnlyTrue(false);

  if (parser->checkArgument("-lui") ||
      parser->checkArgument("-logUserInfo"))
  {
    myLogUserInfo = true;
  }
  
  if (parser->checkArgument("-lcg") ||
      parser->checkArgument("-logCommandGroups"))
  {
    myLogCommandGroups = true;
  }

  if (parser->checkArgument("-serverTcpOnly"))
  {
    myTcpOnly = true;
  }

  if (!parser->checkParameterArgumentString("-ui",
					    &myUserFile) ||
      !parser->checkParameterArgumentString("-userInfo",
					    &myUserFile) ||
      !parser->checkParameterArgumentInteger("-sp",
					     &myServerPort) ||
      !parser->checkParameterArgumentInteger("-serverPort", 
					     &myServerPort) ||
      !parser->checkParameterArgumentString("-sip",
					    &myOpenOnIP) ||
      !parser->checkParameterArgumentString("-serverIP",
					    &myOpenOnIP))

    /*||
      !parser->checkParameterArgumentString("-setServerKey", 
					     &myServerKey) ||
      !parser->checkParameterArgumentString("-ssk",
      &myServerKey))*/
  {
    return false;
  }
  
  bool wasReallySet;
  const char *serverInfoFile = NULL;
  while (myParser->checkParameterArgumentString(
		 "-serverInfoFile", &serverInfoFile, 
		 &wasReallySet, true) && 
	 wasReallySet)
  {
    if (serverInfoFile != NULL && !parseFile(serverInfoFile))
    {
      parser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
      return false;
    }
  }
  parser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
  return true;
}

AREXPORT bool ArServerSimpleOpener::parseArgsWithPrefix(
	ArArgumentParser *parser)
{
  
  bool wasReallySetOnlyTrue = parser->getWasReallySetOnlyTrue();
  parser->setWasReallySetOnlyTrue(true);

  if (parser->checkArgumentVar("-%sLogUserInfo", myPrefix.c_str()))
  {
    myLogUserInfo = true;
  }

  if (parser->checkArgumentVar("-%sLogCommandGroups", myPrefix.c_str()))
  {
    myLogCommandGroups = true;
  }

  if (parser->checkArgumentVar("-%sTcpOnly", myPrefix.c_str()))
  {
    myTcpOnly = true;
  }

  if (!parser->checkParameterArgumentStringVar(
	      NULL, &myUserFile, "-%sUserInfo", myPrefix.c_str()) || 
      !parser->checkParameterArgumentStringVar(
	      NULL, &myOpenOnIP, "-%sServerIP", myPrefix.c_str()) || 
      !parser->checkParameterArgumentIntegerVar(
	      NULL, &myServerPort, "-%sServerPort", myPrefix.c_str()))
    /*||
      !parser->checkParameterArgumentStringVar(
      NULL, &myServerKey, "-%sSetServerKey", myPrefix.c_str()))*/
  {
    parser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
    return false;
  }

  bool wasReallySet;
  const char *serverInfoFile = NULL;
  if (myParser->checkParameterArgumentStringVar(
	      &wasReallySet, &serverInfoFile, "-%sServerInfoFile", 
	      myPrefix.c_str()) && 
      wasReallySet)
  {
    if (serverInfoFile != NULL && !parseFile(serverInfoFile))
    {
      parser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
      return false;
    }
  }
  
  parser->setWasReallySetOnlyTrue(wasReallySetOnlyTrue);
  return true;
}

AREXPORT void ArServerSimpleOpener::logOptions(void) const
{
  if (myPrefix.size() > 0)
    logOptionsWithPrefix();
  else
    logOptionsWithOutPrefix();
}

AREXPORT void ArServerSimpleOpener::logOptionsWithOutPrefix(void) const
{
 // ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "Options for ArServerSimpleOpener:");
  ArLog::log(ArLog::Terse, "-serverPort <serverPortNumber>");
  ArLog::log(ArLog::Terse, "-sp <serverPortNumber>");
  ArLog::log(ArLog::Terse, "-serverIP <serverIPToOpenOn>");
  ArLog::log(ArLog::Terse, "-sip <serverIPToOpenOn>");
  ArLog::log(ArLog::Terse, "-serverTcpOnly");
  ArLog::log(ArLog::Terse, "-userInfo <userInfoFileName>");
  ArLog::log(ArLog::Terse, "-ui <userInfoFileName>");
  ArLog::log(ArLog::Terse, "-logUserInfo");
  ArLog::log(ArLog::Terse, "-lui");
  ArLog::log(ArLog::Terse, "-logCommandGroups");
  ArLog::log(ArLog::Terse, "-lcg");
  /*
    ArLog::log(ArLog::Terse, "-setServerKey <key>");
  ArLog::log(ArLog::Terse, "-ssk <key>");
  */
  ArLog::log(ArLog::Terse, "-serverInfoFile <file>");
  ArLog::log(ArLog::Terse, "");
}

AREXPORT void ArServerSimpleOpener::logOptionsWithPrefix(void) const
{
  //ArLog::log(ArLog::Terse, "");
  ArLog::log(ArLog::Terse, "Options for ArServerSimpleOpener:");
  ArLog::log(ArLog::Terse, "-%sServerPort <serverPortNumber>", 
	     myPrefix.c_str());
  ArLog::log(ArLog::Terse, "-%sServerIP <serverIPToOpenOn>", 
	     myPrefix.c_str());
  ArLog::log(ArLog::Terse, "-%sServerTcpOnly",
	     myPrefix.c_str());
  ArLog::log(ArLog::Terse, "-%sUserInfo <userInfoFileName>",
	     myPrefix.c_str());
  ArLog::log(ArLog::Terse, "-%sLogUserInfo", myPrefix.c_str());
  ArLog::log(ArLog::Terse, "-%sLogCommandGroups", myPrefix.c_str());
  /*ArLog::log(ArLog::Terse, "-%sSetServerKey <key>",
	     myPrefix.c_str());
  */
  ArLog::log(ArLog::Terse, "-%sServerInfoFile <file>",
	     myPrefix.c_str());
  ArLog::log(ArLog::Terse, "");
}

/**
   @param server the server to operate on
   
   @param baseDirectory the base directory (solely for loading user
   information)

   @param secondsToTryFor this is the number of seconds to try opening
   for, 0 means only try to open once
 **/
AREXPORT bool ArServerSimpleOpener::open(ArServerBase *server,
					 const char *baseDirectory,
					 int secondsToTryFor)
{
  ArTime startedServer;
  bool serverOpened;
  startedServer.setToNow();
  myBadUserFile = false;
  myOpenFailed = false;

  if (secondsToTryFor < 0)
    secondsToTryFor = 0;
  
  server->setServerKey(myServerKey.c_str());
  
  if (myUserFile != NULL && myUserFile[0] != '\0' &&
      !server->loadUserInfo(myUserFile, baseDirectory))
  {
    ArLog::log(ArLog::Normal, "ArServerSimpleOpener: Bad user file");
    myBadUserFile = true;
    return false;
  }
  
  while (Aria::getRunning() && 
	 (serverOpened = 
	  server->open(myServerPort, myOpenOnIP, myTcpOnly)) == false)
  {
    if (secondsToTryFor == 0)
    {
      ArLog::log(ArLog::Normal, "Could not open server on port %d", myServerPort);
      myOpenFailed = true;
      return false;
    }
    if (startedServer.secSince() > secondsToTryFor)
    {
      ArLog::log(ArLog::Normal, "Could not open server on port %d even after %d seconds", 
     myServerPort, secondsToTryFor);
      myOpenFailed = true;
      return false;
    }
    ArLog::log(ArLog::Normal, "Can't open server on port %d yet, waiting", myServerPort);
    ArUtil::sleep(1000);
  }

  if (serverOpened)
    myServer = server;
    
  return serverOpened;
}

AREXPORT bool ArServerSimpleOpener::checkAndLog(void) const
{
  if (myServer != NULL && myLogCommandGroups)
    myServer->logCommandGroups();

  if (myServer != NULL && myLogUserInfo)
    myServer->logUserInfo();


  return true;
}

bool ArServerSimpleOpener::fileServerKeyCallback(ArArgumentBuilder *arg)
{
  if (arg->getArgc() > 1)
  {
    ArLog::log(ArLog::Normal, "Bad serverKey line: %s %s", 
	       arg->getExtraString(), arg->getFullString());
    return false;
  }
  if (arg->getArgc() == 0)
    myServerKey = "";
  else
    myServerKey = arg->getArg(0);
  return true;
}

AREXPORT bool ArServerSimpleOpener::parseFile(const char *fileName)
{
  ArLog::log(ArLog::Normal, "Loading server key from %s", 
	     fileName);
  if (!myFileParser.parseFile(fileName))
  {
    ArLog::log(ArLog::Normal, "Failed parsing server key file %s", 
	     fileName);
    return false;
  }
  return true;
}
