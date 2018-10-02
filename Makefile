CC=ia16-elf-gcc
AS=ia16-elf-as
LD=ia16-elf-ld
OBJCOPY=ia16-elf-objcopy
OBJDUMP=ia16-elf-objdump
ifeq ($(OS),Windows_NT)
RM=del
endif

all ::
clean ::

%.elf : %.o
	$(LD) -Ttext=0x7c00 -nostartfiles -nostdlib -o $@ $^

%.bin : %.elf
	$(OBJCOPY) -O binary $< $@

%.dump.txt : %.elf
	$(OBJDUMP) -M data16 -d $< > $@
##

all :: hello.bin
clean :: ; $(RM) hello.bin
.PRECIOUS : hello.elf

##

all :: tools-all
clean :: tools-clean
tools-% :: ; $(MAKE) -f Makefile.tools $*
# .PHONY : tools-clean tools-all
