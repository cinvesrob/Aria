
default: makemex

all: default 

clean: FORCE
	rm -f test testcs tests ../lib/libariac.so ../lib/libariacs.so ../lib/libariac.a

../../lib/libAria.so: FORCE
	$(MAKE) -C ../.. lib/libAria.so

../../lib/libAria.a: FORCE
	$(MAKE) -C ../.. lib/libAria.a

../../lib/libArNetworking.so: FORCE
	$(MAKE) -C .. ../lib/libArNetworking.so

../../lib/libArNetworking.a: FORCE
	$(MAKE) -C .. ../lib/libArNetworking.a

%.so: ../../lib/%.so
	cp $< $@

dist: default doc

dist-all: dist libAria.so libArNetworking.so

doc: README.txt README.rtf README.pdf 

%.rtf: %.md
	pandoc -s --toc --template pandoc_template -f markdown -t rtf -o $@ $<

%.txt: %.md
	pandoc -s --toc -f markdown -t plain -o $@ $<

%.pdf: %.md
	pandoc -s --toc -f markdown -o $@ $<

PWD=$(shell pwd)

makemex: makemex.m 
	matlab -r "path(path, '$(PWD)') ; makemex ; quit"

FORCE:

.PHONY: default all clean doc dist makemex
