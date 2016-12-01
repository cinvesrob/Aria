
/* MEX function for Matlab */

#include "mex.h"
#include "ArClientBase.h"
#include "ArClientHandlerRobotUpdate.h"
#include <cstdint>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{   
   if(nrhs != 1)
       mexErrMsgIdAndTxt( "arnetc:arnetc_robot_update_get_vels:minrhs", "Incorrect number of input arguments. Must provide robot update handler object reference returned by arnetc_new_robot_update_handler.");
   uint64_t *p = (uint64_t*)mxGetData(prhs[0]);
   ArClientHandlerRobotUpdate *updateHandler = (ArClientHandlerRobotUpdate*) *p;
   if(nlhs < 1)
       mexErrMsgIdAndTxt("arnetc:arnetc_robot_update_get_vels:minlhs", "Incorrect number of output arguments. Must assign to a vector which will hold [xvel yvel rotvel] or to two scalars (xvel, rotvel) or three scalars (xvel, yvel, rotvel).");

   updateHandler->lock();
   ArClientHandlerRobotUpdate::RobotData data = updateHandler->getData();
   updateHandler->unlock();
   
   if(nlhs == 1)
   {
       plhs[0] = mxCreateDoubleMatrix(1, 3, mxREAL);
       double* p = mxGetPr(plhs[0]);
       p[0] = data.vel;
       p[1] = data.latVel;
       p[2] = data.rotVel;
   }
   else if(nlhs == 2)
   {
         plhs[0] = mxCreateDoubleScalar(data.vel);
         plhs[1] = mxCreateDoubleScalar(data.rotVel);
   }
   else if(nlhs > 2)
   {
       plhs[0] = mxCreateDoubleScalar(data.vel);
       plhs[1] = mxCreateDoubleScalar(data.latVel);
       plhs[2] = mxCreateDoubleScalar(data.rotVel);
   }
}
	