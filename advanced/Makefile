# A simple $(MAKE)file to cause make to go look in the top directory. A simple
# convenience.

%.so: %.cpp 
	$(MAKE) -C .. advanced/$@

%: %.cpp 
	$(MAKE) -C .. advanced/$@

all:
	$(MAKE) -C .. advanced

clean:
	$(MAKE) -C .. cleanAdvanced

# netAudioAndSpeech needs extra libraries:

ifndef CXX
CXX=g++
endif

netAudioAndSpeech: netAudioAndSpeech.cpp ../lib/libArNetAudio.so ../lib/libArNetworking.so ../lib/libArSpeechSynth_Cepstral.so ../lib/libAria.so
	$(CXX) $(CFLAGS) -o $@ -I../include -I../ArNetworking/include -I../ArNetAudio/include -I../ArSpeechSynth_Cepstral/include $< -L../lib -lArNetAudio -lArNetworking -lArSpeechSynth_Cepstral -lAria -lswift -lspeex -lportaudio -lportmixer


# Make dependent libraries. Assumes that lib is in sibling directory with
# matching name.
../lib/lib%.so: ../%/src/*.cpp ../%/include/*.h
	$(MAKE) -C ../$* $@

../lib/libAria.so:
	$(MAKE) -C .. lib/libAria.so
