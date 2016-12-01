#ifndef ARCLIENTSIMPLECONNECTOR_H
#define ARCLIENTSIMPLECONNECTOR_H

#include "Aria.h"
#include "ArClientBase.h"

/**
   This will let you connect to different hosts, ports, and with
   different users and passwords more easily.

   Some program command line options affect behavior:
   @verbinclude ArClientSimpleConnector_options
   
   To set the host the client will connect to use '<code>-host</code> <i>hostName</i>'
   in the command line parameters
   (no abbreviation for this since <code>-h</code> is for help and it's only 4
   letters).  To set the port the client will connect to use '<code>-port</code>
   <i>portNumber</i>' or '<code>-p</code> <i>portNumber</i>'.  
   To set the user to connect with
   use '<code>-user</code> <i>userName</i>' or '<code>-u</code> <i>userName</i>'.  
   To set the password to
   connect with use '<code>-password</code> <i>password</i>' or 
   '<code>-pwd</code> <i>password</i>'.  To
   use no password use '<code>-nopassword</code>' or '<code>-np</code>'. 
   Note that for using
   passwords you should NOT use that option on the command line since
   that can show everyone what the password is (especially in Linux),
   it's there only for testing.  If you give it a username without a
   password it'll ask you for a password, if you don't have a password
   just use the -nopassword or let it ask you for a password and hit
   enter.  To set the server key (string we need to connect to the
   server) use '<code>-setServerKey</code> <i>serverKey</i>' or 
   '<code>-ssk</code> <i>serverKey</i>'.
 **/
class ArClientSimpleConnector
{
public:
  /// Constructor that takes argument parser (prefered)
  AREXPORT ArClientSimpleConnector(ArArgumentParser *parser);
  /// Constructor that takes args from main (not prefered)
  AREXPORT ArClientSimpleConnector(int *argc, char **argv);
  /// Constructor that takes argument builder (not prefered)
  AREXPORT ArClientSimpleConnector(ArArgumentBuilder *arguments);
  /// Destructor
  AREXPORT ~ArClientSimpleConnector(void);
  /// Connects the client with the options given
  AREXPORT bool connectClient(ArClientBase *client, bool log = true);
  /// Parses the arguments 
  AREXPORT bool parseArgs(void);
  /// Parses the arguments 
  AREXPORT bool parseArgs(ArArgumentParser *parser);
  /// Logs the options the connector has
  AREXPORT void logOptions(void) const;
protected:
  void reset(void);
  const char *myHost;
  const char *myUser;
  const char *myPassword;
  const char *myServerKey;
  int myPort;
  bool myNoPassword;
  bool myLogDataList;
  // our parser
  ArArgumentParser *myParser;
  bool myOwnParser;
  
  ArRetFunctorC<bool, ArClientSimpleConnector> myParseArgsCB;
  ArConstFunctorC<ArClientSimpleConnector> myLogOptionsCB;
};

#endif // ARCLIENTSIMPLECONNECTOR_H
