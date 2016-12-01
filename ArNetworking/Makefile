CFILEEXT:=cpp
# Make sure we're using a compiler (we probably don't need to define
# it but we have been so here it is unless it was already set)
ifndef CXX
CXX=g++
endif
BARECXXFLAGS:=-fPIC -g -Wall -D_REENTRANT
CXXFLAGS+=$(BARECXXFLAGS) -fno-exceptions 
CXXINC:=-I../include -Iinclude 
CXXLINK:=-L../lib -lAria -lArNetworking -lpthread -ldl -lrt \
	-Xlinker -Bstatic -lstdc++ -Xlinker -Bdynamic
CXXSTATICLINK:=-L../lib -Xlinker -Bstatic -lArNetworking -lAria -Xlinker -Bdynamic -lpthread -ldl -lrt -Xlinker -Bstatic -lstdc++ -Xlinker -Bdynamic

ifdef JAVA_BIN
JAVAC:=$(JAVA_BIN)/javac
JAR:=$(JAVA_BIN)/jar
else
JAVAC:=javac
JAR:=jar
endif

CFILES:= \
	ArCentralManager.cpp \
	ArCentralForwarder.cpp \
	ArClientBase.cpp \
	ArClientData.cpp \
	ArClientHandlerConfig.cpp \
	ArClientFileUtils.cpp \
	ArClientSimpleConnector.cpp \
	ArClientSwitchManager.cpp \
	ArClientArgUtils.cpp \
	ArClientHandlerRobotUpdate.cpp \
	ArHybridForwarderVideo.cpp \
	ArMapChanger.cpp \
	ArNetPacket.cpp \
	ArNetPacketReceiverUdp.cpp \
	ArNetPacketReceiverTcp.cpp \
	ArNetPacketSenderTcp.cpp \
	ArServerBase.cpp \
	ArServerClient.cpp \
	ArServerData.cpp \
	ArServerFileUtils.cpp \
	ArServerHandlerCamera.cpp \
	ArServerHandlerCameraCollection.cpp \
	ArServerHandlerCommands.cpp \
	ArServerHandlerCommMonitor.cpp \
	ArServerHandlerConfig.cpp \
	ArServerHandlerMap.cpp \
	ArServerHandlerMapping.cpp \
	ArServerHandlerPopup.cpp \
	ArServerMode.cpp \
	ArServerModeDrive.cpp \
	ArServerModeIdle.cpp \
	ArServerModeRatioDrive.cpp \
	ArServerModeStop.cpp \
	ArServerModeWander.cpp \
	ArServerInfoDrawings.cpp \
	ArServerInfoRobot.cpp \
	ArServerInfoSensor.cpp 	\
	ArServerInfoStrings.cpp \
	ArServerSimpleCommands.cpp \
	ArServerSimpleOpener.cpp \
	ArServerUserInfo.cpp \
	ArTempDirectoryHelper.cpp \
	md5.cpp


OTFILES:=$(patsubst %.$(CFILEEXT),%.o,$(CFILES))
OFILES:=$(patsubst %,../obj/%,$(OTFILES))

TARGETS:=../lib/libArNetworking.so examples

EXAMPLES_CPP:=$(shell find examples -name "*.$(CFILEEXT)" | grep -v Mod.cpp)
EXAMPLES:=$(patsubst %.$(CFILEEXT),%,$(EXAMPLES_CPP))

TESTS_CPP:=$(shell find tests -name "*.$(CFILEEXT)" | grep -v Mod.cpp)
TESTS:=$(patsubst %.$(CFILEEXT),%,$(TESTS_CPP))

SRC_FILES:=$(patsubst %,src/%,$(CFILES))
HEADER_FILES:=$(shell find include -type f -name \*.h)

all: dirs $(TARGETS)

dirs:
	@mkdir -p -v ../obj
	@mkdir -p -v ../lib

../lib/libArNetworking.so: $(OFILES) ../lib/libAria.so 
	@mkdir -p -v ../obj
	@mkdir -p -v ../lib
	$(CXX) -shared 	-Xlinker -Bstatic -lstdc++ -Xlinker -Bdynamic -o $(@) $(OFILES) 

../lib/libArNetworking.a: $(OFILES) ../lib/libAria.a 
	@mkdir -p -v ../obj
	@mkdir -p -v ../lib
	ar -cr $(@) $(OFILES)
	ranlib $(@)


examples: dirs $(EXAMPLES) 


tests: dirs $(TESTS) 

clean: cleanExamples
	rm -f ../lib/libArNetworking.so ../lib/libArNetworking.a \
	`find . -name core` $(OFILES) `find . -name '*~'`

cleanPython:
	-rm ../python/_ArNetworkingPy.so
	-rm ../python/ArNetworkingPy.py
	-rm ../python/ArNetworkingPy.pyc
	-rm ../python/ArNetworkingPy_wrap.cpp
	-rm ../obj/ArNetworkingPy_wrap.o

cleanExamples:
	rm -f $(EXAMPLES)

cleanDoc:
	-rm -f docs/*.html docs/*.png docs/*.png
	-rm -rf docs/tmp

fullClean: clean cleanExamples cleanJava cleanPython cleanDoc

# Rules to generate API documentation for local/personal use (rather than part of a
# released package; that uses the packaging shell scripts instead)
DATESTAMP=$(shell date +%Y%m%d)
DATE=$(shell date +'%B %d, %Y')
docs: doc
doc: docs/index.html
docs/index.html: $(SRC_FILES) $(HEADER_FILES) $(EXAMPLES_CPP) doxygen.conf docs/options/all_options.dox ../docs/index.html
	@echo Removing old ArNetworking documentation...
	$(MAKE) cleanDoc
	@echo
	@echo Making local ArNetworking documentation with version $(DATESTAMP)
	sed -e "s/@VERSION@/dev$(DATESTAMP)/g" -e "s/@DATE@/$(DATE)/g" <doxygen.conf.in >doxygen.conf
	-mkdir docs
	doxygen doxygen.conf
	@echo
	@echo Done making ArNetworking docs. Open docs/index.html in a web browser.
	
../docs/index.html: FORCE
	$(MAKE) -C .. docs/index.html

# Force rebuilding of Makefile.dep:
dep: clean cleanDep FORCE
	$(MAKE) Makefile.dep

Makefile.dep:
	if [ -f `echo src/*.cpp | cut -d' ' -f1` ]; then \
	$(CXX) $(CXXFLAGS) $(CXXINC) -MM src/*.cpp | \
	awk '$$1 ~ /:/{printf "../obj/%s\n", $$0} $$1 !~ /:/' > Makefile.dep; fi
	if [ -f `echo examples/*.cpp | cut -d' ' -f1` ]; then \
        $(CXX) $(CXXFLAGS) $(CXXINC) -MM examples/*.cpp | \
        awk '$$1 ~ /:/{printf "examples/%s\n", $$0} $$1 !~ /:/' | \
        sed 's/\.o//' >> Makefile.dep; fi

../lib/libAria.so: FORCE
	$(MAKE) -C .. dirs lib/libAria.so

../lib/libAria.a: FORCE
	$(MAKE) -C .. dirs lib/libAria.a

../obj/%.o : src/%.cpp 
	$(CXX) $(CXXFLAGS) $(CXXINC) -c $< -o $@

../obj/%.o : src/%.c 
	$(CXX) $(CXXFLAGS) $(CXXINC) -c $< -o $@

# Don't build .o files if their library is up to date with respect to source files:
.INTERMEDIATE: $(OFILES)

# But don't delete .o files if we do make them in order to make a library:
.PRECIOUS: $(OFILES)

examples/%: examples/%.$(CFILEEXT) ../lib/libAria.so ../lib/libArNetworking.so 
	$(CXX) $(CXXFLAGS) $(CXXINC) $< -o $@ $(CXXLINK)

tests/%: tests/%.$(CFILEEXT) ../lib/libAria.so ../lib/libArNetworking.so 
	$(CXX) $(CXXFLAGS) $(CXXINC) $< -o $@ $(CXXLINK)


examples/%Static: examples/%.$(CFILEEXT) ../lib/libAria.a ../lib/libArNetworking.a 
	$(CXX) $(CXXFLAGS) $(CXXINC) $< -o $@ $(CXXSTATICLINK)
	strip $@

tests/%Static: tests/%.$(CFILEEXT) ../lib/libAria.a ../lib/libArNetworking.a 
	$(CXX) $(CXXFLAGS) $(CXXINC) $< -o $@ $(CXXSTATICLINK)
	strip $@

CommandLineOptions.txt.in docs/options/all_options.dox: genCommandLineOptionDocs
	-mkdir docs/options
	./genCommandLineOptionDocs

genCommandLineOptionDocs: ../utils/genCommandLineOptionDocs.cpp ../lib/libAria.so ../lib/libArNetworking.so src/ArServerSimpleOpener.cpp src/ArClientSimpleConnector.cpp src/ArClientSwitchManager.cpp
	$(CXX) $(CXXFLAGS) -DFOR_ARNETWORKING $(CXXINC) $< -o $@ $(CXXLINK)

## Python SWIG wrapper ##

ifdef PYTHON_INCLUDE
PYTHON_INCLUDE_FLAGS=-I$(PYTHON_INCLUDE)
else
ifdef DIST_INSTALL
$(warning WARNING using default value of /usr/include/python2.5 for PYTHON_INCLUDE directory. Set PYTHON_INCLUDE environment variable if you would like to use a different version of the Python C development library to build the Python ARIA wrapper.)
endif
PYTHON_INCLUDE:=/usr/include/python2.5
PYTHON_INCLUDE_FLAGS=-I/usr/include/python2.5
endif

python: ariaPython ../python/_ArNetworkingPy.so

ariaPython:
	$(MAKE) -C .. python

../python/ArNetworkingPy_wrap.cpp ../python/ArNetworkingPy.py: include/wrapper.i $(HEADER_FILES)
	cd ../python; swig -c++ -python -module ArNetworkingPy -Dlinux -DAREXPORT -o ArNetworkingPy_wrap.cpp -I../include -I../ArNetworking/include ../ArNetworking/include/wrapper.i  

../python/_ArNetworkingPy.so: ../obj/ArNetworkingPy_wrap.o ../lib/libArNetworking.so 
	$(CXX) $(BARECXXFLAGS) -shared -o $(@) ../obj/ArNetworkingPy_wrap.o -lpthread -ldl -lrt \
	-L../lib -lAria -lArNetworking

../obj/ArNetworkingPy_wrap.o: ../python/ArNetworkingPy_wrap.cpp
	@ if test -z "$(PYTHON_INCLUDE)"; then echo "*** Error: PYTHON_INCLUDE is not set, cannot build ArNetworking python wrapper! ***"; exit 1; fi
	$(CXX) -c $(BARECXXFLAGS) -g $(CXXINC) -I$(PYTHON_INCLUDE) $< -o $@


## Java SWIG wrapper ##


java: ../lib/libArNetworkingJava.so ../java/ArNetworking.jar

../java/Aria.jar:
	$(MAKE) -C .. java

../java/ArNetworking.jar: ../java/com/mobilerobots/ArNetworking/ArServerBase.class
	cd ../java ; $(JAR) cf ArNetworking.jar com/mobilerobots/ArNetworking/*.class

../java/ArNetworkingJava_wrap.cpp ../java/com/mobilerobots/ArNetworking/ArServerBase.java: include/wrapper.i $(HEADER_FILES)
	-mkdir -p ../java/com/mobilerobots/ArNetworking
	rm ../java/com/mobilerobots/ArNetworking/*.java ../java/ArNetworkingJava_wrap.cpp ../java/ArNetworkingJava_wrap.h ; swig -c++ -java -package com.mobilerobots.ArNetworking -outdir ../java/com/mobilerobots/ArNetworking -module ArNetworkingJava -Dlinux -DAREXPORT -o ../java/ArNetworkingJava_wrap.cpp -I../include -Iinclude include/wrapper.i 
 
../java/com/mobilerobots/ArNetworking/ArServerBase.class: ../java/com/mobilerobots/ArNetworking/ArServerBase.java ../java/Aria.jar
	 rm ../java/com/mobilerobots/ArNetworking/*.class; $(JAVAC) -classpath ../java/Aria.jar:../java ../java/com/mobilerobots/ArNetworking/*.java 

cleanJava:
	-rm ../java/ArNetworking.jar
	-rm -r ../java/com/mobilerobots/ArNetworking
	-rm ../java/ArNetworkingJava_wrap.* ../obj/ArNetworkingJava_wrap.o ../lib/libArNetworkingJava.so

../lib/libArNetworkingJava.so: ../obj/ArNetworkingJava_wrap.o ../lib/libArNetworking.so
	$(CXX) $(BARECXXFLAGS) -shared -o $(@) ../obj/ArNetworkingJava_wrap.o -lpthread -ldl -lrt -L../lib -lAria -lArNetworking

../obj/ArNetworkingJava_wrap.o: ../java/ArNetworkingJava_wrap.cpp
	@ if test -z "$(JAVA_INCLUDE)"; then echo "*** Error: JAVA_INCLUDE is not set, building ArNetworkingPy_wrap.cpp will probably fail! ***"; fi
	$(CXX) -c $(BARECXXFLAGS) $(CXXINC) -I$(JAVA_INCLUDE) -I$(JAVA_INCLUDE)/linux $< -o $@


include Makefile.dep

FORCE:
