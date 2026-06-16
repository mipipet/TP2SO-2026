#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "modulePacker.h"

static int parse_arguments(int argc, char *argv[], struct arguments *arguments);
static void print_usage(const char *program);

int main(int argc, char *argv[]) {
	
	struct arguments arguments;

	arguments.output_file = OUTPUT_FILE;
	arguments.count = 0;

	if (!parse_arguments(argc, argv, &arguments)) {
		return 1;
	}

	array_t fileArray = {arguments.args, arguments.count};

	if(!checkFiles(fileArray)) {
		return 1;
	}	

	return !buildImage(fileArray, arguments.output_file);
}

int buildImage(array_t fileArray, char *output_file) {

	FILE *target;

	if((target = fopen(output_file, "w")) == NULL) {
		printf("Can't create target file\n");
		return FALSE;
	}

	//First, write the kernel
	FILE *source = fopen(fileArray.array[0], "r");
	write_file(target, source);

	//Write how many extra binaries we got.
	int extraBinaries = fileArray.length - 1;
	fwrite(&extraBinaries, sizeof(extraBinaries), 1, target);	
	fclose(source);

	int i;
	for (i = 1 ; i < fileArray.length ; i++) {
		FILE *source = fopen(fileArray.array[i], "r");
		
		//Write the file size;
		write_size(target, fileArray.array[i]);

		//Write the binary
		write_file(target, source);

		fclose(source);

	} 
	fclose(target);
	return TRUE;
}


int checkFiles(array_t fileArray) {

	int i = 0;
	for(; i < fileArray.length ; i++) {
		if(access(fileArray.array[i], R_OK)) {
			printf("Can't open file: %s\n", fileArray.array[i]);
			return FALSE;
		}
	}
	return TRUE;

}

int write_size(FILE *target, char *filename) {
	struct stat st;
	stat(filename, &st);
	uint32_t size = st.st_size;
	fwrite(&size, sizeof(uint32_t), 1, target);
	return TRUE;
}


int write_file(FILE *target, FILE *source) {
	char buffer[BUFFER_SIZE];
	int read;

	while (!feof(source)) {
		read = fread(buffer, 1, BUFFER_SIZE, source);
		fwrite(buffer, 1, read, target);
	}

	return TRUE;
}


static int parse_arguments(int argc, char *argv[], struct arguments *arguments) {
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'o' && argv[i][2] == '\0') {
			if (i + 1 >= argc) {
				print_usage(argv[0]);
				return FALSE;
			}
			arguments->output_file = argv[++i];
		} else if (argv[i][0] == '-' && argv[i][1] == '-' &&
				   argv[i][2] == 'o' && argv[i][3] == 'u' &&
				   argv[i][4] == 't' && argv[i][5] == 'p' &&
				   argv[i][6] == 'u' && argv[i][7] == 't' &&
				   argv[i][8] == '\0') {
			if (i + 1 >= argc) {
				print_usage(argv[0]);
				return FALSE;
			}
			arguments->output_file = argv[++i];
		} else if (argv[i][0] == '-' && argv[i][1] == '-' &&
				   argv[i][2] == 'o' && argv[i][3] == 'u' &&
				   argv[i][4] == 't' && argv[i][5] == 'p' &&
				   argv[i][6] == 'u' && argv[i][7] == 't' &&
				   argv[i][8] == '=') {
			arguments->output_file = &argv[i][9];
		} else if (argv[i][0] == '-') {
			print_usage(argv[0]);
			return FALSE;
		} else {
			if (arguments->count >= MAX_FILES) {
				printf("Too many input files\n");
				return FALSE;
			}
			arguments->args[arguments->count++] = argv[i];
		}
	}

	if (arguments->count < 1) {
		print_usage(argv[0]);
		return FALSE;
	}

	return TRUE;
}

static void print_usage(const char *program) {
	printf("Usage: %s KernelFile Module1 Module2 ... [-o FILE]\n", program);
}
