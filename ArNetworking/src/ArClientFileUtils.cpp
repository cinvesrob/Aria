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
#include "Aria.h"
#include "ArExport.h"
#include "ArClientFileUtils.h"


AREXPORT ArClientFileLister::ArClientFileLister(ArClientBase *client) :
  myGetDirListingCB(this, &ArClientFileLister::netGetDirListing),
  myGetDirListingMultiplePacketsCB(
	  this, 
	  &ArClientFileLister::netGetDirListingMultiplePackets)
{
  myDataMutex.setLogName("ArClientFileLister::myDataMutex");
  myCallbackMutex.setLogName("ArClientFileLister::myCallbackMutex");
  myClient = client;

  if (myClient->dataExists("getDirListingMultiplePackets"))
  {
    ArLog::log(ArLog::Verbose, "ArClientFileLister: Using getDirListingMultiplePackets (new)");
    myClient->addHandler("getDirListingMultiplePackets", 
			 &myGetDirListingMultiplePacketsCB);
  }
  else
  {
    ArLog::log(ArLog::Verbose, "ArClientFileLister: Using getDirListing (old)");
    myClient->addHandler("getDirListing", &myGetDirListingCB);
  }
  myCurrentDir[0] = '\0';
  myWaitingForDir[0] = '\0';
  myLastDirMatched = false;
  myNewDirListing = false;
}

AREXPORT ArClientFileLister::~ArClientFileLister()
{

}

AREXPORT bool ArClientFileLister::isAvailable(void)
{
  return (myClient->dataExists("getDirListingMultiplePackets") || 
	  myClient->dataExists("getDirListing"));
}

void ArClientFileLister::getDirListing(const char *dir)
{
  if (myClient->dataExists("getDirListingMultiplePackets"))
  {
    myClient->requestOnceWithString("getDirListingMultiplePackets", dir);
    myLastDirMatched = false;
    myNewDirListing = true;
  }
  else
  {
    myClient->requestOnceWithString("getDirListing", dir);
  }

}

AREXPORT void ArClientFileLister::changeToTopDir(void)
{
  myDataMutex.lock();
  myCurrentDir[0] = '\0';
  myWaitingForDir[0] = '\0';
  myDataMutex.unlock();
  //myClient->requestOnceWithString("getDirListing", "");
  getDirListing("");

}

AREXPORT void ArClientFileLister::changeToDir(const char *dir)
{
  myDataMutex.lock();
  strcpy(myWaitingForDir, myCurrentDir);
  if (myWaitingForDir[0] != '\0')
    ArUtil::appendSlash(myWaitingForDir, sizeof(myWaitingForDir));
  strncat(myWaitingForDir, dir, 
	  sizeof(myWaitingForDir) - strlen(myWaitingForDir));
  myLastRequested.setToNow();
  //printf("Getting %s\n", myWaitingForDir);
  std::string waitingFor = myWaitingForDir;
  myDataMutex.unlock();
  //myClient->requestOnceWithString("getDirListing", waitingFor.c_str());
  getDirListing(waitingFor.c_str());
}

AREXPORT void ArClientFileLister::changeToAbsDir(const char *dir)
{
  myDataMutex.lock();
  strncpy(myWaitingForDir, dir, sizeof(myWaitingForDir));
  myLastRequested.setToNow();
  //printf("Getting %s\n", myWaitingForDir);
  std::string waitingFor = myWaitingForDir;
  myDataMutex.unlock();
  //myClient->requestOnceWithString("getDirListing", waitingFor.c_str());
  getDirListing(waitingFor.c_str());
}

AREXPORT void ArClientFileLister::upOneDir()
{
  char *str;
  myDataMutex.lock();

  if ( ((str = strrchr(myCurrentDir, '/')) == NULL) && 
	   ((str = strrchr(myCurrentDir, '\\')) == NULL) ) {
	myDataMutex.unlock();
	return changeToTopDir();
  } // end if first level down

  strcpy(myWaitingForDir, myCurrentDir);
  if (myWaitingForDir[0] != '\0')
  {
    ArUtil::appendSlash(myWaitingForDir, sizeof(myWaitingForDir));
    // chop off the last couple slashes (ie last directory)
    if ((str = strrchr(myWaitingForDir, '/')) != NULL || 
	      (str = strrchr(myWaitingForDir, '\\')) != NULL)
      *str = '\0';
    if ((str = strrchr(myWaitingForDir, '/')) != NULL || 
        (str = strrchr(myWaitingForDir, '\\')) != NULL) {
      *str = '\0';
    }
    else {
      myDataMutex.unlock();
  	  return changeToTopDir();
    }
  }
  myLastRequested.setToNow();
  std::string waitingFor = myWaitingForDir;
  myDataMutex.unlock();
  //printf("Getting %s\n", myWaitingForDir);
  //myClient->requestOnceWithString("getDirListing", waitingFor.c_str());
  getDirListing(waitingFor.c_str());

}

AREXPORT const char *ArClientFileLister::getCurrentDir(void) const
{
  return myCurrentDir;
}

AREXPORT const char *ArClientFileLister::getWaitingForDir(void) const
{
  return myWaitingForDir;
}


AREXPORT void ArClientFileLister::netGetDirListing(ArNetPacket *packet)
{
  char name[2048];
  char directory[2048];
  time_t atime;
  time_t mtime;
  ArTypes::UByte4 size = 0;
  unsigned int num = 0;
  unsigned int ret = 0;
  unsigned int i = 0;

//  printf("Got packet...\n");
  myDataMutex.lock();
  ret = packet->bufToUByte2();
  if (ret != 0)
  {
    ArLog::log(ArLog::Normal, 
	       "ArClientFileLister: Bad return for getDirListing of %d", ret);
    myDataMutex.unlock();
    callUpdatedCallbacks(ret);
    return;
  }
  packet->bufToStr(directory, sizeof(directory));
  ArLog::log(ArLog::Verbose, 
	     "ArClientFileLister: Got dir listing for %s", directory);

  // if we got it but it wasn't the directory we were waiting for anymore then just skip
  if (strcmp(myWaitingForDir, directory) != 0)
  {
    ArLog::log(ArLog::Normal, 
	       "ArClientFileLister: Got directory '%s' instead of %s (probably fine)", 
	       directory,
	       myWaitingForDir);
    myDataMutex.unlock();
    callUpdatedCallbacks(-1);
    return;
  }
  // dir was good, remember it and get rid of our old stuff
  strcpy(myCurrentDir, directory);

  myDirectories.clear();
  myFiles.clear();
  num = packet->bufToUByte2();

  for (i = 0; i < num; i++)
  {
    packet->bufToStr(name, sizeof(name));
    atime = packet->bufToUByte4();
    mtime = packet->bufToUByte4();
    size = packet->bufToUByte4();
    myDirectories.push_back(ArClientFileListerItem(name, atime, mtime,
						   size));
  }
  num = packet->bufToUByte2();
  for (i = 0; i < num; i++)
  {
    packet->bufToStr(name, sizeof(name));
    atime = packet->bufToUByte4();
    mtime = packet->bufToUByte4();
    size = packet->bufToUByte4();
    myFiles.push_back(ArClientFileListerItem(name, atime, mtime,
					     size));
  }

  myDataMutex.unlock();
  callUpdatedCallbacks(0);
}

AREXPORT void ArClientFileLister::netGetDirListingMultiplePackets(
	ArNetPacket *packet)
{
  int type;
  char name[2048];
  char directory[2048];
  time_t atime;
  time_t mtime;
  ArTypes::UByte4 size;
  unsigned int num = 0;

  unsigned int ret = 0;
  unsigned int i = 0;
  
  myDataMutex.lock();
  // if the packet is empty we're done with this one...
  if (packet->getDataLength() == 0)
  {
    // if our last dir matched this is the end of a directory listing
    if (myLastDirMatched)
    {
      myNewDirListing = true;

      myDataMutex.unlock();
      callUpdatedCallbacks(0);
    }
    // otherwise its from something we were ignoring
    else
    {
      myDataMutex.unlock();
    }
    return;
  }
  
  ret = packet->bufToUByte2();
  if (ret != 0)
  {
    ArLog::log(ArLog::Normal, 
	       "ArClientFileLister: Bad return for getDirListing of %d", ret);
    myLastDirMatched = false;
    myDataMutex.unlock();
    callUpdatedCallbacks(ret);
    return;
  }
  packet->bufToStr(directory, sizeof(directory));
  // see if this is the dir we want
  if (strcmp(myWaitingForDir, directory) == 0)
  {
    myLastDirMatched = true;
  }
  // if it wasn't the directory we were waiting for anymore then just skip
  else
  {
    ArLog::log(ArLog::Normal, 
	       "ArClientFileLister: Got directory '%s' instead of %s (probably fine)", 
	       directory,
	       myWaitingForDir);
    myLastDirMatched = false;
    myDataMutex.unlock();
    callUpdatedCallbacks(-1);
    return;
  }


  if (myNewDirListing)
  {
    // dir was good, remember it and get rid of our old stuff
    strcpy(myCurrentDir, directory);
    myDirectories.clear();
    myFiles.clear();
    myNewDirListing = false;

  }
  /*** 
  else {
    ArLog::log(ArLog::Verbose, 
	             "ArClientFileLister: Continued dir listing (multiple packets) for %s (%s)", 
               directory, myCurrentDir);

  }
  ***/

  num = packet->bufToUByte2();

  ArLog::log(ArLog::Verbose, 
	           "ArClientFileLister: Got dir listing (multiple packets) for %s (num = %i)", 
             directory, num);


  for (i = 0; i < num; i++)
  {
    type = packet->bufToByte();
    packet->bufToStr(name, sizeof(name));
    atime = packet->bufToUByte4();
    mtime = packet->bufToUByte4();
    size = packet->bufToUByte4();
    if (type == 1)
      myDirectories.push_back(ArClientFileListerItem(name, atime, mtime,
						     size));
    else if (type == 2)
      myFiles.push_back(ArClientFileListerItem(name, atime, mtime,
					       size));
    else
      ArLog::log(ArLog::Normal, 
		 "ArClientFileLister: Got unknown file type (%d) for file %s (ignoring it)", 
		 type, name);
  }



  myDataMutex.unlock();
}

AREXPORT void ArClientFileLister::log(bool withTimes) 
{
  myDataMutex.lock();
  ArLog::log(ArLog::Normal, "In Directory '%s'", getCurrentDir());
  ArLog::log(ArLog::Normal, "%d directories:", myDirectories.size()); 
  logList(&myDirectories, withTimes);
  ArLog::log(ArLog::Normal, "%d Files:", myFiles.size()); 
  logList(&myFiles, withTimes);
  myDataMutex.unlock();
}

AREXPORT void ArClientFileLister::logList(
	std::list<ArClientFileListerItem> *logThis, 
	bool withTimes) 
{
  std::list<ArClientFileListerItem>::const_iterator it;
  char buf[128];  
  unsigned int i;
  time_t itime;

  for (it = logThis->begin(); it != logThis->end(); it++)
  {
    ArLog::log(ArLog::Normal, "\t%s", (*it).getName());
    if (!withTimes)
      continue;
    itime = (*it).getLastAccessedTime();
    strcpy(buf, ctime(&itime));
    // chop the new lines off
    for (i = 0; i < sizeof(buf); i++)
      if (buf[i] == '\r' || buf[i] == '\n')
	buf[i] = '\0';
    ArLog::log(ArLog::Normal, "\t\tlastModified:%s", buf);
	   itime = (*it).getLastModifiedTime();
    strcpy(buf, ctime(&itime));
    // chop the new lines off
    for (i = 0; i < sizeof(buf); i++)
      if (buf[i] == '\r' || buf[i] == '\n')
	buf[i] = '\0';
    ArLog::log(ArLog::Normal, "\t\tlastAccess: %s", buf);
    ArLog::log(ArLog::Normal, "\t\tbytes: %d", (*it).getSize());
  }
}

AREXPORT std::list<ArClientFileListerItem>
ArClientFileLister::getDirectories(void) const
{
  return myDirectories;
}
AREXPORT std::list<ArClientFileListerItem> 
ArClientFileLister::getFiles(void) const
{
  return myFiles;
}


AREXPORT void ArClientFileLister::addUpdatedCallback(ArFunctor1<int> *functor, 
						     ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myUpdatedCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myUpdatedCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "ArClientFileListt::addUpdateCallback: Invalid position.");
  myCallbackMutex.unlock();

}

AREXPORT void ArClientFileLister::remUpdatedCallback(ArFunctor1<int> *functor)
{
  myCallbackMutex.lock();
  myUpdatedCallbacks.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientFileLister::callUpdatedCallbacks(int val)
{
  std::list<ArFunctor1<int> *>::iterator it;

  myCallbackMutex.lock();
  for (it = myUpdatedCallbacks.begin(); it != myUpdatedCallbacks.end(); it++)
    (*it)->invoke(val);
  myCallbackMutex.unlock();
}

AREXPORT ArTime ArClientFileLister::getLastUpdated(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastUpdated;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArTime ArClientFileLister::getLastRequested(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastRequested;
  myDataMutex.unlock();
  return ret;
}

// -----------------------------------------------------------------------------
// ArClientFileToClient
// -----------------------------------------------------------------------------

AREXPORT ArClientFileToClient::ArClientFileToClient(ArClientBase *client) :
  myDataMutex(),
  myCallbackMutex(),
  myClient(client),
  myIsWaitingForFile(false),
  myDirectory(),
  myFileName(),
  myWholeFileName(),
  myClientFileName(),
  myFile(NULL),
  myLastRequested(),
  myLastReceived(),
  myFileReceivedCallbacks(),
  myGetFileCB(this, &ArClientFileToClient::netGetFile),
  myGetFileWithTimestampCB(this, &ArClientFileToClient::netGetFileWithTimestamp)
{
  myDataMutex.setLogName("ArClientFileToClient::myDataMutex");
  myCallbackMutex.setLogName("ArClientFileToClient::myCallbackMutex");

  if (myClient != NULL) {
    myClient->addHandler("getFile", &myGetFileCB);
    myClient->addHandler("getFileWithTimestamp", &myGetFileWithTimestampCB);
  }
}

AREXPORT ArClientFileToClient::~ArClientFileToClient()
{

}

AREXPORT bool ArClientFileToClient::isAvailable(void)
{
  return ((myClient != NULL) &&
          ( myClient->dataExists("getFile") ||
            myClient->dataExists("getFileWithTimestamp") ));
}
AREXPORT bool ArClientFileToClient::isAvailableSetTimestamp(void)
{
  return ((myClient != NULL) &&
          (myClient->dataExists("getFileWithTimestamp")));
}


AREXPORT bool ArClientFileToClient::getFileFromDirectory(const char *directory, 
                                                         const char *fileName, 
                                                         const char *clientFileName,
                                                         bool isSetTimestamp)
{
  myDataMutex.lock();
  if (fileName == NULL || clientFileName == NULL)
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientFileToClient: NULL fileName ('%s') or clientFileName ('%s')", 
	       fileName, clientFileName);
    myDataMutex.unlock();
    return false;
  }
  if (!isAvailable())
  {
    ArLog::log(ArLog::Normal, "ArClientFileToClient::getFileFromDirectory: Tried to get file but the server doesn't support it.");
    return false;
  }

  if (myIsWaitingForFile)
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientFileToClient: already busy downloading a file '%s' cannot download '%s'", 
	       myFileName.c_str(), fileName);
    myDataMutex.unlock();
    return false;
  }
  if (directory != NULL)
    myDirectory = directory;
  else
    myDirectory = "";
  myFileName = fileName;
  myClientFileName = clientFileName;

  char *dirStr = NULL;
  int dirLen;
  if (directory != NULL)
  {
    dirLen = strlen(directory) + 2;
    dirStr = new char[dirLen];
    strncpy(dirStr, directory, dirLen);
    // make sure it has a slash
    ArUtil::appendSlash(dirStr, dirLen);
    // and that the slashes go a consistent direction
    ArUtil::fixSlashes(dirStr, dirLen);
  }

  int fileLen = strlen(fileName) + 1;
  char *fileStr = new char[fileLen];
  strncpy(fileStr, fileName, fileLen);
  // and that the slashes go a consistent direction
  ArUtil::fixSlashes(fileStr, fileLen);

  if (directory == NULL)
    myWholeFileName = "";
  else
    myWholeFileName = dirStr;

  myWholeFileName += fileStr;

  ArNetPacket sendPacket;
  sendPacket.strToBuf(myWholeFileName.c_str());
  sendPacket.uByte2ToBuf(0);

  if (isSetTimestamp && isAvailableSetTimestamp()) {
    myClient->requestOnce("getFileWithTimestamp", &sendPacket);
  } 
  else {
    myClient->requestOnce("getFile", &sendPacket);

    if (isSetTimestamp) {
      ArLog::log(ArLog::Normal,
                 "File timestamps are not available, using getFile");
      // TODO: Special return value?
    }
  }

  myIsWaitingForFile = true;
  myLastRequested.setToNow();
  if (dirStr != NULL)
    delete[] dirStr;
  if (fileStr != NULL)
    delete[] fileStr;    
  myDataMutex.unlock();
  return true;
}

AREXPORT void ArClientFileToClient::netGetFile(ArNetPacket *packet)
{
  doGetFile(packet, false);
}

AREXPORT void ArClientFileToClient::netGetFileWithTimestamp(ArNetPacket *packet)
{
  doGetFile(packet, true);
}

AREXPORT void ArClientFileToClient::doGetFile(ArNetPacket *packet,
                                              bool isSetTimestamp)
{
  char fileName[2048];
  // if its just the empty return packet at the end then don't worry about it
  if (packet->getDataLength() == 0)
    return;

  myDataMutex.lock();
  int ret = packet->bufToUByte2();
  packet->bufToStr(fileName, sizeof(fileName));
  ArUtil::fixSlashes(fileName, sizeof(fileName));
  if (ArUtil::strcasecmp(fileName, myWholeFileName) != 0)
  {
    ArLog::log(ArLog::Normal, 
	       "Got data for a file ('%s') we don't want (we want '%s') (ret %d)",
	       fileName, myWholeFileName.c_str(), ret);
    myDataMutex.unlock();
    return;
  } 
  
  if (ret != 0)
  {
    if (myFile != NULL)
    {
      fclose(myFile);
      unlink(myClientFileName.c_str());
      myFile = NULL;
    }
    ArLog::log(ArLog::Normal, "ArClientFileToClient: Bad return %d on file %s", ret, fileName);
    myIsWaitingForFile = false;
    myLastReceived.setToNow();
    myDataMutex.unlock();    
    callFileReceivedCallbacks(ret);
    return;
  }

  // Otherwise successful get...
 
  if (myFile == NULL)
  {
    ArLog::log(ArLog::Verbose, "Getting file %s", myFileName.c_str());
    if ((myFile = ArUtil::fopen(myClientFileName.c_str(), "wb")) == NULL)
    {
      ArLog::log(ArLog::Normal, "Can't open '%s' to put file into", 
		  myClientFileName.c_str());
      myIsWaitingForFile = false;
      myLastReceived.setToNow();
      myDataMutex.unlock();    
      callFileReceivedCallbacks(-2);
      return;
    }
  }
  
  ArTypes::UByte4 numBytes = 0;
  char buf[32000];
  // file should be good here, so just write into it
  numBytes = packet->bufToUByte4();

  time_t modTime = -1;
  if (isSetTimestamp) {
    modTime = packet->bufToByte4();
  }

  if (numBytes == 0)
  {
    fclose(myFile);


    if (isSetTimestamp) {
      ArUtil::changeFileTimestamp(myClientFileName.c_str(), modTime);
    }

    myFile = NULL;
    myIsWaitingForFile = false;
    ArLog::log(ArLog::Normal, "Received file %s", myFileName.c_str());
    myLastReceived.setToNow();
    myDataMutex.unlock();
    callFileReceivedCallbacks(0);
    return;
  }
  else
  {
    ArLog::log(ArLog::Verbose, "Got %d bytes of file '%s'", 
	       numBytes, myFileName.c_str());
    packet->bufToData(buf, numBytes);
    fwrite(buf, 1, numBytes, myFile);
    myDataMutex.unlock();
  }

}






AREXPORT void ArClientFileToClient::addFileReceivedCallback(
	ArFunctor1<int> *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myFileReceivedCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myFileReceivedCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "ArClientFileToClient::addUpdateCallback: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientFileToClient::remFileReceivedCallback(
	ArFunctor1<int> *functor)
{
  myCallbackMutex.lock();
  myFileReceivedCallbacks.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientFileToClient::callFileReceivedCallbacks(int val)
{
  std::list<ArFunctor1<int> *>::iterator it;
  myCallbackMutex.lock();
  for (it = myFileReceivedCallbacks.begin(); it != myFileReceivedCallbacks.end(); it++)
    (*it)->invoke(val);
  myCallbackMutex.unlock();
}

AREXPORT const char *ArClientFileToClient::getDirectory(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myDirectory;
  myDataMutex.unlock();
  return ret.c_str();
}

AREXPORT const char *ArClientFileToClient::getFileName(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myFileName;
  myDataMutex.unlock();
  return ret.c_str();
}

AREXPORT const char *ArClientFileToClient::getClientFileName(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myClientFileName;
  myDataMutex.unlock();
  return ret.c_str();
}
  
AREXPORT void ArClientFileToClient::cancelGet(void)
{
  // Removed the following line because it causes the received packets
  // to be dropped, and the file getter remains permanently in the 
  // waiting for file state.
  // myWholeFileName = "";
  // TODO!
}

AREXPORT bool ArClientFileToClient::isWaitingForFile(void) 
{
  bool ret;
  myDataMutex.lock();
  ret = myIsWaitingForFile;
  myDataMutex.unlock();
  return ret;
}
  

AREXPORT ArTime ArClientFileToClient::getLastReceived(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastReceived;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArTime ArClientFileToClient::getLastRequested(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastRequested;
  myDataMutex.unlock();
  return ret;
}


AREXPORT ArClientFileFromClient::ArClientFileFromClient(ArClientBase *client) :
  myDataMutex(),
  myCallbackMutex(),
  myClient(client),
  myIsWaitingForReturn(false),
  myInterleaved(false),
  myTimestamp(false),
  myCommandName(),
  myDirectory(),
  myFileName(),
  myWholeFileName(),
  myClientFileName(),
  myReadyForNextPacket(false),
  myFile(),
  myLastStartedSend(),
  myLastCompletedSend(),
  myFileSentCallbacks(),
  myPutFileCB(this, &ArClientFileFromClient::netPutFile)
{
  myDataMutex.setLogName("ArClientFileFromClient::myDataMutex");
  myCallbackMutex.setLogName("ArClientFileFromClient::myCallbackMutex");
  
  if (myClient != NULL) {  
    myClient->addHandler("putFile", &myPutFileCB);
    myClient->addHandler("putFileInterleaved", &myPutFileCB);
    myClient->addHandler("putFileWithTimestamp", &myPutFileCB);
    myClient->addHandler("putFileWithTimestampInterleaved", &myPutFileCB);
  }
}


AREXPORT ArClientFileFromClient::~ArClientFileFromClient()
{

}

AREXPORT bool ArClientFileFromClient::isAvailable(void)
{
  return (myClient->dataExists("putFile") || 
	        myClient->dataExists("putFileInterleaved") ||
          myClient->dataExists("putFileWithTimestamp") ||
          myClient->dataExists("putFileWithTimestampInterleaved"));
}

AREXPORT bool ArClientFileFromClient::isAvailableSetTimestamp()
{
    return((myClient != NULL) && ( myClient->dataExists("putFileWithTimestamp") || myClient->dataExists("putFileWithTimestampInterleaved") ) );
}

AREXPORT bool ArClientFileFromClient::isAvailableSlow(void)
{
  return myClient->dataExists("putFileInterleaved");
}

AREXPORT bool ArClientFileFromClient::isAvailableFast(void)
{
  return myClient->dataExists("putFile");
}

AREXPORT bool ArClientFileFromClient::putFileToDirectory
                                              (const char *directory, 
                                               const char *fileName, 
                                               const char *clientFileName, 
                                               SendSpeed sendSpeed,
                                               bool isSetTimestamp)
{
  bool interleaved;

  myDataMutex.lock();
  if (fileName == NULL || fileName[0] == '\0' || 
      clientFileName == NULL || clientFileName[0] == '\0')
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientFileFromClient: NULL or empty fileName ('%s') or clientFileName ('%s')", 
	       fileName, clientFileName);
    myDataMutex.unlock();
    return false;
  }
  if (myIsWaitingForReturn)
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientFileFromClient: already busy uploading a file '%s' cannot upload '%s'", 
	       myFileName.c_str(), fileName);
    myDataMutex.unlock();
    return false;
  }

  myCommandName = "";

  if (!isSetTimestamp) {

    if (myClient->dataExists("putFile") &&
        (sendSpeed == SPEED_FAST || sendSpeed == SPEED_AUTO))
    {
      myInterleaved = false;
      myTimestamp = false;
      myCommandName = "putFile";
    }
    else if (myClient->dataExists("putFileInterleaved") && 
             (sendSpeed == SPEED_SLOW || sendSpeed == SPEED_AUTO))
    {
      myInterleaved = true;
      myTimestamp = false;
      myReadyForNextPacket = false;
      myCommandName = "putFileInterleaved";
    }
  }
  else {  // is set timestamp

    if (myClient->dataExists("putFileWithTimestamp") &&
        (sendSpeed == SPEED_FAST || sendSpeed == SPEED_AUTO))
    {
      myInterleaved = false;
      myTimestamp = true;
      myCommandName = "putFileWithTimestamp";
    }
    else if (myClient->dataExists("putFileWithTimestampInterleaved") && 
             (sendSpeed == SPEED_SLOW || sendSpeed == SPEED_AUTO))
    {
      myInterleaved = true;
      myTimestamp = true;
      myReadyForNextPacket = false;
      myCommandName = "putFileWithTimestampInterleaved";
    }

  } // end else set timestamp

  if (ArUtil::isStrEmpty(myCommandName.c_str())) {

    ArLog::log(ArLog::Normal, "ArClientFileFromClient::putFileToDirectory: Tried to put file but the server doesn't support it (or doesn't support it at the speed requested).");
    myDataMutex.unlock();
    return false;
  }
    
  interleaved = myInterleaved;

  if (directory != NULL)
    myDirectory = directory;
  else
    myDirectory = "";
  myFileName = fileName;
  myClientFileName = clientFileName;

  char *dirStr = NULL;
  int dirLen;
  if (directory != NULL)
  {
    dirLen = strlen(directory) + 2;
    dirStr = new char[dirLen];
    strncpy(dirStr, directory, dirLen);
    // make sure it has a slash
    ArUtil::appendSlash(dirStr, dirLen);
    // and that the slashes go a consistent direction
    ArUtil::fixSlashes(dirStr, dirLen);
  }

  int fileLen = strlen(fileName) + 1;
  char *fileStr = new char[fileLen];
  strncpy(fileStr, fileName, fileLen);
  // and that the slashes go a consistent direction
  ArUtil::fixSlashes(fileStr, fileLen);

  if (directory == NULL)
    myWholeFileName = "";
  else
    myWholeFileName = dirStr;

  myWholeFileName += fileStr;

  myIsWaitingForReturn = true;
  myLastStartedSend.setToNow();
  if (dirStr != NULL)
    delete[] dirStr;
  if (fileStr != NULL)
    delete[] fileStr;    

  ArNetPacket sendPacket;

  FILE *file;
  if ((file = ArUtil::fopen(myClientFileName.c_str(), "rb")) == NULL)
  {
    ArLog::log(ArLog::Normal, 
      "ArClientFileFromClient::putFile: can't open file '%s'", 
      clientFileName);
    myIsWaitingForReturn = false;
    myDataMutex.unlock();
    return false;
  }
  myDataMutex.unlock();
  // tell the server we're sending

  sendPacket.empty();
  sendPacket.uByte2ToBuf(0);
  sendPacket.strToBuf(myWholeFileName.c_str());

  if (myTimestamp) {

    struct stat fileStat;
    time_t timestamp;

    if (stat(myWholeFileName.c_str(), &fileStat) == 0) {
      timestamp = fileStat.st_mtime;
    }
    else {
      time(&timestamp);
    }

    sendPacket.byte4ToBuf(timestamp);

  } // end if timestamp

  myClient->requestOnce(myCommandName.c_str(), &sendPacket);
  ArLog::log(ArLog::Normal, "Starting send of file %s", 
    myWholeFileName.c_str());

  char buf[30000];
  size_t ret;

  ArTime started;
  started.setToNow();

  // now send the file
  while ((ret = fread(buf, 1, sizeof(buf), file)) == sizeof(buf))
  {
    // if we're interleaved wait for the next packet
    if (interleaved)
    {
      myDataMutex.lock();

      // myReadyForNextPacket is set to true when the response is received
      // from the server.
      while (!myReadyForNextPacket) 
      {
        if (!myIsWaitingForReturn)
        {
          ArLog::log(ArLog::Normal, 
            "ArFileFromClient::putFileToDirectory: Put was cancelled or failed.");
          myDataMutex.unlock();
          return false;
        }
        myDataMutex.unlock();
        ArUtil::sleep(1);

        if (started.secSince() > 30)
        {
          myDataMutex.lock();
          myIsWaitingForReturn = false;
          myDataMutex.unlock();

          ArLog::log(ArLog::Normal, 
            "ArFileFromClient::putFileToDirectory: No return from client within 30 seconds, failing put.");	 
          return false;
        }
        myDataMutex.lock();

      } // end while not ready for next packet

      // Reset the flag so we'll wait for a response during the next loop iteration.
      myReadyForNextPacket = false;

      myDataMutex.unlock();

    } // end if interleaved

    // Reset the time this packet was sent.
    started.setToNow();

    sendPacket.empty();
    sendPacket.uByte2ToBuf(1);
    sendPacket.strToBuf(myWholeFileName.c_str());
    sendPacket.uByte4ToBuf(ret);
    sendPacket.dataToBuf(buf, ret);
    myClient->requestOnce(myCommandName.c_str(), &sendPacket);
    //ArLog::log(ArLog::Normal, "Sent packet with %d", ret);

  } // end while more to read... 

  if (feof(file))
  {
    //printf("end of file\n");
  }
  if (ferror(file))
  {
    ArLog::log(ArLog::Normal, "ArServerFileFromClient: Error sending file %s", 
      fileName);
    sendPacket.empty();
    sendPacket.uByte2ToBuf(3);
    sendPacket.strToBuf(myWholeFileName.c_str());
    myClient->requestOnce(myCommandName.c_str(), &sendPacket);
    myDataMutex.lock();
    myIsWaitingForReturn = false;
    myDataMutex.unlock();
    return false;
  }

  sendPacket.empty();
  sendPacket.uByte2ToBuf(1);
  sendPacket.strToBuf(myWholeFileName.c_str());
  sendPacket.uByte4ToBuf(ret);
  sendPacket.dataToBuf(buf, ret);
  myClient->requestOnce(myCommandName.c_str(), &sendPacket);
  //ArLog::log(ArLog::Verbose, "Sent packet with %d", ret);


  sendPacket.empty();
  sendPacket.uByte2ToBuf(2);
  sendPacket.strToBuf(myWholeFileName.c_str());
  myClient->requestOnce(myCommandName.c_str(), &sendPacket);

  if (feof(file))
  {
    ArLog::log(ArLog::Normal, "ArServerFileToClient: Sent file %s", fileName);
  }

  fclose(file);
  return true;
}

AREXPORT void ArClientFileFromClient::netPutFile(ArNetPacket *packet)
{
  int ret = 0;
  bool done = false;
  char fileName[2048];
  
  myDataMutex.lock();
  if (!myIsWaitingForReturn)
  {
    myDataMutex.unlock();
    return;
  }
  ret = packet->bufToUByte2();
  packet->bufToStr(fileName, sizeof(fileName));
  if (myInterleaved && ret == 10)
  {
    done = false;
    myReadyForNextPacket = true;
  }
  else
  {
    done = true;
    myIsWaitingForReturn = false;
  }
  myDataMutex.unlock();
  if (done)
    callFileSentCallbacks(ret);
}


AREXPORT void ArClientFileFromClient::addFileSentCallback(
	ArFunctor1<int> *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myFileSentCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myFileSentCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "ArClientFileFromClient::addUpdateCallback: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientFileFromClient::remFileSentCallback(
	ArFunctor1<int> *functor)
{
  myCallbackMutex.lock();
  myFileSentCallbacks.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientFileFromClient::callFileSentCallbacks(int val)
{
  std::list<ArFunctor1<int> *>::iterator it;
  myCallbackMutex.lock();
  for (it = myFileSentCallbacks.begin(); it != myFileSentCallbacks.end(); it++)
    (*it)->invoke(val);
  myCallbackMutex.unlock();
}

AREXPORT const char *ArClientFileFromClient::getDirectory(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myDirectory;
  myDataMutex.unlock();
  return ret.c_str();
}

AREXPORT const char *ArClientFileFromClient::getFileName(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myFileName;
  myDataMutex.unlock();
  return ret.c_str();
}

AREXPORT const char *ArClientFileFromClient::getClientFileName(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myClientFileName;
  myDataMutex.unlock();
  return ret.c_str();
}
  
AREXPORT void ArClientFileFromClient::cancelPut(void)
{
  ArNetPacket sendPacket;

  myDataMutex.lock();
  if (myIsWaitingForReturn)
  {
    myIsWaitingForReturn = false;
    
    sendPacket.uByte2ToBuf(3);
    sendPacket.strToBuf(myWholeFileName.c_str());
    myClient->requestOnce(myCommandName.c_str(), &sendPacket);
  }
  myDataMutex.unlock();
}

AREXPORT bool ArClientFileFromClient::isWaitingForReturn(void) 
{
  bool ret;
  myDataMutex.lock();
  ret = myIsWaitingForReturn;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArTime ArClientFileFromClient::getLastCompletedSend(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastCompletedSend;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArTime ArClientFileFromClient::getLastStartedSend(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastStartedSend;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArClientDeleteFileOnServer::ArClientDeleteFileOnServer(
	ArClientBase *client) :
  myDeleteFileCB(this, &ArClientDeleteFileOnServer::netDeleteFile)
{
  myDataMutex.setLogName("ArClientDeleteFileOnServer::myDataMutex");
  myCallbackMutex.setLogName("ArClientDeleteFileOnServer::myCallbackMutex");
  myClient = client;
  myClient->addHandler("deleteFile", &myDeleteFileCB);
  myIsWaitingForReturn = false;
}

AREXPORT ArClientDeleteFileOnServer::~ArClientDeleteFileOnServer()
{

}

AREXPORT bool ArClientDeleteFileOnServer::isAvailable(void)
{
  return myClient->dataExists("deleteFile");
}

AREXPORT bool ArClientDeleteFileOnServer::deleteFileFromDirectory(
	const char *directory, const char *fileName)
{
  myDataMutex.lock();
  if (fileName == NULL || fileName[0] == '\0')
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientDeleteFileOnServer: NULL or empty fileName ('%s')", 
	       fileName);
    myDataMutex.unlock();
    return false;
  }
  if (!isAvailable())
  {
    ArLog::log(ArLog::Normal, "ArClientDeleteFileOnServer::deleteFileFromDirectory: Tried to delete file but the server doesn't support it.");
    return false;
  }

  if (myIsWaitingForReturn)
  {
    ArLog::log(ArLog::Terse, 
	       "ArClientDeleteFileOnServer: already busy deleting a file '%s' cannot delete '%s'", 
	       myFileName.c_str(), fileName);
    myDataMutex.unlock();
    return false;
  }
  if (directory != NULL)
    myDirectory = directory;
  else
    myDirectory = "";
  myFileName = fileName;

  char *dirStr = NULL;
  int dirLen;
  if (directory != NULL)
  {
    dirLen = strlen(directory) + 2;
    dirStr = new char[dirLen];
    strncpy(dirStr, directory, dirLen);
    // make sure it has a slash
    ArUtil::appendSlash(dirStr, dirLen);
    // and that the slashes go a consistent direction
    ArUtil::fixSlashes(dirStr, dirLen);
  }

  int fileLen = strlen(fileName) + 1;
  char *fileStr = new char[fileLen];
  strncpy(fileStr, fileName, fileLen);
  // and that the slashes go a consistent direction
  ArUtil::fixSlashes(fileStr, fileLen);

  if (directory == NULL)
    myWholeFileName = "";
  else
    myWholeFileName = dirStr;

  myWholeFileName += fileStr;

  myIsWaitingForReturn = true;
  myLastStartedSend.setToNow();
  if (dirStr != NULL)
    delete[] dirStr;
  if (fileStr != NULL)
    delete[] fileStr;    

  ArNetPacket sendPacket;

  myDataMutex.unlock();
  // tell the server to delete it
  
  sendPacket.empty();
  sendPacket.strToBuf(myWholeFileName.c_str());
  myClient->requestOnce("deleteFile", &sendPacket);
  ArLog::log(ArLog::Normal, "Requested delete of file %s", 
	     myWholeFileName.c_str());
  return true;
}

AREXPORT void ArClientDeleteFileOnServer::netDeleteFile(ArNetPacket *packet)
{
  int ret;
  char fileName[2048];

  myDataMutex.lock();
  if (!myIsWaitingForReturn)
  {
    myDataMutex.unlock();
    return;
  }
  ret = packet->bufToUByte2();
  packet->bufToStr(fileName, sizeof(fileName));
  //printf("Ret of '%s' is %d\n", fileName, ret);
  myIsWaitingForReturn = 0;
  myDataMutex.unlock();
  callFileDeletedCallbacks(ret);
}


AREXPORT void ArClientDeleteFileOnServer::addFileDeletedCallback(
	ArFunctor1<int> *functor, ArListPos::Pos position)
{
  myCallbackMutex.lock();
  if (position == ArListPos::FIRST)
    myFileDeletedCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myFileDeletedCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
               "ArClientDeleteFileOnServer::addUpdateCallback: Invalid position.");
  myCallbackMutex.unlock();
}

AREXPORT void ArClientDeleteFileOnServer::remFileDeletedCallback(
	ArFunctor1<int> *functor)
{
  myCallbackMutex.lock();
  myFileDeletedCallbacks.remove(functor);
  myCallbackMutex.unlock();
}

AREXPORT void ArClientDeleteFileOnServer::callFileDeletedCallbacks(int val)
{
  std::list<ArFunctor1<int> *>::iterator it;

  myCallbackMutex.lock();
  for (it = myFileDeletedCallbacks.begin(); 
       it != myFileDeletedCallbacks.end(); 
       it++)
    (*it)->invoke(val);
  myCallbackMutex.unlock();
}

AREXPORT const char *ArClientDeleteFileOnServer::getDirectory(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myDirectory;
  myDataMutex.unlock();
  return ret.c_str();
}

AREXPORT const char *ArClientDeleteFileOnServer::getFileName(void) 
{
  std::string ret;
  myDataMutex.lock();
  ret = myFileName;
  myDataMutex.unlock();
  return ret.c_str();
}

AREXPORT bool ArClientDeleteFileOnServer::isWaitingForReturn(void) 
{
  bool ret;
  myDataMutex.lock();
  ret = myIsWaitingForReturn;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArTime ArClientDeleteFileOnServer::getLastCompletedSend(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastCompletedSend;
  myDataMutex.unlock();
  return ret;
}

AREXPORT ArTime ArClientDeleteFileOnServer::getLastStartedSend(void)
{
  ArTime ret;
  myDataMutex.lock();
  ret = myLastStartedSend;
  myDataMutex.unlock();
  return ret;
}

