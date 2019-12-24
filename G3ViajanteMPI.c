#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

#include <time.h>

/*Declaraciones de constantes, tipos, variables y macros*/

#define true 1
#define false 0
#define StartNode 0

/*MPI defines*/
#define ROOT 0
#define WRONG_ARGUMENT_ERROR_CODE -1
#define WRONG_NUMBER_OF_LAUNCHED_PROCESSES -2
#define NOT_ENOUGHT_DATA_FOR_PROCESSES -3
#define WRONG_FILE_PATH -4
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

tour_t best;
tour_t tour;
my_stack_t pila;

/*Definiciones de funciones usadas*/
tour_t pop();
void push(tour_t tour);

void printBest();
void best_tour(tour_t tour);

int factible(tour_t tour);
int estaEnElRecorrido(tour_t tour, int pob);
tour_t anadir_pob(tour_t tour, int pob);

void Rec_en_profund(tour_t tour);

void freeTour(tour_t tour);

tour_t convertBufferToStruct(int *buffer);
void convertStructToBuffer(int *buffer, tour_t tour);

void neightborsHypercube(int HypercubeDimension, int Rank, int *Neightbors);
void calcMaxHypercubeNetwork(int Rank, int HypercubeDimension,int *Neightbors, int LocalData, int *Max); 

int main(int argc, char *argv[]){
    FILE *diagraph_file;

    int rank, size;
    MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double inicio, fin;
    short StatusCode;

    StackSize=((n_cities*((n_cities-3)/2))+2)/size;

    if(rank==ROOT)
    {
        StatusCode = STATUS_OK;

        if ((diagraph_file = fopen(argv[1],"r"))==NULL)
        {
            printf("Error abriendo el archivo.\n");
            StatusCode=WRONG_FILE_PATH;
        }

        /*Leer de diagraph_file el número de poblaciones, n;*/
        fscanf(diagraph_file,"%d", &n_cities);
        printf("Numero de ciudades: %d\n\n", n_cities);

        /*Leer de diagraph_file el grafo, diagraph;*/
        int NeighbourCost;

        /*Reservar espacio para diagraph;*/
        digraph=(int*) malloc(sizeof(int)*n_cities*n_cities);

        for(int i=0; i < n_cities; i++)
        {
            for(int j=0; j < n_cities; j++)
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

        /*Reservar espacio para diagraph;*/
        digraph=(int*) malloc(sizeof(int)*n_cities*n_cities);

        /*Recibiendo el grafo direccional de las ciudades*/
        MPI_Bcast(digraph, n_cities*n_cities, MPI_INT, ROOT, MPI_COMM_WORLD);
        
    }

    /*Definiendo el tipo de datos mpi para enviar tour struct*/
    MPI_Datatype tour_t_mpi;

    MPI_Type_contiguous(n_cities+3,MPI_INT,&tour_t_mpi);
    MPI_Type_commit(&tour_t_mpi);

    int buffer=(int *) malloc(sizeof(int)*(n_cities+3));

    /*Inicializar best tour*/
    best=(tour_t) malloc(sizeof(tour_struct));
    best->coste=65535;
    best->contador=0;
    best->pobl=(int *) malloc(sizeof(int)*(n_cities+1));
    
    /*
    MPI_Datatype type[3] = {MPI_INT,MPI_INT,MPI_INT};
    int blocklen[3]={n_cities,1,1};

    MPI_Aint int_extension;
    MPI_Type_extent(MPI_INT, &int_extension);

    MPI_Aint offsets[3]={(MPI_Aint) 0, int_extension*(n_cities+1),int_extension*(n_cities+1) + int_extension};
    MPI_Type_create_struct(3,blocklen,offsets,type,&tour_t_mpi);
    */

    /*Inicializar la pila*/
    pila=(my_stack_t) malloc(sizeof(stack_struct));
    pila->list=(tour_t*) malloc(sizeof(tour_t)*StackSize);
    pila->list_sz=0;

    if(rank==ROOT)
    {
        tour=(tour_t) malloc(sizeof(tour_struct));
        tour->pobl=(int *) malloc(sizeof(int));
        tour->pobl[0]=StartNode;
        tour->contador=1;
        tour->coste=0;

        while(pila->list_sz<size)
        {
            Rec_en_profund(tour);
        }

        
    }
    else
    {
        
    }

    GET_TIME(inicio);
    while(pila->list_sz>0)
    {
        tour=pop();
        Rec_en_profund(tour);
    }
    GET_TIME(fin);

    /*Imprimir resultados: besttour, coste y tiempo*/
    int tiempo=fin-inicio;  /*calculamos tiempo*/

    printf("Tiempo empleado en la ejecucion %d s\n",tiempo);

    /*best tour*/
    printf("Best tour:\n");
    printBest();

    /*Liberar memoria dinámica asignada*/
    free(pila->list);
    free(pila);
    freeTour(best);
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
    pila->list[pila->list_sz]=tour;
    pila->list_sz++;
}

tour_t anadir_pob(tour_t tour, int pob)
{
    int poblation_offset=(tour->pobl[tour->contador-1]*n_cities)+pob;

    tour_t newTour=(tour_t) malloc(sizeof(tour_struct));
    newTour->pobl=(int *) malloc(sizeof(int)*(tour->contador+1));

    /*Copia de los valores anteriores del tour*/
    memcpy(newTour->pobl,tour->pobl,sizeof(int)*(tour->contador));

    newTour->pobl[tour->contador]=pob;
    newTour->contador=tour->contador+1;
    newTour->coste=tour->coste+digraph[poblation_offset];

    return newTour;
}

void best_tour(tour_t tour){
    if(tour->coste<best->coste){
        freeTour(best);
        best=tour;
        printf("New best:\n");
        printBest();
    }
    else
    {
        freeTour(tour);
    }
}

void printBest()
{
    for(int i=0; i<best->contador-1;i++)
    {
        printf("%d -> ",best->pobl[i]);
    }
    printf("%d\n",best->pobl[best->contador-1]);
    printf("Coste: %d\n\n",best->coste);
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
            best_tour(checkTour);
        }
    }
    else if(tour->coste<best->coste)
    {
        int i=tour->pobl[tour->contador-1];
        for(int pobId=0;pobId<n_cities;pobId++)
        {
            if((digraph[(i*n_cities)+pobId]>0)&&(estaEnElRecorrido(tour,pobId)==false))
            {
                nuevo_tour=anadir_pob(tour,pobId);
                if(factible(nuevo_tour)==true)
                {
                    push(nuevo_tour);
                }
                else
                {
                    freeTour(nuevo_tour);
                }
            }
        }
        freeTour(tour);
    }
}

int estaEnElRecorrido(tour_t tour, int pob)
{
    for(int i=0; i<tour->contador; i++)
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

    for(int i=0; i<=n_cities;i++)
        tour->pobl[i]=buffer[i+PoblationsPosition];

    return tour;
}

void convertStructToBuffer(int *buffer, tour_t tour)
{
    buffer[CostPosition]=tour->coste;
    buffer[CounterPosition]=tour->contador;
    
    for(int i=0; i<=n_cities;i++)
        buffer[i+PoblationsPosition]=tour->pobl[i];
}

void calcMaxHypercubeNetwork(int Rank, int HypercubeDimension,int *Neightbors, int LocalData, int *Max)
{
	int i,ReceivedData;
	*Max=LocalData;

	for(i=0; i<HypercubeDimension; i++)
	{
		MPI_Send(Max, 1, MPI_INT, Neightbors[i], 0, MPI_COMM_WORLD);
		MPI_Recv(&ReceivedData, 1, MPI_INT, Neightbors[i], MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		if(ReceivedData>*Max)
		{
			*Max=ReceivedData;
		}
	} 
	
}

void neightborsHypercube(int HypercubeDimension, int Rank, int *Neightbors)
{	
	int i;
	for(i=0; i<HypercubeDimension; i++)
	{
		Neightbors[i] = Rank ^ (int) pow(2,i);
	}
}