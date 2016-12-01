
ARIA
====

Adept MobileRobots Advanced Robotics Interface for Applications
---------------------------------------------------------------

Version 2.9.0
  
Linux 

Copyright 2002, 2003, 2004, 2005 ActivMedia Robotics, LLC. All rights reserved.
Copyright 2006, 2007, 2008, 2009 MobileRobots Inc. All rights reserved.
Copyright 2010-2014 Adept Technology. All rights reserved.

See LICENSE.txt for full license information about ARIA.

Please read this document for important details about using ARIA.

Contents
========

*  Introduction
*  Documentation
*  Licenses and Sharing
*  ARIA Package
*  Files of Note
*  Compiling programs that use ARIA
   *  Windows
   *  Linux
      *  Using ARIA's Makefiles
      *  Using your own Makefile or other build system
      *  Setting variables for ARIA Makefiles
   *  Learning more about using Linux
   *  Learning more about C++
*  Using ARIA from Python and Java
*  Using ARIA from Matlab or Simulink
*  Simulator

Introduction
============

Welcome to the Adept MobileRobots Advanced Robotics Interface for Applications (ARIA).
ARIA is an object-oriented, application programming interface (API) for the
Adept MobileRobots (and ActivMedia) line of intelligent mobile robots, including
Pioneer 2/3 DX and AT, PeopleBot, PowerBot, AmigoBot, PatrolBot/Guiabot,
Seekur, SeekurJr and Pioneer LX mobile robots.

Written in the C++ language, ARIA  provides 
access to and management of the robot controller, as well
to many accessory robot sensors and effectors, as well as useful utilities
and infrastructure for cross-platform robot application development.
   
Several levels are available for application to use in ARIA,
from simple command-control of the robot server for direct-drive
navigation, to development of higher-level intelligent actions (aka
behaviors). 

ARIA is provided as open source software under the GNU General Public
License. This allows you to view, modify and rebuild the ARIA library
as desired, provided that software developed with ARIA comply with the
requirements of the license.  Please see LICENSE.txt for the full 
license under which ARIA has been provided.

The ARIA package includes both source code and pre-built libraries
and example programs. These libraries and programs were build with
GCC 3.4 if on Linux, and MS Visual C++ 2010 (10.0), Visual C++ 2012
(11.0) and Visual C++ 2013 (12.0) for Windows Desktop if on Windows.  
Using the above compilers for development is recommended.  See below
for specific instructons on using these compilers to build your 
programs linked to ARIA, or to rebuild ARIA.

NOTE: If you use a different compiler or compiler version, you must 
rebuild the ARIA libraries to ensure link compatability.

To get started immediately with ARIA, refer to the examples in the
"examples" directory (start with "simpleConnect.cpp"), read the information
below, and in the ARIA Reference manual.

See below for more information about building programs with ARIA
on Windows and Linux and using the Windows and Linux development tools.


Documentation and Help
======================

Follow the INSTALL text instructions to install ARIA on your Linux or 
Windows workstation or robot computer. System requirements are included 
in the INSTALL.txt file.

ARIA includes a full API Reference Manual in HTML format. This manual,
Aria-Reference.html (and in the docs/ directory), includes documentation 
about each of the classes and methods in ARIA, as well as a comprehensive 
overview describing how to get stated understanding and using ARIA.  In 
addition, ARIA includes several example programs in the examples/ 
(start with simpleConnect.cpp, then explore the others) and 
advanced/ directories, and the header files and source code are included
in include/ and src/ as well.  

The ArNetworking library has its own reference manual,
ArNetworking-Reference.html in the ArNetworking subdirectory, and examples
in ArNetworking/examples.

If you plan on using the Java or Python wrapper libraries, see the 
javaExamples, pythonExamples, ArNetworking/javaExamples and
ArNetworking/pythonExamples directories for important information in
README files, and example programs. You should also read the Aria Reference
manual for general information about Aria -- the API in the wrapper libraries
are almost identical to the C++ API.

If you have any problems or questions using ARIA or your robot, the 
MobileRobots support site provides:
 
  * A FAQ (Frequently Asked Questions) list, at <http://robots.mobilerobots.com/FAQ.html>
  * A knowlege base of information on robot hardware and software, at <http://robots.mobilerobots.com>
  * All robot and device manuals
  * The Aria-Users mailing list, where you can discuss Aria with other users and 
    MobileRobots software developers:
      * Search the archives at <http://robots.mobilerobots.com/archives/aria-users/threads.html>
      * Join the list at <http://robots.mobilerobots.com/archives/aria-info.html>
  * Information on contacting MobileRobots technical support.

License and Sharing
===================

ARIA is released under the GNU Public License, which means that if you
distribute any work which uses ARIA, you must distribute the entire 
source code to that work under the GPS as well.  Read the included 
LICENSE text for details.  We open-sourced ARIA under GPL with full
source code not only for your convenience, but also so that you can 
share your enhancements to the software.  If you wish your enhancements 
to make it into our future ARIA versions, you will need to assign the 
copyright on those changes to Adept. Contact lafary@mobilerobots.com 
with these changes or with questions about this.

Accordingly, please do share your work, and please sign up for the 
exclusive aria-users@mobilerobots.com newslist so that you can benefit 
from others' work, too.

ARIA may instead be relicensed for proprietary, closed source applications. 
Contact sales@mobilerobots.com for details.

For answers to frequently asked questions about what the GPL allows
and requires, see <http://www.gnu.org/licenses/gpl-faq.html>


The ARIA Package
================

Aria/:
  docs      The API Reference Manual: Extensive documentation of all of ARIA
  examples  ARIA examples -- a good place to start; see examples/README.txt
  include   ARIA header (*.h) files
  src       ARIA source code (*.cpp) files
  lib       Libraries (.lib files for Windows, .so files for Linux) 
  bin       Contains Windows binaries and DLLs.
  params    Robot definition (parameter) files ("p3dx.p", for example)
  tests     Test files, somewhat esoteric but useful during ARIA development
  utils     Utility commands, not generally needed
  advanced  Advanced demos, not for the faint of heart (or ARIA novice)
  python    Python wrapper package
  java      Java wrapper package

Aria/ArNetworking/:  (A library used to facilitate network communication)
  docs      API Reference Manual for ArNetworking 
  examples  ArNetworking examples
  include   ArNetworking header (*.h) files, of course
  src       ArNetworking source (*.cpp) files
  python    Python wrapper package
  java      Java wrapper package
  

Other ARIA Files of Note
========================

LICENSE.txt        GPL license; agree to this to use ARIA
Aria-*.sln    	   MS Visual C++ solutions for building ARIA and 'demo'.
examples/All_examples-*.sln 
                  MS Visual C++ solutions for building ARIA examples.
Makefile          Linux makefile (GNU Make) for building ARIA and examples
Makefile.dep      Linux dependency file
INSTALL.txt       More information about installing ARIA
README.txt        This file; also see READMEs in advanced/, examples/, and tests/
Changes.txt       Summary of changes in each version of ARIA


Building programs that use ARIA
===============================

Windows
-------

On Windows, use Microsoft Visual C++ 2010 (VC10), Visual C++ 2012 (VC11),
or Visual C++ 2013 (VC12) for Windows Desktop (VC++ 2013 projects are
available for most packages, but not all yet).  Free "Express" versions of 
Visual C++ can be downloaded from Microsoft, or full versions purchased.
Only Visual C++ is required. Other Visual Studio options such as C#,
web development tools, .NET, SQL Server or database tools, or Silverlight
are not required by ARIA or other Pioneer SDK libraries, and do not
need to be installed.

Note: If you use a different version of Visual C++, you must rebuild 
the ARIA libraries. We recommand and support only these versions; other
versions may or may not work.

You can add your new project the Visual Studio solution files provided with ARIA if you
wish, or create a new solution and add your new project as well as projects
for the ARIA libraries you use.

Now you must configure your project to find ARIA. These instructions
assume a default installation; if you installed ARIA elsewhere you
must use different paths. If you keep your Visual Studo project within
Aria's directory, you can also use relative paths (e.g. "..\lib" for the
library path).

Aria DLL libraries are provided in two variants, "Debug" libraries that use 
the "Multithreaded Debug DLL" runtime library, and non-Debug (or "Release") libraries
that use the plain "Multithreaded DLL" runtime library.  The ARIA solutions
use a "Debug" and a "Release" configuration to select these variants; you
can also use these configuration names as well if you wish.

It is also possible to build static ARIA libraries. These libraries use 
the "Multithreaded Debug" runtime library for Debug mode, or "Multithreaded"
for Release mode.  These libraries will include the word "Static" in their names.

The steps required to configure a new program project settings for use with 
the ARIA DLL libraries are as follows:

To edit your new project's configuration properties, right click on the 
project in the solution explorer pane and choose Properties.

1. Change the "Configuration" (upper left of the dialog) menu to "All
  Configurations".

2. Open the "General" section

  * Either set "Output Files" to "C:\Program Files\MobileRobots\Aria\bin" 
    (Aria's bin directory, where Aria.dll etc. are), or put Aria's bin directory
    in your system PATH environment variable, or copy the DLLs to your own
    project's output directory.
 
  * You may want to change Intermediate Files. ARIA projects use
    "obj\$(ConfigurationName)-<VC version>" to keep object files 
	separate and out of the way, but Visual C++'s default value is fine
	also.

  * Set "Common Language Runtime Support" to "No Common Language Runtime Support"

3. Open  the "Link" or "Linker" section.

 * To "Additional Library Path" add "C:\Program Files\MobileRobots\Aria\lib"
   (or other correct path to Aria lib directory)
 
 * Open the "Input" subsection.

    * Change the "Configuration" menu (upper left of the window) to "Debug"

	* (Visual C++ 2010) To "Additional Dependencies", add:
        AriaDebugVC10.lib; winmm.lib; advapi32.lib; ws2_32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebugVC10.lib
		
	* (Visual C++ 2012) To "Additional Dependencies", add:
        AriaDebugVC11.lib; winmm.lib; advapi32.lib; ws2_32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebugVC11.lib 
		
	* (Visual C++ 2013) To "Additional Dependencies", add:
        AriaDebugVC12.lib; winmm.lib; advapi32.lib; ws2_32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingDebugVC12.lib
		
	* Aria does not require any of the Windows system libraries included
	  by Visual C++ as "Inherited values" in the Linker Additional Dependencies,
	  so you can uncheck "Inherit from parent or project defaults" after choosing
	  "Edit" from the drop-down menu next to the Additional Dependencies
	  field if you do not require any of those libraries in your program.

    * Change the "Configuration" menu to "Release"
		
    * (Visual C++ 2010) To "Additional Dependencies", add:
        AriaVC10.lib; winmm.lib; advapi32.lib; ws2_32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingVC10.lib

	* (Visual C++ 2012) To "Additional Dependencies", add:
        AriaVC11.lib; winmm.lib; advapi32.lib; ws2_32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingVC11.lib

	* (Visual C++ 2013) To "Additional Dependencies", add:
        AriaVC12.lib; winmm.lib; advapi32.lib; ws2_32.lib
      If you are using the ArNetworking library, also add:
        ArNetworkingVC12.lib
		
    * Aria does not require any of the Windows system libraries included
	  by Visual C++ as "Inherited values" in the Linker Additional Dependencies,
	  so you can uncheck "Inherit from parent or project defaults" after choosing
	  "Edit" from the drop-down menu next to the Additional Dependencies
	  field if you do not require any of those libraries in your program.
		
    * Change the "Configuration" menu back to "All Configurations"

4. Click on the "C++" section.

  * Click on the "General" subsection

    * To "Additional Include Directories" add 
      "C:\Program Files\MobileRobots\Aria\include".
	 (or other correct path to Aria's include directory)

   * Click on the "Code Generation" subsection

     * Change the "Configuration" menu (upper left of the window) to "Debug".

       * Under the "Use run-time library" pulldown select "Debug
         Multithreaded DLL".

     * Change the "Configuration" menu (upper left of the screen) to "Release".

       * Under the "Use run-time library" pulldown select "Multithreaded DLL".

     * Change the "Configuration" menu back to "All Configurations"
	 
To use the static libraries instead of DLLs, set the C++ runtime library to "Multithreaded" 
(Release configuration) and "Multithreaded Debug" (Debug configuration), 
list the static libraries for Additional Dependencies in the Linker Input section,
and also append ARIA_STATIC to the preprocessor definitions in the C++ -> Preprocessor
section (this prevents DLL "export" attributes from being added to method
declarations in the ARIA header files.)

The ARIA DLLs are in the Aria "bin" directory (C:\Program Files\MobileRobots\Aria\bin
if installed).  This directory can be added to the system PATH, or the appropriate
ARIA DLL copied to the same directory as your program executable, or you can place 
your program executable in the Aria bin directory.

ARIA does not require the use of precompiled headers, and precompiled
ARIA headers are not provided.  To disable the use of precompiled headers
in your project, open the "Precompiled Headers" subsection in the "C++"
section of your project's properties.  Change "Use Precompiled Headers"
to "no".  You can then remove "stdafx.h" and "stdafx.cpp" from your
project and remove the #include "stdafx.h" directive from your main 
source file.   

To make your program more portable, you can also change the definition
of your program's main function to standard C++:

  int main(int argc, char **argv)
  {
      ...
  }

See the ARIA API reference documentation and example programs to 
get started writing your programs.


Linux
-----

On GNU/Linux two tools are used: the compiler (GCC), which compiles
and/or links a single file, and Make, which is used to manage 
building multiple files and their dependencies.  If you need to
deal with multiple files, you can copy a Makefile and modify it, 
or create a new one.

Note: In packages for Debian and Ubuntu, the shared libraries were built with the 
standard G++ version provided with the system. You must use 
the same version of G++ or a compatible version to build programs that 
link against it, or rebuild ARIA using your preferrend G++ version. (On
Debian and Ubuntu systems, the default g++ and gcc compiler versions may be changed
using the 'galternatives' program or the 'update-alternatives' command). 

When compiling ARIA or its examples, you may also temporarily override the 
C++ compiler command ('g++' by default) by setting the CXX environment 
variable before or while running make. E.g.: "make CXX=g++-3.4" or 
"export CXX=g++-3.4; make".

More information and documentation about GCC is available at 
<http://gcc.gnu.org>, and in the gcc 'man' page.  More information 
about Make is at <http://www.gnu.org/software/make/>. Several graphical 
IDE applications are available for use with GCC, though none are 
currently supported by MobileRobots.



### Using ARIA's Makefiles on Linux: ###

The ARIA Makefile is able to build any program consisting 
of one source code file in the 'examples', 'tests', or 'advanced' 
directory.  For example, if your source code file is 
'examples/newProgram.cpp', then you can run 'make examples/newProgram' 
from the top-level Aria directory, or 'make newProgram' from within 
the 'examples' directory, to build it.   This makes it easy to copy
one of the example programs to a new name and modify it -- a good way
to learn how to use ARIA.



### Using ARIA from your own Makefile or other build system: ###

If you want to keep your program in a different place than the Aria 
examples directory, and use your own Makefile or other build tool,
you need to use the following g++ options to compile the program:

 -fPIC -I/usr/local/Aria/include 

If you wish, you may also use -g and -Wall for debugging information
and useful warnings.   Aria does not use exceptions, so you may also
use -fno-exceptions if you wish; this will cause any use of exceptions
in your program to trigger a compile error.

For linking with libAria use these options:

  -L/usr/local/Aria/lib -lAria -lpthread -ldl -lrt

If you are also using ArNetworking, use the following compile option
in addition to the Aria options above:

  -I/usr/local/Aria/ArNetworking/include
  
And for linking to libArNetworking:

  -lArNetworking


A Makefile for a program called "program" with source file "program.cpp"
might look something like this:


  all: program

  CFLAGS=-fPIC -g -Wall
  ARIA_INCLUDE=-I/usr/local/Aria/include
  ARIA_LINK=-L/usr/local/Aria/lib -lAria -lpthread -ldl -lrt

  %: %.cpp
	  $(CXX) $(CFLAGS) $(ARIA_INCLUDE) $< -o $@ $(ARIA_LINK)


Refer to the GNU Make manual <http://www.gnu.org/software/make> or 
other books or documentation about Make to learn more.

See the ARIA API reference documentation and example programs to 
get started writing your programs.

### Setting variables for ARIA Makefiles ###

You can add compile options to the following variables used in the ARIA
Makefiles, or replace default values. These can be set on the command
line when running `make`, for example:

  make CXX="g++-4.6"

CXXFLAGS    Additional compile flags passed to the C++ compiler. You
            can add GCC options to enable profiling, optimizations, etc.  
            For example, to rebuild ARIA optimized for the Atom CPU
            found in the LX/MTX embedded computer, add -mtune=atom and -O2:
              make clean; make CXXFLAGS="-mtune=atom -O2"
            These compiler flags will be used in addition to ARIA's 
            default flags.

CXX         Specify the C++ compiler command to use. The default is g++ or c++.
            You can use this to use a different version of g++, or to give the
            full path to g++, or to try a different compiler other than G++.

JAVAC       If rebuilding the Java wrapper library, this specifies the Java 
            compiler command to use. Default is "javac".

JAR         If rebuilding the Java wrapper library, this specifies the JAR
            archiver command to use. Default is "jar".

JAVA_BIN    If rebuilding the Java wrapper library, this specifies a directory
            containing JDK commands such as javac and jar. 

SWIG        If rebuilding the Java and/or Python wrapper libraries, this
            specifies the swig command used.  Default is swig.

PYTHON_INCLUDE  
            If rebuilding the Python wrapper library, this specifies the Python
            include directory. Default is /usr/include/python2.5. You usually
            need to specify this if building on a Linux other than Debian 5.

INSTALL_DIR
SYSTEM_ETC_DIR
            Directories used when installing ARIA from source with the 
            `make install` command.

INSTALL     How to run the `install` tool. Default is "install --preserve-timestamps"



Learning more about using Linux:
--------------------------------

If you are new to using GNU/Linux, some guides on getting started can be 
found at the following sites:

 * If your robot is using RedHat (it displays "RedHat Linux" on the console), 
   start with
   <https://www.redhat.com/docs/manuals/linux/RHL-7.3-Manual/getting-started-guide/>.
   More at <https://www.redhat.com/support/resources/howto/rhl73.html>.

 * If your robot is using Debian, start with 
   <http://www.us.debian.org/doc/manuals/debian-tutorial>. More is at
   <http://www.debian.org/doc/>.

 * If your robot is using Ubuntu 12.04, refer to
   <http://help.ubuntu.com/12.04/ubuntu-help/index.html> for documentation
   and help.

 * <http://www.tldp.org> is a repository of many different guides, FAQ and
   HOWTO documents about Linux in general, including various programming
   and system administration tasks. Note however that many details can 
   differ between Linux distributions and not all HOWTO documents will
   be relevant to your distribution.

For more depth, there are many books about using Linux, consult your
local computer bookseller. The ideal way to learn about Linux is to work 
with an experienced colleague who can demonstrate things and answer 
questions as they arise.



Learning C++
------------

If you are new to C++ programming, the definitive guide is Bjarne
Stroustrup's book "The C++ Programming Language".   The book
"C++ In a Nutshell", published by O'Reilly & Associates, is a
good quick guide and reference. There are also several websites
and many other books about C++.


Using ARIA from Java or Python
==============================

Even though ARIA was written in C++, it is possible to use ARIA from
the Java and Python programming languages as well. The directories
'python' and 'java' contain Python and Java packages automatically generated
by the 'swig' tool (http://www.swig.org) which mirror the ARIA API
closely, and are "wrappers" around the native ARIA library: When you call
an ARIA function in a Python or Java program, the wrapper re-marshalls the
function arguments and makes the equivalent call into the C++ ARIA library.
See the 'pythonExamples' and 'javaExamples' directories for more information
and example programs.  See the ARIA API Reference Manual in 'docs' for 
some more information.

More about the Python programming language is at <http://www.python.org>.
More about the Java programming language is at <http://java.sun.com>.


Using ARIA from Matlab or Simulink
==================================

As of ARIA 2.8, it is possible to use a small but essential subset of 
ARIA features from Matlab and Simulink.  This is done via MEX functions 
and a pure-C wrapper library called ariac.   Matlab functions and Simulink
blocks are provided for essential robot operation such as velocity control,
receiving position estimate and other basic information.  New functions
can be added to this interface using the already-implemented functions
as examples.

See matlab/README.md for details.

See ArNetworking/matlab/README.md for details on sending commands
to an ArNetworking server from Matlab.


MobileSim Simulator
===================

SRIsim is no longer included with Aria.  There is now a seperately
downloadable MobileSim simulator at our support webpage 
<http://robots.mobilerobots.com>.  MobileSim is compatible with SRISim,
but adds many new features.




-------------------------
Adept MobileRobots
10 Columbia Drive
Amherst, NH, 03031. USA

support@mobilerobots.com
http://robots.mobilerobots.com
http://www.mobilerobots.com


