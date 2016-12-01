

/* MEX function for Matlab */

#include "mex.h"
#include "ArClientBase.h"
#include <cstdint>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   if(nrhs != 1)
       mexErrMsgIdAndTxt( "arnetc:arnetc_connect:minrhs", "Incorrect number of input arguments. Must provide connection handle returned by arnetc_connect.");
   uint64_t *p = (uint64_t*)mxGetData(prhs[0]);
   uint64_t i = *p;
   mexPrintf("disconnecting %ld...\n", i);
   ArClientBase *client = (ArClientBase*) i;
   if(client == 0)
       mexErrMsgIdAndTxt("arnetc:arnetc_disconnect", "NULL connection handle.");
   mexPrintf("arnetc_disconnect: Disconnecting from server and destroying client connection...\n");
   client->disconnect();
   client->stopRunning();
   delete client;

}
	