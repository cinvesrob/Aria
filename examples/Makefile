# A simple Makefile to cause make to go look in the top directory. A simple
# convenience.

all: lib 
	$(MAKE) -C .. examples

lib:
	$(MAKE) -C .. lib/libAria.so

%.so: ../lib/libAria.so %.cpp 
	$(MAKE) -C .. examples/$@

%: ../lib/libAria.so %.cpp 
	$(MAKE) -C .. examples/$@

%Static: ../lib/libAria.a %.cpp 
	$(MAKE) -C .. examples/$@

clean: 
	$(MAKE) -C .. cleanExamples

../lib/libAria.so: FORCE
	$(MAKE) -C .. dirs lib/libAria.so

../lib/libAria.a: FORCE	
	$(MAKE) -C .. dirs lib/libAria.a

FORCE:

.PHONY: all FORCE clean lib
