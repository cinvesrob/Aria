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
