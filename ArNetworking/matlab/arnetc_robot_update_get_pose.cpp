
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
   if(nlhs < 1 || nlhs > 3)
       mexErrMsgIdAndTxt("arnetc:arnetc_robot_update_handler:minlhs", "Incorrect number of output arguments. Must assign to a vector [x y theta] or to two scalars (x, y) or to three scalars (x, y, theta).");

   updateHandler->lock();
   ArPose pose = updateHandler->getPose();
   updateHandler->unlock();
   
   if(nlhs == 1)
   {
       plhs[0] = mxCreateDoubleMatrix(1, 3, mxREAL);
       double* p = mxGetPr(plhs[0]);
       p[0] = pose.getX();
       p[1] = pose.getY();
       p[2] = pose.getTh();
       return;
   }
   
   if(nlhs >= 2)
   {
         plhs[0] = mxCreateDoubleScalar(pose.getX());
         plhs[1] = mxCreateDoubleScalar(pose.getY());
         if(nlhs > 2)
             plhs[2] = mxCreateDoubleScalar(pose.getTh());
   }
}
	