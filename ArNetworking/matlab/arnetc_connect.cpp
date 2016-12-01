

/* MEX function for Matlab */

#include "mex.h"
#include "ArClientBase.h"
#include <cstdint>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   int port = 7272;
   if(nrhs < 1 || nrhs > 2)
       mexErrMsgIdAndTxt( "arnetc:arnetc_connect:minrhs", "Incorrect number of input arguments. Specify host or host, port.");
   const char *host = mxArrayToString(prhs[0]);
   if(nrhs > 1)
       port = mxGetScalar(prhs[1]);
   if(nlhs != 1)
       mexErrMsgIdAndTxt( "arnetc:arnetc_connect:minlhs", "Must assign result to an output variable.");
   plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
   uint64_t* p = (uint64_t*)mxGetData(plhs[0]);
   ArClientBase *client = new ArClientBase;
   mexPrintf("arnetc_connect: Connecting to %s:%d...\n", host, port);
   if(!client->blockingConnect(host, port))
   {
        mexPrintf("arnetc_connect: Error connecting to host %s at port %d.\n", host, port);
		delete client;
        *p = 0;
		return;
   }
   client->setRobotName(client->getHost());
   client->runAsync();
   mexPrintf("arnetc_connect: Connected to %s:%d. Running communications thread in background.\n", host, port);
   *p = (uint64_t)client;
}
	