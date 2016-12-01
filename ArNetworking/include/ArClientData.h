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
#ifndef ARCLIENTDATA_H
#define ARCLIENTDATA_H

#include "Aria.h"
class ArNetPacket;

/// class to hold information about the different data the client has
/**
   This class is mostly just for holding all the information about one
   of the client data entires in one convenient spot

   Information about the data flags held here... The recognized ones
   right now are just for return values so some forwarding can be done
   intelligently, you should only have one return value, they are:
   RETURN_NONE (There are no return packets), RETURN_SINGLE (There is
   exactly one return packet), RETURN_UNTIL_EMPTY (There return goes
   until an empty packet), RETURN_VIDEO (this is the special case for
   video where basically its something thats always requested at an
   interval and everyone'll want it after we've transfered it),
   RETURN_COMPLEX (The return is more complex (so you'll need a helper
   class))
**/

class ArClientData
{
public:
  /// Constructor
  AREXPORT ArClientData(const char *name, const char *description, 
			unsigned int command, 
			ArFunctor1<ArNetPacket *> *functor);
  /// Destructor
  AREXPORT virtual ~ArClientData();
  /// Gets the name
  const char *getName(void) { return myName.c_str(); }
  /// Gets the description
  const char *getDescription(void) { return myDescription.c_str(); }
  unsigned int getCommand(void) { return myCommand; }
  /// Gets the argument description
  const char *getArgumentDescription(void) 
    { return myArgumentDescription.c_str(); }
  /// Gets the return description
  const char *getReturnDescription(void) 
    { return myReturnDescription.c_str(); }
  /// Gets the command group
  const char *getCommandGroup(void) { return myCommandGroup.c_str(); }
  AREXPORT bool hasDataFlag(const char *dataFlag);
  const char *getDataFlagsString(void) 
    { return myDataFlagsBuilder.getFullString(); }
  /// Gets the list of functors
  const std::list<ArFunctor1<ArNetPacket *> *> *getFunctorList(void) const
    { return &myFunctorList; };
  /// Locks the functor list so we can walk it without it changing
  AREXPORT int lockFunctorList(void) { return myMutex.lock(); }
  /// Tries to lock the functor list so we can walk it without it changing
  AREXPORT int tryLockFunctorList(void) { return myMutex.tryLock(); }
  /// Unlocks the functor list so we can walk it without it changing
  AREXPORT int unlockFunctorList(void) { return myMutex.unlock(); }
  
  /// Adds a new functor the end of the list
  void addFunctor(ArFunctor1<ArNetPacket *> *functor) 
    { myFunctorList.push_back(functor); }
  /// Removes a functor from the list all together
  void remFunctor(ArFunctor1<ArNetPacket *> *functor) 
    { myFunctorList.remove(functor); }
  /// Sets the argument and return descriptions
  void setArgRetDescs(const char *argDesc, const char *retDesc)
    { myArgumentDescription = argDesc; myReturnDescription = retDesc; }
  /// Sets the command group
  void setCommandGroup(const char *commandGroup) 
    { myCommandGroup = commandGroup; }
  /// Sets the data flags
  AREXPORT void addDataFlags(const char *dataFlags);
protected:
  std::string myName;
  std::string myDescription;
  unsigned int myCommand;
  std::string myArgumentDescription;
  std::string myReturnDescription;
  std::string myCommandGroup;
  ArMutex myDataMutex;
  ArArgumentBuilder myDataFlagsBuilder;
  ArMutex myMutex;
  std::list<ArFunctor1<ArNetPacket *> *> myFunctorList;
};

#endif // ARCLIENTDATA_H
