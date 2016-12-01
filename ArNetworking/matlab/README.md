% ArNetworking Client Interface for Matlab
% November 25, 2013

ArNetworking Client Interface for Matlab
========================================

This interface provides a basic interface to an ArNetworking server.  It can 
be used to send command requests to the server. An interface to server mode,
status and robot data (position, velocity, etc.) updates is also provided.

The interface uses MEX to provide Matlab functions that call the appropriate
API in the ArNetworking library.  ArNetworking and ARIA DLLs or shared
libraries must be present at runtime.   You must use compatible ArNetworking
and ARIA DLLs for the platform and archtecture; if using 32-bit Matlab on
Windows, you must use `ArNetworkingVC10.dll` and `ARIAVC10.dll` 32-bit
libraries; if using 64-bit Matlab on Windows, you must use
`ArNetworkingVC10.dll` and  `ARIAVC10.dll` 64-bit libraries.

To run scripts/programs in Matlab, the matlab directory (containing the compiled
MEX objects as well as ArNetworkingVC10.dll and AriaVC10.dll) must be in your
Matlab path. It can be added by right-clicking on the matlab directory in the
file browser in Matlab and selecting "Add To Path", then "This Folder".

Or use the path command, 

    path(path, 'C:\Program Files\MobileRobots\Aria\ArNetworking\matlab')

Or if your current directory is the directory containing this interface:

    path(path, '.')  

API Summary
-----------

`arnetc_init` - call once at start of script to initialize ARIA and
ArNetworking. OS sockets are initialized, some default internal setup is
performed, and ARIA logging is redirected to Matlab output.

`arnetc_shutdown` - call at end of a script to shut down and uninitialize ARIA
and ArNetworking. Any running threads are stopped, and ARIA log redirection to
Matlab output window is removed. While a script may use `arnetc_connect` and
`arnetc_disconnect` multiple times if neccesary as it runs, `arnetc_init` and
`arnetc_shutdown` should only be called once each per script run.

`c = arnetc_connect(host)` - Connect to ArNetworking server running on default 
port (7272) on host with hostname `host`. A handle for the connection is 
returned, or 0 on error connecting.  It must be cleaned up by calling 
`arnetc_disconnect(c)` when done.

`c = arnetc_connect(host, port)` - Connect to ArNetworking server running on 
host with hostname `host` on TCP port `port`. A handle for this connection is 
returned, or 0 on error connecing. It must be cleaned up by calling 
`arnetc_disconnect(c)` when done.

`arnetc_disconnect(c)` - Disconnect connection with connection handle `c`. This
must be called to disconnect and clean up resources allocated by 
`arnetc_connect`.

`arnetc_request(c, request)` - Send command request. `c` is a connection handle
returned by `arnetc_connect`.  `request` is a string containing name of the 
request. (See examples below.)

`arnetc_request(c, request, param)` - Send command request. `c` is a connection
handle returned by `arnetc_connect`.  `request` is a string containing name of
the request. (See examples below.)  `param` is a parameter for `request`; may be
a string, integer or vector (matrix).

`ru = arnetc_new_robot_update_handler(c, freq)` - Create a new handler for robot
status and data updates, and begin requesting updates.  `c` is a connection
handle returned by `arnetc_connect`. `freq` is an optional update frequency 
to request; it may be omitted (default is 100ms).  Use the functions below to 
read the most recently received data.   Must be cleaned up by calling
`arnetc_delete_robot_update_handler(ru)` when done.

`arnetc_delete_robot_update_handler(ru)` - This must be called to stop
requesting updates and clean up resources allocated by
`arnetc_new_robot_update_handler`.

`[x, y, th] = arnetc_robot_update_get_pose(ru)` - Get most recently received
robot position estimate.

`[xvel, yvel, thvel] = arnetc_robot_update_get_vels(ru)` - Get most recently
received robot velocity measurement.  `xvel` will be translational
(forward/backward) speed. `yvel` will be lateral (left/right) speed, and will
always be 0 for differential drive robots (all robots except Seekur).  `thvel`
will be rotational velocity (clockwise/counterclockwise).

`s = arnetc_robot_update_get_status(ru)` - Get most recently received robot
status string.   This depends on the server mode. See the "Status" field
displayed in MobileEyes for examples.

`m = arnetc_robot_update_get_mode(ru)` - Get most recently received robot mode
name.  This is displayed in the "Mode" field in MobileEyes.

### Important notes: ###
* `arnetc_connect` and `arnetc_new_robot_update_handler` create objects and
background threads which reference objects.  Therefore,
you must use `arnetc_disconnect` and `arnetc_delete_robot_update_handler` to
avoid invalid references which would cause Matlab to crash, 
as well as resource leaks that consume memory.  
* `arnetc_init` redirects ARIA logging to the Matlab output window. You must
call `arnetc_shutdown` at the end of a script to clear this to avoid reference
to an invalid callback.
* All functions use the ArNetworking and ARIA shared libraries, as do background
threads created. Therefore these libraries must remain
loaded by Matlab, or Matlab will crash.  A command such as `clear all` can cause
the libraries to be unloaded and objects referenced by threads to become
invalid, causing Matalb to crash.  Disconnect from server(s) and delete robot
update handlers, and call `arnetc_shutdown` before clearing variables or shared
libraries.

Examples
--------

See `example.m` for a simple example.

Some ArNetworking server requests that you may wish to use with the
`arnetc_request` function include the following. In the examples below, `c` is
a nonzero value returned by `arnetc_connect`. Some requests may or may not be
provided by different ArNetworking servers. See server program and ArNetworking
C++ library reference documentation for more information on requests and their
arguments.

* `arnetc_request(c, 'gotoGoal', name)` - Navigate to a goal by name. Provide
  name as string. Provided by ARNL servers (`ArServerModeGoto`).
* `arnetc_request(c, 'gotoPose', position)` - Navigate to a position. Provide
  position as vector [x, y, th]. Provided by ARNL servers (`ArServerModeGoto`).
* `arnetc_request(c, 'tourGoals')` - Navigate to each goal in the map in turn.
Provided by ARNL servers only (`ArServerModeGoto`).
* `arnetc_request(c, 'TourGoalsInList', 'Goal1,Goal2,Goal3')` - Navigate to each
  goal in the comma-separated list provided, in turn. Provided by ARNL servers
  only (`ArServerModeGoto`)
* `arnetc_request(c, 'home')` - Navigate to (the first) home point in the map,
  if present. Provided by ARNL servers only (`ArServerModeGoto`).
* `arnetc_request(c, 'stop')` - Enter stop mode (`ArServerModeStop`) 
* `arnetc_request(c, 'wander')` - Enter wander mode, if server provides it
  (`ArServerModeWander`)

You can also use `arnetc_request` to send any commands provided on the server
via `ArServerHandlerCommands` (e.g. see the dropdown list of "custom commands"
in MobileEyes.)

Use `arnetc_new_robot_update_handler` to create a new object in the background
which will receive and store robot data (e.g. position, battery state),
mode and status updates from the server.   

Examples of server modes are:
* `Stop` - Stopped, doing nothing.
* `Goto goal` - ARNL is going to a goal or has reached the goal.
* `Goto point` - Going to a given (x, y, th) position, or reached the point.
* `Dock` - Going to rechanging station, or at recharging station.
* `Touring goals` - Touring goals
* `Idle` - Performing pending work before entering a new mode.  Used when
  configuration or map changes.  Status will be `Idle processing`.
* `Drive` - Manual drive via MobileEyes or joystick.
* `Wander` - Wander mode

Examples of status strings provided by the `Goto goal` and `Goto point` modes
are:
* Any string starting with the word `Failed` - goal navigation failed.
* Any string starting with the word `Going` - going to a goal
* Any string starting with the word `Arrived` - arrived successfully at a goal.


More examples of mode and status strings can be seen in MobileEyes, or in the
output of `example.m`.

The server mode can change due to requests received that switch to a new mode,
either from your program or from other clients connected to the server (e.g.
MobileEyes). Server status is specific to each mode.  Each mode has a set of
status states it may show at any given time.  Sometimes a status string contains
variable text (e.g. the name of a goal in `Goto goal` mode.), but the rest of
the string will be predictible (especially the beginning) and can be parsed by
software.  Mode and status strings are considered part of the ArNetworking/ARNL
interface and will not be changed in new software releases.


Building on Windows
-------------------

If you wish to modify and rebuild the MEX functions, use the following
instructions.  The ArNetworking client matlab package must be placed
in a `matlab` subdirectory of ArNetworking. (E.g. on Windows, place it
in `C:\Program Files\MobileRobots\Aria\ArNetworking\matlab`)

Requirements: 

* ARIA 2.7.5.2 or later <http://robots.mobilerobots.com/wiki/ARIA>
* Visual Studio 2010 
  <http://www.microsoft.com/visualstudio/eng/downloads#d-2010-express>
* If building on 64-bit you must also have the Windows Platform SDK 7.1 (See
  ARIA README.txt)
* Matlab 2012b or 2013a (other versions may work but are untested)
* Windows 7 
 
Build:
 
1. This directory should be a subdirectory of ArNetworking named `matlab` (see
   above).
2. Open Matlab and navigate to this `matlab` directory.
3. Add this `matlab` directory to the Matlab path by right-clicking on it in the
   Matlab file browser and selecting "Add To Path" then "This Folder".
4. If you have not yet done so already, run `mex -setup` in Matlab.  Select MS
   Visual C++ 2010 as the compiler if on Windows 32-bit, or Windows Platform SDK
   with MS Visual C++ 2010 on 64-bit.
5. Run `makemex` in Matlab to compile the MEX interfaces. It will also copy 
   `AriaVC10.dll` and `ArNetworkingVC10.dll` from bin or bin64 into the `matlab`
   directory.
 
 
The MEX interfaces require `ariac.dll` and `AriaVC10.dll` to be present in the
matlab directory to build and run.
 
To run scripts/programs in Matlab, the matlab directory (containing the compiled
MEX objects as well as ariac.dll and AriaVC10.dll) must be in your Matlab path.
It can be added by right-clicking on the matlab directory in the file browser in
Matlab and selecting "Add To Path", then "This Folder".

Or use the path command: 

    path(path, 'C:\Program Files\MobileRobots\Aria\ArNetworking\matlab')

Or if your current directory is the Aria matlab subdirectory: 

    path(path, '.')  


Building on Linux
------------------

Requirements:

* ARIA 2.7.5.2 or later <http://robots.mobilerobots.com/wiki/ARIA>
* G++ compiler and GNU make installed (standard Linux development tools). The
  compiler version must match supported MEX compiler (see below).
* Matlab 2012a or later

Build:

1. This directory should be a subdirectory of ArNetworking named `matlab` (see
   above).
2. Enter the matlab directory and `makemex` within Matlab. If the 
   `matlab` command is not in your PATH, you can add it with this shell command:
   `export PATH=$PATH:/usr/local/MATLAB/R2012b/bin`
   
Substitute the correct installation location and version for your Matlab
installation. If any warnings are printed regarding unsupported compiler
versions, you may need to switch to a supported compiler as reported by Matlab.
You can do so by installing the correct compiler, setting the CC and CXX
environment variables to that compiler command, and rebuilding ARIA (enter 
`/usr/local/Aria`, run `make clean`, then run `make`.

The  matlab directory (containing the compiled MEX interfaces) must be in your
Matlab path. It can be added by right-clicking on the Aria matlab directory in
the GUI and selecting "Add To Path" then "This Folder", or by using the path
command:

    path(path, '/usr/local/Aria/ArNetworking/matlab')

If you wish to always add the Aria matlab directory to your Matlab path, you can
do so in one of two ways:

You an append `/usr/local/Aria/ArNetworking/matlab` to the `MATLABPATH` 
environment variable in your `.matlab7rc.sh` script in your home directory if 
you wish to permanently add the Aria matlab directory to your Matlab path:

    export MATLABPATH=$MATLABPATH:/usr/local/Aria/ArNetworking/matlab

(See <http://www.mathworks.com/help/matlab/ref/matlabunix.html> for more about
the `.matlab7rc.sh` script)

Or, you can create a `startup.m` script in a directory where you will keep your
Matlab scripts that use the Aria interface.  If you run the `matlab` command
from within this directory, then Matlab will automatically run `startup.m`.
Your `startup.m` can use the `path` function to add the Aria matlab directory 
to the path, and execute any other commands you wish.

    path(path, '/usr/local/Aria/ArNetworking/matlab')




How to add new functions to the Matlab interface
------------------------------------------------

The Aria-Matlab interface is intended to be a base upon which any additional
features  and functions you require can be easily added.

To add a function to the Matlab interface that is not currently available,
create a new C++ file definining a `mexFunction`. See the existing MEX
functions for examples of how to cast the pointer value provided as an argument
to `ArClientBase*`.  

If you have called any of the functions in your currently running Matlab
instance, run `clear all` in Matlab to unload the functions and the ariac
library/DLL (after disconnecting from all servers and deleting all robot
update handler objects; see notes above.)

Finally, add the new function to the list of functions in `makemex.m`, and
re-run `makemex.m` in Matlab.




