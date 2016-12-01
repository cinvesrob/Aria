#ifndef ARTEMPDIRECTORYHELPER_H
#define ARTEMPDIRECTORYHELPER_H

#include "ariaTypedefs.h"
#include "ArFunctor.h"

/// Helper class that manages the transition of temporary files to the base directory.
class ArTempDirectoryHelper 
{
public:

  /// Constructor
  AREXPORT ArTempDirectoryHelper(const char *baseDirectory = "",
				                         const char *tempDirectory = "");

  /// Destructor
  AREXPORT virtual ~ArTempDirectoryHelper();


  /// Returns the name of the directory for temporary files.
  AREXPORT const char *getTempDirectory();
  /// Returns the name of the target base directory.
  AREXPORT const char *getBaseDirectory();


  /// Creates a complete file path name for the specified file in the temporary directory.
  AREXPORT std::string makeTempFilePathName(const char *fileName);
 
  /// Creates a complete file path name for the specified file in the base directory.
  AREXPORT std::string makeBaseFilePathName(const char *fileName);


  /// Moves the specified file from the temporary directory to the base directory.
  AREXPORT bool moveFileToBaseDirectory(const char *fileName);
  
  /// Moves the specified files from the temporary directory to the base directory.
  AREXPORT bool moveFilesToBaseDirectory
                           (const std::list<std::string> &fileNameList);

  
  /// Adds a callback to be called before moving from temp dir to base dir
  AREXPORT void addPreMoveCallback(ArFunctor *functor, 
                                   ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called before moving from temp dir to base dir
  AREXPORT void remPreMoveCallback(ArFunctor *functor);
  /// Adds a callback to be called after moving from temp dir to base dir
  AREXPORT void addPostMoveCallback(ArFunctor *functor, 
                                    ArListPos::Pos position = ArListPos::LAST);
  /// Removes a callback to be called after moving from temp dir to base dir
  AREXPORT void remPostMoveCallback(ArFunctor *functor);


protected:

  /// Creates a complete file path name for the specified file and directory.
  AREXPORT std::string makeFilePathName(const char *fileName,
                                        const char *dirName);
  
protected:

  /// Path name of the base directory  
  std::string myBaseDirectory;
  /// Path name of the temporary directory
  std::string myTempDirectory;

  /// List of callbacks invoked before the files are moved
  std::list<ArFunctor *> myPreMoveCallbacks;
  /// List of callbacks invoked after the files are moved
  std::list<ArFunctor *> myPostMoveCallbacks;

}; // end class ArTempDirectoryHelper

#endif // ARTEMPDIRECTORYHELPER_H


