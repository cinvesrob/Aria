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
