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

all : dirs ViajeroMPI

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)
	
ViajeroMPI: $(DIROBJ)G3ViajanteMPI.o 
	$(MPI) $(DIROBJ)G3ViajanteMPI.o -o $(DIRMAIN)G3ViajanteMPI -lm
	 
$(DIROBJ)%.o: $(DIRSRC)%.c
	$(MPI) $(CFLAGS) $^ -o $@

test:
	$(MPIRUN) -n 8 ./G3ViajanteMPI ./archivo_matriz.txt

clean : 
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~ $(DIRUTILITIES) 
