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

  // visualize the reservoir synapses before stdp training:
  // _network->VisualizeReservoirSyns(0);

  // detect total number of hubs in the reservoir BEFORE stdp training:
  //cout<<"Before STDP training:"<<endl;
  //_network->LSMHubDetection();

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

  gettimeofday(&val1, &zone);
  // repeatedly training the reservoir for a certain amount of iterations:
  for(int i = 0; i < 0; ++i){
      _network->LSMReservoirTraining(networkmode);

#if NUM_THREADS == 1  
      // Write the weight back to file after training the reservoir with STDP:
      sprintf(filename, "reservoir_weights_%d.txt", i);
      _network->WriteSynWeightsToFile("reservoir",filename);
#endif       

  }
  // visualize the reservoir synapses after stdo training:
  //_network->VisualizeReservoirSyns(1);


  ////////////////////////////////////////////////////////////////////////
  // REMEMBER TO REMOVE THESE CODES!!
  //sprintf(filename, "r_weights_info_best.txt");
  //_network->LoadSynWeightsFromFile("reservoir", filename);
  ////////////////////////////////////////////////////////////////////////


  // detect total number of hubs in the reservoir AFTER stdp training:
  //cout<<"After STDP training:"<<endl;
  //_network->LSMHubDetection();

#ifdef ADAPTIVE_POWER_GATING
  // apply the power gating scheme to turn off some neurons with low connectivity
  _network->LSMAdaptivePowerGating(); 
  // retrain the network for few echos:
  for(int i = 0; i < 5; ++i)
      _network->LSMReservoirTraining(networkmode);
#endif
  _network->LSMSumGatedNeurons();
  gettimeofday(&val2, &zone);
  cout<<"Total time spent in training the reservoir: "<<((val2.tv_sec - val1.tv_sec) + double(val2.tv_usec - val1.tv_usec)*1e-6)<<" seconds"<<endl;

  // Write the weight back to file after training the reservoir with STDP:
  if(tid == 0){
    sprintf(filename, "r_weights_info.txt");
    _network->WriteSynWeightsToFile("reservoir", filename);
  }
  
  // Load the weight from file:
  // sprintf(filename, "r_weights_info_best.txt");
  //_network->LoadSynWeightsFromFile("reservoir", filename);
#endif  

  ////////////////////////////////////////////////////////////////////////
  // REMEMBER TO REMOVE THESE CODES!!
  // sprintf(filename, "r_weights_info_best.txt");
  // _network->LoadSynWeightsFromFile("reservoir", filename);
  // _network->TruncateIntermSyns("reservoir");
  // sprintf(filename, "r_weights_info_best_tmp.txt");
  // _network->WriteSynWeightsToFile("reservoir", filename);
  //ofstream f1("spike_freq.txt"), f2("spike_speech_label.txt");
  //assert(f1.is_open() && f2.is_open());
  ////////////////////////////////////////////////////////////////////////

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
    //_network->SpeechPrint(info);
    // print the firing frequency into the file:
    //_network->SpeechSpikeFreq("input", f1, f2);
    
#ifdef _VARBASED_SPARSE
    // Collect the firing activity of each reservoir neurons from sp
    // and scatter the frequency back to the network:
    vector<double> fs;
    _network->CollectSpikeFreq("reservoir", fs, time);
    _network->ScatterSpikeFreq("reservoir", fs);
#endif

    _network->LSMClearSignals();
    info = _network->LoadNextSpeech(false, networkmode);
  }

  //////////////////////////////////////////////////////////////////////////////
  // REMEMBER TO REMOVE THESE CODES!
  //_network->LSMSumGatedNeurons();
  // f2<<endl;
  // f1.close(); f2.close();
  /////////////////////////////////////////////////////////////////////////////

#ifdef _VARBASED_SPARSE
  // sparsify the reservoir to readout connection using var-based technique:
  _network->VarBasedSparsify("readout");
#endif

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


//* Print out the frequencies to file:
void Simulator::PrintOutFreqs(const vector<vector<double> >& all_fs){
  ofstream f_out("all_readout_freq.txt");
  assert(f_out.is_open());
  for(size_t i = 0; i < all_fs.size(); ++i){
      for(size_t j = 0; j < all_fs[i].size(); ++j){
	f_out<<all_fs[i][j]<<"\t";
      }
      f_out<<endl;
      f_out<<"*******************************************************************"<<endl;
      f_out<<endl;
  }
  f_out.close();
}
