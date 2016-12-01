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

