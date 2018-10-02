/* hex.c : hex dump utility */
#include <stdio.h>
#include <ctype.h>

int
dump(FILE *f, const char *name)
{
	unsigned char buf[512];
	unsigned long cur = 0;
	unsigned n, i, j;


	while (!feof(f)) {
		if (ferror(f)) {
			perror(name);
			return -1;
		}
		n = fread(buf, 1, sizeof(buf), f);

		for (i = 0; i < n; i += 16, cur += 16) {

			/* output address */
			printf("%07lX:", cur);

			/* output 16 bytes in hex */
			for (j = 0; j < 16 && j + i < n; j++) {
				printf(" %02X", buf[j + i]);
			}
			
			/* fill truncated blocks with spaces */
			for (; j < 16; j++) {
				printf("   ");
			}

			printf(" | "); /* separater */

			/* output 16 bytes as characters */
			for (j = 0; j < 16 && j + i < n; j++) {
				int c = buf[j+i];
				if (isalnum(c) || isgraph(c)) {
					putchar(c);
				} else {
					putchar('.'); /* non-printable use '.' */
				}
			}
			putchar('\n');
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	int i;
	
	// TODO: print usage
	// TODO: parse option arguments
	for (i = 1; i < argc; i++) {
		const char *name = argv[i];
		FILE *f = fopen(name, "rb");
		if (!f) {
			perror(name);
			return 1;
		}
		if (dump(f, name)) {
			fclose(f);
			return 1;
		}

		fclose(f);
	}	

	return 0;
}
