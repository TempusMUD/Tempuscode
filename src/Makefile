CXX=g++
INCLUDE=include
CCFLAGS=-g -Wall -pg -Wcast-qual
CXXFLAGS=$(CCFLAGS) -I$(INCLUDE)

SUBDIRS= \
	ban boards bomb clan classes combat db dyntext editor elevators \
	flow_room guns help house intermud interpreter magic mail \
	matrix misc mobiles net objsave olc paths quest search shop \
	smokes social specials util vehicle weather structs 

MAKEFLAGS=-s -k 4
OBJ=ObjFiles
LIBS=-lcrypt
EXE=../bin/circle

default all:
	$(MAKE) $(INCLUDE) TARGET=$@
	$(MAKE) $(SUBDIRS) TARGET=$@
	$(MAKE) $(EXE)

clean:
	$(MAKE) $(SUBDIRS) TARGET=$@
	rm -f $(EXE)

depend:
	$(MAKE) $(SUBDIRS) TARGET=$@

$(INCLUDE) : FORCE
	@printf "[33m :: compiling %-20s[0m | [36m%s[0m\n" $@ $(TARGET)
	@cd $@; $(MAKE) $(TARGET) INCLUDE=../$(INCLUDE) OBJ=../$(OBJ) CCFLAGS="$(CCFLAGS)" MAKEFLAGS=-s

$(SUBDIRS) : FORCE
	@printf "[33m :: compiling %-20s[0m | [36m%s[0m\n" $@ $(TARGET)
	@cd $@; $(MAKE) $(TARGET) INCLUDE=../$(INCLUDE) OBJ=../$(OBJ) CCFLAGS="$(CCFLAGS)" MAKEFLAGS=-s


$(EXE): $(OBJ)/*.o
	@printf "[33m :: linking...[0m\n"
	$(CXX) $(CCFLAGS) $(OBJ)/*.o $(LIBS) -o $@ $(LDFLAGS)

FORCE :
