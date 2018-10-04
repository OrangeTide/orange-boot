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

	if (offset + 32 >= 512) {
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

	if (ent.filename[0] & 0x80)
		return 0; /* deleted file - do not print ... */

	// TODO: return the structure
	printf("%06X:\t%.8s %.3s %12u\n", offset, ent.filename, ent.ext, ent.size);

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

	return 0;
}

static int
read_root_directory(void)
{
	unsigned root_offset = disk_info.number_of_fats * disk_info.size_of_fat + disk_info.reserved_blocks;
	unsigned i, offset;

	fprintf(stderr, "INFO:root directory at block %06X, contains %u entries\n",
		root_offset, disk_info.root_entries);

	if (read_sector(root_offset))
		return -1;

	/* loop through every directory entry */
	offset = 0;
	for (i = 0; i < disk_info.root_entries; i++) {
		if (read_directory_entry(offset))
			return -1;
		offset += 32;
		if (offset >= 512) {
			offset = 0;
			root_offset++;
			if (read_sector(root_offset))
				return -1;
		}
	}

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
			fprintf(stderr, "usage: fat [-h] [-o <filename>] [DIR|TYPE|COPY|REN|DEL] <args...> \n");
			return 1;
		case 'o':
			imagefilename = optarg;
			break;
		}
	}

	if (open_image())
		return 1;

	// TODO: parse command-line arguments
	for (i = optind; i < argc; i++) {
		printf("TODO: argv[%d]=%s\n", i, argv[i]);
	}

	read_boot_block();

	/* This program only supports filesystems with 512 byte sectors. */
	if (disk_info.bytes_per_block != 512) {
		fprintf(stderr, "%s:block size %d not supported\n", imagefilename, disk_info.bytes_per_block);
		goto failure;
	}

	if (read_root_directory())
		goto failure;

	cleanup();

	return 0;
failure:
	cleanup();

	fprintf(stderr, "%s:errors were detected!\n", imagefilename);

	return 1;
}
