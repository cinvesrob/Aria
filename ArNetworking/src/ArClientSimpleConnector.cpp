/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2015 Adept Technology, Inc.
Copyright (C) 2016 Omron Adept Technologies, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

If you wish to redistribute ARIA under different terms, contact 
Adept MobileRobots for information about a commercial version of ARIA at 
robots@mobilerobots.com or 
Adept MobileRobots, 10 Columbia Drive, Amherst, NH 03031; +1-603-881-7960
*/
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
