DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/
DIRMAIN := ./

CFLAGS := -I$(DIRHEA) -c -Wall -ansi -g -lm
IFLAGS :=  -c -Wall -ansi -g -lm
LDLIBS := -lpthread -lrt
CC := gcc
MPI := mpicc
MPIRUN := mpirun

all : dirs G3ViajanteMPI

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)
	
G3ViajanteMPI: $(DIROBJ)G3ViajanteMPI.o 
	$(MPI) $(DIROBJ)G3ViajanteMPI.o -o $(DIRMAIN)G3ViajanteMPI -lm
	 
$(DIROBJ)%.o: $(DIRSRC)%.c
	$(MPI) $(CFLAGS) $^ -o $@

test:
	$(MPIRUN) -n 4 ./G3ViajanteMPI ./archivo_matriz19

debug:
	$(MPIRUN) -np 4 xterm -e gdb -ex run --args ./G3ViajanteMPI ./archivo_matriz

clean : 
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~ $(DIRUTILITIES) 
