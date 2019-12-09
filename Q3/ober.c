#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define maxarrivaltime 10

pthread_cond_t * condarray;

struct passenger{
    int passid;
    int cabstatus;
    int paymentstatus;
    int cabtype;
    int maxwtime ;
};



struct cab {
    int cabid ;
    int cabstatus ;
    int inpassengerid;
    int inpassengerid2;
};

struct ridedata{
    int cabid ;
    struct passenger * passdata;  
};

#define max_ride_time 10

pthread_mutex_t pass_array_lock;
struct cab * cabarray[400];

int pno;
int passenarray[100];

int p1;
int empty;
int cno;

struct timespec * gettime(int seconds)
{
    struct timespec * tm = (struct timespec *)malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME , tm);
    tm->tv_sec +=seconds;
    return tm;
}


struct server {
    int serverid;

};

struct server  serarr[200];



sem_t payment_semaphore;
int paymentarr[400];
pthread_mutex_t servermutex;


void * server (void * inp)
{
    struct server * self = (struct server * )inp;
    while (1)
    {
        int found =0 ;
//        printf("\033[1;36m payment waiting   \n \033[0m");
        sem_wait(&payment_semaphore);
//        printf("\033[1;36m payment triggered   \n \033[0m");
        pthread_mutex_lock(&servermutex);
        int plol;
        for (int i=0;i<pno;i++)
        {
            if (paymentarr[i]==1)
            {
                plol=i;
                found++;
                printf("\033[1;36m passenger %d is paying on server %d \n\033[0m",
                        i,self->serverid);
                paymentarr[i]=0;
            }
        }
        pthread_mutex_unlock(&servermutex);
        if (found !=0)
        {
            sleep(2);
            printf("\033[1;36m passenger %d payment completed\n \033[0m",
                    plol);
        }
        if (found==0)
        {
//            printf("triggered found none server %d exits \n",self->serverid);
            break;
        }

    }
    return NULL;
}





void enjoyride(struct ridedata args)
{
     int ridetime = rand () % max_ride_time ;
     struct cab * cabdata = cabarray[args.cabid];
     printf("\x1b[1;32mpassenger %d riding cab %d with type %d for %d seconds\n\x1b[0m",
             args.passdata->passid,cabdata->cabid, cabdata->cabstatus,ridetime);
     sleep(ridetime);
     printf("\x1b[1;33m passenger %d wants to leave  cab %d \x1b[0m\n",
             args.passdata->passid,cabdata->cabid);
     pthread_mutex_lock(&pass_array_lock);
     int assigned =0;
     for (int i=0;i<pno;i++)
     {
//         printf("looping in thread for %d give to %d but state %d and cabstatus %d\n",args.passdata->passid,i,passenarray[i],cabdata->cabstatus);
         if (passenarray[i] ==1)
         {
             if (cabdata->cabstatus==1)
                 printf("KYA BHAIIII\n");
             if(cabdata->cabstatus==2)
             {
                 printf("\x1b[1;33m passenger %d left cab %d to passenger %d\n",
                         args.passdata->passid,cabdata->cabid,i);
                 printf("\x1b[0m");
                 cabdata->inpassengerid=args.passdata->passid;
                 passenarray[i]=0;
                 cabdata->cabstatus =1;
                 assigned++;
                 pthread_cond_signal (&(condarray[i]));
                 pthread_mutex_unlock(&pass_array_lock);
                 break;

             }
             else if (cabdata->cabstatus==3)
             {
                 printf("\x1b[1;33m cabtype %d was with passenger %d who wanted %d given to %d \x1b[0m\n",
                         cabdata->cabstatus,args.passdata->passid,args.passdata->cabtype,i);
                 cabdata->cabstatus=0;
//*********      p1++;
                 passenarray[i]=0;
                 assigned++;
//                 printf("\n\nplease %d\n\n",cabarray[args.cabid]->cabstatus);
                 pthread_cond_signal (&(condarray[i]));
                 pthread_mutex_unlock(&pass_array_lock);
                 break;
             }
         }
         else if (passenarray[i]==2)
         {
             if (cabdata->cabstatus == 3 )
             {
                 assigned ++;
                 passenarray[i]=0;
                 cabdata->cabstatus=0;
                 pthread_cond_signal (&(condarray[i]));
                 pthread_mutex_unlock(&pass_array_lock);
                 break;
             }
             if (cabdata->cabstatus == 1 )
             {
                 cabdata->cabstatus=0;
                 p1--;
                 assigned ++;
                 passenarray[i]=0;
//                 printf("SIGNAAAAAL\n");
                 pthread_cond_signal (&(condarray[i]));
                 pthread_mutex_unlock(&pass_array_lock);
                 break;
             }
         }
     }
     if (assigned==0)
     {
         printf ("could not assign cab %d with status%d\n",cabdata->cabid,cabdata->cabstatus);
         if (cabdata->cabstatus==2)
         {
//             printf("\n\nhereherehere1\n\n");
             cabdata->cabstatus=1;
             p1++;
         }
         else if (cabdata->cabstatus==3)
         {
//             printf("\n\nhereherehere2\n\n");
             cabdata->cabstatus=0;
             empty++;
         }
         else if (cabdata->cabstatus==1)
         {
//             printf("\n\nhereherehere3\n\n");
             cabdata->cabstatus=0;
             p1--;
             empty++;
         }
         printf("CHANGED STATUS TO %d\n",cabdata->cabstatus);
         pthread_mutex_unlock(&pass_array_lock);
     }
     else 
     {
         printf("CAB ASSIGNED\n");
     }
     printf("\033[1;36m passenger  %d wants to pay\n \033[0m",
             args.passdata->passid);
     pthread_mutex_lock(&servermutex);
     paymentarr[args.passdata->passid]=1;
     pthread_mutex_unlock(&servermutex);
     sem_post (&payment_semaphore);

     
}



/*
 * 0 = waiting for none
 * 1 = waiting for pool
 * 2 = waiting for premier 
*/




void *  passenger_thread (void * passinp)
{

    int willwait =4;

//???

    struct passenger* args  = (struct passenger*) passinp;
//    printf("in thread %d\n",args->passid);
    int k= rand()%maxarrivaltime;
    int ctype = rand()%2;
    if (ctype==0)
        args->cabtype=0;
    else 
        args->cabtype=1;
//    printf("sleeping for %d with ctype %d pid %d\n",k,ctype,args->passid);
    sleep(k);
    if (ctype==0)
        printf("\033[1;31m passenger %d has arrived and wants a pool cab\n\033[0m"
                ,args->passid);
    else 
      printf("\033[1;31mpassenger %d has arrived and wants a premier cab\n\033[0m"
                ,args->passid);

    struct ridedata rd;
    rd.passdata=args;
    pthread_mutex_lock(&(pass_array_lock));
 
    // wanted pool 
    if (ctype==0)
    {
        printf("wanted pool %d\n",args->passid);
        if (p1>0)
        {
            // got pool1 
            printf("passenger %d got pool1 %d \n",args->passid,p1);
            p1--;
            for (int i=0;i<cno;i++)
            {
                if (cabarray[i]->cabstatus == 1)
                {
                    printf(" cab with id %d\n",i);
                    cabarray[i]->cabstatus = 2;
                    cabarray[i]->inpassengerid=args->passid;
                    rd.cabid=i;
                    break;
                }
            }

            pthread_mutex_unlock(&(pass_array_lock));
//            printf("\n\ncabid 1%d \n\n",rd.cabid);
            enjoyride(rd);

        }
        else 
        {
            if (empty>0)
            {
                //did not get pool1 but got an empty
                printf("passenger %d did not get pool1 but got an empty"
                        ,args->passid);
                
                empty --;
                p1++;
                for (int i=0;i<cno;i++)
                {
                    if (cabarray[i]->cabstatus == 0)
                    {
                        printf(" cab with id %d \n",i);
                        cabarray[i]->cabstatus = 1;
                        cabarray[i]->inpassengerid=args->passid;
                        rd.cabid=i;
                        break;
                    }
                }
                pthread_mutex_unlock(&(pass_array_lock));
//                printf("\n\ncabid 2%d \n\n",rd.cabid);
                enjoyride(rd);
            }

            else
            {
                // got none
                passenarray[args->passid]=1;


                int timeflag = pthread_cond_timedwait(&(condarray[args->passid]),&(pass_array_lock),gettime(willwait));
               

                //mutex locked again by wait 
                if (timeflag!=0)
                {
//                    perror("why .... ");
                    passenarray[args->passid]=0;
                    printf("\x1b[1;33m passenger %d timed out  \n\x1b[0m",
                            args->passid);
                    passenarray[args->passid]=0;

                    pthread_mutex_unlock(&(pass_array_lock));

                }

                else if ( timeflag==0)
                {
                    // got pool1 
                    printf("passenger %d got AFTER WAIT \n",args->passid);
                    for (int i=0;i<cno;i++)
                    {
                        if (cabarray[i]->cabstatus == 1)
                        {
                            printf(" cab with id %d\n",i);
                            cabarray[i]->cabstatus = 2;
                            cabarray[i]->inpassengerid=args->passid;
                            rd.cabid=i;
                            p1--;
                            break;
                        }
                        else if (cabarray[i]->cabstatus == 0)
                        {
                            printf(" cab with id %d \n",i);
                            empty --;
                            p1++;
                            cabarray[i]->cabstatus = 1;
                            cabarray[i]->inpassengerid=args->passid;
                            rd.cabid=i;
                            break;
                        }
                    }
                    passenarray[args->passid]=0;

                    pthread_mutex_unlock(&(pass_array_lock));
//                    printf("\n\ncabid 3%d \n\n",rd.cabid);
                    enjoyride(rd);


                }

                //do sth that requires mutex

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
*/
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                
            }
        }
    }
    //wanted premier  
    else 
    {
        printf("wanted premier %d\n",args->passid);
        if (empty>0)
        {
            //got empty
            printf("passenger %d got an empty ",args->passid);
            for (int i=0;i<cno;i++)
            {
                if (cabarray[i]->cabstatus == 0)
                {
                    printf("with cabid %d \n",i);
                    cabarray[i]->cabstatus = 3;
                    cabarray[i]->inpassengerid=args->passid;
                    empty--;
                    rd.cabid=i;
                    break;
                }
            }
            pthread_mutex_unlock(&(pass_array_lock));
//            printf("\n\ncabid 4%d \n\n",rd.cabid);
            enjoyride(rd);
        }
        else 
        {
            // did not get empty
            passenarray[args->passid]=2;
            int timeflag = pthread_cond_timedwait(&(condarray[args->passid]),&(pass_array_lock),gettime(willwait));
            //do sth that requires mutex 
//            printf("\n\n\n\n\ni %d got signalled\n\n\n\n",args->passid);
            if (timeflag!=0)
            {
                passenarray[args->passid]=0;
//                perror("why .... ");
                printf("\x1b[1;33m passenger %d timed out \n\x1b[0m",
                        args->passid);
                pthread_mutex_unlock(&(pass_array_lock));
//                printf("timed out again %d\n",args->passid);

            }
            else if (timeflag==0)
            {
                //got empty
                printf("passenger %d got an empty AFTER WAIT\n",args->passid);
                for (int i=0;i<cno;i++)
                {
//                    printf("looping ra %d \n",i);
                    if (cabarray[i]->cabstatus == 0)
                    {
                        printf("with cabid %d \n",i);
                        cabarray[i]->cabstatus = 3;
                        cabarray[i]->inpassengerid=args->passid;
                        empty--;
                        rd.cabid=i;
                        break;
                    }
                }
                pthread_mutex_unlock(&(pass_array_lock));
                printf ("\n \033[1;31mSENDING %d TO RIDE cab %d\n\033[0m",
                        rd.passdata->passid,rd.cabid );
//                printf("\n\ncabid 5%d \n\n",rd.cabid);
                enjoyride(rd);
            }
        }
    }


    return NULL;
}



int sno ;


int main ()
{
    printf("enter number of passengers: ");
    scanf("%d",&pno);
    printf("enter number of cabs: ");
    scanf("%d",&cno);
    printf("enter number of servers: ");
    scanf("%d",&sno);
    
    empty=cno;
    p1=0;

    pthread_t thid [pno];
    pthread_t servthid [sno];
    struct passenger passarray1[pno];

    sem_init(&payment_semaphore,0,0);
    condarray = (pthread_cond_t *)malloc(sizeof(pthread_cond_t)*pno);
    for (int i=0;i<pno;i++)
    {
        pthread_cond_init(&(condarray[i]),NULL);
        passenarray[i]=0;
    }

    for (int i=0;i<sno;i++)
    {
        paymentarr[i]=0;
    }


    for (int i=0;i<cno ;i++)
    {
        cabarray[i]=(struct cab *)malloc(sizeof (struct cab));
        cabarray[i]->cabid=i;
        cabarray[i]->cabstatus=0;
        cabarray[i]->inpassengerid=-1;
    }


    for (int i=0;i<pno;i++)
    {
//        printf("pass %d\n",i);
        struct passenger passin;
        passin.passid=i;
        passin.cabtype=-1;
        passin.cabstatus=-1;
        passin.paymentstatus =-1;
        passarray1[i]=passin;
        pthread_create(&(thid[i]),NULL,passenger_thread,(void*)&(passarray1[i]));
//        printf("passdone %d\n",passin.passid);
    }

    for (int i=0;i<sno;i++)
    {
//        printf("\033[1;36m created server thread \n \033[0m");
        struct server myserv;
        myserv.serverid=i;
        serarr[i]= myserv ;
        pthread_create(&(servthid[i]),NULL,server,(void*)&(serarr[i]));
    }


    for (int i=0;i<pno;i++)
    {
//        printf("joined\n");
        pthread_join(thid[i],NULL);
    }



//        passenger(i);
}

