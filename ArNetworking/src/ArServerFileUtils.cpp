#ifndef WIN32
#include "Aria.h"
#include "ArExport.h"
#include "ArServerFileUtils.h"

#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>


AREXPORT ArServerFileLister::ArServerFileLister(
	ArServerBase *server, const char *topDir,
	const char *defaultUploadDownloadDir) :
  myGetDirListingCB(this, &ArServerFileLister::getDirListing),
  myGetDirListingMultiplePacketsCB(this, &ArServerFileLister::getDirListingMultiplePackets),
  myGetDefaultUploadDownloadDirCB(
	  this, 
	  &ArServerFileLister::getDefaultUploadDownloadDir)
{
  myServer = server;
  myServer->addData("getDirListing", 
		    "Gets the directory listing of a given directory (from the current working directory), you should probably use a class from ArClientFileUtils instead of calling this directly",
		    &myGetDirListingCB, "string: directory to get listing of",
		    "ubyte2: return code, 0 = good, 1 = tried to go outside allowed area, 2 = no such directory (or can't read); string: directoryListed;  IF return was 0 then ubyte2: numDirectories; repeating numDirectories (string: name; ubyte4: access_time; ubyte4: modified_time); ubyte2: numFiles; repeating numFiles (string: name; ubyte4: access_time; ubyte4: modified_time);", 
		    "FileAccess", "RETURN_SINGLE|SLOW_PACKET");

  myServer->addData("getDirListingMultiplePackets", 
		    "Gets the directory listing of a given directory possibly broken up into multiple packets (from the current working directory), you should probably use a class from ArClientFileUtils instead of calling this directly",
		    &myGetDirListingMultiplePacketsCB, 
		    "string: directory to get listing of",
		    "ubyte2: return code, 0 = good, 1 = tried to go outside allowed area, 2 = no such directory (or can't read); string: directoryListed;  IF return was 0 then ubyte2: numEntries; repeating numEntries (byte: type (1 for dir or 2 for file) string: name; ubyte4: access_time; ubyte4: modified_time)", 
		    "FileAccess", "RETURN_UNTIL_EMPTY|SLOW_PACKET");

  if (defaultUploadDownloadDir == NULL)
    myDefaultUploadDownloadDir = "";
  else
    myDefaultUploadDownloadDir = defaultUploadDownloadDir;

  if (!myDefaultUploadDownloadDir.empty())
    myServer->addData("getDefaultUploadDownloadDir", 
      "Gets the default directory that should be used for upload/download",
		      &myGetDefaultUploadDownloadDirCB, "none",
		      "string: defaultUploadDownloadDir",
		      "FileAccess", "RETURN_SINGLE|SLOW_PACKET");


  // snag our base dir and make sure we have enough room for a /
  strncpy(myBaseDir, topDir, sizeof(myBaseDir) - 2);
  myBaseDir[sizeof(myBaseDir) - 2] = '\0';
  // make sure it has a slash
  ArUtil::appendSlash(myBaseDir, sizeof(myBaseDir));
  // make sure the slashes go the right direction
  ArUtil::fixSlashes(myBaseDir, sizeof(myBaseDir));  
}

AREXPORT ArServerFileLister::~ArServerFileLister()
{

}

AREXPORT void ArServerFileLister::getDirListing(ArServerClient *client,
						ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  size_t ui;
  size_t len;

  bool printing = false;

  char directory[2048];
  packet->bufToStr(directory, sizeof(directory));
  
  if (printing)
    printf("'%s' requested\n", directory);
  len = strlen(directory);
  // first advance to the first non space
  for (ui = 0; 
       ui < len && directory[ui] != '\0' && isspace(directory[ui]); 
       ui++);

  char *dirStr = new char[len + 3];

  // now copy in the rest
  strncpy(dirStr, directory, len - ui);
  // make sure its null terminated
  dirStr[len - ui] = '\0';
    
  if (dirStr[0] == '/' || dirStr[0] == '~' ||
      dirStr[0] == '\\' || strstr(dirStr, "..") != NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerFileLister: '%s' tried to access outside allowed area", dirStr);
    delete[] dirStr;
    sendPacket.uByte2ToBuf(1);
    // put in the directory name
    sendPacket.strToBuf(directory);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    return;
  }

  // if its not empty make sure its set up right
  if (strlen(dirStr) > 0)
  {
    // make sure it has a slash
    ArUtil::appendSlash(dirStr, len + 2);
    // make sure the slashes go the right direction
    ArUtil::fixSlashes(dirStr, len + 2);
  }

  // put our base and where we want to go together
  std::string wholeDir;
  wholeDir = myBaseDir;
  wholeDir += dirStr;

  DIR *dir;
  struct dirent *ent;
  struct stat statBuf;
  std::set<std::string, ArStrCaseCmpOp> dirs;
  std::map<std::string, ArTypes::UByte4> dirToATime;
  std::map<std::string, ArTypes::UByte4> dirToMTime;
  std::map<std::string, ArTypes::UByte4> dirToSize;
  std::set<std::string, ArStrCaseCmpOp> files;
  std::map<std::string, ArTypes::UByte4> fileToATime;
  std::map<std::string, ArTypes::UByte4> fileToMTime;
  std::map<std::string, ArTypes::UByte4> fileToSize;
  std::set<std::string, ArStrCaseCmpOp>::iterator it;
  std::string str;
  
  if ((dir = opendir(wholeDir.c_str())) == NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerFileLister: No such directory '%s' from base '%s' plus directory '%s'", 
	       directory, myBaseDir, dirStr);
    delete[] dirStr;
    sendPacket.uByte2ToBuf(2);
    // put in the directory name
    sendPacket.strToBuf(directory);
    if (client != NULL)
      client->sendPacketTcp(&sendPacket);
    return;
  }
  while ((ent = readdir(dir)) != NULL)
  {
    // this works because if the first one goes it short circuits the second
    if ((it = dirs.find(ent->d_name)) != dirs.end() || 
	(it = files.find(ent->d_name)) != files.end())
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerFileLister: %s duplicates '%s'", 
		 ent->d_name, (*it).c_str());
      continue;
    }      
    if (ent->d_name[0] == '.')
      continue;
    str = wholeDir.c_str();
    str += ent->d_name;
    if (printing)
      printf("name %s\n", str.c_str());
    if (stat(str.c_str(), &statBuf) == 0)
    {
      if (S_ISREG(statBuf.st_mode))
      {
	files.insert(ent->d_name);
	fileToATime[ent->d_name] = statBuf.st_atime;
	fileToMTime[ent->d_name] = statBuf.st_mtime;
	fileToSize[ent->d_name] = statBuf.st_size;
	//printf("File %s\n", ent.d_name);
      }
      if (S_ISDIR(statBuf.st_mode))
      {
	dirs.insert(ent->d_name);
	dirToATime[ent->d_name] = statBuf.st_atime;
	dirToMTime[ent->d_name] = statBuf.st_mtime;
	dirToSize[ent->d_name] = statBuf.st_size;
	//printf("Dir %s\n", ent.d_name);
      }
    }
    else
    {
      ArLog::log(ArLog::Normal, "Cannot stat %s in %s", ent->d_name, wholeDir.c_str());
    }
  }
  // we got here so the return is 0 (good)
  sendPacket.uByte2ToBuf(0);
  // put in the directory name
  sendPacket.strToBuf(directory);
  if (printing)
    printf("Sending %s\n", directory);
  if (printing)
    printf("Dirs:\n");
  // put in how many directories we have
  sendPacket.uByte2ToBuf(dirs.size());
  for (it = dirs.begin(); it != dirs.end(); it++)
  {
    sendPacket.strToBuf((*it).c_str());
    sendPacket.uByte4ToBuf(dirToATime[(*it)]);
    sendPacket.uByte4ToBuf(dirToMTime[(*it)]);
    sendPacket.uByte4ToBuf(dirToSize[(*it)]);
  }
  
  if (printing)
  {
    printf("\n");
    printf("Files:\n");
  }
  sendPacket.uByte2ToBuf(files.size());
  for (it = files.begin(); it != files.end(); it++)
  {
    sendPacket.strToBuf((*it).c_str());
    sendPacket.uByte4ToBuf(fileToATime[(*it)]);
    sendPacket.uByte4ToBuf(fileToMTime[(*it)]);
    sendPacket.uByte4ToBuf(fileToSize[(*it)]);
    if (printing)
    {
      time_t atime = fileToATime[(*it)];
      time_t mtime = fileToMTime[(*it)];
      printf("\tname: %s\n", (*it).c_str());
      printf("\t\tlastAccessed: %s", ctime(&atime));
      printf("\t\tlastModified: %s", ctime(&mtime));
      printf("\t\tsize: %d", fileToSize[(*it)]);
    }
  }
  delete[] dirStr;
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);
  closedir(dir);
}

AREXPORT void ArServerFileLister::getDirListingMultiplePackets(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  size_t ui;
  size_t len;

  bool printing = false;

  char directory[2048];
  packet->bufToStr(directory, sizeof(directory));
  
  if (printing)
    printf("'%s' requested\n", directory);
  len = strlen(directory);
  // first advance to the first non space
  for (ui = 0; 
       ui < len && directory[ui] != '\0' && isspace(directory[ui]); 
       ui++);

  char *dirStr = new char[len + 3];

  // now copy in the rest
  strncpy(dirStr, directory, len - ui);
  // make sure its null terminated
  dirStr[len - ui] = '\0';
    
  if (dirStr[0] == '/' || dirStr[0] == '~' ||
      dirStr[0] == '\\' || strstr(dirStr, "..") != NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerFileLister: '%s' tried to access outside allowed area", dirStr);
    delete[] dirStr;
    sendPacket.uByte2ToBuf(1);
    // put in the directory name
    sendPacket.strToBuf(directory);
    if (client != NULL)
    {
      client->sendPacketTcp(&sendPacket);
      sendPacket.empty();
      client->sendPacketTcp(&sendPacket);
    }
    return;
  }

  // if its not empty make sure its set up right
  if (strlen(dirStr) > 0)
  {
    // make sure it has a slash
    ArUtil::appendSlash(dirStr, len + 2);
    // make sure the slashes go the right direction
    ArUtil::fixSlashes(dirStr, len + 2);
  }

  // put our base and where we want to go together
  std::string wholeDir;
  wholeDir = myBaseDir;
  wholeDir += dirStr;

  DIR *dir;
  struct dirent *ent;
  struct stat statBuf;
  std::set<std::string, ArStrCaseCmpOp> dirs;
  std::map<std::string, ArTypes::UByte4> dirToATime;
  std::map<std::string, ArTypes::UByte4> dirToMTime;
  std::map<std::string, ArTypes::UByte4> dirToSize;
  std::set<std::string, ArStrCaseCmpOp> files;
  std::map<std::string, ArTypes::UByte4> fileToATime;
  std::map<std::string, ArTypes::UByte4> fileToMTime;
  std::map<std::string, ArTypes::UByte4> fileToSize;
  std::set<std::string, ArStrCaseCmpOp>::iterator it;
  std::string str;
  ArTypes::UByte2 numEntries = 0;
  int type;

  if ((dir = opendir(wholeDir.c_str())) == NULL)
  {
    ArLog::log(ArLog::Normal, "ArServerFileLister: No such directory '%s' from base '%s' plus directory '%s'", 
	       directory, myBaseDir, dirStr);
    delete[] dirStr;
    sendPacket.uByte2ToBuf(2);
    // put in the directory name
    sendPacket.strToBuf(directory);
    if (client != NULL)
    {
      client->sendPacketTcp(&sendPacket);
      sendPacket.empty();
      client->sendPacketTcp(&sendPacket);
    }
    return;
  }

  // we got here so the return is 0 (good)
  sendPacket.uByte2ToBuf(0);
  // put in the directory name
  sendPacket.strToBuf(directory);

  ArTypes::UByte2 numEntriesLen = sendPacket.getLength();
  ArTypes::UByte2 realLen;
  // put in a placeholder for how many directories we have
  sendPacket.uByte2ToBuf(0);

  while ((ent = readdir(dir)) != NULL)
  {
    // this works because if the first one goes it short circuits the second
    if ((it = dirs.find(ent->d_name)) != dirs.end() || 
	(it = files.find(ent->d_name)) != files.end())
    {
      ArLog::log(ArLog::Normal, 
		 "ArServerFileLister: %s duplicates '%s'", 
		 ent->d_name, (*it).c_str());
      continue;
    }      
    if (ent->d_name[0] == '.')
      continue;
    str = wholeDir.c_str();
    str += ent->d_name;
    if (printing)
      printf("name %s\n", str.c_str());
    if (stat(str.c_str(), &statBuf) == 0)
    {
      if (S_ISREG(statBuf.st_mode))
	type = 2;
      else if (S_ISDIR(statBuf.st_mode))
	type = 1;
      else
	type = 0;

      if (type == 0)
	continue;

      if (sendPacket.getDataLength() + strlen(ent->d_name) + 12 >= 
	  ArNetPacket::MAX_DATA_LENGTH)
      {
	// put in the number of entries in that packet
	realLen = sendPacket.getLength();
	sendPacket.setLength(numEntriesLen);
	sendPacket.uByte2ToBuf(numEntries);
	sendPacket.setLength(realLen);

	// and send it
	if (client != NULL)
	  client->sendPacketTcp(&sendPacket);	

	sendPacket.empty();
	// then rebuild the start of it
	sendPacket.uByte2ToBuf(0);
	// put in the directory name
	sendPacket.strToBuf(directory);
	
	numEntriesLen = sendPacket.getLength();
	// put in a placeholder for how many directories we have
	sendPacket.uByte2ToBuf(0);
	
	// and reset our counter
	numEntries = 0;
      }
      numEntries++;
      sendPacket.byteToBuf(type);
      sendPacket.strToBuf(ent->d_name);
      sendPacket.uByte4ToBuf(statBuf.st_atime);
      sendPacket.uByte4ToBuf(statBuf.st_mtime);
      sendPacket.uByte4ToBuf(statBuf.st_size);
    }
    else
    {
      ArLog::log(ArLog::Normal, "Cannot stat %s in %s", ent->d_name, wholeDir.c_str());
    }
  }


  // put in the number of entries in that packet
  realLen = sendPacket.getLength();
  sendPacket.setLength(numEntriesLen);
  sendPacket.uByte2ToBuf(numEntries);
  sendPacket.setLength(realLen);
  
  // and send it
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);	

  delete[] dirStr;

  // then send our empty packet to say we're done
  sendPacket.empty();
  if (client != NULL)
    client->sendPacketTcp(&sendPacket);

  closedir(dir);
}

AREXPORT void ArServerFileLister::getDefaultUploadDownloadDir(
	ArServerClient *client, ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  sendPacket.strToBuf(myDefaultUploadDownloadDir.c_str());
  client->sendPacketTcp(&sendPacket);
}



// -----------------------------------------------------------------------------
// ArServerFileToClient
// -----------------------------------------------------------------------------


AREXPORT ArServerFileToClient::ArServerFileToClient(ArServerBase *server, 
						                                        const char *topDir) :
  myGetFileCB(this, &ArServerFileToClient::getFile),
  myGetFileWithTimestampCB(this, &ArServerFileToClient::getFileWithTimestamp)
{
  myServer = server;
  myServer->addData("getFile", 
		    "Gets a file (use ArClientFileToClient instead of calling this directly since this interface may change)",
		    &myGetFileCB, "string: file to get; byte2: operation, 0 == get, 1 == cancel (these aren't implemented yet but will be)",
		    "ubyte2: return code, 0 = good (sending file), 1 = tried to go outside allowed area, 2 = no such file (or can't read), 3 = empty file name, 4 = error reading file (can happen after some good values) ; string: fileGotten;IF return was 0 then  ubyte4: numBytes (number of bytes in this packet, 0 means end of file; numBytes of data", 
		    "FileAccess", "RETURN_UNTIL_EMPTY|SLOW_PACKET");
  
  myServer->addData("getFileWithTimestamp", 
		    "Gets a file (use ArClientFileToClient instead of calling this directly since this interface may change)",
		    &myGetFileWithTimestampCB, 
        "string: file to get; byte2: operation, 0 == get, 1 == cancel (these aren't implemented yet but will be)",
		    "ubyte2: return code, 0 = good (sending file), 1 = tried to go outside allowed area, 2 = no such file (or can't read), 3 = empty file name, 4 = error reading file (can happen after some good values) ; string: fileGotten; IF return was 0 then byte4: time_t that file was last modified; ubyte4: numBytes (number of bytes in the file buffer in this packet, 0 means end of file); data buffer that is numBytes in length", 
		    "FileAccess", "RETURN_UNTIL_EMPTY|SLOW_PACKET");

  // snag our base dir and make sure we have enough room for a /
  strncpy(myBaseDir, topDir, sizeof(myBaseDir) - 2);
  myBaseDir[sizeof(myBaseDir) - 2] = '\0';
  // make sure it has a slash
  ArUtil::appendSlash(myBaseDir, sizeof(myBaseDir));
  // make sure the slashes go the right direction
  ArUtil::fixSlashes(myBaseDir, sizeof(myBaseDir));  
}

AREXPORT ArServerFileToClient::~ArServerFileToClient()
{

}



AREXPORT void ArServerFileToClient::getFile(ArServerClient *client,
                                            ArNetPacket *packet)
{
  doGetFile(client, packet, false);
}

AREXPORT void ArServerFileToClient::getFileWithTimestamp
                                           (ArServerClient *client,
                                            ArNetPacket *packet)
{
  doGetFile(client, packet, true);
}



AREXPORT void ArServerFileToClient::doGetFile(ArServerClient *client,
                                              ArNetPacket *packet,
                                              bool isSetTimestamp)
{
  ArNetPacket sendPacket;

  char fileNameRaw[2048];
  packet->bufToStr(fileNameRaw, sizeof(fileNameRaw));

  // should check for operation here, but thats not implemented yet

  char fileNameCooked[2048];
  strcpy(fileNameCooked, fileNameRaw);
  ArUtil::fixSlashes(fileNameCooked, sizeof(fileNameCooked));


  char fileName[2048];
  if (!ArUtil::matchCase(myBaseDir, fileNameCooked, 
                         fileName, sizeof(fileName)))
  {
    ArLog::log(ArLog::Normal, 
	             "ArServerFileToClient: can't open file '%s'", fileNameRaw);
    sendPacket.uByte2ToBuf(2);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    // send an empty packet so that forwarding knows we're done
    sendPacket.empty();
    client->sendPacketTcp(&sendPacket);
    return;
  }

  size_t len = strlen(fileName);
  size_t ui = 0;

  // first advance to the first non space
  for (ui = 0; 
       ui < len && fileName[ui] != '\0' && isspace(fileName[ui]); 
       ui++);

  char *fileStr = new char[len + 3];

  // now copy in the rest
  strncpy(fileStr, fileName, len - ui);
  // make sure its null terminated
  fileStr[len - ui] = '\0';
    
  if (fileStr[0] == '/' || fileStr[0] == '~' ||
      fileStr[0] == '\\' || strstr(fileStr, "..") != NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerFileToClient: '%s' tried to access outside allowed area",
	       fileStr);
    delete[] fileStr;
    sendPacket.uByte2ToBuf(1);    
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    // send an empty packet so that forwarding knows we're done
    sendPacket.empty();
    client->sendPacketTcp(&sendPacket);
    return;
  }

  if (strlen(fileStr) > 0)
  {
    ArUtil::fixSlashes(fileStr, len + 2);
  }
  else
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerFileToClient: can't get file, empty filename");
    delete[] fileStr;
    sendPacket.uByte2ToBuf(3);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    // send an empty packet so that forwarding knows we're done
    sendPacket.empty();
    client->sendPacketTcp(&sendPacket);
    return;
  }

  // walk from our base down and try to find the first by name
  // ignoring case
  
  // put our base and where we want to go together
  std::string wholeName;
  wholeName = myBaseDir;
  wholeName += fileStr;

  delete[] fileStr;

  ArLog::log(ArLog::Verbose, 
	     "ArServerFileToClient: Trying to open %s from base %s", 
	     wholeName.c_str(), myBaseDir);
  
  struct stat fileStat;
  stat(wholeName.c_str(), &fileStat);
 
  FILE *file = NULL;
  if ((file = ArUtil::fopen(wholeName.c_str(), "rb")) == NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerFileToClient: can't open file '%s'", fileName);
    sendPacket.uByte2ToBuf(2);
    sendPacket.strToBuf(fileNameRaw);


    client->sendPacketTcp(&sendPacket);
    // send an empty packet so that forwarding knows we're done
    sendPacket.empty();
    client->sendPacketTcp(&sendPacket);
    return;
  }



  char buf[30000];
  size_t ret = 0;
  // now send the file
  while ((ret = fread(buf, 1, sizeof(buf), file)) == sizeof(buf))
  {
    sendPacket.empty();
    sendPacket.uByte2ToBuf(0);
    sendPacket.strToBuf(fileNameRaw);
    sendPacket.uByte4ToBuf(ret);
    if (isSetTimestamp) {
      sendPacket.byte4ToBuf(fileStat.st_mtime);
    }
    sendPacket.dataToBuf(buf, ret);
    client->sendPacketTcp(&sendPacket);
    //printf("Sent packet with %d\n", ret);
  } // end while more to read


  if (ferror(file))
  {
    ArLog::log(ArLog::Normal, "ArServerFileToClient: Error sending file %s", 
	       fileName);
    sendPacket.empty();
    sendPacket.uByte2ToBuf(4);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    // send an empty packet so that forwarding knows we're done
    sendPacket.empty();
    client->sendPacketTcp(&sendPacket);
    return;
  }
 
  // Send the remnants from the last iteration of the while loop above
  if (ret != 0) {

    sendPacket.empty();
    sendPacket.uByte2ToBuf(0);
    sendPacket.strToBuf(fileNameRaw);
    sendPacket.uByte4ToBuf(ret);
    if (isSetTimestamp) {
      sendPacket.byte4ToBuf(fileStat.st_mtime);
    }
    sendPacket.dataToBuf(buf, ret);
    //printf("Sent packet with %d\n", ret);
    client->sendPacketTcp(&sendPacket);
  }

  // Send a zero size to indicate that the file is ndone
  sendPacket.empty();
  sendPacket.uByte2ToBuf(0);
  sendPacket.strToBuf(fileNameRaw);
  sendPacket.uByte4ToBuf(0);
  if (isSetTimestamp) {
    sendPacket.byte4ToBuf(fileStat.st_mtime);
  }


  //printf("Sent packet end\n");
  client->sendPacketTcp(&sendPacket);
  
  
  // send an empty packet so that forwarding knows we're done
  sendPacket.empty();
  client->sendPacketTcp(&sendPacket);

  if (feof(file))
  {
    ArLog::log(ArLog::Normal, "ArServerFileToClient: Sent file %s to %s", 
	             fileName, client->getIPString());
  }
  fclose(file);

}

// -----------------------------------------------------------------------------
// ArServerFileFromClient
// -----------------------------------------------------------------------------

AREXPORT ArServerFileFromClient::ArServerFileFromClient(ArServerBase *server, 
                                                        const char *topDir,
                                                        const char *tempDir) :
  myPutFileCB(this, &ArServerFileFromClient::putFile),
  myPutFileInterleavedCB(this, &ArServerFileFromClient::putFileInterleaved),
  myPutFileWithTimestampCB(this, &ArServerFileFromClient::putFileWithTimestamp),
  myPutFileWithTimestampInterleavedCB(this, &ArServerFileFromClient::putFileWithTimestampInterleaved)
{

  myServer = server;

  myServer->addData("putFile", 
		    "Puts a file (use ArClientFileFromClient instead of calling this directly since this interface may change)",
		    &myPutFileCB, "uByte2: command, 0 = start file, 1 = continue file, 2 done with file, 3 = cancel put; string: file being sent; IF command == 1 then uByte4: numBytes; numBytes of data",
		    "uByte2: return code, 0 = good (got file), 1 = getting file, 2 = tried to go outside allowed area, 3 = bad directory, 4 = empty file name (or other problem with fileName), 5 = can't write temp file, 6 = error moving file from temp to perm, 7 = another client putting file, 8 = timeout (no activity for 15 seconds) and another client wanted to put the file, 9 = client adding to, finishing, or canceling a file the server doesn't have;  string: fileName",
		    "FileAccess", "RETURN_COMPLEX|SLOW_PACKET|DO_NOT_FORWARD");

  myServer->addData("putFileInterleaved", 
		    "Puts a file (use ArClientFileFromClient instead of calling this directly since this interface may change)",
		    &myPutFileInterleavedCB, "uByte2: command, 0 = start file, 1 = continue file, 2 done with file, 3 = cancel put; string: file being sent; IF command == 1 then uByte4: numBytes; numBytes of data",
		    "uByte2: return code, 0 = good (got file), 1 = getting file, 2 = tried to go outside allowed area, 3 = bad directory, 4 = empty file name (or other problem with fileName), 5 = can't write temp file, 6 = error moving file from temp to perm, 7 = another client putting file, 8 = timeout (no activity for 15 seconds) and another client wanted to put the file, 9 = client adding to, finishing, or canceling a file the server doesn't have, 10 = gotPacket, awaiting next packet, 11 = cancelled put; string: fileName",
		    "FileAccess", "RETURN_SINGLE|SLOW_PACKET|DO_NOT_FORWARD");
  
  myServer->addData("putFileWithTimestamp", 
		    "Puts a file and sets its modified time (use ArClientFileFromClient instead of calling this directly since this interface may change)",
		    &myPutFileWithTimestampCB, 
        "uByte2: command, 0 = start file, 1 = continue file, 2 done with file, 3 = cancel put; string: file being sent; IF command == 0 then byte4: time_t last modified; IF command == 1 then uByte4: numBytes; numBytes of data",
		    "uByte2: return code, 0 = good (got file), 1 = getting file, 2 = tried to go outside allowed area, 3 = bad directory, 4 = empty file name (or other problem with fileName), 5 = can't write temp file, 6 = error moving file from temp to perm, 7 = another client putting file, 8 = timeout (no activity for 15 seconds) and another client wanted to put the file, 9 = client adding to, finishing, or canceling a file the server doesn't have;  string: fileName",
		    "FileAccess", "RETURN_COMPLEX|SLOW_PACKET|DO_NOT_FORWARD");

  myServer->addData("putFileWithTimestampInterleaved", 
		    "Puts a file and sets its modified time (use ArClientFileFromClient instead of calling this directly since this interface may change)",
		    &myPutFileInterleavedCB, 
        "uByte2: command, 0 = start file, 1 = continue file, 2 done with file, 3 = cancel put; string: file being sent; IF command == 0 then byte4: time_t last modified; IF command == 1 then uByte4: numBytes; numBytes of data",
		    "uByte2: return code, 0 = good (got file), 1 = getting file, 2 = tried to go outside allowed area, 3 = bad directory, 4 = empty file name (or other problem with fileName), 5 = can't write temp file, 6 = error moving file from temp to perm, 7 = another client putting file, 8 = timeout (no activity for 15 seconds) and another client wanted to put the file, 9 = client adding to, finishing, or canceling a file the server doesn't have, 10 = gotPacket, awaiting next packet, 11 = cancelled put; string: fileName",
		    "FileAccess", "RETURN_SINGLE|SLOW_PACKET|DO_NOT_FORWARD");

  myFileNumber = 0;
  // snag our base dir and make sure we have enough room for a /
  strncpy(myBaseDir, topDir, sizeof(myBaseDir) - 2);
  myBaseDir[sizeof(myBaseDir) - 2] = '\0';
  // make sure it has a slash
  ArUtil::appendSlash(myBaseDir, sizeof(myBaseDir));
  // make sure the slashes go the right direction
  ArUtil::fixSlashes(myBaseDir, sizeof(myBaseDir));  
  // snag our base dir and make sure we have enough room for a /
  strncpy(myTempDir, tempDir, sizeof(myTempDir) - 2);
  myTempDir[sizeof(myTempDir) - 2] = '\0';
  // make sure it has a slash
  ArUtil::appendSlash(myTempDir, sizeof(myTempDir));
  // make sure the slashes go the right direction
  ArUtil::fixSlashes(myTempDir, sizeof(myTempDir));  
}

AREXPORT ArServerFileFromClient::~ArServerFileFromClient()
{

}


AREXPORT void ArServerFileFromClient::putFile(ArServerClient *client,
                                              ArNetPacket *packet)
{
  internalPutFile(client, packet, false, false);
}

AREXPORT void ArServerFileFromClient::putFileInterleaved
                                             (ArServerClient *client, 
                                              ArNetPacket *packet)
{
  internalPutFile(client, packet, true, false);
}

AREXPORT void ArServerFileFromClient::putFileWithTimestamp
                                             (ArServerClient *client,
                                              ArNetPacket *packet)
{
  internalPutFile(client, packet, false, true);
}

AREXPORT void ArServerFileFromClient::putFileWithTimestampInterleaved
                                             (ArServerClient *client, 
                                              ArNetPacket *packet)
{
  internalPutFile(client, packet, true, true);
}

AREXPORT void ArServerFileFromClient::internalPutFile(ArServerClient *client, 
                                                      ArNetPacket *packet,
                                                      bool interleaved,
                                                      bool isSetTimestamp)
{
  ArNetPacket sendPacket;
  std::list<ArFunctor *>::iterator fit;
  std::map<std::string, FileInfo *>::iterator it;

  FileInfo *info = NULL;
  
  std::string fileName;
  char fileNameRaw[2048];
  fileNameRaw[0] = '\0';
  
  ArTypes::UByte2 doing = packet->bufToUByte2();

  packet->bufToStr(fileNameRaw, sizeof(fileNameRaw));
  if (doing == 0)
  {
    char directoryRaw[2048];
    directoryRaw[0] = '\0';
    char fileNamePart[2048];
    fileNamePart[0] = '\0';
    if (!ArUtil::getDirectory(fileNameRaw, 
					                    directoryRaw, 
                              sizeof(directoryRaw)) ||
	      !ArUtil::getFileName(fileNameRaw, 
				                     fileNamePart, 
                             sizeof(fileNamePart)))
    {
      ArLog::log(ArLog::Normal, 
                 "ArServerFileFromClient: Problem with filename '%s'", 
                 fileNameRaw);
      sendPacket.uByte2ToBuf(4);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      // just added this 4/5/06
      return;
    }
   
//    ArUtil::appendSlash(directoryRaw, sizeof(directoryRaw));

    char directory[2048];
    printf("DirectoryRaw %s\n", directoryRaw);
    if (strlen(directoryRaw) == 0)
    {
      //strcpy(directory, ".");
      strcpy(directory, myBaseDir);
    }
    else if (!ArUtil::matchCase(myBaseDir, 
                                directoryRaw, 
                                directory, 
                                sizeof(directory)))
    {
      ArLog::log(ArLog::Normal, 
		             "ArServerFileFromClient: Bad directory for '%s'", 
		             fileNameRaw);
      sendPacket.uByte2ToBuf(3);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      return;
    }

    char tmpDir[2048];
    tmpDir[0] = '\0';
    //sprintf(tmpDir, "%s", tmpDir, directory);
    strcpy(tmpDir, directory);
    ArUtil::appendSlash(tmpDir, sizeof(tmpDir));
    char matchedFileName[2048];
    
    if (ArUtil::matchCase(tmpDir, fileNamePart, 
				     matchedFileName, 
				     sizeof(matchedFileName)))
    {
      fileName = tmpDir;
      fileName += matchedFileName;
      //printf("matched from %s %s\n", tmpDir, matchedFileName);
    }
    else
    {
      fileName = tmpDir;
      fileName += fileNamePart;
      //printf("unmatched from %s %s\n", tmpDir, fileNamePart);
    }
    
    ArLog::log(ArLog::Normal, 
               "ArServerFileFromClient: Checking file %s (as %s)",
               fileNameRaw, fileName.c_str());

    for (it = myMap.begin(); it != myMap.end(); it++)
    {
      info = (*it).second;
      // see if a client is messing with this file
      if (ArUtil::strcasecmp(info->myRealFileName, fileName) == 0)
      {
        // see if the other one messing with this timed out, if so let 'em know
        if (info->myLastActivity.secSince() > 15)
        {
          ArLog::log(ArLog::Normal, 
               "ArServerFileFromClient: Another client wants to start putting file '%s' and old client was inactive", 
               (*it).first.c_str());
          if (!interleaved)
          {
            sendPacket.uByte2ToBuf(8);
            sendPacket.strToBuf((*it).first.c_str());
            // NOT_EXCLUDING this isn't excluding anymore because it
            // used to cause problems...  this may cause some
            // problems, but hopefully the event doesn't happen much,
            // and at least this way things'll be able to retry easier
            myServer->broadcastPacketTcp(&sendPacket, "putFile");//, client);
          }
          myMap.erase((*it).first);
          delete info;
          break;
        }
        // otherwise let this guy know its busy
        else
        {
          ArLog::log(ArLog::Normal, 
               "ArServerFileFromClient: Another client putting file '%s'", 
               fileNameRaw);
          sendPacket.uByte2ToBuf(7);
          sendPacket.strToBuf(fileNameRaw);
          client->sendPacketTcp(&sendPacket);
          return;
        }
      }
    }
    ArLog::log(ArLog::Normal, 
	             "ArServerFileFromClient: Receiving file %s (as %s)", 
	             fileNameRaw, fileName.c_str());

    char tempFileName[3200];
    tempFileName[0] = '\0';
    sprintf(tempFileName, "%sArServerFileFromClient.%d.%d", myTempDir, getpid(), myFileNumber);
    myFileNumber++;

    ArLog::log(ArLog::Normal,
               "Using temp file name %s\n", tempFileName);
    // see if we can open the temp file
    FILE *file;
    if ((file = ArUtil::fopen(tempFileName, "wb")) != NULL)
    {
      info = new FileInfo;
      info->myRealFileName = fileName;      
      info->myTempFileName = tempFileName;
      info->myFile = file;
      info->myStartedTransfer.setToNow();
      info->myLastActivity.setToNow();
      // We can compare against this client pointer but we cannot
      // access anything on it since it could have disconnected and be
      // an invalid pointer now
      info->myClient = client;
      info->myClientCreationTime = client->getCreationTime();
      if (isSetTimestamp) {
        info->myFileTimestamp = packet->bufToByte4();
  
        char timeBuf[500];
        strftime(timeBuf, sizeof(timeBuf), "%c", localtime(&info->myFileTimestamp));

        ArLog::log(ArLog::Normal,
                   "ArServerFileFromClient will set timestamp to %s",
                   timeBuf);
      }
      else {
        info->myFileTimestamp = -1;
      }

      myMap[fileNameRaw] = info;
      if (interleaved)
      {
        //printf("Sending continue\n");
        sendPacket.uByte2ToBuf(10);
        sendPacket.strToBuf(fileNameRaw);
        client->sendPacketTcp(&sendPacket);
      }
    }
    else
    {
      ArLog::logErrorFromOS(ArLog::Normal, 
                 "ArServerFileFromClient: Can't open tmp file for '%s' (tried '%s')", 
                 fileNameRaw, tempFileName);
      sendPacket.uByte2ToBuf(5);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      return;
    }
  }
  else if (doing == 1)
  {
    // see if we can find this file
    if ((it = myMap.find(fileNameRaw)) == myMap.end())
    {
      ArLog::log(ArLog::Normal, 
                 "ArServerFileFromClient: Couldn't find entry for '%s'", 
                 fileNameRaw);
      sendPacket.uByte2ToBuf(9);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      return;
    }
    info = (*it).second;

    // make sure this client is the one sending the file, otherwise
    // just ignore it
    if (info->myClient != client || 
	!info->myClientCreationTime.isAt(client->getCreationTime()))
    {
      ArLog::log(ArLog::Normal, 
                 "ArServerFileFromClient: Got contents packet for file '%s' from the wrong client, ignoring it", 
                 fileNameRaw);
      // we don't send back a packet here, because we already did in
      // the 0 command (starting send), but the client ignored it
      // (probably cause it was doing a fast/stacked file transfer)
      return;
    }

    // write the data to the file and increment last activity
    ArTypes::UByte4 numBytes;
    char buf[32000];
    numBytes = packet->bufToUByte4();
    packet->bufToData(buf, numBytes);
    fwrite(buf, 1, numBytes, info->myFile);
    info->myLastActivity.setToNow();
    
    if (interleaved)
    {
      sendPacket.uByte2ToBuf(10);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
    }
    ArLog::log(ArLog::Verbose, "Continuing put file %s (%d bytes)", 
	       fileNameRaw, numBytes);
    
  }
  else if (doing == 2)
  {
    // see if we can find this file
    if ((it = myMap.find(fileNameRaw)) == myMap.end())
    {
      ArLog::log(ArLog::Normal, "ArUtil: Couldn't find entry for '%s'", fileNameRaw);
      sendPacket.uByte2ToBuf(9);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      return;
    }
    info = (*it).second;

    // make sure this client is the one sending the file, otherwise
    // just ignore it
    if (info->myClient != client || 
	!info->myClientCreationTime.isAt(client->getCreationTime()))
    {
      ArLog::log(ArLog::Normal, 
                 "ArServerFileFromClient: Got end packet for file '%s' from the wrong client, ignoring it", 
                 fileNameRaw);
      // we don't send back a packet here, because we already did in
      // the 0 command (starting send), but the client ignored it
      // (probably cause it was doing a fast/stacked file transfer)
      return;
    }

    fclose(info->myFile);
    info->myFile = NULL;

    char systemBuf[6400];
#ifndef WIN32
    char *mvName = "mv -f";
#else
    char *mvName = "move";
#endif
    sprintf(systemBuf, "%s \"%s\" \"%s\"", mvName, info->myTempFileName.c_str(), 
            info->myRealFileName.c_str());
    

    myMovingFileName = info->myRealFileName;

    // call our pre move callbacks
    for (fit = myPreMoveCallbacks.begin(); 
	       fit != myPreMoveCallbacks.end(); 
	       fit++)
      (*fit)->invoke();

    // move file
    int ret;
    if ((ret = system(systemBuf)) == 0)
    {
      if (isSetTimestamp) {
        ArUtil::changeFileTimestamp(info->myRealFileName.c_str(),
                                    info->myFileTimestamp);
      }

      sendPacket.uByte2ToBuf(0);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      ArLog::log(ArLog::Normal, "Done with file %s from %s", fileNameRaw,
		             client->getIPString());
      myMap.erase((*it).first);
      delete info;
    }
    else
    {
      sendPacket.uByte2ToBuf(6);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      unlink(info->myTempFileName.c_str());
      myMap.erase((*it).first);
      delete info;
      ArLog::log(ArLog::Normal, "Couldn't move temp file for %s (ret of '%s' is %d)", fileNameRaw, systemBuf, ret);
    }

    // call our post move callbacks
    for (fit = myPostMoveCallbacks.begin(); 
	       fit != myPostMoveCallbacks.end(); 
	       fit++) {
      (*fit)->invoke();
    }

    myMovingFileName = "";
  }
  else if (doing == 3)
  {
    if ((it = myMap.find(fileNameRaw)) == myMap.end())
    {
      ArLog::log(ArLog::Normal, "ArUtil: Couldn't find entry to cancel for '%s'", fileNameRaw);
      sendPacket.uByte2ToBuf(9);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
      return;
    }
    info = (*it).second;

    // make sure this client is the one sending the file, otherwise
    // just ignore it
    if (info->myClient != client || 
	!info->myClientCreationTime.isAt(client->getCreationTime()))
    {
      ArLog::log(ArLog::Normal, 
                 "ArServerFileFromClient: Got cancel packet for file '%s' from the wrong client, ignoring it", 
                 fileNameRaw);
      // we don't send back a packet here, because we already did in
      // the 0 command (starting send), but the client ignored it
      // (probably cause it was doing a fast/stacked file transfer)
      return;
    }

    fclose(info->myFile);
    info->myFile = NULL;

    unlink(info->myTempFileName.c_str());
    myMap.erase((*it).first);
    delete info;

    ArLog::log(ArLog::Normal, "Cancelling file %s", fileNameRaw);
    if (interleaved)
    {
      sendPacket.uByte2ToBuf(11);
      sendPacket.strToBuf(fileNameRaw);
      client->sendPacketTcp(&sendPacket);
    }
  }
  else
  {
    ArLog::log(ArLog::Normal, "Unknown command %d for file %s", doing, 
	       fileNameRaw);
  }
}

AREXPORT void ArServerFileFromClient::addPreMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPreMoveCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPreMoveCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerFileFromClient::addPreMoveCallback: Invalid position.");
}

AREXPORT void ArServerFileFromClient::remPreMoveCallback(
	ArFunctor *functor)
{
  myPreMoveCallbacks.remove(functor);
}

AREXPORT void ArServerFileFromClient::addPostMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPostMoveCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPostMoveCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerFileFromClient::addPostMoveCallback: Invalid position.");
}

AREXPORT void ArServerFileFromClient::remPostMoveCallback(
	ArFunctor *functor)
{
  myPostMoveCallbacks.remove(functor);
}


AREXPORT ArServerDeleteFileOnServer::ArServerDeleteFileOnServer(
	ArServerBase *server, const char *topDir) :
  myDeleteFileCB(this, &ArServerDeleteFileOnServer::deleteFile)
{
  myServer = server;
  myServer->addData("deleteFile", 
		    "Deletes a file (use ArClientDeleteFileOnServer instead of calling this directly since this interface may change)",
		    &myDeleteFileCB, "string: file to delete",
		    "ubyte2: return code, 0 = good (deleted file file), 1 = tried to go outside allowed area, 2 = no such file (or can't read it), 3 = empty file name, string: fileDeleted", 
		    "FileAccess", "RETURN_SINGLE|SLOW_PACKET");

  // snag our base dir and make sure we have enough room for a /
  strncpy(myBaseDir, topDir, sizeof(myBaseDir) - 2);
  myBaseDir[sizeof(myBaseDir) - 2] = '\0';
  // make sure it has a slash
  ArUtil::appendSlash(myBaseDir, sizeof(myBaseDir));
  // make sure the slashes go the right direction
  ArUtil::fixSlashes(myBaseDir, sizeof(myBaseDir));  
}

AREXPORT ArServerDeleteFileOnServer::~ArServerDeleteFileOnServer()
{

}



AREXPORT void ArServerDeleteFileOnServer::deleteFile(ArServerClient *client,
					  ArNetPacket *packet)
{
  ArNetPacket sendPacket;
  size_t ui;
  size_t len;
  std::list<ArFunctor *>::iterator fit;
  char fileNameRaw[2048];
  packet->bufToStr(fileNameRaw, sizeof(fileNameRaw));

  // should check for operation here, but thats not implemented yet

  char fileNameCooked[2048];
  strcpy(fileNameCooked, fileNameRaw);
  ArUtil::fixSlashes(fileNameCooked, sizeof(fileNameCooked));


  char fileName[2048];
  if (!ArUtil::matchCase(myBaseDir, fileNameCooked, 
				    fileName, sizeof(fileName)))
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerDeleteFileOnServer: can't read file '%s'", 
	       fileNameRaw);
    sendPacket.uByte2ToBuf(2);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    return;
  }

  len = strlen(fileName);
  // first advance to the first non space
  for (ui = 0; 
       ui < len && fileName[ui] != '\0' && isspace(fileName[ui]); 
       ui++);

  char *fileStr = new char[len + 3];

  // now copy in the rest
  strncpy(fileStr, fileName, len - ui);
  // make sure its null terminated
  fileStr[len - ui] = '\0';
    
  if (fileStr[0] == '/' || fileStr[0] == '~' ||
      fileStr[0] == '\\' || strstr(fileStr, "..") != NULL)
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerDeleteFileOnServer: '%s' tried to access outside allowed area",
	       fileStr);
    delete[] fileStr;
    sendPacket.uByte2ToBuf(1);    
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    return;
  }

  if (strlen(fileStr) > 0)
  {
    ArUtil::fixSlashes(fileStr, len + 2);
  }
  else
  {
    ArLog::log(ArLog::Normal, 
       "ArServerDeleteFileOnServer: can't delete file, empty filename");
    delete[] fileStr;
    sendPacket.uByte2ToBuf(3);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
    return;
  }

  // walk from our base down and try to find the first by name
  // ignoring case
  
  // put our base and where we want to go together
  std::string wholeName;
  wholeName = myBaseDir;
  wholeName += fileStr;

  delete[] fileStr;

  ArLog::log(ArLog::Verbose, 
	     "ArServerDeleteFileOnServer: Trying to delete %s from base %s", 
	     wholeName.c_str(), myBaseDir);

  myDeletingFileName = fileNameRaw;

  // call our pre delete callbacks
  for (fit = myPreDeleteCallbacks.begin(); 
       fit != myPreDeleteCallbacks.end(); 
       fit++)
    (*fit)->invoke();

  if (unlink(wholeName.c_str()) == 0)
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerDeleteFileOnServer: Deleted file %s for %s", 
	       fileName, client->getIPString());
    sendPacket.uByte2ToBuf(0);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
  }
  else
  {
    ArLog::log(ArLog::Normal, 
	       "ArServerDeleteFileOnServer: can't unlink file '%s'", fileName);
    sendPacket.uByte2ToBuf(2);
    sendPacket.strToBuf(fileNameRaw);
    client->sendPacketTcp(&sendPacket);
  }

  // call our post delete callbacks
  for (fit = myPostDeleteCallbacks.begin(); 
       fit != myPostDeleteCallbacks.end(); 
       fit++)
    (*fit)->invoke();

  myDeletingFileName = "";  
}

AREXPORT void ArServerDeleteFileOnServer::addPreDeleteCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPreDeleteCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPreDeleteCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerDeleteFileOnServer::addPreDeleteCallback: Invalid position.");
}

AREXPORT void ArServerDeleteFileOnServer::remPreDeleteCallback(
	ArFunctor *functor)
{
  myPreDeleteCallbacks.remove(functor);
}

AREXPORT void ArServerDeleteFileOnServer::addPostDeleteCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPostDeleteCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPostDeleteCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArServerDeleteFileOnServer::addPostDeleteCallback: Invalid position.");
}

AREXPORT void ArServerDeleteFileOnServer::remPostDeleteCallback(
	ArFunctor *functor)
{
  myPostDeleteCallbacks.remove(functor);
}

#endif // WIN32
