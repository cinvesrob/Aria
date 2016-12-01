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

%{
/* SWIG wrapper.i for ArNetworking */
%}

#ifdef SWIGPYTHON
%module(docstring="Python wrapper library for ArNetworking", directors="1") ArNetworkingPy
#else
# ifdef SWIGJAVA
%module(docstring="Java wrapper library for ArNetworking", directors="1") ArNetworkingJava
# else
%module(docstring="Wrapper library for ArNetworking", directors="1") ArNetworking
# endif
#endif

#ifdef SWIGIMPORTED
#warning Imported ArNetworking wrapper
#endif

%feature("autodoc", "1");
/*
%feature("directors", "1");*/ /* Enable on all classes. */

%{
#include <cstddef>
#include "Aria.h"
#include "ArNetworking.h"
#include "ArClientHandlerRobotUpdate.h"
#include "ArClientRatioDrive.h"
#include "ArServerModeJogPosition.h"
#include "../include/wrapper_ExtraClasses.h"
%}
%warnfilter(451) ArUtil;


/* Import all of Aria's wrapper classes */
#ifndef SWIG_IMPORTED_ARIA
#warning ArNetworking importing ARIA wrapper configuration...
%import ../include/wrapper.i
#define SWIG_IMPORTED_ARIA 1
#endif


/* In Java, we need to import the Aria package's namespace */
%typemap(javaimports) SWIGTYPE %{import com.mobilerobots.Aria.*;%}
%pragma(java) jniclassimports=%{import com.mobilerobots.Aria.*; %}
%pragma(java) moduleimports=%{import com.mobilerobots.Aria.*; %}


/* In Java, we need to override how SWIG defines some methods in the wrappers,
 * to be public not protected (the defaut javabody typemap is defined in 
 * java.swg) so that other packages such as ArNetworking and ARNL can access them: 
 */
%typemap(javabody) SWIGTYPE %{
  private long swigCPtr;
  protected boolean swigCMemOwn;

  /* for internal use by swig only */
  public $javaclassname(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  /* for internal use by swig only */
  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }
%}

%typemap(javabody_derived) SWIGTYPE %{
  private long swigCPtr;

  /* for internal use by swig only */
  public $javaclassname(long cPtr, boolean cMemoryOwn) {
    super($imclassname.SWIG$javaclassnameUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  /* for internal use by swig only */
  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }
%}


%typemap(javabody) SWIGTYPE *, SWIGTYPE &, SWIGTYPE [], SWIGTYPE (CLASS::*) %{
  private long swigCPtr;

  /* for internal use by swig only */
  public $javaclassname(long cPtr, boolean bFutureUse) {
    swigCPtr = cPtr;
  }

  protected $javaclassname() {
    swigCPtr = 0;
  }

  /* for internal use by swig only */
  public static long getCPtr($javaclassname obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }
%}



/* In Python, use typemaps to convert functions to functors: */
#ifdef SWIGPYTHON

%{
#include "../include/wrapper_Functors.h"
%}



/*
%{

  class ArPyFunctor_ServerData : 
    public ArFunctor2<ArServerClient*, ArNetPacket*>
  {
  protected:
    PyObject *pyFunction;
  public:
    ArPyFunctor_ServerData(PyObject* _f) : 
      pyFunction(_f),
      ArFunctor2<ArServerClient*, ArNetPacket*>()
    {
      Py_INCREF(pyFunction);
    }

    ~ArPyFunctor_ServerData() {
      Py_DECREF(pyFunction);
    }

    virtual void invoke() {
      Py_FatalError("ArPyFunctor_ServerData (for <ArServerClient*, ArNetPacket*>) invoked with no arguments!");
    }

    virtual void invoke(ArServerClient *cli) {
      Py_FatalError("ArPyFunctor_ServerData must be invoked with exactly 2 arguments (ArServerClient*, ArNetPacket*), 1 given.");
    }

    virtual void invoke(ArServerClient *cli, ArNetPacket *pkt)
    {
      PyObject *args = PyTuple_New(2);
      PyObject *cliObj = SWIG_Python_NewPointerObj(cli, SWIGTYPE_p_ArServerClient, 0);
      PyObject *pktObj = SWIG_Python_NewPointerObj(pkt, SWIGTYPE_p_ArNetPacket, 0);
      PyTuple_SetItem(args, 0, cliObj);
      PyTuple_SetItem(args, 1, pktObj);
      PyObject *r = PyObject_CallObject(pyFunction, args);
      if(!r)
      {
        fputs("** ArPyFunctor_ServerData: Error calling Python function: ", stderr);
        PyErr_Print();
      }
      Py_DECREF(args);
    }

    virtual const char* getName() {
      return (const char*) PyString_AsString(PyObject_Str(pyFunction));
    }
  };


%}
*/

%typemap(in) ArFunctor1<ArNetPacket *>* {
  $1 = new ArPyFunctor1<ArNetPacket *>($input);
}
%typecheck(SWIG_TYPECHECK_POINTER) ArFunctor1<ArNetPacket *>* {
  $1 = PyCallable_Check($input);
}

/*
%typemap(in) ArFunctor2<ArServerClient*, ArNetPacket*>* {
  $1 = new ArPyFunctor_ServerData($input); // XXX memory leak
}
%typecheck(SWIG_TYPECHECK_POINTER) ArFunctor2<ArServerClient*, ArNetPacket*>* {
  $1 = PyCallable_Check($input);
}
*/

%typemap(in) ArFunctor2<ArServerClient*, ArNetPacket*>* {
  $1 = new ArPyFunctor2<ArServerClient*, ArNetPacket*>($input);
}
%typecheck(SWIG_TYPECHECK_POINTER) ArFunctor2<ArServerClient*, ArNetPacket*>* {
  $1 = PyCallable_Check($input);
}


%typemap(in) ArFunctor1<const char*>* {
  $1 = new ArPyFunctor1_String($input);
}
%typecheck(SWIG_TYPECHECK_POINTER) ArFunctor1<const char*>* {
  $1 = PyCallable_Check($input);
}


#endif //SWIGPYTHON

/* But in Java, just name the functor templates and enable directors so you can subclass them: */
#ifdef SWIGJAVA
%feature("director") ArFunctor1<ArServerClient*>;
%feature("director") ArFunctor1<ArNetPacket*>;
%feature("director") ArFunctor2<ArServerClient*, ArNetPacket*>;
%template(ArFunctor_ServerClient) ArFunctor1<ArServerClient*>;
%template(ArFunctor_NetPacket) ArFunctor1<ArNetPacket*>;
%template(ArFunctor_ServerData) ArFunctor2<ArServerClient*, ArNetPacket*>;
#endif



/* include files */
/* Don't include md5.h */

/* base classes */
%include "ArNetPacket.h"
%include "ArServerBase.h"
%include "ArServerMode.h"

/* derived classes */
%include "ArClientArgUtils.h"
%include "ArClientBase.h"
%include "ArClientCommands.h"
%include "ArClientData.h"
%include "ArClientFileUtils.h"
%include "ArClientHandlerConfig.h"
%include "ArClientSimpleConnector.h"
%include "ArHybridForwarderVideo.h"
%include "ArNetPacketReceiverTcp.h"
%include "ArNetPacketReceiverUdp.h"
%include "ArNetPacketSenderTcp.h"
%include "ArServerClient.h"
%include "ArServerClientIdentifier.h"
%include "ArServerClientData.h"
%include "ArServerCommands.h"
%include "ArServerData.h"
#ifndef WIN32
%include "ArServerFileUtils.h"
#endif
%include "ArServerHandlerCamera.h"
%include "ArServerHandlerCameraCollection.h"
%include "ArServerHandlerCommMonitor.h"
%include "ArServerHandlerCommands.h"
%include "ArServerHandlerConfig.h"
%include "ArServerHandlerMap.h"
%include "ArServerHandlerMapping.h"
%include "ArServerInfoDrawings.h"
%include "ArServerInfoRobot.h"
%include "ArServerInfoSensor.h"
%include "ArServerInfoStrings.h"
%include "ArServerModeDrive.h"
%include "ArServerModeIdle.h"
%include "ArServerModeRatioDrive.h"
%include "ArServerModeStop.h"
%include "ArServerModeWander.h"
%include "ArServerSimpleCommands.h"
%include "ArServerSimpleOpener.h"
%include "ArServerUserInfo.h"
%include "ArClientHandlerRobotUpdate.h"

#include "../include/wrapper_ExtraClasses.h"

%{
/* End SWIG wraper.i for ArNetworking */
%}

