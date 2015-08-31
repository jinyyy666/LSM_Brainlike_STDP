#include"neuron.h"
#include"synapse.h"
#include"parser.h"
#include"network.h"
#include"simulator.h"
#include<sys/time.h>
#include<iostream>
#include<math.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>

using namespace std;
int file[4142];

struct NTParg{
  long tid;
  Network * network;
};

void * ParallelSim(void * NTPargptr){

//  char filename[64];
  NTParg * arg = (NTParg *)NTPargptr;
  Network * network = arg->network;
  long tid = arg->tid;
  delete arg;
    
  
//  Parser parser(&network);
//  parser.Parse(argv[1]);
  
//  sprintf(filename,"netlist/netlist_%ld.txt",tid);
//  cout<<"filename:"<<filename<<"\n"<<endl;
//  parser.Parse(filename);
//  cout<<"aaaa"<<endl;
  Simulator simulator(network);

  (network)->AnalogToSpike();
  simulator.LSMRun(tid);
  cout<<"Thread "<<tid<<" done!"<<endl;
  pthread_exit(NULL);
}


int main(int argc, char * argv[]){
  struct timeval val1,val2;
  struct timezone zone;
  struct NTParg* NTPargptr;
  int rc;
  int i;
  void * status;
  long t;
  
  gettimeofday(&val1,&zone);
//  srand(time(NULL));
//  Network network;
  for(i = 0; i < 4142; ++i) file[i] = -1;
 
  Network array_network[NUM_THREADS];
  
  for(i = 0; i < NUM_THREADS; i++){
    Parser parser(&array_network[i]);
    parser.Parse("netlist/netlist_brain_new.txt");
#ifdef CV
    array_network[i].CrossValidation(NFOLD);
#endif
  }

  pthread_t threads[NUM_THREADS];
  pthread_attr_t attr;
  
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
  
  for(t = 0; t < NUM_THREADS; t++){
    NTPargptr = new(struct NTParg);
    NTPargptr->tid = t;
    NTPargptr->network = &array_network[t];

    printf("In main: creating thread %ld \n",t);
    rc = pthread_create(&threads[t],&attr,ParallelSim,(void *) NTPargptr);
    if(rc){
      printf("ERROR; return code form pthread_create() is %d \n",rc);
      exit(-1);
    }
  }
  
  pthread_attr_destroy(&attr);
  for(t = 0; t < NUM_THREADS; t++){
    rc = pthread_join(threads[t],&status);
    if(rc){
      printf("ERROR; return code form pthread_join() is %d \n",rc);
      exit(-1);
    }
    printf("Main: completed joining with thread %ld ! \n",t);
  }
  printf("Main: program completed. Exiting. \n");
  
  gettimeofday(&val2,&zone);
  cout<<"Wall clock time: "<<((val2.tv_sec-val1.tv_sec)+double(val2.tv_usec-val1.tv_usec)*1e-6)<<" seconds"<<endl;
  
  pthread_exit(NULL);

  return 0;
}






