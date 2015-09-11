#include "def.h"
#include "neuron.h"
#include "simulator.h"
#include "network.h"
#include "pattern.h"
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

void Simulator::Run(bool load){
  Pattern * pattern;
  double t_start;
  int index = 0;
  int ind;

  if(_t < 0) _t = _t_step;

  _network->SetProbe("output");
  _network->ProbeStat(0);

if(0){
  (_network->SearchForNeuronGroup("output"))->ReadInputSyns();
  _network->ReadLabel();
}else{
  if(load == true) (_network->SearchForNeuronGroup("output"))->ReadInputSyns();
  _network->LoadPatternsTrain();
  _network->SetLearning(1);
#ifdef SUPERVISED
  (_network->SearchForNeuronGroup("output"))->SetTeacherSignal(0);
  _network->SetLabel("output");
#endif
  cout<<"start training ..."<<endl;

int index0 = 0;
int index1 = 0;
int index2 = 0;
  for(pattern = _network->FirstPattern(); pattern != NULL; pattern = _network->NextPattern()){
//if(index == 1) break;
/*
if(strcmp(pattern->Name(),"0")==0){
  if(index0 < 80) index0++;
  else continue;
}else if(strcmp(pattern->Name(),"4")==0){
  if(index1 < 80) index1++;
  else continue;
}else if(strcmp(pattern->Name(),"9")==0){
  if(index2 < 80) index2++;
  else continue;
}else continue;
*/

#ifdef SUPERVISED
    TSstrength = 1;// - ((double)(index))/((double)(_network->PatternNum()));
//    _network->SetTeacherSignal("output",pattern->Name(),1);
//    _network->SetTeacherSignalOneInGroup("output",pattern->Name(),1);
    _network->SetTeacherSignalGroup("output",pattern->Name(),1);
//    (_network->SearchForNeuronGroup("output"))->PrintTeacherSignal();
#endif
#ifdef AUTOTIMER
    _network->ClearProbe();
#endif
    _network->ActivateInputs(pattern);

    t_start = _t;
cout<<"starts from:\t"<<t_start<<endl;
    for(; _t < t_start + _t_end - _t_step/2; _t += _t_step){
      //_network->UpdateNeurons(_t_step,_t);
//if((_t-t_start) > 200*_t_step) (_network->SearchForNeuron("output","0"))->Fire();
      _network->UpdateExcitatoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
      _network->UpdateInhibitoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
//      (_network->SearchForNeuronGroup("output"))->PrintMembranePotential(_t);

#ifdef AUTOTIMER
//      if(_network->ReadProbe() > 0) break;
#endif
    }

    (_network->SearchForNeuronGroup("output"))->PrintFiringStat(_network->GetProbe());
    (_network->SearchForNeuronGroup("output"))->ClearNeuronFiringStat();

#ifndef SUPERVISED
    _network->NeuronStatAccumulate();	// also clear the state variable counter for spikes
#endif

//    pattern->Deactivate();
#ifdef SUPERVISED
//    _network->SetTeacherSignal("output",pattern->Name(),0);
    _network->SetTeacherSignalGroup("output",pattern->Name(),-1);
#endif
    _network->ClearInputs();
//    if(_t < t_start + _t_end + 1e-12) t_start = _t + _t_step;
//    else t_start = _t;

    t_start = _t;
    for(; _t < t_start + INTVL - _t_step/2; _t += _t_step){
      _network->UpdateExcitatoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
      _network->UpdateInhibitoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
//      (_network->SearchForNeuronGroup("output"))->PrintMembranePotential(_t);
    }

#ifdef SUPERVISED
//    _network->SetTeacherSignal("output",pattern->Name(),0);
    _network->SetTeacherSignalGroup("output",pattern->Name(),0);
#endif
    cout<<"finished training letter "<<++index<<"\t\tpattern name: "<<pattern->Name()<<"\t@ "<<_t<<endl;
//    if(index%10 == 0) _network->Print();	// to files
  }

  _network->Print();
#ifndef SUPERVISED
  _network->SetLabel();	// based on _network->NeuronStatAccumulate()
#endif
  _network->SaveLabel();
  (_network->SearchForNeuronGroup("output"))->ClearNeuronFiringStat();

  _network->PrintTrainingResponse();
  _network->PatternStat();

//  (_network->SearchForNeuronGroup("output"))->ClearSubIndices();
//  return;
}

  _network->LoadPatternsTest();
  _network->ProbeStat(1);
  _network->SetLearning(0);
#ifdef SUPERVISED
  (_network->SearchForNeuronGroup("output"))->SetTeacherSignal(-1);
#endif
  index = 0;
  ind = 0;

  for(pattern = _network->FirstPattern(); pattern != NULL; pattern = _network->NextPattern()){
    ind++;
//if(index > 100)break;
//if((strcmp(pattern->Name(),"0")!=0)&&(strcmp(pattern->Name(),"1")!=0)&&(strcmp(pattern->Name(),"2")!=0)) continue;
//if((strcmp(pattern->Name(),"4")!=0)&&(strcmp(pattern->Name(),"9")!=0)) continue;
    _network->ActivateInputs(pattern);
    _network->ClearProbe();
    t_start = _t;
    for(; _t < t_start + _t_end - _t_step/2; _t += _t_step){
      _network->UpdateExcitatoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
      _network->UpdateInhibitoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
//      if(_network->ReadProbe() > 0) break;
    }
    (_network->SearchForNeuronGroup("output"))->PrintFiringStat(_network->GetProbe());
    _network->AddPatternStat(ind, pattern);	// also clear state variables storing the firing information

    _network->ClearInputs();
    t_start = _t;
    for(; _t < t_start + INTVL - _t_step/2; _t += _t_step){
      _network->UpdateExcitatoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
      _network->UpdateInhibitoryNeurons(_t_step,_t);
      _network->UpdateSynapses();
    }
    cout<<"finish testing letter "<<++index<<"\t\tpattern name: "<<pattern->Name()<<endl;
  }
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
  // train the reservoir using STDP rule:
  networkmode = TRAINRESERVOIR;
  _network->LSMSetNetworkMode(networkmode);

  gettimeofday(&val1, &zone);
  // repeatedly training the reservoir for a certain amount of iterations:
  for(int i = 0; i < 10; ++i){
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
      while(!_network->LSMEndOfSpeech(networkmode)){
        _network->LSMNextTimeStep(++time, false, 1, NULL, Fp);
      }
#if NUM_THREADS == 1
      fclose(Fp);
#endif
      cout<<"Speech : "<<info<<endl;
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

  assert(0);

#if NUM_THREADS == 1  
  // Write the weight back to file after training the reservoir with STDP:
  sprintf(filename, "reservoir_weights_%ld.txt", tid);
  _network->WriteSynWeightsToFile("reservoir",filename);
#endif
  // Load the weight from file:
  
  
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
        _network->LSMNextTimeStep(++time,true,iii,NULL,NULL);
      }
      _network->LSMClearSignals();
#ifdef CV
      info = _network->LoadNextSpeechTrainCV(networkmode);
#else
      info = _network->LoadNextSpeech(true, networkmode);
#endif
    }
//    _network->SearchForNeuronGroup("output")->LSMPrintInputSyns();
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


