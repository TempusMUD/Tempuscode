UTILS=hcontrol-view showplay economic_stats demographic_stats scan_rent purgeplay \
                    newdyncontrol clanview playerutil
SUBDIRS=hed ped oed
INCLUDE=../src/include
CXX=g++ -lcrypt -lxml2 -Wall -O3 -I$(INCLUDE)

default all:
	$(MAKE) $(UTILS) TARGET=$@
	$(MAKE) $(SUBDIRS) TARGET=$@

clean:
	@printf "Cleaning\n" $@ $(TARGET)
	@for file in $(UTILS); do\
		rm -f bin/$$file;\
	done;
	#$(MAKE) $(SUBDIRS) TARGET=$@

$(UTILS):
	$(CXX) -o bin/$@ src/$@.cc 

$(SUBDIRS) : FORCE
	cd $@ ; $(MAKE) $(TARGET) INCLUDE=../$(INCLUDE) MAKEFLAGS=-s

FORCE:
# -- blah
