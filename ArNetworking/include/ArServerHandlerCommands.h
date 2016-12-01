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
#ifndef ARSERVERHANDLERCOMMANDS_H
#define ARSERVERHANDLERCOMMANDS_H

#include "Aria.h"
#include "ArServerBase.h"

/// Class for having commands that can be called easily on the client
/**
   You can add commands with addCommand and addStringCommand, you can
   get the list of commands by requesting the data "listCommands" and
   "listStringCommands".
 **/
class ArServerHandlerCommands
{
public:
  /// Constructor
  AREXPORT ArServerHandlerCommands(ArServerBase *server);
  /// Destructor
  AREXPORT virtual ~ArServerHandlerCommands();
  /// Sets the text server 
  AREXPORT void setTextServer(ArNetServer *textServer)
		{ myTextServer = textServer; }
  /// Adds a command with no arguments
  AREXPORT bool addCommand(const char *name, const char *description,
			   ArFunctor *functor, 
			   const char *commandGroup = NULL);
  /// Adds a command that takes a string argument
  AREXPORT bool addStringCommand(const char *name, const char *description,
				 ArFunctor1<ArArgumentBuilder *> *functor, 
				 const char *commandGroup = NULL);
  /// Sets a prefix for commands 
  AREXPORT void setPrefix(const char *prefix);
  /// Gets the prefix
  AREXPORT const char *getPrefix(void);
  /// The function that lists the commands
  AREXPORT void netListCommands(ArServerClient *client, ArNetPacket *packet);
  /// The function that lists the string commands
  AREXPORT void netListStringCommands(ArServerClient *client, 
				      ArNetPacket *packet);
  
protected:
  ArServerBase *myServer;
  ArNetServer *myTextServer;
  void netParseCommand(ArServerClient *client, ArNetPacket *packet, 
		       ArFunctor *functor);
  void netParseStringCommand(ArServerClient *client, ArNetPacket *packet, 
			     ArFunctor1<ArArgumentBuilder *> *functor);

  void textParseCommand(char **argv, int argc, ArSocket *socket, ArFunctor *functor);
  void textParseStringCommand(char **argv, int argc, ArSocket *socket, 
																		ArFunctor1<ArArgumentBuilder *> *functor);

  std::list<std::string> myCommands;
  std::list<std::string> myCommandDescriptions;
  std::list<std::string> myStringCommands;
  std::list<std::string> myStringCommandDescriptions;
  std::list<ArFunctor3<ArServerClient *, ArNetPacket *,
		       ArFunctor *> *> myFunctors;
  std::list<ArFunctor3<ArServerClient *, ArNetPacket *, 
		       ArFunctor1<ArArgumentBuilder *> *> *> myStringFunctors;
  std::string myPrefix;
  ArFunctor2C<ArServerHandlerCommands, 
      ArServerClient *, ArNetPacket *> myNetListCommandsCB;
  ArFunctor2C<ArServerHandlerCommands, 
      ArServerClient *, ArNetPacket *> myNetListStringCommandsCB;

};

#endif 
