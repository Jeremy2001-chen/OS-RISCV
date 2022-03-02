#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BMAX (4 << 25)
#define FNAMEMAX 1024

static int size;
static unsigned char binary[BMAX];

static void help(void) {
	printf(
		"convert ELF binary file to C file.\n"
		" -h            print this message\n"
		" -f <file>     tell the binary file  (input)\n"
		" -o <file>     tell the c file       (output)\n"
		" -p <prefix>   add prefix to the array name\n"
	);
}

int main(int argc, char *args[]) {
	char *prefix = NULL;
	char *binFile = NULL;
	char *outFile = NULL;
	int i;
	for (i = 1; i < argc; i++) {
		if (args[i][0] != '-') {
			continue;
		}
		if (strcmp(args[i], "-f") == 0) {
			assert(i + 1 < argc);
			assert(binFile == NULL);
			binFile = args[i + 1];
			i++;
		} else if (strcmp(args[i], "-o") == 0) {
			assert(i + 1 < argc);
			assert(outFile == NULL);
			outFile = args[i + 1];
			i++;
		} else if (strcmp(args[i], "-p") == 0) {
			assert(i + 1 < argc);
			assert(prefix == NULL);
			prefix = args[i + 1];
			i++; 
		} else if (strcmp(args[i], "-h") == 0 || args[i][0] == '-') {
			help();
			return 0;
		}
	}
	assert(binFile != NULL);
	assert(outFile != NULL);
	prefix = prefix != NULL ? prefix : "";
	FILE *bin = fopen(binFile, "rb");
	FILE *out = fopen(outFile, "w");
	for (i = 0; binFile[i] != '\0'; i++) {
		if (binFile[i] == '.') {
			binFile[i] = '\0';
			break;
		}
	}
	assert(bin != NULL);
	assert(out != NULL);
	int byte;
	fseek(bin, 0, SEEK_END);
	int binSize = ftell(bin);
	fseek(bin, 0, SEEK_SET);
	while (binSize--) {
		byte = fgetc(bin);
		assert(size < BMAX);
		binary[size] = byte;
		size++;
	}
	fprintf(
		out,
		"unsigned int binary%s%sSize = %d;\n"
		"unsigned char binary%s%sStart[] = {",
		prefix, binFile, size, prefix, binFile
	);
	for (i = 0; i < size; i++) {
		fprintf(out, "0x%x%c", binary[i], i < size - 1 ? ',' : '}');
	}
	fputc(';', out);
	fputc('\n', out);
	fclose(bin);
	fclose(out);
	return 0;
}
