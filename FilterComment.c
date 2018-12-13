#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define HELPMESSAGE "\nusage: ment [option] <LaTeX file>\nOptions\n\t-h\tdisplay help\n"
#define IS_SEP_LINE(c) (c == '\n' || c == '\r')

void filterComment(FILE *fi, FILE *fo);

int main(int argc, char **argv) {		
	char option[256];
	FILE *fi, *fo;
	int i, j;
	for(i=0; i<256; i++)
		option[i] = 0; 
	for(i=1; i<argc && *(argv[i]) == '-'; i++) {
		for(j=1; argv[i][j] != '\0'; j++)
			option[argv[i][j]] = 1;
		if(option['h']) {
			option['h'] = 0;
			printf("%s\n", HELPMESSAGE);
			exit(0);
		}
	}
	if(i<argc && (fi = fopen(argv[i], "r"))) {
		i++;
		if(i<argc && (fo = fopen(argv[i], "w"))) {
			filterComment(fi, fo);
			fclose(fo);
		} else {
			fprintf(stderr, "Error while opening %s\n", argv[i]);
			exit(1);
		}
		fclose(fi);
	} else {
		fprintf(stderr, "Error while opening %s\n", argv[i]);
		exit(1);
	}
	return 0;
}

void filterComment(FILE *fi, FILE *fo) {
	char c;
	int state;
	state = 0;
	c = getc(fi);
	while(c!=EOF) {
		if(c == '"') {
			if(state == 2)
				state = 0;
			else
				state = 2;
		}
		if(c == '\\')
			state = 1;
		if(state == 0 && c == '%')
			while(c != EOF && !IS_SEP_LINE(c))
				c = getc(fi);
		else
			fprintf(fo, "%c", c);
		if(state == 1 && c != '\\')
			state = 0;
		c = getc(fi);
	}
}
