#       Requires System V make
#       @(#)Makefile 1.5 86/05/13
.SUFFIXES:      .o .c .c~ .h .h~
.PRECIOUS:      scclib.a
#       You'll probably want to change these.  These are used by the compilers
#       to figure out where the include files should go.
TARGDIR = /home/notebook/Public/work/smallCextended/lib
INCDIR = "/home/notebook/Public/work/smallCextended/src/scc/include/"

INSTFLAGS = -DINCDIR=$(INCDIR)
#CFLAGS = '$(INSTFLAGS)' -O
CFLAGS = 

SRC = data.c error.c expr.c function.c gen.c io.c lex.c main.c preproc.c \
      primary.c stmt.c sym.c while.c
OBJ = $(addsuffix .o, $(basename $(SRC)))

all:    scc8080 sccm68k sccas09 sccvax
#all:    scc8080

#$(FE) code8080.o codeas09.o codevax.o codem68k.o: defs.h data.h

install:        all
	mv sccvax scc8080 sccas09 sccm68k $(TARGDIR)

#Alternately, you may have to do an lorder
#$(LIB): $(FE)
#	-ranlib $(LIB)
#	-ucb ranlib $(LIB)

#scc8080:        code8080.o $(LIB)
#	$(CC) -o scc8080 $(CFLAGS) $(LIB) code8080.o
scc8080: code8080.o $(OBJ)
	$(CC) -o scc8080 $(CFLAGS) $(OBJ) code8080.o

sccm68k: codem68k.o $(OBJ)
	$(CC) -o sccm68k $(CFLAGS) $(OBJ) codem68k.o

sccas09:        codeas09.o $(OBJ)
	$(CC) -o sccas09 $(CFLAGS) $(OBJ) codeas09.o

sccvax:         codevax.o $(OBJ)
	$(CC) -o sccvax $(CFLAGS) $(OBJ) codevax.o

print:
	pr -n defs.h data.h data.c error.c expr.c function.c gen.c \
	        io.c lex.c main.c preproc.c primary.c stmt.c \
	        sym.c while.c code*.c | lp
clean:
	rm -f code8080.o codem68k.o codevax.o codeas09.o \
		     $(OBJ) *.exe \
	               sccvax scc8080 sccas09 sccm68k

