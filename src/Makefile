CXX=g++
INCLUDE=include
CCFLAGS=-g -Wall -p
CXXFLAGS=$(CCFLAGS) -I$(INCLUDE)

SUBDIRS= \
	ban boards bomb clan classes combat db dyntext editor elevators flow_room guns house \
	intermud interpreter magic mail matrix misc mobiles net objsave olc paths            \
	quest search shop smokes social specials util vehicle weather structs

MAKEFLAGS=-s
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
	@cd $@; $(MAKE) $(TARGET) INCLUDE=../$(INCLUDE) OBJ=../$(OBJ) CCFLAGS="$(CCFLAGS)"

$(SUBDIRS) : FORCE
	@cd $@; $(MAKE) $(TARGET) INCLUDE=../$(INCLUDE) OBJ=../$(OBJ) CCFLAGS="$(CCFLAGS)"


$(EXE): $(OBJ)/*.o
	$(CXX) $(CCFLAGS) $(OBJ)/*.o $(LIBS) -o $@

FORCE :
