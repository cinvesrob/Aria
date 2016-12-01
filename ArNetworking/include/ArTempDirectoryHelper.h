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


