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
#ifndef ARSERVERHANDLERSTRINGS_H
#define ARSERVERHANDLERSTRINGS_H

#include "Aria.h"
#include "ArServerBase.h"

class ArServerClient;

/**
   This will set up strings to send to MobileEyes so that server
   developers can easily add more information.  There are different
   ways ways you can add a string (they all are the same way
   internally).  The raw way is addString which takes a functor with a
   buffer and a buffer len, the functor then has to fill in the string
   when called.  The other ways are set up for convenience and are
   wrappers in this class around the addString, the other ways are
   addStringInt, addStringDouble, addStringBool, these all take a
   functor that returns the type and a format string (in addition to
   the name and maxLen again).
**/
class ArServerInfoStrings
{
public:
  /// Constructor
  AREXPORT ArServerInfoStrings(ArServerBase *server);
  /// Destructor
  AREXPORT ~ArServerInfoStrings();
  /// Gets the information about the strings being sent
  AREXPORT void netGetStringsInfo(ArServerClient *client, 
				  ArNetPacket *packet);
  /// Gets the strings
  AREXPORT void netGetStrings(ArServerClient *client, 
			      ArNetPacket *packet);
  /// Adds a string to the list in the raw format
  AREXPORT void addString(const char *name, ArTypes::UByte2 maxLen, 
			  ArFunctor2<char *, ArTypes::UByte2> *functor);
  /// Gets the functor for adding a string (for ArStringInfoGroup)
  ArFunctor3<const char *, ArTypes::UByte2,
				    ArFunctor2<char *, ArTypes::UByte2> *> *
                     getAddStringFunctor(void) { return &myAddStringFunctor; }

  /// Gets a string info holder, for internal purposes 
  AREXPORT ArStringInfoHolder *internalGetStringInfoHolder(const char *name);

protected:
  ArServerBase *myServer;

  AREXPORT void buildStringsInfoPacket(void);
  ArNetPacket myStringInfoPacket;
  AREXPORT void buildStringsPacket(void);
  ArNetPacket myStringPacket;
  ArTime myLastStringPacketBuild;

  std::list<ArStringInfoHolder *> myStrings;
  ArTypes::UByte2 myMaxMaxLength;
  ArMutex myStringsMutex;
  ArFunctor3C<ArServerInfoStrings, const char *, ArTypes::UByte2,
	    ArFunctor2<char *, ArTypes::UByte2> *> myAddStringFunctor;

  ArFunctor2C<ArServerInfoStrings, ArServerClient *, 
	      ArNetPacket *> myNetGetStringsInfoCB;
  ArFunctor2C<ArServerInfoStrings, ArServerClient *, 
	      ArNetPacket *> myNetGetStringsCB;

};

#endif // ARSERVERHANDLERSTRINGS_H
