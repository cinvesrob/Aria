
/* MEX function for Matlab */

#include "mex.h"
#include "ArClientBase.h"
#include "ArClientHandlerRobotUpdate.h"
#include <cstdint>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{   
   if(nrhs != 1)
       mexErrMsgIdAndTxt( "arnetc:arnetc_robot_update_handler:minrhs", "Incorrect number of input arguments. Must provide robot update handler object reference returned by arnetc_new_robot_update_handler.");
   uint64_t *p = (uint64_t*)mxGetData(prhs[0]);
   ArClientHandlerRobotUpdate *updateHandler = (ArClientHandlerRobotUpdate*) *p;
   if(nlhs != 1)
       mexErrMsgIdAndTxt("arnetc:arnetc_robot_update_handler:minlhs", "Incorrect number of output arguments. Must assign to a string variable.");

   updateHandler->lock();
   plhs[0] = mxCreateString(updateHandler->getMode());
   updateHandler->unlock();
   
}
	