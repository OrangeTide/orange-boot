#include <stdio.h>
#include <string.h>

const char *bootloader = "hello.bin";
const unsigned long size = 1440 * 1024;
const char *image = "floppy.img";

int
main(int argc, char **argv)
{
	FILE *inf = fopen(bootloader, "rb");
	if (!inf) {
		perror(bootloader);
		return 1;
	}
	FILE *outf = fopen(image, "wb");
	if (!outf) {
		perror(image);
		fclose(inf);
		return 1;
	}
	char buf[512];
	unsigned long total;

	total = 0;
	/* copy the boot sector */
	while (fread(buf, sizeof(buf), 1, inf)) {
		fwrite(buf, sizeof(buf), 1, outf);
		total += sizeof(buf);
	}

	/* fill with zeros */
	memset(buf, 0, sizeof(buf));	
	while (total < size) {
		fwrite(buf, sizeof(buf), 1, outf);
		total += sizeof(buf);
	}	

	fprintf(stderr, "Successfully written %s\n", image);

	return 0;
}
