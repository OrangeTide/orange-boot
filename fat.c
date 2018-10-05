/* fat.c : utility for accessing a raw image containing a FAT filesystem */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define READ_BYTE(ptr) \
	(((uint8_t*)(ptr))[0])
#define READ_WORD(ptr) \
	((((uint8_t*)(ptr))[0]) | (((uint16_t)((uint8_t*)(ptr))[1]) << 8))
#define READ_DWORD(ptr) \
	((((uint8_t*)(ptr))[0]) | \
	(((uint32_t)((uint8_t*)(ptr))[1]) << 8) | \
	(((uint32_t)((uint8_t*)(ptr))[2]) << 16) | \
	(((uint32_t)((uint8_t*)(ptr))[3]) << 24))

#define ATTR_READ_ONLY (1)
#define ATTR_HIDDEN (2)
#define ATTR_SYSTEM (4)
#define ATTR_VOLUME_LABEL (8)
#define ATTR_SUBDIR (16)
#define ATTR_ARCHIVE (32)

static FILE *imagef;
static const char *imagefilename = "floppy.img";
static uint8_t buf[512];
static struct {
	unsigned bytes_per_block;
	unsigned cluster_size; /* number of blocks per allocation unit */
	unsigned reserved_blocks; /* usually 1 */
	unsigned number_of_fats; /* number of FATs */
	unsigned root_entries; /* number of root directory entries */
	unsigned total_blocks;
	unsigned size_of_fat; /* number of blocks for one FAT */
} disk_info;

int
read_sector(unsigned n)
{
	fseek(imagef, n * 512, SEEK_SET);
	if (!fread(buf, sizeof(buf), 1, imagef) || ferror(imagef)) {
		perror(imagefilename);
		return -1;
	}
	return 0;
}

static int
read_directory_entry(unsigned offset)
{
	// 32-byte record:
	// 0x00 8	filename
	// 0x08 3	extension
	// 0x0B 1	attributes
	// 0x0C 10	reserved
	// 0x16 2	time (hour * 2048 + min * 32 + sec / 2)
	// 0x18 2	date ((year - 1980) * 512 + month * 32 + day)
	// 0x1A 2	starting cluster number
	// 0x1C 4	file size (bytes)
	//

	struct file_entry {
		char filename[8]; // NOTE: not nul-terminated
		char ext[3]; // NOTE: not nul-terminated
		uint8_t attr;
		uint8_t reserved[10];
		uint16_t time;
		uint16_t date;
		uint16_t starting_cluster;
		uint32_t size;
	} ent;

	if (offset + 32 > 512) {
		fprintf(stderr, "%s:offset exceeds sector size\n", imagefilename);
		return -1;
	}

	memcpy(ent.filename, buf + offset + 0x00, 8);
	memcpy(ent.ext, buf + offset + 0x08, 3);
	ent.attr = READ_BYTE(buf + offset + 0x0b);
	memcpy(ent.reserved, buf + offset + 0x0c, 10);
	ent.time = READ_WORD(buf + offset + 0x16);
	ent.date = READ_WORD(buf + offset + 0x18);
	ent.starting_cluster = READ_WORD(buf + offset + 0x1a);
	ent.size = READ_DWORD(buf + offset + 0x1c);

	if (ent.filename[0] == 0 || ent.filename[0] == (char)0xe5)
		return 0; /* deleted file - do not print ... */

	// TODO: return the structure
	printf("%06X:\t%.8s %.3s %c%c%c%c", offset, ent.filename, ent.ext,
		ent.attr & ATTR_HIDDEN ? 'H' : '-',
		ent.attr & ATTR_ARCHIVE ? 'A' : '-',
		ent.attr & ATTR_READ_ONLY ? 'R' : '-',
		ent.attr & ATTR_SYSTEM ? 'S' : '-');
	if (ent.attr & ATTR_SUBDIR)
		printf(" <DIR>       ");
	else if (ent.attr & ATTR_VOLUME_LABEL)
		printf(" <VOLUME>    ");
	else
		printf(" %12u", ent.size);

	printf(" (first $%04X)\n",
			ent.starting_cluster);

	return 0;
}

/* access important data in the boot block */
static int
read_boot_block(void)
{
	if (read_sector(0))
		return -1;

	disk_info.bytes_per_block = READ_WORD(buf + 0x00b);
	disk_info.cluster_size = READ_BYTE(buf + 0x00d);
	disk_info.reserved_blocks = READ_WORD(buf + 0x00e);
	disk_info.number_of_fats = READ_BYTE(buf + 0x010);
	disk_info.root_entries = READ_WORD(buf + 0x011);
	disk_info.total_blocks = READ_WORD(buf + 0x013);
	if (!disk_info.total_blocks)
		disk_info.total_blocks = READ_DWORD(buf + 0x020);
	disk_info.size_of_fat = READ_WORD(buf + 0x016);

	/* sanity check */
	if (disk_info.number_of_fats == 0 || disk_info.cluster_size == 0 || disk_info.bytes_per_block == 0) {
		fprintf(stderr, "%s:not a valid FAT file-system\n", imagefilename);
		return -1;
	}

	/* This program only supports filesystems with 512 byte sectors. */
	if (disk_info.bytes_per_block != 512) {
		fprintf(stderr, "%s:block size %d not supported\n", imagefilename, disk_info.bytes_per_block);
		return -1;
	}

	return 0;
}

static int
dir_sub_directory(int cluster)
{
	unsigned sector;

	//
	// TODO: look up chain of clusters in FAT
	//

	abort();

	if (read_sector(sector))
		return -1;

	return 0;
}

/* state related to accessing directories */
static unsigned current_directory_sector_origin;
static unsigned current_directory_sector_i;
static unsigned current_directory_entries;

/* load the first root sector */
static int
root_directory_first(void)
{
	current_directory_sector_origin = disk_info.number_of_fats * disk_info.size_of_fat + disk_info.reserved_blocks;
	current_directory_sector_i = 0;
	current_directory_entries = disk_info.root_entries;

	if (read_sector(current_directory_sector_origin))
		return -1;

	return 0;
}

/* continue loading next sector - return 0 when done, 1 to continue, and -1 on error */
static int
root_directory_next(void)
{
	if (current_directory_sector_i * 512 / 32 >= current_directory_entries)
		return 0;

	current_directory_sector_i++;
	if (read_sector(current_directory_sector_origin + current_directory_sector_i))
		return -1;

	return 1;
}

static int
dir_root_directory(void)
{
	int e;
	unsigned i;
	unsigned short offset;

	if (root_directory_first())
		return -1;

	i = 0;
	do {
		for (offset = 0; offset < 512; offset += 32, i++) {
			if (i >= current_directory_entries)
				break;
			if (read_directory_entry(offset))
				return -1;
		}
	} while ((e = root_directory_next()) > 0);

	return 0;
}

int
open_image(void)
{
	if (imagef)
		fclose(imagef);
	imagef = fopen(imagefilename, "rb"); // TODO: add write support
	if (!imagef) {
		perror(imagefilename);
		return -1;
	}

	return 0;
}

/* DIR */
static int
dir(const char *path)
{
	read_boot_block();

	//
	// TODO: parse path
	//

	if (dir_root_directory())
		return -1;

	return 0;
}

/* sector = 0, then use root directory */
static unsigned
lookup_file(unsigned sector, const char *filename)
{

	if (!sector) {
		if (root_directory_first())
			return -1;
	}

	// TODO: implement this
	abort();
}

/* CAT or TYPE a file */
static int
cat(const char *path)
{
	read_boot_block();

	abort(); // TODO: implement this

	/* Step 1. find the file */
	lookup_file(0, path);

	return 0;
}

/* creates an empty image. open imagef for writing */
static int
make_image(unsigned size)
{
	unsigned long total;

	if (!size)
		size = 1440 * 1024;

	imagef = fopen(imagefilename, "w+b");
	if (!imagef) {
		perror(imagefilename);
		fclose(imagef);
		return -1;
	}

	/* fill with zeros */
	memset(buf, 0, sizeof(buf));
	for (total = 0; total < size; total += sizeof(buf)) {
		fwrite(buf, sizeof(buf), 1, imagef);
		if (ferror(imagef)) {
			perror(imagefilename);
			return -1;
		}
	}

	rewind(imagef);

	return 0;
}

/* FORMAT */
static int
format(unsigned size, const char *bootloader)
{
	FILE *inf;

	if (!imagef)
		return -1; /* imagef must be opened for writing before calling this function */

	inf = fopen(bootloader, "rb");
	if (!inf) {
		perror(bootloader);
		return -1;
	}

	fseek(imagef, 0, SEEK_SET);

	/* copy the boot sector */
	while (fread(buf, sizeof(buf), 1, inf)) {
		fwrite(buf, sizeof(buf), 1, imagef);
		if (ferror(imagef)) {
			perror(imagefilename);
			fclose(inf);
			return -1;
		}
	}

	//
	// TODO: generate a proper file-system structure
	//

	fprintf(stderr, "Successfully written %s\n", imagefilename);

	fclose(inf);

	return 0;
}

void
cleanup(void)
{
	if (imagef) {
		fclose(imagef);
		imagef = NULL;
	}

	memset(&disk_info, 0, sizeof(disk_info));
}

int
main(int argc, char **argv)
{
	int c, i;

	while ((c = getopt(argc, argv, "ho:")) > 0) {
		switch (c) {
		case 'h':
usage:
			fprintf(stderr, "usage: fat [-h] [-o <filename>] [DIR|TYPE|COPY|REN|DEL] <args...> \n");
			return 1;
		case 'o':
			imagefilename = optarg;
			break;
		}
	}

	if (optind == argc) {
		if (open_image())
			return 1;

		if (dir("/"))
			goto failure;
	} else if (strcasecmp(argv[optind], "DIR") == 0) {
		if (open_image())
			return 1;

		for (i = optind; i < argc; i++) {
			if (dir(argv[i]))
				goto failure;
		}
	} else if (strcasecmp(argv[optind], "TYPE") == 0 || strcasecmp(argv[optind], "TYPE") == 0) {
		const char *filename = optind + 1 < argc ? argv[optind + 1] : "README";

		if (open_image())
			return 1;

		if (cat(filename))
			goto failure;
	} else if (strcasecmp(argv[optind], "FORMAT") == 0) {
		unsigned size = 1440 * 1024;
		const char *bootsectorfilename = "hello.bin";

		if (make_image(size))
			return -1;

		if (format(size, bootsectorfilename))
			goto failure;
	} else {
		for (i = optind; i < argc; i++) {
			printf("TODO: argv[%d]=%s\n", i, argv[i]);
		}
		goto usage;
	}
	cleanup();

	return 0;
failure:
	cleanup();

	fprintf(stderr, "%s:errors were detected!\n", imagefilename);

	return 1;
}
