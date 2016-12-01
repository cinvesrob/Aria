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
#ifndef ARSERVERCLIENTIDENTIFIER_H
#define ARSERVERCLIENTIDENTIFIER_H


/// Class that holds an identifier for a particular serverClient
class ArServerClientIdentifier
{
public:
  /// Constructor
  ArServerClientIdentifier() { myConnectionID = 0; rebuildIDString(); }
  /// Destructor
  virtual ~ArServerClientIdentifier() {}
  /// Gets an ID string which is just the other information in this combined
  const char *getIDString(void) { return myIDString.c_str(); }
  /// set the id num
  void setConnectionID(ArTypes::UByte4 idNum) { myConnectionID = idNum; rebuildIDString(); }
  /// Gets the id num
  ArTypes::UByte4 getConnectionID(void) { return myConnectionID; }
  /// Sets the self identifier
  void setSelfIdentifier(const char *selfIdentifier) 
    { 
      if (selfIdentifier != NULL)
	mySelfIdentifier = selfIdentifier;
      else
	mySelfIdentifier = "";
      rebuildIDString();
    }
  /// Gets the self identifier
  const char *getSelfIdentifier(void) { return mySelfIdentifier.c_str(); }
  /// Sets the here goal
  void setHereGoal(const char *selfIdentifier) 
    { 
      if (selfIdentifier != NULL)
	myHereGoal = selfIdentifier;
      else
	myHereGoal = "";
      rebuildIDString();
    }
  /// Gets the here goal
  const char *getHereGoal(void) { return myHereGoal.c_str(); }
  /// Sees if an identifier matches
  /** 
      It can either match id, or the other parameters (for the other
      parameters if the self id matches its a match, otherwise it
      checks the here goal, and if that matches its a match)... this
      is so that if someone's set a self id but changes their here
      goal they'll still get their messages
  */
  bool matches(ArServerClientIdentifier identifier, 
	       bool matchConnectionID)
    {
      // if the ID's don't match then they don't match, this doesn't
      // make sure they've been set like the others because here if it
      // isn't set it means its an old client, but if its set it means
      // its a new client... and if its one old client, we want to
      // send stuff to all the old clients (since we don't know which
      // one did it)
      if (matchConnectionID && getConnectionID() != identifier.getConnectionID())
	return false;
      else if (matchConnectionID)
	return true;

      // if they both have identifiers see if they match (if they do
      // then we're good)
      if (getSelfIdentifier() != NULL && getSelfIdentifier()[0] != '\0' && 
	  identifier.getSelfIdentifier() != NULL && 
	  identifier.getSelfIdentifier()[0] != '\0' && 
	  strcasecmp(getSelfIdentifier(), identifier.getSelfIdentifier()) == 0)
	return true;

      // if they both have here goals see if they match
      if (getHereGoal() != NULL && getHereGoal()[0] != '\0' && 
	  identifier.getHereGoal() != NULL && 
	  identifier.getHereGoal()[0] != '\0' && 
	  strcasecmp(getHereGoal(), identifier.getHereGoal()) != 0)
	return false;

      return true;
    }
  void rebuildIDString(void)
    {
      char buf[1024];
      bool first = true;
      buf[0] = '\0';
      sprintf(buf, "connectionID %u", myConnectionID);
      if (myConnectionID != 0 || !mySelfIdentifier.empty() || 
	  !myHereGoal.empty())
      {
	if (myConnectionID != 0)
	{
	  if (first)
	    myIDString = "(";
	  else
	    myIDString += " ";
	  first = false;
	  myIDString += buf;
	}
	if (!mySelfIdentifier.empty())
	{
	  if (first)
	    myIDString = "(";
	  else
	    myIDString += " ";
	  first = false;
	  myIDString += "selfID ";
	  myIDString += mySelfIdentifier;
	}
	if (!myHereGoal.empty())
	{
	  if (first)
	    myIDString = "(";
	  else
	    myIDString += " ";
	  first = false;
	  myIDString += "hereGoal ";
	  myIDString += myHereGoal;
	}
	myIDString += ")";
      }
      else
	myIDString = "";
    }
protected:
  // the identifiers
  ArTypes::UByte4 myConnectionID;
  std::string mySelfIdentifier;
  std::string myHereGoal;
  
  // the string that describes them just for debugging
  std::string myIDString;
};

#endif // ArServerClientIdentifier
