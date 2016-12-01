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
#ifndef ARSERVERFILEUTILS_H
#define ARSERVERFILEUTILS_H

#include "Aria.h"
#include "ArServerBase.h"


/// Provides a list of files to clients
/**
 * @linuxonly
 *
   This class is set up so that a client can get file information from
   the robot, this class ONLY lists files, and doesn't do any
   manipulation of them.  You should use this class by using
   ArClientFileLister and NOT by using this classes interface
   directly, this is because the API is and will remain fairly
   volatile, just use ArClientFileLister, if you need more
   functionality let us know and we'll add it if its reasonable.
**/
class ArServerFileLister
{
public:
  /// Constructor
  AREXPORT ArServerFileLister(ArServerBase *server, const char *topDir,
			      const char *defaultUploadDownloadDir = NULL);
  /// Destructor
  AREXPORT virtual ~ArServerFileLister();
  /// The function that gets the directory listing
  AREXPORT void getDirListing(ArServerClient *client,
			      ArNetPacket *packet);
  /// The function that gets the directory listing in a better way
  AREXPORT void getDirListingMultiplePackets(ArServerClient *client,
				      ArNetPacket *packet);
  /// The function that gets the default upload/download dir
  AREXPORT void getDefaultUploadDownloadDir(
	  ArServerClient *client, ArNetPacket *packet);
protected:
  char myBaseDir[2048];
  std::string myDefaultUploadDownloadDir;
  ArServerBase *myServer;
  ArFunctor2C<ArServerFileLister, ArServerClient *, 
      ArNetPacket *> myGetDirListingCB;
  ArFunctor2C<ArServerFileLister, ArServerClient *, 
      ArNetPacket *> myGetDirListingMultiplePacketsCB;
  ArFunctor2C<ArServerFileLister, ArServerClient *, 
      ArNetPacket *> myGetDefaultUploadDownloadDirCB;
};

// ----------------------------------------------------------------------------
// ArServerFileToClient 
// ----------------------------------------------------------------------------

/// Gets files from the server
/**
 * @linuxonly
 *
   This class is set up so that a client can get files the robot, this
   class ONLY gets files.  You should use this class by using
   ArClientFileToClient and NOT by using this classes interface
   directly, this is because the API is and will remain fairly
   volatile... if you need more functionality let us know and we'll
   add it if its reasonable.
**/
class ArServerFileToClient
{
public:
  /// Constructor 
  AREXPORT ArServerFileToClient(ArServerBase *server, const char *topDir);
  /// Destructor
  AREXPORT virtual ~ArServerFileToClient();
  /// Gets the file
  AREXPORT void getFile(ArServerClient *client,
                        ArNetPacket *packet);
  
  /// Gets the file, and sets its timestamp to the original server value
  AREXPORT void getFileWithTimestamp(ArServerClient *client,
                                     ArNetPacket *packet);


protected:

  AREXPORT void doGetFile(ArServerClient *client,
                          ArNetPacket *packet,
                          bool isSetTimestamp);

protected:
  
  char myBaseDir[2048];
  ArServerBase *myServer;
  ArFunctor2C<ArServerFileToClient, 
              ArServerClient *, 
              ArNetPacket *> myGetFileCB;
  
  ArFunctor2C<ArServerFileToClient, 
              ArServerClient *, 
              ArNetPacket *> myGetFileWithTimestampCB;
};

// ----------------------------------------------------------------------------
// ArServerFileToClient 
// ----------------------------------------------------------------------------

/// Puts files onto the server
/**
 * @linuxonly
 *
   This class is set up so that a client can put files on the robot,
   this class ONLY puts files.  This class will take a file from a
   client, writing it into the tempDir given in the constructor, then
   after receiving the file it'll move it over to the actual location
   requested.  You should use this class by using ArClientFileFromClient
   and NOT by using this classes interface directly, this is because
   the API is and will remain fairly volatile... if you need more
   functionality let us know and we'll add it if its reasonable.
**/
class ArServerFileFromClient
{
public:
  /// Constructor 
  AREXPORT ArServerFileFromClient(ArServerBase *server, const char *topDir, 
				  const char *tempDir);
  /// Destructor
  AREXPORT virtual ~ArServerFileFromClient();
  /// Puts the file
  AREXPORT void putFile(ArServerClient *client, 
                        ArNetPacket *packet);
  /// Puts the file with interleaved responses
  AREXPORT void putFileInterleaved(ArServerClient *client, 
                                   ArNetPacket *packet);
  /// Puts the file
  AREXPORT void putFileWithTimestamp(ArServerClient *client, 
                                     ArNetPacket *packet);
  /// Puts the file with interleaved responses
  AREXPORT void putFileWithTimestampInterleaved(ArServerClient *client, 
                                                ArNetPacket *packet);

  /// Adds a callback to be called before moving from temp dir to final loc
  AREXPORT void addPreMoveCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called before moving from temp dir to final loc
  AREXPORT void remPreMoveCallback(ArFunctor *functor);
  /// Adds a callback to be called after moving from temp dir to final loc
  AREXPORT void addPostMoveCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called after moving from temp dir to final loc
  AREXPORT void remPostMoveCallback(ArFunctor *functor);  
  /// Internal call for getting the name of the file we're moving
  const char *getMovingFileName(void) { return myMovingFileName.c_str(); }
protected:

  AREXPORT void internalPutFile(ArServerClient *client, 
                                ArNetPacket *packet,
				                        bool interleaved,
                                bool isSetTimestamp);

  char myBaseDir[2048];
  char myTempDir[2048];
  ArServerBase *myServer;
  std::list<ArFunctor *> myPreMoveCallbacks;
  std::list<ArFunctor *> myPostMoveCallbacks;

  ArFunctor2C<ArServerFileFromClient, ArServerClient *, 
      ArNetPacket *> myPutFileCB;
  ArFunctor2C<ArServerFileFromClient, ArServerClient *, 
      ArNetPacket *> myPutFileInterleavedCB;
  ArFunctor2C<ArServerFileFromClient, ArServerClient *, 
      ArNetPacket *> myPutFileWithTimestampCB;
  ArFunctor2C<ArServerFileFromClient, ArServerClient *, 
      ArNetPacket *> myPutFileWithTimestampInterleavedCB;

  std::string myMovingFileName;

  int myFileNumber;
  
  class FileInfo
  {
  public:
    FileInfo() { myFile = NULL; }
    virtual ~FileInfo() {}
    std::string myRealFileName;
    std::string myTempFileName;
    FILE *myFile;
    ArTime myStartedTransfer;
    ArTime myLastActivity;
    time_t myFileTimestamp;
    // pointer to the client that is sending the file, we can compare
    // this against other client pointers but WE CANNOT ACCESS THE OLD
    // ONE since its possible that it disconnected and is an invalid
    // pointer
    ArServerClient *myClient;
    ArTime myClientCreationTime;
  };

  // map of raw filenames to FileInfo (so we don't have to walk it all the time)
  std::map<std::string, FileInfo *> myMap;
};


/// Deletes files from the server
/**
 * @linuxonly
 *
   This class is set up so that a client can get delete files on the
   robot, this class ONLY deletes files.  You should use this class by
   using ArClientDeleteFileOnServer and NOT by using this classes
   interface directly, this is because the API is and will remain
   fairly volatile... if you need more functionality let us know and
   we'll add it if its reasonable.
**/
class ArServerDeleteFileOnServer
{
public:
  /// Constructor 
  AREXPORT ArServerDeleteFileOnServer(ArServerBase *server, 
				      const char *topDir);
  /// Destructor
  AREXPORT virtual ~ArServerDeleteFileOnServer();
  /// Deletes the file
  AREXPORT void deleteFile(ArServerClient *client,
			   ArNetPacket *packet);
  /// Adds a callback to be called before moving from temp dir to final loc
  AREXPORT void addPreDeleteCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called before moving from temp dir to final loc
  AREXPORT void remPreDeleteCallback(ArFunctor *functor);
  /// Adds a callback to be called after moving from temp dir to final loc
  AREXPORT void addPostDeleteCallback(
	  ArFunctor *functor, ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called after moving from temp dir to final loc
  AREXPORT void remPostDeleteCallback(ArFunctor *functor);  
  /// Gets the filename of the file we're deleting
  const char *getDeletingFileName(void) { return myDeletingFileName.c_str(); }
protected:
  char myBaseDir[2048];
  ArServerBase *myServer;
  ArFunctor2C<ArServerDeleteFileOnServer, ArServerClient *, 
      ArNetPacket *> myDeleteFileCB;
  std::string myDeletingFileName;
  std::list<ArFunctor *> myPreDeleteCallbacks;
  std::list<ArFunctor *> myPostDeleteCallbacks;

};

#endif // ARSERVERFILEUTILS_H
