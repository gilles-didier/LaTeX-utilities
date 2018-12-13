#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#define HELPMESSAGE "\nusage: flat [option] <LaTeX file> [output file]\nOptions\n\t-h\tdisplay help\n"
#define INC_SIZE_DICT 100
#define MAX_DICT_LENGTH 1000
#define END_LEXI_TREE INT_MAX

typedef struct LEXI_NODE {
    char symbol;
    int child, sibling;
} TypeLexiNode;

typedef struct LEXI_TREE {
    TypeLexiNode *node;
    int root, size, sizeBuf;
} TypeLexiTree;

typedef struct COMMAND {
    char *name, *defArg, *command;
    int index, nArg;
} TypeCommand;

static int findWordLexi(char *w, TypeLexiTree *dict);
static int addWordLexi(char *w, int index, TypeLexiTree *dict);
static void initLexiNode(char symbol, TypeLexiNode *n);
static TypeLexiTree *newLexiTree();
static void freeLexiTree(TypeLexiTree *dict);

static int isSpace(char c);
static int isEndCommand(char c);
static char *strdpl(char *s);
static void printCommand(FILE *f, TypeCommand c);
static char *readFile(FILE *f);
static void writeFile(FILE *f, char *text);
static char *parse(char *in);
static char *substituteCommand(char *in, TypeLexiTree *dict, TypeCommand *command);
static char *getCommandResult(TypeCommand command, char **arg);

int main(int argc, char **argv) {		
	char option[256], *in, *out, buffer[100], *outputFileName;
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
	if(i<argc) {
		if((fi = fopen(argv[i++], "r"))) {
			in = readFile(fi);
			fclose(fi);
		} else {
			fprintf(stderr, "Error while opening %s\n", argv[i]);
			exit(1);
		}
	} else {
		fprintf(stderr, "Error No file name provided\n");
		exit(1);
	}
	if(i<argc) {
		outputFileName = argv[i];
	} else {
		char *tmp;
		strcpy(buffer, argv[i-1]);
		if((tmp = strrchr(buffer, '.')) != NULL)
			tmp[0] = '\0';
		strcat(buffer,"_flat.tex");
		outputFileName = buffer;
	}
	if((fo = fopen(outputFileName, "w"))) {
		out = parse(in);
		writeFile(fo, out);
		fclose(fo);
	} else {
		fprintf(stderr, "Error while opening %s\n", argv[i]);
		exit(1);
	}
	free((void*)in);
	free((void*)out);
	return 0;
}

#define INC_COMMAND 20;
#define SIZE_BUFFER_CHAR 1000;

char *getCommandResult(TypeCommand command, char **arg) {
	char *out;
	int k, sizeOut, posCom, posOut;
	sizeOut = strlen(command.command);
	for(k=0; k<command.nArg; k++)
		sizeOut += strlen(arg[k]);
	out = (char*) malloc(sizeOut*sizeof(char));
	posCom = 0;
	posOut = 0;
	while(command.command[posCom] != '\0') {
		char buffer[10];
		if(command.command[posCom] == '#' && (posCom == 0 || command.command[posCom-1]!='\\')) {
			int j;
			posCom++;
			for(j=0; isdigit(command.command[posCom]); posCom++, j++)
				buffer[j] = command.command[posCom];
			buffer[j] = '\0';
			k = atol(buffer)-1;
			if(posOut >= sizeOut) {
				sizeOut += SIZE_BUFFER_CHAR;
				out = (char*) realloc(out, sizeOut*sizeof(char));
			}
			out[posOut++] = '{';
			for(j=0; arg[k][j] != '\0'; j++) {
				if(posOut >= sizeOut) {
					sizeOut += SIZE_BUFFER_CHAR;
					out = (char*) realloc(out, sizeOut*sizeof(char));
				}
				out[posOut++] = arg[k][j];
			}
			if(posOut >= sizeOut) {
				sizeOut += SIZE_BUFFER_CHAR;
				out = (char*) realloc(out, sizeOut*sizeof(char));
			}
			out[posOut++] = '}';
		} else {
			if(posOut >= sizeOut) {
				sizeOut += SIZE_BUFFER_CHAR;
				out = (char*) realloc(out, sizeOut*sizeof(char));
			}
			out[posOut++] = command.command[posCom++];
		}		
	}
	if(posOut >= sizeOut) {
		sizeOut += SIZE_BUFFER_CHAR;
		out = (char*) realloc(out, sizeOut*sizeof(char));
	}
	out[posOut] = '\0';
	return out;
}

char *substituteCommand(char *in, TypeLexiTree *dict, TypeCommand *command) {
	char *out, *buffer;
	int sizeIn = strlen(in), sizeOut, posIn, posOut;
	sizeOut = sizeIn;
	out = (char*) malloc(sizeOut*sizeof(char));
	buffer = (char*) malloc(sizeIn*sizeof(char));
	posIn = 0;
	posOut = 0;
	while(posIn < sizeIn) {
		if(in[posIn] != '\\') {
			if(in[posIn] == '%' && (posIn == 0 || in[posIn-1] != '\\')) {
				while(posIn < sizeIn && in[posIn] != '\r' && in[posIn] != '\n') {
					if(posOut >= sizeOut) {
						sizeOut += SIZE_BUFFER_CHAR;
						out = (char*) realloc(out, sizeOut*sizeof(char));
					}
					out[posOut++] = in[posIn++];
				}
			} else {
				if(posOut >= sizeOut) {
					sizeOut += SIZE_BUFFER_CHAR;
					out = (char*) realloc(out, sizeOut*sizeof(char));
				}
				out[posOut++] = in[posIn++];
			}
		} else {
			int i, j, current;
			for(i = posIn+1, j = 0; i<sizeIn && !isEndCommand(in[i]); i++, j++)
				buffer[j] = in[i];
			buffer[j] = '\0';
			current = findWordLexi(buffer, dict);
			if(current != END_LEXI_TREE) {
				char **arg = NULL, *result;
				int k, del0 = 1;
				posIn = i;
				if(command[current].nArg>0) {
					arg = (char**) malloc(command[current].nArg*sizeof(char*));
					k = 0;
					if(command[current].defArg != NULL) {
						for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
							;	
						if(in[posIn] == '[') {
							int depth = 0;
							posIn++;
							for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
								;
							for(j = 0; posIn<sizeIn && !(in[posIn] == ']' && depth == 0); posIn++, j++) {
								buffer[j] = in[posIn];
								if(in[posIn] == '[')
									depth++;
								if(in[posIn] == ']')
									depth--;
							}
							if(in[posIn] != ']') {
								fprintf(stderr, "Error command default arg not closed with ] (%s)\n", buffer);
								exit(1);
							} else
								posIn++;
							buffer[j] = '\0';
							arg[k++] = substituteCommand(buffer, dict, command);
						} else {
							arg[k++] = command[current].defArg;
							del0 = 0;	
						}
					}
					for(; k<command[current].nArg; k++) {
						for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
							;	
						if(in[posIn] == '{') {
							int depth = 0;
							posIn++;
							for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
								;
							for(j = 0; posIn<sizeIn && !(in[posIn] == '}' && depth == 0); posIn++, j++) {
								buffer[j] = in[posIn];
								if(in[posIn] == '{')
									depth++;
								if(in[posIn] == '}')
									depth--;
							}
							if(in[posIn] != '}') {
								fprintf(stderr, "Error command arg not closed with } (%s)\n", buffer);
								exit(1);
							} else
								posIn++;
							buffer[j] = '\0';
							arg[k] = substituteCommand(buffer, dict, command);
						} else {
							fprintf(stderr, "Error command arg not starting with { (%s %c)\n", buffer, in[posIn]);
							exit(1);
						}
							
					}
				}
				result = getCommandResult(command[current], arg);
				if(arg != NULL) {
					if(del0)
						free((void*)arg[0]);
					for(k=1; k<command[current].nArg; k++)
						free((void*)arg[k]);
					free((void*)arg);
				}
				for(j=0; result[j] != '\0'; j++) {
					if(posOut >= sizeOut) {
						sizeOut += SIZE_BUFFER_CHAR;
						out = (char*) realloc(out, sizeOut*sizeof(char));
					}
					out[posOut++] = result[j];
				}
				free((void*)result);
			} else {
				if(posOut >= sizeOut) {
					sizeOut += SIZE_BUFFER_CHAR;
					out = (char*) realloc(out, sizeOut*sizeof(char));
				}
				out[posOut++] = in[posIn++];
			}
		}
	}
	free((void*)buffer);
	if(posOut >= sizeOut) {
		sizeOut += SIZE_BUFFER_CHAR;
		out = (char*) realloc(out, sizeOut*sizeof(char));
	}
	out[posOut] = '\0';
	return out;
}

char *parse(char *in) {
	char *out, *buffer;
	int sizeIn = strlen(in), sizeOut, posIn, posOut, sizeCommand, index, i;
	TypeCommand *command;
	TypeLexiTree *dictA, *dictB;
	
	sizeCommand = INC_COMMAND;
	command = (TypeCommand*) malloc(sizeCommand*sizeof(TypeCommand));
	dictA = newLexiTree();
	addWordLexi("newcommand", 0, dictA);
	addWordLexi("renewcommand", 1, dictA);
	addWordLexi("providecommand", 1, dictA);
	dictB = newLexiTree();
	sizeOut = sizeIn;
	out = (char*) malloc(sizeOut*sizeof(char));
	buffer = (char*) malloc(sizeIn*sizeof(char));
	index = 0;
	posIn = 0;
	posOut = 0;
	while(posIn < sizeIn) {
		if(in[posIn] != '\\') {
			if(in[posIn] == '%' && (posIn == 0 || in[posIn-1] != '\\')) {
				while(posIn < sizeIn && in[posIn] != '\r' && in[posIn] != '\n') {
					if(posOut >= sizeOut) {
						sizeOut += SIZE_BUFFER_CHAR;
						out = (char*) realloc(out, sizeOut*sizeof(char));
					}
					out[posOut++] = in[posIn++];
				}
			} else {
				if(posOut >= sizeOut) {
					sizeOut += SIZE_BUFFER_CHAR;
					out = (char*) realloc(out, sizeOut*sizeof(char));
				}
				out[posOut++] = in[posIn++];
			}
		} else {
			int i, j;
			for(i = posIn+1, j = 0; i<sizeIn && !isEndCommand(in[i]); i++, j++)
				buffer[j] = in[i];
			buffer[j] = '\0';
			if(findWordLexi(buffer, dictA) != END_LEXI_TREE) {
				if(index >= sizeCommand) {
					sizeCommand += INC_COMMAND;
					command = (TypeCommand*) realloc((void*)command, sizeCommand*sizeof(TypeCommand));
				}
				posIn = i;
				for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
					;
				if(in[posIn] == '{') {
					posIn++;
					for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
						;
					if(in[posIn] != '\\') {
						fprintf(stderr, "Command must start with \\\n");
						exit(1);
					}
					posIn++;
					for(j = 0; posIn<sizeIn && in[posIn] != '}' && !isSpace(in[posIn]); posIn++, j++)
						buffer[j] = in[posIn];
					if(posIn<sizeIn && isSpace(in[i])) {
						for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
							;
						if(in[posIn] != '}') {
							fprintf(stderr, "Error command name not closed with }\n");
							exit(1);
						} else
							posIn++;
					} else
						posIn++;
					buffer[j] = '\0';
					command[index].name = strdpl(buffer);
					for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
						;
					if(posIn<sizeIn && in[posIn] == '[') {
						posIn++;
						for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
							;
						for(j = 0; posIn<sizeIn && in[posIn] != ']' && !isSpace(in[posIn]); posIn++, j++)
							buffer[j] = in[posIn];
						if(posIn<sizeIn && isSpace(in[i])) {
							for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
								;
							if(in[posIn] != ']') {
								fprintf(stderr, "Error command number args not closed with ]\n");
								exit(1);
							} else
								posIn++;
						} else
							posIn++;
						buffer[j] = '\0';
						command[index].nArg = atol(buffer);
					} else {
						command[index].nArg = 0;
						command[index].defArg = NULL;
					}
					for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
						;
					if(posIn<sizeIn && in[posIn] == '[') {
						posIn++;
						for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
							;
						for(j = 0; posIn<sizeIn && in[posIn] != ']'; posIn++, j++)
							buffer[j] = in[posIn];
						if(in[posIn] != ']') {
							fprintf(stderr, "Error command default args not closed with ] (%s)\n", buffer);
							exit(1);
						} else
							posIn++;
						buffer[j] = '\0';
						command[index].defArg = substituteCommand(buffer, dictB, command);
					} else {
						command[index].defArg = NULL;
					}
					for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
						;
					if(posIn<sizeIn && in[posIn] == '{') {
						int depth = 0;
						posIn++;
						for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
							;
						for(j = 0; posIn<sizeIn && !(in[posIn] == '}' && depth == 0); posIn++, j++) {
							buffer[j] = in[posIn];
							if(in[posIn] == '{')
								depth++;
							if(in[posIn] == '}')
								depth--;
						}
						if(in[posIn] != '}') {
							fprintf(stderr, "Error command definition not closed with } (%s)\n", buffer);
							exit(1);
						} else
							posIn++;
						buffer[j] = '\0';
						command[index].command = substituteCommand(buffer, dictB, command);
					} else {
						fprintf(stderr, "Error empty command (%s)\n", buffer);
						exit(1);
					}				
				} else {
					fprintf(stderr, "Error command name not closed with } (%s)\n", buffer);
					exit(1);
				}
				addWordLexi(command[index].name, index, dictB);
				index++;
			} else {
				int current = findWordLexi(buffer, dictB);
				if(current != END_LEXI_TREE) {
					char *result, **arg = NULL;
					int k, del0 = 1;
					posIn = i;
					if(command[current].nArg>0) {
						arg = (char**) malloc(command[current].nArg*sizeof(char*));
						k = 0;
						if(command[current].defArg != NULL) {
							for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
								;	
							if(in[posIn] == '[') {
								int depth = 0;
								posIn++;
								for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
									;
								for(j = 0; posIn<sizeIn && !(in[posIn] == ']' && depth == 0); posIn++, j++) {
									buffer[j] = in[posIn];
									if(in[posIn] == '[')
										depth++;
									if(in[posIn] == ']')
										depth--;
								}
								if(in[posIn] != ']') {
									fprintf(stderr, "Error command default arg not closed with ] (%s)\n", buffer);
									exit(1);
								} else
									posIn++;
								buffer[j] = '\0';				
								arg[k++] = substituteCommand(buffer, dictB, command);
							} else {
								arg[k++] = command[current].defArg;
								del0 = 0;	
							}
						}
						for(; k<command[current].nArg; k++) {
							for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
								;	
							if(in[posIn] == '{') {
								int depth = 0;
								posIn++;
								for(; posIn<sizeIn && isSpace(in[posIn]); posIn++)
									;
								for(j = 0; posIn<sizeIn && !(in[posIn] == '}' && depth == 0); posIn++, j++) {
									buffer[j] = in[posIn];
									if(in[posIn] == '{')
										depth++;
									if(in[posIn] == '}')
										depth--;
								}
								if(in[posIn] != '}') {
									fprintf(stderr, "Error command arg not closed with } (%s)\n", buffer);
									exit(1);
								} else
									posIn++;
								buffer[j] = '\0';
								arg[k] = substituteCommand(buffer, dictB, command);
							} else {
								fprintf(stderr, "Error command arg not starting with { (%s %c)\n", buffer, in[posIn]);
								exit(1);
							}
						}
					}
					result = getCommandResult(command[current], arg);
					if(arg != NULL) {
						if(del0)
							free((void*)arg[0]);
						for(k=1; k<command[current].nArg; k++)
							free((void*)arg[k]);
						free((void*)arg);
					}
					if(posOut >= sizeOut) {
						sizeOut += SIZE_BUFFER_CHAR;
						out = (char*) realloc(out, sizeOut*sizeof(char));
					}
					out[posOut++] = '{';
					for(j=0; result[j] != '\0'; j++) {
						if(posOut >= sizeOut) {
							sizeOut += SIZE_BUFFER_CHAR;
							out = (char*) realloc(out, sizeOut*sizeof(char));
						}
						out[posOut++] = result[j];
					}
					if(posOut >= sizeOut) {
						sizeOut += SIZE_BUFFER_CHAR;
						out = (char*) realloc(out, sizeOut*sizeof(char));
					}
					out[posOut++] = '}';
					free((void*)result);
				} else {
					if(posOut >= sizeOut) {
						sizeOut += SIZE_BUFFER_CHAR;
						out = (char*) realloc(out, sizeOut*sizeof(char));
					}
					out[posOut++] = in[posIn++];
				}
			}
		}
	}
	free((void*)buffer);
	freeLexiTree(dictA);
	freeLexiTree(dictB);
	for(i=0; i<index; i++) {
		if(command[i].name != NULL)
			free((void*)command[i].name);
		if(command[i].command != NULL)
			free((void*)command[i].command);
		if(command[i].defArg != NULL)
			free((void*)command[i].defArg);
	}
	free((void*)command);
	if(posOut >= sizeOut) {
		sizeOut += SIZE_BUFFER_CHAR;
		out = (char*) realloc(out, sizeOut*sizeof(char));
	}
	out[posOut] = '\0';
	return out;
}
				
int isSpace(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int isEndCommand(char c) {
	return !isalpha(c);
}

char *strdpl(char *s) {
	int l = strlen(s);
	char *res = (char*) malloc((l+1)*sizeof(char));
	memcpy(res, s, (l+1)*sizeof(char));
	return res;
}

void printCommand(FILE *f, TypeCommand c) {
	fprintf(f, "Command '%s'\n", c.name);
	fprintf(f, "\t%d arguments\n", c.nArg);
	fprintf(f, "\t%s\n", c.command);
	if(c.defArg != NULL)
		fprintf(f, "\tdefault: %s\n", c.defArg);
	else
		fprintf(f, "\tno default\n");
}
					
static char *readFile(FILE *f) {
	int size, i;
	char c, *res;
	size = SIZE_BUFFER_CHAR;
	res = (char*) malloc(size*sizeof(char));
	c = getc(f);
	i = 0;
	while(c != EOF) {
		if(i>=size) {
			size += SIZE_BUFFER_CHAR;
			res = (char*) realloc((void*)res, size*sizeof(char));
		}
		res[i++] = c;
		c = getc(f);
	}
	res[i++] = '\0';
	res = (char*) realloc((void*)res, i*sizeof(char));	
	return res;
}

void writeFile(FILE *f, char *text) {
	int i;
	for(i=0; text[i] != '\0'; i++)
		fputc(text[i], f);
}

int findWordLexi(char *w, TypeLexiTree *dict) {
    int i, n;
    n = dict->root;
    i = 0;
    while(i<MAX_DICT_LENGTH && w[i] != '\0') {
        int c;
        if(dict->node[n].symbol == '\0')
			return END_LEXI_TREE;
        c = dict->node[n].child;
			while(c != END_LEXI_TREE && dict->node[c].symbol<w[i])
				c = dict->node[c].sibling;
			if(c != END_LEXI_TREE && dict->node[c].symbol == w[i]) {
				n = c;
				i++;
			} else
				return END_LEXI_TREE;
    }
    if(dict->node[n].child  != END_LEXI_TREE && dict->node[dict->node[n].child].symbol == '\0')
        return dict->node[dict->node[n].child].child;
    else
        return END_LEXI_TREE;
}

int addWordLexi(char *w, int index, TypeLexiTree *dict) {
    int i, n;
    n = dict->root;
    i = 0;
    if(w == NULL)
		return END_LEXI_TREE;
    while(i<MAX_DICT_LENGTH && w[i] != '\0') {
        int *prec, c;
        prec = &(dict->node[n].child);
        c = dict->node[n].child;
        while(c != END_LEXI_TREE && dict->node[c].symbol<w[i]) {
            prec = &(dict->node[c].sibling);
            c = dict->node[c].sibling;
        }
        if(c != END_LEXI_TREE && dict->node[c].symbol == w[i]) {
            n = c;
            i++;
        } else {
            *prec = dict->size;
            if(dict->size >= dict->sizeBuf) {
                dict->sizeBuf += INC_SIZE_DICT;
                dict->node = (TypeLexiNode*) realloc((void*)dict->node, dict->sizeBuf*sizeof(TypeLexiNode));
            }
            initLexiNode(w[i], &(dict->node[dict->size]));
            dict->node[dict->size].sibling = c;
            n = dict->size;
            dict->size++;
            i++;
            while(i<MAX_DICT_LENGTH && w[i] != '\0') {
                if(dict->size >= dict->sizeBuf) {
                    dict->sizeBuf += INC_SIZE_DICT;
                    dict->node = (TypeLexiNode*) realloc((void*)dict->node, dict->sizeBuf*sizeof(TypeLexiNode));
                }
                initLexiNode(w[i], &(dict->node[dict->size]));
                dict->node[n].child = dict->size;
                n = dict->node[n].child;
                dict->size++;
                i++;
            }
        }
    }
    if(dict->node[n].child != END_LEXI_TREE && dict->node[dict->node[n].child].symbol == '\0')
        return dict->node[dict->node[n].child].child;
    if(dict->size >= dict->sizeBuf) {
        dict->sizeBuf += INC_SIZE_DICT;
        dict->node = (TypeLexiNode*) realloc((void*)dict->node, dict->sizeBuf*sizeof(TypeLexiNode));
    }
    initLexiNode('\0', &(dict->node[dict->size]));
    dict->node[dict->size].sibling = dict->node[n].child;
    dict->node[dict->size].child = index;
    dict->node[n].child = dict->size;
    dict->size++;
    return END_LEXI_TREE;
}

void initLexiNode(char symbol, TypeLexiNode *n) {
    n->symbol = symbol;
    n->child = END_LEXI_TREE;
    n->sibling = END_LEXI_TREE;
}

TypeLexiTree *newLexiTree() {
    TypeLexiTree* dict;
    dict = (TypeLexiTree*) malloc(sizeof(TypeLexiTree));
    dict->sizeBuf = INC_SIZE_DICT;
    dict->node = (TypeLexiNode*) malloc(dict->sizeBuf*sizeof(TypeLexiNode));
    dict->root = 0;
    initLexiNode('X', &(dict->node[dict->root]));
    dict->size = 1;
    return dict;
}

void freeLexiTree(TypeLexiTree *dict) {
    if(dict->node != NULL)
        free((void*)dict->node);
    free((void*) dict);
}
