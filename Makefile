DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/
DIRMAIN := ./
DIRUTILITIES := $(DIRHEA)utilities.o

CFLAGS := -I$(DIRHEA) -c -Wall -ansi -g -lm
IFLAGS :=  -c -Wall -ansi -g -lm
LDLIBS := -lpthread -lrt
CC := gcc
MPI := mpicc
MPIRUN := mpirun

all : dirs utilities Hypercube

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

utilities: $(DIRHEA)utilities.c
	$(CC) $(IFLAGS) -o $(DIRUTILITIES) $^ $(LDLIBS)
	
Hypercube: $(DIROBJ)Hypercube.o 
	$(MPI) $(DIRUTILITIES) $(DIROBJ)Hypercube.o -o $(DIRMAIN)Hypercube -lm
	 
$(DIROBJ)%.o: $(DIRSRC)%.c
	$(MPI) $(CFLAGS) $^ -o $@

test:
	$(MPIRUN) -n 8 ./Hypercube ./datos.dat 3

clean : 
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~ $(DIRUTILITIES) 
