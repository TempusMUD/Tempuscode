all: ped finderr

finderr: finderr.c
	gcc finderr.c -g -Wall -o finderr

ped: ped.c structs.h newstructs.h
	gcc -g -Wall ped.c -o ped

spec: spec.c
	g++ -I/usr/local/tempus/TempusMUD/src/include -g -Wall spec.c -o spec

pconvr: pconvr.c structs.h
	gcc -g -Wall pconvr.c -o pconvr
