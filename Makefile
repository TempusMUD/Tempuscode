SRC_FILES = src/*.c src/*.h src/Specs/*.spec src/Specs/*/*.spec src/Makefile \
	lib/tomes/src/* src/util/*

OBJ_FILES = src/*.o

DB_FILES  = lib/world/wld/*.wld lib/world/zon/*.zon\
	lib/world/mob/*.mob lib/world/obj/*.obj lib/world/shp/*.shp \
	lib/world/wld/index lib/world/wld/index.mini lib/world/zon/index \
	lib/world/zon/index.mini lib/world/mob/index lib/world/mob/index.mini \
	lib/world/obj/index lib/world/obj/index.mini lib/world/shp/index \
	lib/world/shp/index.mini

MISC_FILES = lib/misc/socials lib/misc/messages lib/misc/nasty \
	lib/misc/xnames lib/text/* 

all: FORCE
	@echo "[32m[5mMake what you fool!![0m"

countdb: FORCE
	bin/linecnt $(DB_FILES) > count_db.txt

countsrc: FORCE
	bin/linecnt $(SRC_FILES) > count_src.txt

cj: FORCE
	tar -cf - TARFILES src/*.o | (cd /home1/cj/DevelMUD; tar -xvf -)


coder:   FORCE
	tar -czf cdr_update.tar.gz $(SRC_FILES)

tempus: FORCE
	(cd ../TempusMUD;tar -czf oldsrc.tgz src/*)
	tar -cf - src/*.c src/*.h src/Makefile \
	  src/Specs/* | (cd ../TempusMUD; tar -xf -)
	echo "Replacing production intermud.h"
	cp src/tempus_intermud.h ../TempusMUD/src/intermud.h

library: FORCE 
	cp lib/misc/messages ../TempusMUD/lib/misc
	cp lib/misc/socials ../TempusMUD/lib/misc
	cp lib/text/* ../TempusMUD/lib/text
	cp lib/world/mob/*.mob ../TempusMUD/lib/world/mob/
	cp lib/world/mob/index* ../TempusMUD/lib/world/mob/
	cp lib/world/obj/*.obj ../TempusMUD/lib/world/obj/
	cp lib/world/obj/index* ../TempusMUD/lib/world/obj/
	cp lib/world/wld/*.wld ../TempusMUD/lib/world/wld/
	cp lib/world/wld/index* ../TempusMUD/lib/world/wld/
	cp lib/world/zon/*.zon ../TempusMUD/lib/world/zon/
	cp lib/world/zon/index* ../TempusMUD/lib/world/zon/
	cp lib/world/shp/*.shp ../TempusMUD/lib/world/shp/
	cp lib/world/shp/index* ../TempusMUD/lib/world/shp/

src_update: FORCE
	mv backup/src_update2.tgz backup/src_update1.tgz
	mv backup/src_update3.tgz backup/src_update2.tgz
	mv backup/src_update4.tgz backup/src_update3.tgz
	mv backup/src_update5.tgz backup/src_update4.tgz
	mv backup/src_update6.tgz backup/src_update5.tgz
	tar -czf backup/src_update6.tgz src/*.c src/*.h src/Makefile \
	src/Specs/*.spec src/Specs/*/*.spec src/util/*
	(cd backup;tar xzf src_update6.tgz)

small_update: FORCE
	tar -czf backup/src_update6.tgz src/*.c src/*.h src/Makefile \
	src/Specs/*.spec src/Specs/*/*.spec

update: FORCE
	make src_update
	scp backup/src_update6.tgz realm@sv:TempusMUD

jwatson-test: FORCE
	echo test

FORCE:
