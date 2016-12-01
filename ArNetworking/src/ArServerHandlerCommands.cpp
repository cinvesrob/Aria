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
#include "ArServerHandlerCommands.h"

AREXPORT ArServerHandlerCommands::ArServerHandlerCommands(
	ArServerBase *server) :
  myNetListCommandsCB(this, &ArServerHandlerCommands::netListCommands),
  myNetListStringCommandsCB(this, 
			    &ArServerHandlerCommands::netListStringCommands)
{
  myServer = server;
	myTextServer = NULL;
  if (myServer != NULL)
  {
    myServer->addData("listCommands", 
		      "Gets a list of simple commands that can be sent to the server",
			 &myNetListCommandsCB, "none", "byte2: numberOfCommands, <repeats numberOfCommands> string: name, string: description",
		      "CustomCommands", "RETURN_SINGLE");

    myServer->addData("listStringCommands", 
			 "Gets a list of commands that can be sent to the server with string arguments",
			 &myNetListStringCommandsCB, "none", "byte2: numberOfCommands, <repeats numberOfCommands> string: name, string: description", "CustomCommands", "RETURN_SINGLE");
  }

}

AREXPORT ArServerHandlerCommands::~ArServerHandlerCommands()
{
  ArUtil::deleteSet(myFunctors.begin(), myFunctors.end());
  myFunctors.clear();
  ArUtil::deleteSet(myStringFunctors.begin(), myStringFunctors.end());
  myStringFunctors.clear();
}

/**
   @param name the name of the command to add
   @param description the description used for this command
   @param functor the functor to call
   @param commandGroup the command group this should be in, if NULL defaults 
   to CustomCommands
 **/
AREXPORT bool ArServerHandlerCommands::addCommand(
	const char *name, const char *description, ArFunctor *functor,
	const char *commandGroup)
{
  std::string realName;

  if (myPrefix.size() != 0)
    realName = myPrefix;

  realName += name;

  ArFunctor3<ArServerClient *, ArNetPacket *, ArFunctor *> *fun = 
  new ArFunctor3C<ArServerHandlerCommands, ArServerClient *, 
          ArNetPacket *, ArFunctor *>(this,
                               &ArServerHandlerCommands::netParseCommand,
				      NULL, NULL, functor);
	
  std::string group;
  if (myServer == NULL)
  {
    ArLog::log(ArLog::Normal, "Commands::addCommand: server is NULL");
    delete fun;
    return false;
  }
  if (commandGroup != NULL)
    group = commandGroup;
  else
    group = "CustomCommands";
  if (myServer->addData(realName.c_str(), description, fun, "none", "none", 
			group.c_str(), "RETURN_NONE"))
  {
    myCommands.push_back(realName.c_str());
    myCommandDescriptions.push_back(description);
    myFunctors.push_back(fun);
    ArLog::log(ArLog::Verbose, "Added simple command %s", realName.c_str());

		if (myTextServer != NULL) {
			std::string temp = "CustCmd";
			std::string custCmd = temp + name;

			ArFunctor4<char **, int, ArSocket *, ArFunctor *> *custFun = 
			new ArFunctor4C<ArServerHandlerCommands, char **, int, ArSocket *, ArFunctor *>(this,
								&ArServerHandlerCommands::textParseCommand,
								NULL, 0, NULL, functor);

			std::string desc = description;
			std::string trimmedDesc = desc.substr(0, desc.find("\n",0));
			
			myTextServer->addCommand (custCmd.c_str(), custFun,
	                          trimmedDesc.c_str());

		}

		
    return true;
  }
  else
  {
    delete fun;
    ArLog::log(ArLog::Normal, 
	       "Could not add simple command %s", realName.c_str());
    return false;
  }
 
}

/**
   @param name the name of the command to add
   @param description the description used for this command
   @param functor the functor to call (takes an argument builder)
   @param commandGroup the command group this should be in, if NULL defaults 
   to CustomCommands
 **/
AREXPORT bool ArServerHandlerCommands::addStringCommand(
	const char *name, const char *description, 
	ArFunctor1<ArArgumentBuilder *> *functor, const char *commandGroup)
{
  std::string realName;

  if (myPrefix.size() != 0)
    realName = myPrefix;

  realName += name;

  ArFunctor3<ArServerClient *, ArNetPacket *, 
                      ArFunctor1<ArArgumentBuilder *> *> *fun = new 
                                     ArFunctor3C<ArServerHandlerCommands,
                                            ArServerClient *, ArNetPacket *, 
                                       ArFunctor1<ArArgumentBuilder *> *>(this,
                      &ArServerHandlerCommands::netParseStringCommand,
							 NULL, NULL, functor);
	
  if (myServer == NULL)
  {
    ArLog::log(ArLog::Normal, "Commands::addStringCommand: server is NULL");
    delete fun;
    return false;
  }		 
  std::string group;
  if (commandGroup != NULL)
    group = commandGroup;
  else
    group = "CustomCommands";
  if (myServer->addData(realName.c_str(), description, fun, 
			"string: argumentToCommand", "none", group.c_str(),
			"RETURN_NONE"))
  {
    myStringCommands.push_back(realName.c_str());
    myStringCommandDescriptions.push_back(description);
    myStringFunctors.push_back(fun);
    ArLog::log(ArLog::Verbose, "Added simple command with string %s", 
	       realName.c_str());


		if (myTextServer != NULL) {
			std::string temp = "CustCmd";
			std::string custCmd = temp + name;

			ArFunctor4<char **, int, ArSocket *, ArFunctor1<ArArgumentBuilder *> *> *custFun = 
			new ArFunctor4C<ArServerHandlerCommands, char **, int, ArSocket *, ArFunctor1<ArArgumentBuilder *> *>(this,
								&ArServerHandlerCommands::textParseStringCommand,
								NULL, 0, NULL, functor);

			std::string desc = description;
			std::string trimmedDesc = desc.substr(0, desc.find("\n",0));

			myTextServer->addCommand (custCmd.c_str(), custFun,
	                          trimmedDesc.c_str());

		}

    return true;
  }
  else
  {
    delete fun;
    ArLog::log(ArLog::Normal, 
	       "Could not add simple command with string %s", 
	       realName.c_str());
    return false;
  }
}

void ArServerHandlerCommands::netParseCommand(ArServerClient *client, 
						    ArNetPacket *packet, 
						    ArFunctor *functor)
{
  if (functor == NULL)
  {
    ArLog::log(ArLog::Terse, "Command has NULL functor");
    return;
  }
  functor->invoke();
}

void ArServerHandlerCommands::netParseStringCommand(
	ArServerClient *client, ArNetPacket *packet, 
	ArFunctor1<ArArgumentBuilder *> *functor)
{
  char buf[1024];
  if (packet == NULL)
  {
    ArLog::log(ArLog::Terse, "String command has NULL packet");
    return;
  }
  if (functor == NULL)
  {
    ArLog::log(ArLog::Terse, "String command has NULL functor");
    return;
  }
  packet->bufToStr(buf, sizeof(buf));
  ArArgumentBuilder arg;
  arg.add(buf);
  arg.setFullString(buf);
  functor->invoke(&arg);
}


void ArServerHandlerCommands::textParseCommand(
								char **argv, int argc,
						    ArSocket *socket,
								ArFunctor *functor)
{

  if (functor == NULL)
  {
    ArLog::log(ArLog::Terse, "Command has NULL functor");
    return;
  }
  functor->invoke();

}


void ArServerHandlerCommands::textParseStringCommand(
								char **argv, int argc,
						    ArSocket *socket,
								ArFunctor1<ArArgumentBuilder *> *functor)
{

  if (functor == NULL)
  {
    ArLog::log(ArLog::Terse, "String command has NULL functor");
    return;
  }
  ArArgumentBuilder arg;
  arg.addStrings(argc - 1, &argv[1]);
  functor->invoke(&arg);
}

AREXPORT void ArServerHandlerCommands::netListCommands(ArServerClient *client,
						       ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  std::list<std::string>::iterator commIt;
  std::list<std::string>::iterator descIt;
  sendPacket.byte2ToBuf(myCommands.size());
  for (commIt = myCommands.begin(), descIt = myCommandDescriptions.begin();
       commIt != myCommands.end() && descIt != myCommandDescriptions.end();
       commIt++, descIt++)
  {
    sendPacket.strToBuf((*commIt).c_str());
    sendPacket.strToBuf((*descIt).c_str());
  }
  client->sendPacketTcp(&sendPacket);
}

AREXPORT void ArServerHandlerCommands::netListStringCommands(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  std::list<std::string>::iterator commIt;
  std::list<std::string>::iterator descIt;
  sendPacket.byte2ToBuf(myStringCommands.size());
  for (commIt = myStringCommands.begin(), 
        descIt = myStringCommandDescriptions.begin();
       commIt != myStringCommands.end() && 
        descIt != myStringCommandDescriptions.end();
       commIt++, descIt++)
  {
    sendPacket.strToBuf((*commIt).c_str());
    sendPacket.strToBuf((*descIt).c_str());
  }
  client->sendPacketTcp(&sendPacket);
}

AREXPORT void ArServerHandlerCommands::setPrefix(const char *prefix)
{
  myPrefix = prefix;
}

AREXPORT const char *ArServerHandlerCommands::getPrefix(void)
{
  return myPrefix.c_str();
}
