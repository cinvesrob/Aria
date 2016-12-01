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
#ifndef ARSERVERDATA_H
#define ARSERVERDATA_H

/// class to hold information about the different data the server has
/**
   This class is mostly just for holding all the information about one
   of the server data entires in one convenient spot
**/

#include "Aria.h"

class ArServerClient;
class ArNetPacket;


class ArServerData
{
public:
  /// Constructor
  AREXPORT ArServerData(
	  const char *name, const char *description, unsigned int command,
	  ArFunctor2<ArServerClient *, ArNetPacket *> *functor,
	  const char *argumentDescription, const char *returnDescription,
	  const char *commandGroup = NULL, const char *dataFlags = NULL,
	  ArRetFunctor1<long, unsigned int> *getFrequencyFunctor = NULL,
	  ArFunctor2<long, unsigned int> *requestChangedFunctor = NULL, 
	  ArRetFunctor2<bool, ArServerClient *, ArNetPacket *> 
	  *requestOnceFunctor = NULL);
  /// Destructor
  AREXPORT virtual ~ArServerData();
  const char *getName(void) { return myName.c_str(); }
  const char *getDescription(void) { return myDescription.c_str(); }
  unsigned int getCommand(void) { return myCommand; }
  ArFunctor2<ArServerClient *, ArNetPacket *> *getFunctor(void) 
    { return myFunctor; };
  const char *getArgumentDescription(void) 
    { return myArgumentDescription.c_str(); }
  const char *getReturnDescription(void) 
    { return myReturnDescription.c_str(); }
  const char *getCommandGroup(void) 
    { return myCommandGroup.c_str(); }
  ArRetFunctor2<bool, ArServerClient *, ArNetPacket *> *getRequestOnceFunctor(void)
    { return myRequestOnceFunctor; }
  AREXPORT bool hasDataFlag(const char *dataFlag);
  AREXPORT bool addDataFlags(const char *dataFlags);
  AREXPORT bool remDataFlag(const char *dataFlag);
  bool isSlowPacket(void) { return mySlowPacket; }
  bool isIdlePacket(void) { return myIdlePacket; }
  const char *getDataFlagsString(void) 
    { return myDataFlagsBuilder.getFullString(); }
  AREXPORT void callRequestChangedFunctor(void);
protected:
  std::string myName;
  std::string myDescription;
  std::string myArgumentDescription;
  std::string myReturnDescription;
  std::string myCommandGroup;
  std::string myRawDataFlags;
  ArMutex myDataMutex;
  ArArgumentBuilder myDataFlagsBuilder;
  unsigned int myCommand;
  ArFunctor2<ArServerClient *, ArNetPacket *> *myFunctor;
  ArRetFunctor1<long, unsigned int> *myGetFrequencyFunctor;
  ArFunctor2<long, unsigned int> *myRequestChangedFunctor;
  ArRetFunctor2<bool, ArServerClient *, ArNetPacket *> *myRequestOnceFunctor;
  bool mySlowPacket;
  bool myIdlePacket;
};

#endif // ARSERVERDATA_H
