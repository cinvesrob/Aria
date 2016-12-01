

/* MEX function for Matlab */

#include "mex.h"
#include "Aria.h"
#include <cstdint>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  // TODO somehow get pointer to ArFunctor object creeated in arnetc_init
  // mexFunction and delete it.
   ArLog::clearFunctor();
   Aria::shutdown();
}
	
