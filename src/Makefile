CXX=g++
INCLUDE=include
CCFLAGS=-g -Wall -Winline -Wcast-qual -I/usr/include/libxml2 -I/usr/local/pgsql/include -I/usr/include/pgsql -pg
CXXFLAGS=$(CCFLAGS) -I$(INCLUDE)

SUBDIRS= \
	clan classes combat db dyntext editor \
	flow_room help house interpreter magic \
	misc mobiles net objects olc quest search \
	social specials util weather structs map

MAKEFLAGS=-s -k 4 $(PARALLEL_MAKE)
OBJ=ObjFiles
LIBS=-L/usr/local/pgsql/lib -lcrypt -lxml2 -lpq
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
# DO NOT DELETE
