# list of executable files to produce
MAND = mand
MENT = ment
FLAT = flat

# .o files necessary to build the executables
OBJ_MAND = FilterCommand.o 
OBJ_MENT = FilterComment.o 
OBJ_FLAT = FlattenLatex.o

########### MODIFY ONLY TO CHANGE OPTIONS ############

# compiler and its options
CC = gcc -g
CFLAGS = -Wall -Wno-char-subscripts -D_POSIX_SOURCE -std=c99 -Wall -pedantic 

# linker and its options
LD = $(CC)

############ LIST OF EXECUTABLE TARGETS (MODIFY ONLY TO ADD AN EXECUTABLE) ##############

all: Makefile.d $(MAND) $(MENT) $(FLAT)


# build the executable
$(MAND): $(OBJ_MAND)
	$(LD) $^ -o $@  -L/usr/local/lib -L/usr/lib/ -O3
	
# build the executable
$(MENT): $(OBJ_MENT)
	$(LD) $^ -o $@  -L/usr/local/lib -L/usr/lib/ -O3
	
# build the executable
$(FLAT): $(OBJ_FLAT)
	$(LD) $^ -o $@  -L/usr/local/lib -L/usr/lib/ -O3

############ DO NOT MODIFY ANYTHING BELOW THIS LINE ##############

# create .o from .c
.c.o:
	$(CC) $(CFLAGS) -c $<

# remove non essential files
clean:
	$(RM) *.o *~ *.log Makefile.d

# clean everything but sources
distclean: clean
	$(RM) $(EXE)

# dependencies
Makefile.d:
	$(CC) -MM $(CFLAGS) *.c > Makefile.d

# only real files can be non phony targets
.PHONY: all clean distclean debug release
