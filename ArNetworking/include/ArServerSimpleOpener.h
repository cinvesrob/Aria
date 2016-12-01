#ifndef ARSERVERSIMPLEOPENER_H
#define ARSERVERSIMPLEOPENER_H

#include "Aria.h"

class ArServerBase;

/**
   @brief Set up and open an ArNetworking server

   Some program command line options affect behavior:
   @verbinclude ArServerSimpleOpener_options 

   To set the
   port the server uses use '-serverPort <i>serverPortNumber</i>' or
   '-sp <i>serverPortNumber</i>'.  To set the file to look in for user
   information use '-userInfo <i>fileName</i>' or "-ui
   <i>fileName</i>'.  To log out the user information use
   '-logUserInfo' or 'lui'.  To log out the command groups use
   '-logCommandGroups' or '-lcg'.  To set the key used for the server
   (that the client has to know to connect if using user and password)
   use '-serverInfoFile <i>file</i>'.  With a file that has in it
   'serverKey <i>serverKey</i>'.  You should obviously make sure no one you
   don't want to know that server key can read your file that it is
   in.

   For more details about all of these options see ArServerBase.

   @sa ArServerBase
**/
class ArServerSimpleOpener
{
public:
  /// Constructor 
  AREXPORT ArServerSimpleOpener(ArArgumentParser *parser, 
				const char *prefix = "", 
				bool addAriaCallbacks = true);
  /// Destructor
  AREXPORT ~ArServerSimpleOpener();
  /// Function to open up the server
  AREXPORT bool open(ArServerBase *server, const char *baseDirectory = "",
		     int secondsToTryFor = 240);
  /// Function to parse the arguments given in the constructor
  AREXPORT bool parseArgs(void);
  /// Function to parse the arguments given in an arbitrary parser
  AREXPORT bool parseArgs(ArArgumentParser *parser);
  /// Log the options the simple connector has
  AREXPORT void logOptions(void) const;
  /// Logs the things requested for logging, may check things later
  AREXPORT bool checkAndLog(void) const;
  /// Returns true if the open failed because of a bad user file
  bool wasUserFileBad(void) { return myBadUserFile; }
  /// Returns true if the open failed because it couldn't open
  bool didOpenFail(void) { return myOpenFailed; }
  /** Get the server's port number
   *  @return the server's port number
   */
  int getPort() { return myServerPort; } 

  /** Set default server port number (normally 7272). This must be called before
  * parsing command line arguments. 
  * @since 2.7.6
  */
  void setDefaultPort(int port) { myServerPort = port; }

  /// Parses the file for holding the server key
  AREXPORT bool parseFile(const char *fileName);
  /// Sets the tcpOnly flag
  void setServerTcpOnly(bool serverTcpOnly) { myTcpOnly = serverTcpOnly; }
  /// Gets the server key
  const char *getServerKey(void) { return myServerKey.c_str(); } 
protected:
  AREXPORT bool parseArgsWithOutPrefix(ArArgumentParser *parser);
  AREXPORT bool parseArgsWithPrefix(ArArgumentParser *parser);
  AREXPORT void logOptionsWithOutPrefix(void) const;
  AREXPORT void logOptionsWithPrefix(void) const;

  bool fileServerKeyCallback(ArArgumentBuilder *arg);
  
  void reset(void);
  const char *myUserFile;
  //const char *myServerKey;
  std::string myServerKey;
  int myServerPort;
  const char *myOpenOnIP;
  ArServerBase *myServer;
  bool myLogUserInfo;
  bool myLogCommandGroups;
  bool myTcpOnly;
  bool myBadUserFile;
  bool myOpenFailed;
  // our parser
  ArArgumentParser *myParser;
  bool myOwnParser;
  std::string myPrefix;

  // file parser for the server key file
  ArFileParser myFileParser;
  ArRetFunctorC<bool, ArServerSimpleOpener> myParseArgsCB;
  ArConstFunctorC<ArServerSimpleOpener> myLogOptionsCB;
  ArRetFunctor1C<bool, ArServerSimpleOpener, 
      ArArgumentBuilder *> myFileServerKeyCB;
};

#endif // ARSERVERSIMPLEOPENER_H
