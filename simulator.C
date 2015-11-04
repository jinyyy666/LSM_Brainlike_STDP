#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "simulator.h"
#include "network.h"
#include <sys/time.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace std;
double TSstrength;
extern int file[500];
//FILE * Fp;
//FILE * Foutp;
//std::fstream Fs;
//std::ofstream Fouts;

Simulator::Simulator(Network * network){
  _network = network;
  _t = -1;
}

void Simulator::SetEndTime(double endTime){
  assert(endTime > 0);
  _t_end = endTime;
}

void Simulator::SetStepSize(double stepSize){
  assert(stepSize > 0);
  _t_step = stepSize;
}



void Simulator::LSMRun(long tid){
//_network->PrintAllNeuronName();
  struct timeval val1, val2;
  struct timezone zone;
  
  char filename[64];
  int info, count;
  int x,y,loss_neuron;
  int Tid;
  double temp;
  FILE * Fp = NULL;
  FILE * Foutp = NULL;
  networkmode_t networkmode;
  
  srand(0);
  _network->IndexSpeech();
  cout<<"Number of speeches: "<<_network->NumSpeech()<<endl;
#ifndef CV // This is for the case without cross-validation
  for(int count = 0; count < _network->NumSpeech(); count++){
    // empty files
    sprintf(filename,"outputs/spikepattern%d.dat\0",count);
    Foutp = fopen(filename,"w");
    assert(Foutp != NULL);
    fprintf(Foutp,"%d\t%d\n",-1,-1);
    fclose(Foutp);
  }
#endif
  //simulate the permanent neuron loss
/*  x = _network->LSMSizeAllNeurons();
  loss_neuron = LOST_RATE*(x-83-26);
  cout<<"# of neuron loss:"<<loss_neuron<<endl;
  _network->LSMTruncNeurons(loss_neuron);
  y = _network->LSMSizeAllNeurons();
  temp = ((double)y)/((double)x);
  cout<<"x = "<<x<<"y = "<<y<<"\t"<<"Lost rate: "<<temp<<"\t"<<"Desired loss rate: "<<LOST_RATE<<endl; */

#if NUM_THREADS == 1
  sprintf(filename, "reservoir_weights_%ld_org.txt", tid);
  _network->WriteSynWeightsToFile("reservoir",filename);
#endif

#ifndef STDP_TRAINING_RESERVOIR
  if(tid == 0){
    sprintf(filename, "r_weights_info.txt");
    _network->WriteSynWeightsToFile("reservoir", filename);
  }
#endif

#ifdef STDP_TRAINING_RESERVOIR
  // train the reservoir using STDP rule:
  networkmode = TRAINRESERVOIR;
  _network->LSMSetNetworkMode(networkmode);

  // just a test to verify whether or not the weight will stay still!
  //_network->LoadSynWeightsFromFile("reservoir", "reservoir_weights_9.txt");

  gettimeofday(&val1, &zone);
  // repeatedly training the reservoir for a certain amount of iterations:
  for(int i = 0; i < 1; ++i){
    _network->LSMClearSignals();
    // no training of the readout synapses happens during this stage:
    info = _network->LoadFirstSpeech(false, networkmode);
    while(info != -1){
#if NUM_THREADS == 1
      sprintf(filename,"results/spikepattern%d.dat", info);
      Fp = fopen(filename, "w");
      assert(Fp != NULL);
#endif
      Foutp = NULL;
      int time = 0;
      cout<<"Speech : "<<info<<endl;
      while(!_network->LSMEndOfSpeech(networkmode)){
        _network->LSMNextTimeStep(++time, false, 1, NULL, Fp);
      }
#if NUM_THREADS == 1
      fclose(Fp);
#ifdef _PRINT_SYN_ACT
      PrintSynAct(info);
#endif
#endif
      // print the speech information into file:
      _network->SpeechPrint(info);
      _network->LSMClearSignals();
      info = _network->LoadNextSpeech(false, networkmode);
    }
#if NUM_THREADS == 1  
  // Write the weight back to file after training the reservoir with STDP:
   sprintf(filename, "reservoir_weights_%d.txt", i);
   _network->WriteSynWeightsToFile("reservoir",filename);
#endif       

  }
  gettimeofday(&val2, &zone);
  cout<<"Total time spent in training the reservoir: "<<((val2.tv_sec - val1.tv_sec) + double(val2.tv_usec - val1.tv_usec)*1e-6)<<" seconds"<<endl;
  /*
  cout<<"Print speech response into the file. Load the Transient mode! "<<endl;
  
  networkmode = TRANSIENTSTATE;
  _network->LSMSetNetworkMode(networkmode);

  count = 0;
  _network->LSMClearSignals();
  info = _network->LoadFirstSpeech(false, networkmode);
  while(info != -1){
    Fp = NULL;
    Foutp = NULL;
    count++;
    int time = 0;
    while(!_network->LSMEndOfSpeech(networkmode)){
      _network->LSMNextTimeStep(++time,false,1,NULL,NULL);
    }
//cout<<"Speech "<<count<<endl;
//_network->SpeechInfo();
    _network->SpeechPrint(info);
    _network->LSMClearSignals();
    info = _network->LoadNextSpeech(false, networkmode);
  }
  

  assert(0);
  */
  // Write the weight back to file after training the reservoir with STDP:
  if(tid == 0){
    sprintf(filename, "r_weights_info.txt");
    _network->WriteSynWeightsToFile("reservoir", filename);
  }
  
  // Load the weight from file:
  sprintf(filename, "r_weights_info.txt");
  //_network->LoadSynWeightsFromFile("reservoir", filename);
  assert(0);
#endif  
  // produce transient state
  networkmode = TRANSIENTSTATE;
  _network->LSMSetNetworkMode(networkmode);

  count = 0;
  _network->LSMClearSignals();
  info = _network->LoadFirstSpeech(false, networkmode);
  while(info != -1){
    Fp = NULL;
    Foutp = NULL;
    count++;
    int time = 0;
    while(!_network->LSMEndOfSpeech(networkmode)){
      _network->LSMNextTimeStep(++time,false,1,NULL,NULL);
    }
//cout<<"Speech "<<count<<endl;
//_network->SpeechInfo();
    _network->SpeechPrint(info);
    _network->LSMClearSignals();
    info = _network->LoadNextSpeech(false, networkmode);
  }

  // train the readout layer
  networkmode = READOUT;
  _network->LSMSetNetworkMode(networkmode);
#ifdef CV
#if NUM_THREADS == 1
// network->CrossValidation(NUM_THREADS);
  cout<<"aaaa"<<endl;
  for(int fff = 0; fff < NFOLD; ++fff){
    _network->Fold(fff);
    _network->LSMClearWeights();
    Tid = (int)tid;
    cout<<"Only one thread is running:Tid_"<<Tid<<endl;
#else 
  cout<<"bbbb"<<endl;
     Tid = (int)tid;
     cout<<"Tid:"<<Tid<<endl;
     _network->Fold(Tid);
#endif
//  _network->LSMClearWeights();   //Clear all weights before each fold of CV
  info = _network->LoadFirstSpeechTestCV(networkmode);
  while(info != -1){
    // write the file array for the purpose of parallel writing protectation:
    file[info] = Tid;
    sprintf(filename,"outputs/spikepattern%d.dat",info);
    
    Foutp = fopen(filename,"w");
    assert(Foutp != NULL);
    fprintf(Foutp,"%d\t%d\n",-1,-1);
    fclose(Foutp);
    info = _network->LoadNextSpeechTestCV(networkmode);
  }
#endif
  for(int iii = 0; iii < 500; iii++){
    count = 0;
    _network->LSMClearSignals();
#ifdef CV
    info = _network->LoadFirstSpeechTrainCV(networkmode);
#else
    info = _network->LoadFirstSpeech(true, networkmode);
#endif
   
    while(info != -1){
      Fp = NULL;
      Foutp = NULL;
      count++;
      int time = 0;
      while(!_network->LSMEndOfSpeech(networkmode)){
        _network->LSMNextTimeStep(++time,true,iii,NULL,NULL);;
      }
      _network->LSMClearSignals();
#ifdef CV
      info = _network->LoadNextSpeechTrainCV(networkmode);
#else
      info = _network->LoadNextSpeech(true, networkmode);
#endif
    }
    //_network->SearchForNeuronGroup("output")->LSMPrintInputSyns();
//  }

    count = 0;
#ifdef CV
    info = _network->LoadFirstSpeechTestCV(networkmode);
#else
    info = _network->LoadFirstSpeech(false, networkmode);
#endif
    while(info != -1){
      Fp = NULL;
      sprintf(filename,"outputs/spikepattern%d.dat",info);
      Foutp = fopen(filename,"a");
      assert(Foutp != NULL);
      count++;
      int time = 0;
      while(!_network->LSMEndOfSpeech(networkmode)){
        _network->LSMNextTimeStep(++time,false,1,Foutp,NULL);
      }
      if(file[info] != -1){
        if(file[info] != Tid){
        cout<<"Thread "<<Tid<<" tried to write file: "<<filename<<" but Thread "<<file[info]<<" is writing it!"<<endl;
        assert(0);
        }
      }
      file[info] = Tid;
      assert(Foutp != NULL);
      fprintf(Foutp,"%d\t%d\n",-1,-1);
      fclose(Foutp);
      _network->LSMClearSignals();
#ifdef CV
      info = _network->LoadNextSpeechTestCV(networkmode);
#else
      info = _network->LoadNextSpeech(false, networkmode);
#endif
    }
//    _network->SearchForNeuronGroup("output")->LSMPrintInputSyns();
  }
#ifdef CV
#if NUM_THREADS == 1
}
#endif
#endif
}


//* Print the synaptic activity to a file:
void Simulator::PrintSynAct(int info){
  char name1[64], name2[64];
  char filename[128];
  sprintf(name1, "reservoir_1");
  sprintf(name2, "reservoir_15");
  sprintf(filename, "activity/speech_%d.txt", info);
  
  _network->WriteSynActivityToFile(name1, name2, filename);
}
