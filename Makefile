all: tags check
binary:
	+make -C build all
check: binary
	+make -C build check
tags:
	+gtags
clean:
	+make -C build clean
