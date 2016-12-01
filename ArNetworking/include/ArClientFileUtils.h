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
#ifndef ARCLIENTFILEUTILS_H
#define ARCLIENTFILEUTILS_H

#include "Aria.h"
#include "ArClientBase.h"

/// The item type that the ArCLientFileLister gets back
class ArClientFileListerItem
{
public:
  /// Constructor
  ArClientFileListerItem(const char *name, time_t atime, time_t mtime,
			 ArTypes::UByte4 size)
    { myName = name; myATime = atime; myMTime = mtime; mySize = size; }
  /// Copy Constructor
  ArClientFileListerItem(const ArClientFileListerItem &item)
    { myName = item.myName; myATime = item.myATime; myMTime = item.myMTime; 
    mySize = item.mySize; }
  /// Assignment operator
  AREXPORT ArClientFileListerItem &operator=(const 
					     ArClientFileListerItem &item)
    {
      if (this != &item) 
      {
	myName = item.myName; myATime = item.myATime; myMTime = item.myMTime; 
	mySize = item.mySize; 
      }
      return *this;
    }
    /// Destructor
  virtual ~ArClientFileListerItem() {}
  /// Gets the name of the list item
  const char *getName(void) const { return myName.c_str(); }
  /// Gets the time this list item was last accessed in seconds from 1970
  time_t getLastAccessedTime(void) const { return myATime; }
  /// Gets the time this list item was last modified in seconds from 1970
  time_t getLastModifiedTime(void) const { return myMTime; }
  /// Gets the size of this list item in bytes
  ArTypes::UByte4 getSize(void) const { return mySize; }
protected:
  std::string myName;
  time_t myATime;
  time_t myMTime;
  ArTypes::UByte4 mySize; 
};

/// Class for getting file list information from the server
/**
   This class will interact with the ArServerFileLister and find out
   whats in directories and change directories and such.  This class
   is all that should be used to get the information from
   ArServerFileLister, the API between the two is fairly volatile and
   will remain so.  If you need more functionality let us know and
   we'll add it if its reasonable.
   
   When you change directories or change to the top dir it doesn't
   happen right away, but when it happens the update callbacks will be
   called, 0 as the int for the callback means everything is good,
   positive error messages are from the server (1 == tried to go
   outside allowed area, 2 == no such directory), negative are from
   this class (-1 == got directory but it wasn't what we wanted (if
   you wait the right one might come in, like if someone selects one
   dir then the other)).
**/
class ArClientFileLister
{
public:
  /// Constructor
  AREXPORT ArClientFileLister(ArClientBase *client);
  /// Destructor
  AREXPORT virtual ~ArClientFileLister();
  /// Sees if the server supports what this class needs
  AREXPORT bool isAvailable(void);      
  /// Goes to the top directory
  AREXPORT void changeToTopDir(void);
  /// Goes to this directory in the current directory
  AREXPORT void changeToDir(const char *dir);
  /// Goes up one directory from current directory
  AREXPORT void upOneDir(void);
  /// Goes to this absolute directory
  AREXPORT void changeToAbsDir(const char *dir);
  /// Gets the name of the directory we're in
  AREXPORT const char *getCurrentDir(void) const;
  /// Gets the name of the directory that we're currently waiting for
  AREXPORT const char *getWaitingForDir(void) const;

  /// Gets the directories in the current directory
  AREXPORT std::list<ArClientFileListerItem> getDirectories(void) const;
  /// Gets the files in the current directory
  AREXPORT std::list<ArClientFileListerItem> getFiles(void) const;
  /// Adds a callback for when we get the desired directory info 
  AREXPORT void addUpdatedCallback(ArFunctor1<int> *functor, 
				   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for when we get the desired directory info 
  AREXPORT void remUpdatedCallback(ArFunctor1<int> *functor);
  /// Gets the last time we were updated
  AREXPORT ArTime getLastUpdated(void);
  /// Gets the last time we requested an update
  AREXPORT ArTime getLastRequested(void);
  /// Logs the current directory
  AREXPORT void log(bool withTimes);
protected:
  AREXPORT void netGetDirListing(ArNetPacket *packet);
  AREXPORT void netGetDirListingMultiplePackets(ArNetPacket *packet);
  AREXPORT void callUpdatedCallbacks(int val);
  AREXPORT void logList(
	  std::list<ArClientFileListerItem> *logThis,
	  bool withTimes);
  void getDirListing(const char *dir);
  ArMutex myDataMutex;
  ArMutex myCallbackMutex;
  ArClientBase *myClient;
  char myCurrentDir[2048];
  char myWaitingForDir[2048];
  bool myLastDirMatched;
  bool myNewDirListing;
  ArTime myLastRequested;
  ArTime myLastUpdated;
  std::list<ArClientFileListerItem> myDirectories;
  std::list<ArClientFileListerItem> myFiles;
  std::list<ArFunctor1<int> *> myUpdatedCallbacks;
  ArFunctor1C<ArClientFileLister, ArNetPacket *> myGetDirListingCB;
  ArFunctor1C<ArClientFileLister, 
	      ArNetPacket *> myGetDirListingMultiplePacketsCB;
};


/// Class for getting files from the server
/**
   This class will interact with the ArServerFileToClient and get a
   file on the server.  If you want to find out what files are on the
   server use ArClientFileLister.

   When get a file it doesn't happen right away, but when the file is
   received (or failes) the fileGotten callbacks will be called, 0 as
   the int for the callback means everything is good, positive error
   messages are from the server (1 == tried to go outside allowed
   area, 2 == no such directory, 3 == empty file name, 4 == problem
   reading file), negative are from this class (-1 == got directory
   but it wasn't what we wanted (if you wait the right one might come
   in, like if someone selects one dir then the other), -2 == can't
   open file to put result into).
**/
class ArClientFileToClient
{
public:
  /// Constructor
  AREXPORT ArClientFileToClient(ArClientBase *client);
  /// Destructor
  AREXPORT virtual ~ArClientFileToClient();
  /// Sees if the server supports what this class needs
  AREXPORT bool isAvailable(void);  

  AREXPORT bool isAvailableSetTimestamp(void);

  /// Get the file from a directory 
  AREXPORT bool getFileFromDirectory(const char *directory, 
                                     const char *fileName, 
                                     const char *clientFileName,
                                     bool isSetTimestamp = false);
  /// Cancels getting a file
  AREXPORT void cancelGet(void);
  /// If we're getting a file now
  AREXPORT bool isWaitingForFile(void);
  /// Gets the directory we're getting from
  AREXPORT const char *getDirectory(void);
  /// Gets the filename we're getting
  AREXPORT const char *getFileName(void);
  /// Gets the filename we'll save the gotten file in
  AREXPORT const char *getClientFileName(void);
  /// Adds a callback for when we get the desired file (or fail)
  AREXPORT void addFileReceivedCallback(ArFunctor1<int> *functor, 
				   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for when we get the desired file (or fail)
  AREXPORT void remFileReceivedCallback(ArFunctor1<int> *functor);

  
  /// Gets the last time we finished getting a file
  AREXPORT ArTime getLastReceived(void);
  /// Gets the last time we asked for a file
  AREXPORT ArTime getLastRequested(void);

protected:

  AREXPORT void netGetFile(ArNetPacket *packet);
  AREXPORT void netGetFileWithTimestamp(ArNetPacket *packet);
  AREXPORT void doGetFile(ArNetPacket *packet,
                          bool isSetTimestamp);
  AREXPORT void callFileReceivedCallbacks(int val);


protected:

  ArMutex myDataMutex;
  ArMutex myCallbackMutex;
  ArClientBase *myClient;
  bool myIsWaitingForFile;
  std::string myDirectory;
  std::string myFileName;
  std::string myWholeFileName;
  std::string myClientFileName;
  FILE *myFile;

  ArTime myLastRequested;
  ArTime myLastReceived;

  std::list<ArFunctor1<int> *> myFileReceivedCallbacks;

  ArFunctor1C<ArClientFileToClient, ArNetPacket *> myGetFileCB;
  ArFunctor1C<ArClientFileToClient, ArNetPacket *> myGetFileWithTimestampCB;
};

/// Class for putting files to the server
/**
   This class will interact with the ArServerFileFromClient and put a
   file on to the server.  If you want to find out what files are on
   the server use ArClientFileLister.

   When get a file it doesn't happen right away, but when the file is
   received (or failes) the fileGotten callbacks will be called, 0 as
   the int for the callback means everything is good, positive error
   messages are from the server (0 = good (got file), 1 = getting
   file, 2 = tried to go outside allowed area, 3 = bad directory, 4 =
   empty file name (or other problem with fileName), 5 = can't write
   temp file, 6 = error moving file from temp to perm, 7 = another
   client putting file, 8 = timeout (no activity for 15 seconds) and
   another client wanted to put the file, 9 = client adding to,
   finishing, or canceling a file the server doesn't have), negative
   would be from this class but there aren't any of those yet
**/
class ArClientFileFromClient
{
public:
  /// Constructor
  AREXPORT ArClientFileFromClient(ArClientBase *client);
  /// Destructor
  AREXPORT virtual ~ArClientFileFromClient();
  /// Sees if the server supports what this class needs
  AREXPORT bool isAvailable(void);      
  /// Sees if the server supports what this class needs to send slowly
  AREXPORT bool isAvailableSlow(void);      
  /// Sees if the server supports what this class needs to send fast
  AREXPORT bool isAvailableFast(void);      
  /// Sees if the server supports the ability to set the file timestamp
  AREXPORT bool isAvailableSetTimestamp(void);

  /// Enum that describes the speed to send at
  enum SendSpeed 
  {
    SPEED_AUTO, ///< Send it fast if available, if not then send it slow
    SPEED_FAST, ///< Send it fast
    SPEED_SLOW ///< Send it slow
  };
  /// Puts the specified client file on the server in the given directory and file
  AREXPORT bool putFileToDirectory(const char *directory, 
 				                           const char *fileName, 
				                           const char *clientFileName,
				                           SendSpeed sendSpeed = SPEED_AUTO, 
                                   bool isSetTimestamp = false);
  /// Cancels putting a file
  AREXPORT void cancelPut(void);

  /// If we're waiting for completion now
  AREXPORT bool isWaitingForReturn(void);
  /// Gets the directory we're putting to
  AREXPORT const char *getDirectory(void);
  /// Gets the filename we're putting
  AREXPORT const char *getFileName(void);
  /// Gets the filename we're taking from the client
  AREXPORT const char *getClientFileName(void);
  /// Adds a callback for when we get the desired file (or fail)
  AREXPORT void addFileSentCallback(ArFunctor1<int> *functor, 
				   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for when we get the desired file (or fail)
  AREXPORT void remFileSentCallback(ArFunctor1<int> *functor);
  /// Gets the last time we finished putting a file
  AREXPORT ArTime getLastCompletedSend(void);
  /// Gets the last time we started putting a file
  AREXPORT ArTime getLastStartedSend(void);
protected:
  AREXPORT void netPutFile(ArNetPacket *packet);
  AREXPORT void callFileSentCallbacks(int val);

  ArMutex myDataMutex;
  ArMutex myCallbackMutex;
  ArClientBase *myClient;
  bool myIsWaitingForReturn;
  bool myInterleaved;
  bool myTimestamp;
  std::string myCommandName;
  std::string myDirectory;
  std::string myFileName;
  std::string myWholeFileName;
  std::string myClientFileName;
  
  bool myReadyForNextPacket;
  FILE *myFile;
  ArTime myLastStartedSend;
  ArTime myLastCompletedSend;
  std::list<ArFunctor1<int> *> myFileSentCallbacks;
  ArFunctor1C<ArClientFileFromClient, ArNetPacket *> myPutFileCB;
};


/// Class for deleting a file on the server
/**
   This class will interact with the ArServerFileFromClient and put a
   file on to the server.  If you want to find out what files are on
   the server use ArClientFileLister.

   When get a file it doesn't happen right away, but when the file is
   received (or failes) the fileGotten callbacks will be called, 0 as
   the int for the callback means everything is good, positive error
   messages are from the server (0 = good (got file), 1 = getting
   file, 2 = tried to go outside allowed area, 3 = bad directory, 4 =
   empty file name (or other problem with fileName), 5 = can't write
   temp file, 6 = error moving file from temp to perm, 7 = another
   client putting file, 8 = timeout (no activity for 15 seconds) and
   another client wanted to put the file, 9 = client adding to,
   finishing, or canceling a file the server doesn't have), negative
   would be from this class but there aren't any of those yet
**/
class ArClientDeleteFileOnServer
{
public:
  /// Constructor
  AREXPORT ArClientDeleteFileOnServer(ArClientBase *client);
  /// Destructor
  AREXPORT virtual ~ArClientDeleteFileOnServer();
  /// Sees if the server supports what this class needs
  AREXPORT bool isAvailable(void);      
  /// Get the file from a directory 
  AREXPORT bool deleteFileFromDirectory(const char *directory, 
					const char *fileName);
  /// If we're waiting for completion now
  AREXPORT bool isWaitingForReturn(void);
  /// Gets the directory we're putting to
  AREXPORT const char *getDirectory(void);
  /// Gets the filename we're putting
  AREXPORT const char *getFileName(void);
  /// Adds a callback for when we get the desired file (or fail)
  AREXPORT void addFileDeletedCallback(ArFunctor1<int> *functor, 
				   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback for when we get the desired file (or fail)
  AREXPORT void remFileDeletedCallback(ArFunctor1<int> *functor);
  /// Gets the last time we finished putting a file
  AREXPORT ArTime getLastCompletedSend(void);
  /// Gets the last time we started putting a file
  AREXPORT ArTime getLastStartedSend(void);
protected:
  AREXPORT void netDeleteFile(ArNetPacket *packet);
  AREXPORT void callFileDeletedCallbacks(int val);
  ArMutex myDataMutex;
  ArMutex myCallbackMutex;
  ArClientBase *myClient;
  bool myIsWaitingForReturn;
  std::string myDirectory;
  std::string myFileName;
  std::string myWholeFileName;
  FILE *myFile;
  ArTime myLastStartedSend;
  ArTime myLastCompletedSend;
  std::list<ArFunctor1<int> *> myFileDeletedCallbacks;
  ArFunctor1C<ArClientDeleteFileOnServer, ArNetPacket *> myDeleteFileCB;
};

#endif //ARCLIENTFILEUTILS_H
