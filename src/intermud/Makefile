CXXFLAGS=$(CCFLAGS) -I$(INCLUDE)

default: all

include Makefile-depend

all: $(OBJFILES)

depend:
	../../bin/mkdep.sh *.cc > Makefile-depend

clean:
	rm -f $(OBJFILES) core \#*\# *~




