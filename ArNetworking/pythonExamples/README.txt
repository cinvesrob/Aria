
USING THE PYTHON WRAPPER FOR ARNETWORKING

A "wrapper" module for Python has been provided in ARIA's "python" directory.
This wrapper layer provides a Python API which simply makes calls into the 
regular ArNetworking C++ implementation. In general, the Python API mirrors 
the C++ API, with some exceptions which are noted in the Reference Manual
for the C++ library.

The ArNetworking wrapper should be used in conjunction with the ARIA wrapper.
See the README.txt for the ARIA pythonExamples for details.

To use the wrapper modules, you must add ARIA's python directory to
an environment variable called PYTHONPATH. On Windows, the Aria 'bin'
directory must also be in your PATH variable.

Note that the Python wrapper API is not as well tested as Aria itself. If
you encounter problems, please notify the aria-users mailing list. Furthermore,
some methods have been omitted or renamed, and you have to do a few things
differently.


