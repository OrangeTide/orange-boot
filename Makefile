# top-level Makefile that recursively calls other makefiles

all :: tools-all
clean :: tools-clean
tools-% :: ; $(MAKE) -f Makefile.tools $*

all :: bootsector-all
clean :: bootsector-clean
bootsector-% :: | tools-%
	$(MAKE) -f Makefile.bootsector $*
