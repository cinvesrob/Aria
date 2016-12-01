

/* MEX function for Matlab */

#include "mex.h"
#include "Aria.h"
#include <cstdint>

void mexLog(const char *s)
{
    mexPrintf("aria: %s\n", s);
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   Aria::init();
   ArLog::setFunctor(new ArGlobalFunctor1<const char*>(&mexLog));
    // TODO delete this functor object in arnetc_shutdown mexFunction.
}
	

