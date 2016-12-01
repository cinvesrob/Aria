

USING THE JAVA WRAPPER FOR ARNETWORKING

"Wrapper" libraries for Java have been provided for both Aria and ArNetworking
in Aria's "java" directory ("../java"). This wrapper layer provides a Java API which simply 
makes calls into the regular ArNetworking C++ implementation (using JNI).

To use the ArNetworking Java wrapper you need to download a Java SDK. Sun J2SE JDK
1.6 (Java SE6) was used to build the wrapper.  You can download it from:

    http://java.sun.com/javase/downloads

Find the Java SE Development Kit 6 and click "Download". (Only the JDK
is needed, ARIA does not need Java EE or any other extra Java technologies.)

The JDK also includes the runtime environment (JRE) that allows you to run 
Java programs.  If you only need the JRE, it can also
be obtained from <http://java.sun.com>.   

Install it on your system,  then you should put the bin
directory of the SDK into your PATH environment variable.

For information about Java on Debian GNU/Linux systems, install the 
"java-common" package, and see the Debian Java FAQ in 
    /usr/share/doc/java-common/debian-java-faq/index.html
Chapter 11 describes how to install the Java 2 SDK from Blackdown using 
Apt. 



In Windows you must then put the Aria\bin directory into your PATH enviornment
variable so that Java can find the dll needed to run the wrapper (use the System
control panel, or set in in the command shell prior to running java). On Linux,
Aria/lib should be in your LD_LIBRARY_PATH or in /etc/ld.so.conf.

For an example you should start the simulator, then go into
the javaExamples directory and compile the simple Java
example, and then run it.

On Windows:
    javac -classpath ../../java/Aria.jar;../../java/ArNetworking.jar simple.java
    java -cp ../../java/Aria.jar;../../java/ArNetworking.jar;. simple

On Linux:
    javac -classpath ../../java/Aria.jar:../../java/ArNetworking.jar simple.java
    java -cp ../../java/Aria.jar:../../java/ArNetworking.jar:. simple

The difference is the separator in the class path (; vs :).

The Java wrapper is not as well tested as ArNetworking itself. If you encounter
problems using it, please notify the aria-users mailing list. 


REBUILDING THE JAVA WRAPPER

If you want to rebuild the Java wrapper you need to install SWIG:
See <http://www.swig.org/download.html>  You should get at
least version 1.3.17.  You then need to set the environment variable
JAVA_INCLUDE to the include directory in your Java SDK, and
JAVA_BIN to the bin directory in your Java SDK.   (If you installed
the j2sdk1.4 from Blackdown on Debian, JAVA_INCLUDE should be 
/usr/lib/j2se/1.4/include).

In Linux, set your path so that swig is in the path then run 
'make java' in the Aria directory, then again in the ArNetworking directory. 

In Windows with Visual Studio .NET, open java/ArNetworkingJava.vcproj, select 
Tools menu -> Options -> Projects -> VC++ Directories and add your SWIG 
directory.  Then you should just be able to build the library.

