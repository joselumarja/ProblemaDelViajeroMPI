#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

#include <timer.h>

/*Declaraciones de constantes, tipos, variables y macros*/

#define true 1
#define false 0
#define StartNode 0

/*MPI defines*/
#define ROOT 0
#define WRONG_ARGUMENT_ERROR_CODE -1

#define STATUS_OK 0

/*MPI packet positions*/
#define CostPosition 0
#define CounterPosition 1
#define PoblationsPosition 2

typedef struct{
    int* pobl;
    int contador;
    int coste;
}tour_struct;
typedef tour_struct *tour_t;

typedef struct{
    tour_t *list;
    int list_sz;
}stack_struct;
typedef stack_struct *my_stack_t;

int n_cities;
int StackSize;
int *digraph;

/*MPI variables*/
int rank;
int size;
MPI_Status status;
MPI_Request req;

int *buffer;
int *neightbors;

tour_t best;
tour_t tour;
my_stack_t pila;

/*Definiendo el tipo de datos mpi para enviar tour struct*/
MPI_Datatype tour_t_mpi;

/*Definiciones de funciones usadas*/
tour_t pop();
void push(tour_t tour);

void printTour(tour_t tour);
int best_tour(tour_t tour);

int factible(tour_t tour);
int estaEnElRecorrido(tour_t tour, int pob);
tour_t anadir_pob(tour_t tour, int pob);

void checkReceivedBests();

void Rec_en_profund(tour_t tour);

void freeTour(tour_t tour);

/*Funciones para facilitar el envio y la recepcion de tours a traves de mpi*/
tour_t convertBufferToStruct(int *buffer);
void convertStructToBuffer(int *buffer, tour_t tour);

/*Calcular los vecinos de cada procesador*/
void setNeightbors(int rank, int size);

int main(int argc, char *argv[]){
    FILE *diagraph_file;

    int n_tours_procesador;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double inicio, fin;
    short StatusCode;

    /*Guardar el resto de nodos con los que hay que comunicarse*/
    setNeightbors(rank,size);

    if(rank==ROOT)
    {
        StatusCode = STATUS_OK;

        if ((diagraph_file = fopen(argv[1],"r"))==NULL)
        {
            printf("Error abriendo el archivo.\n");
            StatusCode=WRONG_ARGUMENT_ERROR_CODE;
        }

        /*Leer de diagraph_file el número de poblaciones, n;*/
        fscanf(diagraph_file,"%d", &n_cities);
        printf("Numero de ciudades: %d\n\n", n_cities);

        /*Reservar espacio para la pila*/
        StackSize=(n_cities*((n_cities-3)/2))+2;

        /*Leer de diagraph_file el grafo, diagraph;*/
        int NeighbourCost;

        /*Reservar espacio para diagraph;*/
        digraph=(int*) malloc(sizeof(int)*n_cities*n_cities);

        int i,j;

        for(i=0; i < n_cities; i++)
        {
            for(j=0; j < n_cities; j++)
            {
                fscanf(diagraph_file, "%d", &NeighbourCost);
                digraph[(i*n_cities)+j]=NeighbourCost;
                printf("%d ",NeighbourCost);
            }
            printf("\n");
        }
		
        /*Notificando a los procesos si no se han producido fallos*/
		if(MPI_Bcast(&StatusCode, 1, MPI_SHORT, ROOT, MPI_COMM_WORLD)!=MPI_SUCCESS)
		{
			fprintf(stderr,"Fail in processes notification\n");
			exit(EXIT_FAILURE);
		}

        /*Enviando al resto de procesos el numero de ciudades del problema*/
        if(MPI_Bcast(&n_cities, 1, MPI_INT, ROOT, MPI_COMM_WORLD)!=MPI_SUCCESS)
	    {
		    fprintf(stderr,"Fail in processes notification, number of cities\n");
            MPI_Finalize();
	        exit(EXIT_FAILURE);
	    }

        /*Enviando al resto de procesos el grafo direccional de las ciudades*/
        if(MPI_Bcast(digraph, n_cities*n_cities, MPI_INT, ROOT, MPI_COMM_WORLD)!=MPI_SUCCESS)
	    {
		    fprintf(stderr,"Fail in processes notification, digraph\n");
            MPI_Finalize();
	        exit(EXIT_FAILURE);
	    }
	    
    }
    else
    {
        MPI_Bcast(&StatusCode, 1, MPI_SHORT, ROOT, MPI_COMM_WORLD);
		/*Si se ha producido algun fallo finaliza*/
		if(StatusCode<0)
		{
			fprintf(stderr, "Stop process %d\n",rank);
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}

        /*Recibiendo el numero de ciudades*/
        MPI_Bcast(&n_cities, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

        /*Reservar espacio para la pila*/
        StackSize=(n_cities*((n_cities-3)/2))+2;

        /*Reservar espacio para diagraph;*/
        digraph=(int*) malloc(sizeof(int)*n_cities*n_cities);

        /*Recibiendo el grafo direccional de las ciudades*/
        MPI_Bcast(digraph, n_cities*n_cities, MPI_INT, ROOT, MPI_COMM_WORLD);
        
    }

    /*Definiendo el tipo de datos con el que vamos a enviar los tours*/
    MPI_Type_contiguous(n_cities+3,MPI_INT,&tour_t_mpi);
    MPI_Type_commit(&tour_t_mpi);

    int bufferOffset=n_cities+3;
    buffer=(int *) malloc(sizeof(int)*bufferOffset);

    tour_t tour_aux;
    
    /*Inicializar best tour*/
    best=(tour_t) malloc(sizeof(tour_struct));
    best->coste=65535;
    best->contador=0;
    best->pobl=(int *) malloc(sizeof(int)*(n_cities+1));
 
    /*Inicializar la pila*/
    pila=(my_stack_t) malloc(sizeof(stack_struct));
    pila->list=(tour_t*) malloc(sizeof(tour_t)*StackSize);
    pila->list_sz=0;

    if(rank==ROOT)
    {
        /*Tour inicial*/
        tour=(tour_t) malloc(sizeof(tour_struct));
        tour->pobl=(int *) malloc(sizeof(int)*(n_cities+1));
        tour->pobl[0]=StartNode;
        tour->contador=1;
        tour->coste=0;

        /*Introducir primer tour en la pila*/
        push(tour);
        free(tour);

        /*Generar tour hasta que almenos haya tantos como procesadores*/
        while(pila->list_sz<size)
        {
            tour=pop();
            Rec_en_profund(tour);
        }

        /*NUmero de tours por procesador*/
        n_tours_procesador=pila->list_sz/size;

        /*Enviando al resto de procesos el numero de tours que van a guardar*/
        if(MPI_Bcast(&n_tours_procesador, 1, MPI_INT, ROOT, MPI_COMM_WORLD)!=MPI_SUCCESS)
	    {
		    fprintf(stderr,"Fail in processes notification, numero de trozos\n");
            MPI_Finalize();
	        exit(EXIT_FAILURE);
	    }

        /*Enviar tours a los procesadores*/
        int i,j;

        for(i=0; i<size-1;i++)
        {
            for(j=0;j<n_tours_procesador;j++)
            {
                tour_aux=pop();
                convertStructToBuffer(buffer,tour_aux);
                MPI_Isend(buffer,1,tour_t_mpi,neightbors[i],0,MPI_COMM_WORLD,&req);
                free(tour_aux);
            }
        }
    }
    else
    {
        /*Recibir numero de recorridos a ejecutar*/
        MPI_Bcast(&n_tours_procesador, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

        /*Recibir recorridos a ejecutar*/
        int j;

        for(j=0;j<n_tours_procesador;j++)
        {
            MPI_Recv(buffer,1,tour_t_mpi,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
            tour_aux=convertBufferToStruct(buffer);
            push(tour_aux);
            freeTour(tour_aux);
        }
    }

    GET_TIME(inicio);
    while(pila->list_sz>0)
    {
        tour=pop();
    
        Rec_en_profund(tour);

        /*Comprobar si se ha recibido algun best nuevo*/
        checkReceivedBests();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    GET_TIME(fin);

    if(rank==ROOT)
    {
        /*Comprobar si se ha recibido algun best nuevo*/
        checkReceivedBests();

        /*Imprimir resultados: besttour, coste y tiempo*/
        int tiempo=fin-inicio;  /*calculamos tiempo*/

        printf("Tiempo empleado en la ejecucion %d s\n",tiempo);

        /*best tour*/
        printf("Best tour:\n");
        printTour(best);
    }

    MPI_Finalize();

    /*Liberar memoria dinámica asignada*/
    free(pila->list);
    free(pila);
    freeTour(best);
    free(buffer);
    free(digraph);

    return EXIT_SUCCESS;
}

tour_t pop()
{
    tour_t tour=NULL;

    if(pila->list_sz>0)
    {
        pila->list_sz--;
        tour=pila->list[pila->list_sz];
        pila->list[pila->list_sz]=NULL;
    }
    
    return tour;
}

void push(tour_t tour)
{
    tour_t tour_aux = (tour_t) malloc(sizeof(tour_struct));
    tour_aux->pobl=(int*) malloc(sizeof(int)*(n_cities+1));

    tour_aux->contador=tour->contador;
    tour_aux->coste=tour->coste;

    memcpy(tour_aux->pobl,tour->pobl,sizeof(int)*tour->contador);

    pila->list[pila->list_sz]=tour_aux;
    pila->list_sz++;
}

tour_t anadir_pob(tour_t tour, int pob)
{
    int poblation_offset=(tour->pobl[tour->contador-1]*n_cities)+pob;

    tour_t newTour=(tour_t) malloc(sizeof(tour_struct));
    newTour->pobl=(int *) malloc(sizeof(int)*(n_cities+1));

    /*Copia de los valores anteriores del tour*/
    memcpy(newTour->pobl,tour->pobl,sizeof(int)*(tour->contador));

    newTour->pobl[tour->contador]=pob;
    newTour->contador=tour->contador+1;
    newTour->coste=tour->coste+digraph[poblation_offset];

    return newTour;
}

int best_tour(tour_t tour)
{
    if(tour->coste<best->coste)
    {
        return true;
    }

        return false;
}

void printTour(tour_t tour)
{
    int i;

    for(i=0; i<tour->contador-1;i++)
    {
        printf("%d -> ",tour->pobl[i]);
    }
    printf("%d\n",tour->pobl[tour->contador-1]);
    printf("Coste: %d\n\n",tour->coste);
}

void Rec_en_profund(tour_t tour)
{

    tour_t nuevo_tour;

    if(tour->contador==n_cities)
    {
        int pobOffset=(tour->pobl[tour->contador-1]*n_cities)+StartNode;

        if(digraph[pobOffset]>0)
        {
            tour_t checkTour=anadir_pob(tour,StartNode);
            if(best_tour(checkTour)==true)
            {
                freeTour(best);
                best=checkTour;

                printf("Nuevo best encontrado en %d\n",rank);
                printTour(best);

                /*Notificar al resto de procesos*/
                int i;

                for(i=0;i<size-1;i++)
                {
                    convertStructToBuffer(buffer,best);
                    MPI_Isend(buffer,1,tour_t_mpi,neightbors[i],0,MPI_COMM_WORLD,&req);
                }
            }
            else
            {
                freeTour(checkTour);
            }
        }
    }
    else
    {
        int i=tour->pobl[tour->contador-1];
        int pobId;

        for(pobId=0;pobId<n_cities;pobId++)
        {
            if((digraph[(i*n_cities)+pobId]>0)&&(estaEnElRecorrido(tour,pobId)==false))
            {
                nuevo_tour=anadir_pob(tour,pobId);
                if(factible(nuevo_tour)==true)
                {
                    push(nuevo_tour);
                }

                freeTour(nuevo_tour);
            }
        }
        freeTour(tour);
    }
}

int estaEnElRecorrido(tour_t tour, int pob)
{
    int i;

    for(i=0; i<tour->contador; i++)
    {
        if(tour->pobl[i]==pob) return true;
    }

    return false;
}

void freeTour(tour_t tour)
{
    free(tour->pobl);
    free(tour);
}

int factible(tour_t tour)
{
    if(tour->coste>best->coste)
    {
        return false;
    }

    return true;
}

tour_t convertBufferToStruct(int *buffer)
{
    tour_t tour = (tour_t) malloc(sizeof(tour_struct));
    tour->pobl = (int *) malloc(sizeof(int)*(n_cities+1));

    tour->contador=buffer[CounterPosition];
    tour->coste=buffer[CostPosition];

    int i;

    for(i=0; i<=n_cities;i++)
        tour->pobl[i]=buffer[i+PoblationsPosition];

    return tour;
}

void convertStructToBuffer(int *buffer, tour_t tour)
{
    buffer[CostPosition]=tour->coste;
    buffer[CounterPosition]=tour->contador;
    
    int i;

    for(i=0; i<=n_cities;i++)
        buffer[i+PoblationsPosition]=tour->pobl[i];
}

void checkReceivedBests()
{
    int b_message;
    MPI_Status status;
    tour_t received_best;

    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b_message, &status);
	while(b_message==true)
	{
	    MPI_Recv(buffer,1,tour_t_mpi,MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD,&status);
        received_best=convertBufferToStruct(buffer);

        if(best_tour(received_best)==true)
        {
            freeTour(best);
            best=received_best;
        }
        else
        {
            freeTour(received_best);
        }
                
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b_message, &status);
	}
}

void setNeightbors(int rank, int size)
{
    neightbors=(int *) malloc(sizeof(int)*(size-1));

    int i,j;

    for(i=0,j=0;i<size;i++)
    {
        if(i!=rank)
        {
            neightbors[j++]=i;
        }
    }
}