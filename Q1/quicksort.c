#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <wait.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <unistd.h>

void insertionSort(int beg,int arr[], int n) 
{ 
   int i, key, j; 
   for (i = beg; i < n; i++) 
   { 
       key = arr[i]; 
       j = i-1; 
  
       while (j >= 0 && arr[j] > key) 
       { 
           arr[j+1] = arr[j]; 
           j = j-1; 
       } 
       arr[j+1] = key; 
   } 
} 




int partition(int * arr , int start , int end)
{
    int pivind = start+ rand() % (end-start)  ;
    int pivot = arr[pivind];
//    printf("\npivot %d pivind %d \n",pivot,pivind);
    arr[pivind]=arr[end-1];
    arr[end-1]=pivot;
    int smallind = start ;
    for (int i=start;i<end-1;i++)
    {
        if (arr[i]<pivot)
        {
            int temp = arr[i];
            arr[i]=arr[smallind];
            arr[smallind]=temp;
            smallind++;
        }
    }
    arr[end-1]=arr[smallind];
    arr[smallind]=pivot;
    /*
    printf("array after pivot %d partition \n",pivot);
    for (int i =start ;i < end ; i++)
    {
        printf("%d ",arr[i]);
    }
    printf("\n");
    */
    return smallind;
}




void concqsort (int * arr,int start,int end)
{
    if(end-start<5 && start < end )
    {
        insertionSort(start,arr,end);
    }
    if (start < end )
    {
        int partind = partition(arr,start,end);
        pid_t leftarr ;
        leftarr=fork();
        int statusl;
        int statusr;
        if (leftarr==0)
        {
  //          printf("sortinglrft\n");
            concqsort(arr , start , partind);
            exit(0);
        }
        else 
        {
            pid_t rightarr= fork();
            if (rightarr==0)
            {
//                printf("sortingrite\n");
                concqsort(arr , partind+1 , end );
                exit(0);
            }
            else 
            {
               waitpid(rightarr,&statusr,0);
               waitpid(leftarr,&statusl,0);
            }
        }
    }
}

struct thrdarg {
    int start;
    int end ;
    int * arr;
};

void* threadedqsort(void * arrstruct)
{
    struct thrdarg * args = (struct thrdarg *) arrstruct;
    if ( args->start >= args->end )
        return NULL;

    if(args->end-args->start<5)
    {
        insertionSort(args->start,args->arr,args->end);
    }
    

    else 
    {
        int pivind=partition(args->arr,args->start,args->end);
        struct thrdarg  leftarg  ;
        leftarg.start=args->start;
        leftarg.end=pivind;
        leftarg.arr=args->arr;
        pthread_t leftid;
        pthread_create(&leftid,NULL,threadedqsort,&leftarg);

        struct thrdarg  rightarg;
        rightarg.start=pivind +1 ;
        rightarg.end=args->end;
        rightarg.arr=args->arr;
        pthread_t rightid;
        pthread_create(&rightid,NULL,threadedqsort,&rightarg);

        pthread_join(leftid,NULL);
        pthread_join(rightid,NULL);
    }
    return NULL;

}



void normalqsort (int * arr,int start,int end)
{
    if(end-start<5 && start < end )
    {
        insertionSort(start,arr,end);
    }
    if (start < end )
    {
        int partind = partition(arr,start,end);
        normalqsort(arr , start , partind);
        normalqsort(arr , partind+1 , end );
    }
}

int * sharedmem (size_t n)
{
    key_t thekey = IPC_PRIVATE;
    int shm_id = shmget(thekey,n,IPC_CREAT | 0666);
    return (int * )shmat(shm_id,NULL,0);
}


int main ()
{
    int n;
    srand(time(0));
    scanf("%d",&n);
    key_t key = IPC_PRIVATE;
    size_t memsixe = 4*n;
    int ShmId = shmget(key,memsixe,IPC_CREAT | 0666);
    int * arr ;
    if (ShmId==-1)
    {

    }
    else 
    {
        arr=(int *)shmat(ShmId,NULL,0);
    }
    int inparr[n];

//    printf("asd %lu\n",sizeof(int));
    for (int i =0 ;i < n ; i++)
    {
        int k;
        scanf("%d",&k);
        inparr[i]=k;
     //   printf("%d ",inparr[i]);
    }
    for (int i =0 ;i < n ; i++)
    {
        arr[i]=inparr[i];
    }
    printf("Running normal quicksort \n");
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;
    clock_t t; 
    t = clock(); 
    normalqsort(arr,0,n);   t = clock() - t; 
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
//    printf("time = %Lf\n", en - st);
    printf("time by clock = %f\n",time_taken);

    /*
     printf("\n");
    for (int i =0 ;i < n ; i++)
    {
        printf("%d ",arr[i]);
    }
    printf("\n");
    */
    for (int i =0 ;i < n ; i++)
    {
        arr[i]=inparr[i];
    }
    
    
    printf("Running concurrent quicksort \n");
    printf("\n");

    struct timespec ts1;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
    long double st1 = ts1.tv_nsec/(1e9)+ts1.tv_sec;
   
    t = clock(); 
    concqsort(arr,0,n);
    time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
    long double en1 = ts1.tv_nsec/(1e9)+ts1.tv_sec;
//    printf("time = %Lf\n", en1 - st1);
    printf("time by clock = %f\n",time_taken);
    for (int i =0 ;i < n ; i++)
    {
        printf("%d ",arr[i]);
    }
    printf("\n");
    
    for (int i =0 ;i < n ; i++)
    {
        arr[i]=inparr[i];
    }


    
    printf("starting threadds \n");

    struct thrdarg  input;
    input.start=0;
    input.end=n;
    input.arr=arr;

    printf("seg threadds \n");
    pthread_t tid;

    
    printf("Running threaded  quicksort \n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    pthread_create(&tid,NULL,threadedqsort,&input);
    pthread_join(tid,NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);

    for (int i =0 ;i < n ; i++)
    {
        printf("%d ",arr[i]);
    }
    printf("\n");
    return 0;


}












