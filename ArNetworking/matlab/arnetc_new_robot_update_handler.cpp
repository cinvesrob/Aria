

/* MEX function for Matlab */

#include "mex.h"
#include "ArClientBase.h"
#include "ArClientHandlerRobotUpdate.h"
#include <cstdint>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{   
   if(nrhs != 1 || nrhs != 2)
       mexErrMsgIdAndTxt( "arnetc:arnetc_robot_update_handler:minrhs", "Incorrect number of input arguments. Must provide connection handle returned by arnetc_connect, and optional request frequency");
   if(nlhs != 1)
       mexErrMsgIdAndTxt( "arnetc:arnetc_connect:minlhs", "Must assign result to an output variable.");
   
   uint64_t *p = (uint64_t*)mxGetData(prhs[0]);
   ArClientBase *client = (ArClientBase*) *p;

   double freq = 100;
   if(nrhs == 2)
     freq = mxGetScalar(prhs[0]);
   
   mexPrintf("arnetc_robot_update_handler: Requesting robot status, mode and data updates (%dms rate)...", freq);
   ArClientHandlerRobotUpdate *updateHandler = new ArClientHandlerRobotUpdate(client);
   updateHandler->requestUpdates((int)freq);
   
   plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
   uint64_t* r = (uint64_t*)mxGetData(plhs[0]);
   *r = (uint64_t)updateHandler;
}
	
