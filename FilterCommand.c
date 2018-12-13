#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define HELPMESSAGE "\nusage: mand [option] <LaTeX file>\nOptions\n\t-c <Command name>\tset the command name\n\t-h\tdisplay help\n"

typedef enum {
	inText=0,
	inCommand,
	inArg} TypeState;

void filterCommand(FILE *fi, FILE *fo, char *command);

int main(int argc, char **argv) {		
	char option[256], *command;
	FILE *fi, *fo;
	int i, j;
	command = "\\modif";
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
		if(option['c']) {
			option['c'] = 0;
			command = argv[++i];
			i++;
		}
	}
	if(i<argc && (fi = fopen(argv[i], "r"))) {
		i++;
		if(i<argc && (fo = fopen(argv[i], "w"))) {
			filterCommand(fi, fo, command);
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

void filterCommand(FILE *fi, FILE *fo, char *command) {
	char c;
	int pos, depth = 0, length, startCommand=0, precCommand=0;
	TypeState state;
	length = strlen(command);
	state = inText;
	c = getc(fi);
	while(c!=EOF) {
		switch(state) {
			case inCommand:
				if(pos != length) {
					if(c != command[pos]) {
						if(pos>0) {
							int i;
							for(i=0; i<pos; i++)
								fprintf(fo, "%c", command[i]);
							state = 0;
						}
						fprintf(fo, "%c", c);
					} else
						pos++;
				} else {
					while(c == ' ' || c == '\t' || c == '\n' || c == '\r')
						c = getc(fi);
					if(c == '{') {
						state = inArg;
						depth = 1;
					} else
						fprintf(fo, " %c", c);
				}
				break;
			case inArg:
				switch(c) {
					case '{':
						if(!precCommand)
							depth++;
						fprintf(fo, "%c", c);
						break;
					case '}':
						if(!precCommand) {
							depth--;
							if(depth>0)
								fprintf(fo, "%c", c);
							else
								state = inText;
						} else
							fprintf(fo, "%c", c);
						break;
					default:
						fprintf(fo, "%c", c);
				}
				break;
			case inText:
			default:
				fprintf(fo, "%c", c);
		}
		c = getc(fi);
		precCommand = c != '\\' && startCommand;
		startCommand = c == '\\' && !startCommand;
		if(startCommand && state == inText) {
			pos = 0;
			state = inCommand;
		}
	}
}
		
					

				
				
					
	
