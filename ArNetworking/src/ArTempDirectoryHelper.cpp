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
#include "ArExport.h"

#include "ArTempDirectoryHelper.h"

#include "ArLog.h"
#include "ariaUtil.h"

AREXPORT ArTempDirectoryHelper::ArTempDirectoryHelper
                                     (const char *baseDirectory,
				                              const char *tempDirectory) :
  myBaseDirectory((baseDirectory != NULL) ? baseDirectory : ""),
  myTempDirectory((tempDirectory != NULL) ? tempDirectory : ""),
  myPreMoveCallbacks(), 
  myPostMoveCallbacks()
{
  if (myTempDirectory.empty()) {
    myTempDirectory = myBaseDirectory;
  }
} // end ctor

AREXPORT ArTempDirectoryHelper::~ArTempDirectoryHelper()
{
}

  
AREXPORT const char *ArTempDirectoryHelper::getTempDirectory()
{
  return myTempDirectory.c_str();
}


AREXPORT const char *ArTempDirectoryHelper::getBaseDirectory()
{
  return myBaseDirectory.c_str();
}

  

AREXPORT bool ArTempDirectoryHelper::moveFileToBaseDirectory
                           (const char *fileName)
{
  if (!ArUtil::isStrEmpty(fileName)) {
    std::list<std::string> fileNameList;
    fileNameList.push_back(fileName);
    return moveFilesToBaseDirectory(fileNameList);
  }
  else {
    ArLog::log(ArLog::Normal,
               "ArTempDirectoryHelper::moveFileToBaseDirectory() cannot move empty file name");
    return false;
  }

} // end method moveFileToBaseDirectory

  

AREXPORT bool ArTempDirectoryHelper::moveFilesToBaseDirectory
                           (const std::list<std::string> &fileNameList)
{

  bool isSuccess = true;

  // now, if our temp directory and base directory are different we
  // need to move it and put the result in the packet, otherwise put
  // in we're okay
  if (ArUtil::strcasecmp(myBaseDirectory, myTempDirectory) != 0)
  {
#ifndef WIN32
    char *mvName = "mv";
#else
    char *mvName = "move";
#endif

    char systemBuf[6400];
    char fromBuf[1024];
    char toBuf[1024];

    bool isFirstMoveCmd = true;

    for (std::list<std::string>::const_iterator iter = fileNameList.begin();
         iter != fileNameList.end();
         iter++) {


      std::string sourceFileName = *iter;

      if (sourceFileName.empty()) {
        continue; // Nothing to be done for this file...
      }

      if (myTempDirectory.size() > 0) {
        snprintf(fromBuf, sizeof(fromBuf), "%s%s", 
	               myTempDirectory.c_str(), sourceFileName.c_str());
      }
      else {
        snprintf(fromBuf, sizeof(fromBuf), "%s", 
                 sourceFileName.c_str());
      }

      ArUtil::fixSlashes(fromBuf, sizeof(fromBuf));

      if (myBaseDirectory.size() > 0) {
        snprintf(toBuf, sizeof(toBuf), "%s%s", 
	               myBaseDirectory.c_str(), sourceFileName.c_str());
      }
      else {
        snprintf(toBuf, sizeof(toBuf), "%s", sourceFileName.c_str());
      }

      ArUtil::fixSlashes(toBuf, sizeof(toBuf));

      // Create the move command
      sprintf(systemBuf, "%s \"%s\" \"%s\"", mvName, fromBuf, toBuf);

      ArLog::log(ArLog::Normal, "Moving with '%s'", systemBuf);
    
      // Call our pre move callbacks
      if (isFirstMoveCmd) {
        isFirstMoveCmd = false;
    
        for (std::list<ArFunctor *>::iterator preIt = myPreMoveCallbacks.begin(); 
	           preIt != myPreMoveCallbacks.end(); 
          	 preIt++) {
          (*preIt)->invoke();
        }
      } // end if first move command
    
      // move file
      int ret = system(systemBuf);
      if (ret == 0)
      {
        ArLog::log(ArLog::Verbose, 
                   "ArTempDirectoryHelper: Moved file %s (with %s)", 
		               sourceFileName.c_str(), 
                   systemBuf);
     
        if (iter == fileNameList.begin()) {
          isSuccess = true;
        }
      }
      else // error moving file
      {
        ArLog::log(ArLog::Normal, 
		               "ArTempDirectoryHelper: Couldn't move %s (ret of '%s' is %d) removing file", 
		               sourceFileName.c_str(), 
                   systemBuf, 
                   ret);

         unlink(fromBuf);
         if (iter == fileNameList.begin()) { 
           isSuccess = false;
         }
      } // end else error moving file 
    } // end for each file

    // If files were moved, then call the post move callbacks
    if (!isFirstMoveCmd) {
      for (std::list<ArFunctor *>::iterator postIt = myPostMoveCallbacks.begin(); 
           postIt != myPostMoveCallbacks.end(); 
           postIt++) {
        (*postIt)->invoke();
      }
    }

  } // end if files need to be moved

  return isSuccess;

} // end method moveFilesToBaseDirectory


AREXPORT std::string ArTempDirectoryHelper::makeTempFilePathName
                                                (const char *fileName)
{
  return makeFilePathName(fileName,
                          myTempDirectory.c_str());
}

AREXPORT std::string ArTempDirectoryHelper::makeBaseFilePathName
                                                (const char *fileName)
{
  return makeFilePathName(fileName,
                          myBaseDirectory.c_str());
}

AREXPORT std::string ArTempDirectoryHelper::makeFilePathName
                                                (const char *fileName,
                                                 const char *dirName)
{

  if (ArUtil::isStrEmpty(fileName)) {
    return "";
  }
  
  std::string filePathName;

  if ((!ArUtil::isStrEmpty(dirName)) &&
      (fileName[0] != '/') &&
      (fileName[0] != '\\') ) {
    filePathName = myTempDirectory;
    filePathName += fileName;
  }
  else {
    filePathName = fileName;
  }

  return filePathName;

} // end method makeFilePathName
  
  
AREXPORT void ArTempDirectoryHelper::addPreMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPreMoveCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPreMoveCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArTempDirectoryHelper::addPreMoveCallback: Invalid position.");
}


AREXPORT void ArTempDirectoryHelper::remPreMoveCallback(
	ArFunctor *functor)
{
  myPreMoveCallbacks.remove(functor);
}


AREXPORT void ArTempDirectoryHelper::addPostMoveCallback(
	ArFunctor *functor, ArListPos::Pos position)
{
  if (position == ArListPos::FIRST)
    myPostMoveCallbacks.push_front(functor);
  else if (position == ArListPos::LAST)
    myPostMoveCallbacks.push_back(functor);
  else
    ArLog::log(ArLog::Terse, 
       "ArTempDirectoryHelper::addPostMoveCallback: Invalid position.");
}


AREXPORT void ArTempDirectoryHelper::remPostMoveCallback(
	ArFunctor *functor)
{
  myPostMoveCallbacks.remove(functor);
}

