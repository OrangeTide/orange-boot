all :: tools-all
clean :: tools-clean
tools-% :: ; $(MAKE) -f Makefile.tools $*

all :: bootsector-all
clean :: bootsector-clean
bootsector-% :: ; $(MAKE) -f Makefile.bootsector $*
