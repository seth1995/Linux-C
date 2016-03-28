/*****************************************/
/* pthread barrier for synchronization   */

/*****************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#define threadNum 2048


/*BARRIER_FLAG =INF  unsigned long 2^31*/
#define BARRIER_FLAG (1UL<<30)
//#define PTHREAD_BARRIER_SERIAL_THREAD 1


//let our function replace the default
#define pj2_pthread_barrier_init(B, A, N) (barrier_init_pj2((B), (N)), 0)
#define pj2_pthread_barrier_destroy(B) (barrier_destroy_pj2(B), 0)
#define pj2_pthread_barrier_wait barrier_wait_pj2
//the barrier
typedef struct barrier_pj2_ barrier_pj2;

struct barrier_pj2_
{
	unsigned count;
	unsigned total;
	pthread_mutex_t m;// the mutex to protect itself
	pthread_cond_t cv;//condition lock
};

static int *array;  //current array
static int *array_temp;  //to store our maximum value

static barrier_pj2 barr;  // barrier variable

typedef struct  // to pass parameters
{
	int start;
	int end;
}_parameter_;
_parameter_ *para[threadNum];

int arraySize=0;

//pthread_barrier_t barr;				// barrier variable
pthread_mutex_t lock;				// define a mutual exclusion lock for threads.

// read array data from indata.txt .
void readArray(char *fileName){
	FILE *fp;
	char c;
	int i=0,len1,len2;
	fp=fopen(fileName,"r");
	if(!fp){
	printf("Invaild file %s \n",fileName);
	exit;
}
    fseek(fp, 0L, SEEK_END);//文件指针置于结尾
    len1 = ftell(fp);//获取结尾指针值
    fseek(fp, 0L, SEEK_SET);//文件指针至于开头
    len2 = ftell(fp);//获取开头指针值
   while(len2 != len1)//循环判断
   {
	fscanf(fp," %d",&array[i]); i++;
   	len2 = ftell(fp);
   }

 /* if (feof(fp))
  {
	//printf("Empty file\n");
	arraySize=0;
	return;
   }
//fscanf(fp," %d",&array[i]); i++;
  while(!feof(fp)){
	fscanf(fp," %d",&array[i]); i++;
	//printf("read%d\n",array[i-1] );  //debug
  }
  */
  fclose(fp);
  arraySize=i;
//printf("arraySize = %d \n",arraySize);

}
/*Prototypes*/

static void barrier_destroy_pj2(barrier_pj2 *b);
static void barrier_init_pj2(barrier_pj2 *b, unsigned count);
static  int barrier_wait_pj2(barrier_pj2 *b);



void *findMax(void *para){
	int start,end,i,j;
	//int *leftArray,*rightArray;
	int maxValue=0;

	_parameter_ *para_=para;
	start=para_->start;
	end=para_->end;
	//printf("debug %d",array[left]);
	for(i = start; i <= end; i++)
	{
		if(array[i] > maxValue)
		{
			maxValue = array[i];
		}
	}
	//printf("test%d %d\n",start,end);
	j=start/(end-start+1);
	array_temp[j]=maxValue;


	pj2_pthread_barrier_wait(&barr);
	//pthread_mutex_unlock(&lock);
	//pthread_exit(NULL);
}

int main(int argc,char *argv[]){

	int i,j,k,*temp;
	pthread_t tid[threadNum];			// to store the id of each threads.
	pthread_mutex_init(&lock,NULL);		// initiate the mutual exclusion lock.
    array=malloc(threadNum*2*sizeof(int));
    array_temp=malloc(threadNum*sizeof(int));
	if (argc!= 2)
{
	printf("Not enough args :( Process terminated!\n");
	exit(1);
}

if (strcmp(argv[1],"inData.txt")!=0)
{
	printf("Incorrect file name :( Process terminated!\n");
	exit(1);
}
	readArray("inData.txt");

	if(arraySize!=0)
	{
		i=arraySize/2;					// thread number
		while(1)
		{
			j=arraySize/i;				// actually j=2
			if(pj2_pthread_barrier_init(&barr,NULL,i+1)) // initiate barrier with a count of i+1;
			{
				printf("Error, Cannot initialize barrier\n");
				return -1;
			}
			for(k=0;k<i;k++)
			{
				para[k]=(_parameter_*)malloc(sizeof(_parameter_));
				para[k]->start=k*j;
				para[k]->end=(k+1)*j-1;
				if(pthread_create(&tid[k],NULL,findMax,para[k])){
					printf("Cannot not create thread\n");
					return -1;
				}
			}
			pj2_pthread_barrier_wait(&barr);// wait for all threads being finished.
			pj2_pthread_barrier_destroy(&barr);
			for(k=0;k<i;k++)free(para[k]);
            temp=array_temp;  //swap them
            array_temp=array;
            array=temp;
			if(i==1)break;				// i=1 means rounds are over
			else i=i/2;					// else go to next round
		}
	printf("Max value=%d\n",array[0]);
	}
	else
	{
		printf("Empty file!\n");
		
	}
	pthread_mutex_destroy(&lock);		// destroy the mutual exclusion lock.


	return 0;
}




void barrier_destroy_pj2(barrier_pj2 *b)
{
    pthread_mutex_lock(&b->m);

    while (b->total > BARRIER_FLAG)
    {
        /* 等待所有线程退出 barrier. */
        pthread_cond_wait(&b->cv, &b->m);
    }

    pthread_mutex_unlock(&b->m);

    pthread_cond_destroy(&b->cv);
    pthread_mutex_destroy(&b->m);
}

void barrier_init_pj2(barrier_pj2 *b, unsigned count)
{
    pthread_mutex_init(&b->m, NULL);
    pthread_cond_init(&b->cv, NULL);
    b->count = count;
    b->total = BARRIER_FLAG;
    //printf("barrier_init %d %d\n",b->count,b->total);
}

int barrier_wait_pj2(barrier_pj2 *b)
{
	//down the mutex to protect barrier
    pthread_mutex_lock(&b->m);
   // printf("barrier_wait start %d %d\n",b->count,b->total);

   /* while (b->total > BARRIER_FLAG)
    {
        pthread_cond_wait(&b->cv, &b->m);
        printf("now wait\n");
    }
    */

    /* 是否为第一个到达 barrier 的线程? */
    if (b->total == BARRIER_FLAG) b->total = 0;

    b->total++;
   // printf("barrier_wait++ %d %lo\n",b->count,b->total);  //debug
    //all thread comes
    if (b->total == b->count)
    {
        b->total += BARRIER_FLAG - 1;  //make sure total>FLAG
      //  printf("barrier_wait + %d %d\n",b->count,b->total);  //debug
        pthread_cond_broadcast(&b->cv);  // wake other thread,the current thread is the last arrived one

        pthread_mutex_unlock(&b->m);  //unlock m

        return PTHREAD_BARRIER_SERIAL_THREAD;
    }
    else
    {
        while (b->total < BARRIER_FLAG)
        {
            pthread_cond_wait(&b->cv, &b->m);  //wait for the last one
        }

        b->total--;
       // printf("barrier_wait-- %d %d\n",b->count,b->total);  //debug

        /* 唤醒所有进入 barrier 的线程. */
        if (b->total == BARRIER_FLAG) pthread_cond_broadcast(&b->cv);  //down to the FLAG

        pthread_mutex_unlock(&b->m);

        return 0;
    }
}
