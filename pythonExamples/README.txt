


USING THE PYTHON WRAPPER FOR ARIA
---------------------------------

A "wrapper" library for Python has been provided in the "python" directory.
This wrapper layer provides a Python API which simply makes calls into the 
regular Aria C++ library (libAria.so/AriaVC10.dll). In general, the Python API 
mirrors the C++ API. 

The Aria Python wrapper currently requires Python version 2.4.4 if on Debian 3,
Python 2.5 if on Debian 5, or Python 2.7 if on Ubuntu 12 or Windows.  Multiple versions of
Python may be installed on Debian Linux; if so, Debian provides commands such as 
'python2.4' or 'python2.5' or 'python2' to invoke the different versions of
Python.

	NOTE: You should use the versions of Python mentioned above, since the wrapper 
	library code is generally only compatible with the runtime libraries provided 
	by these versions.  To use a different version of Python, you must 
	rebuild the wrapper library (see below).

You can download Python from http://www.python.org/

On Debian, you can install Python using the following command
when connected to the Internet:

    sudo apt-get install python

Or, you can download Python 2.5.5 from:
    http://www.python.org/download/releases/2.5.5

On Ubuntu, you can install Python 2.7 with the following command:

    sudo apt-get install python2.7


  (NOTE if using Python 2.6 or later: Python 2.6 allows native C++ DLLs to be 
  named with a ".pyd" file extension, rather than .dll/.so as was 
  allowed in previous versions.  So if Python is unable to import the 
  AriaPy DLL  module, rename _AriaPy.dll or _AriaPy.so to _AriaPy.pyd 
  in the python/ subdirectory.)

Documentation about the Python language and libraries is at:
    http://docs.python.org

To use the wrapper library you must add ARIA's python directory to PYTHONPATH and the system PATH:  

First, create an environment variable called PYTHONPATH 
that has the full path to Aria's 'python' directory in it:

On Linux, set it to:
	/usr/local/Aria/python  

On Windows, set it to:
	C:\Program Files\MobileRobots\Aria\python

On Windows, also add the following to PATH:
	C:\Program Files\MobileRobots\Aria\python;C:\Program Files\MobileRobots\Aria\bin

Environment variables are set in Windows in the Advanced settings section of the System control panel.

If using the Windows command terminal, you can also temporarily add the ARIA bin and python directories to your PATH environment variable like this:
 	set PATH=%PATH%;C:\Program Files\MobileRobots\Aria\bin;C:\Program Files\MobileRobots\Aria\python

On Linux you can add bin to your PATH like this:
	export PATH=$PATH:/usr/local/Aria/bin

For an example you can start the simulator, then enter 
the pythonExamples directory and run:

    python simple.py

On Windows, you can just double-click simple.py (if you have configured PYTHONPATH in the System control panel), or you may need to reference the Python interpreter by full path:

	C:\Python27\python.exe simple.py

	NOTE: The Python wrapper API is not as well tested as Aria itself. If
	you encounter problems, please notify the aria-users mailing list. Furthermore,
	some methods have been omitted or renamed, and you have to do a few things
	differently (see next section).




USING ACTIONS IN PYTHON
-----------------------

Writing classes in Python that subclass classes from Aria is not as
straightforward as making subclasses of native Python classes.  Subclasses
of classes in the wrapper are not direct subclasses of the C++ classes,
they are only subclasses of the Python wrapper classes, which implement the logic
to redirect calls between the C++ and wrapper classes. The practical
consequences of this include the following:

1. You cannot call a virtual method in the parent class which the subclass
   overrides -- it will always be directed to the subclass's implementation.
   For example, you can override ArAction::setRobot(), which is a virtual
   method. But then, any call to setRobot() on the subclass will always 
   be directed to the subclass's implementation; calling
   self.ArAction.setRobot() from you subclass's setRobot() override would
   result in an infinite recursion, so Swig throws an exception instead.
   There is no workaround for this other than extending each parent class 
   in wrapper.i ad-hoc to include an additional method with a new name to 
   invoke the method defined in the parent.  We have done this for a few
   cases (such as ArAction::setRobot()), but If this ever becomes neccesary
   for any others, please let us know and we will add the extension.

2. Swig currently does not make protected methods available to
   subclasses, though this may be fixed in the future.


OTHER DIFFERENCES
-----------------

See the C++ API docs for various classes for notes about how they might be
used differently in Python. For example, ArConfigArg and ArFunctor are used
slightly differently (since those classes use features unique to C++).



REBUILDING THE PYTHON WRAPPER
-----------------------------

You must rebuild the Python wrapper if you make any interface changes to
ARIA and want them to be available in Python.  You may also need to rebuild
the wrapper if you want to use another version of Python than was used to build the Python wrapper.

If you want to rebuild the Python wrapper you need to install SWIG, you
can get it from http://www.swig.org/download.html (use version 1.3.29 or newer.)  
You'll also need Python installed (see above).

On Linux, you may also need to install a Python "Development" package in addition
to the main Python runtime package.  e.g. on Debian, install python-dev. On 
Windows, if the python24_d.lib library is missing, you can download the
Python 2.4 source code and compile the "python" project in the PCbuild subdirectory.

Set the environment variable PYTHON_INCLUDE to the full path of Python's
include directory.  On Linux, this is usually /usr/include/python2.4 for 
Python 2.4 or /usr/include/python2.2 for Python 2.2 or /usr/include/python2.7 
for Python 2.7.  On Windows, it depends
on where you installed Python but a typical value is C:\Python27\include for
Ptyhon 2.7).  

On Windows you'll also need to set the environment variables
PYTHON_LIB to the full path of the Python library and PYTHON_LIBDIR to the directory
containing all the Python libs.  These depend on where
you installed Python, but a typical value would be "C:\Python24\libs\python27.lib" for
PYTHON_LIB and "C:\Python27\libs" for PYTHON_LIBDIR for Python 2.7.
Set environment variables in the Advanced section of the System control panel.

On Linux Run 'make python' in the Aria directory, and again run 'make python' in
the ArNetworking subdirectory if you also want the wrapper library for
ArNetworking.  On Windows, open the AriaPy-vc2012.sln file in the 'python'
directory with Visual Studio 2012 and build AriaPy and, if desired, 
ArNetworkingPy in Release configuration. 

 