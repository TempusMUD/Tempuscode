all: ped finderr

finderr: finderr.c
	gcc finderr.c -g -Wall -o finderr

ped: ped.c structs.h newstructs.h
	gcc -g -Wall ped.c -o ped

pconvr: pconvr.c structs.h
	gcc -g -Wall pconvr.c -o pconvr
