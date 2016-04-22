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
#include <algorithm>

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
#ifdef _WRITE_STAT
  for(int count = 0; count < _network->NumSpeech(); count++){
    // empty files
    sprintf(filename,"outputs/spikepattern%d.dat\0",count);
    Foutp = fopen(filename,"w");
    assert(Foutp != NULL);
    fprintf(Foutp,"%d\t%d\n",-1,-1);
    fclose(Foutp);
  }
#endif
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

#ifndef STDP_TRAINING
  if(tid == 0){
    sprintf(filename, "r_weights_info.txt");
    _network->WriteSynWeightsToFile("reservoir", filename);
  }
#endif

#ifdef STDP_TRAINING
#ifdef STDP_TRAINING_RESERVOIR
  // train the reservoir using STDP rule:
  networkmode = TRAINRESERVOIR;
  _network->LSMSetNetworkMode(networkmode);

  gettimeofday(&val1, &zone);
  // repeatedly training the reservoir for a certain amount of iterations:
  for(int i = 0; i < 25; ++i){
      _network->LSMReservoirTraining(networkmode);

#if NUM_THREADS == 1  
      // Write the weight back to file after training the reservoir with STDP:
      sprintf(filename, "reservoir_weights_%d.txt", i);
      _network->WriteSynWeightsToFile("reservoir",filename);
#endif       
  }
  gettimeofday(&val2, &zone);
  cout<<"Total time spent in training the reservoir: "<<((val2.tv_sec - val1.tv_sec) + double(val2.tv_usec - val1.tv_usec)*1e-6)<<" seconds"<<endl;
#endif

  // visualize the reservoir synapses after stdp training:
  //_network->VisualizeReservoirSyns(1);

#ifdef STDP_TRAINING_INPUT
  // train the input using STDP rule:
  networkmode = TRAININPUT;
  _network->LSMSetNetworkMode(networkmode);

  gettimeofday(&val1, &zone);
  // repeatedly training the input for a certain amount of iterations:
  for(int i = 0; i < 20; ++i){
      _network->LSMInputTraining(networkmode);

#if NUM_THREADS == 1
      // Write the weight back to file after training the input with STDP:
      sprintf(filename, "input_weights_%d.txt", i);
      _network->WriteSynWeightsToFile("input",filename);
#endif
  }
  gettimeofday(&val2, &zone);
  cout<<"Total time spent in training the input: "<<((val2.tv_sec - val1.tv_sec) + double(val2.tv_usec - val1.tv_usec)*1e-6)<<" seconds"<<endl;
#endif

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
  _network->LSMSumGatedNeurons();
#endif

  // Load the weight from file:
  // sprintf(filename, "r_weights_info.txt");
  // _network->LoadSynWeightsFromFile("reservoir", filename);

  // Write the weight back to file after training the reservoir with STDP:
  if(tid == 0){
    sprintf(filename, "r_weights_info.txt");
    _network->WriteSynWeightsToFile("reservoir", filename);
    sprintf(filename, "i_weights_info.txt");
    _network->WriteSynWeightsToFile("input", filename);
  }

#ifdef _RM_ZERO_RES_WEIGHT
  _network->RemoveZeroWeights("reservoir");
#endif  
 
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

#ifdef _RES_FIRING_CHR
  vector<double> prob, avg_intvl;
  vector<int> max_intvl;
  double prob_f = 0.0, avg_intvl_f = 0.0;
  int max_intvl_f = 0;
#endif
  count = 0;
  _network->LSMClearSignals();
  info = _network->LoadFirstSpeech(false, networkmode);
  while(info != -1){
    Fp = NULL;
    Foutp = NULL;
    count++;
    int time = 0;
    while(!_network->LSMEndOfSpeech(networkmode)){
      _network->LSMNextTimeStep(++time,false,1, 1, NULL,NULL);
    }

    //cout<<"Speech "<<count<<endl;
    //_network->SpeechPrint(info);
    // print the firing frequency into the file:
    //_network->SpeechSpikeFreq("input", f1, f2);
#ifdef _RES_FIRING_CHR
    _network->PreActivityStat("reservoir", prob, avg_intvl, max_intvl);
#endif
    
#ifdef _VARBASED_SPARSE
    // Collect the firing activity of each reservoir neurons from sp
    // and scatter the frequency back to the network:
    cout<<"Begin readout sparsification...."<<endl;
    vector<double> fs;
    _network->CollectSpikeFreq("reservoir", fs, time);
    _network->ScatterSpikeFreq("reservoir", fs);
    cout<<"Done with the readout sparsification!"<<endl;
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

#ifdef _RES_FIRING_CHR
  CollectPAStat(prob, avg_intvl, max_intvl, prob_f, avg_intvl_f, max_intvl_f);
  cout<<"The average probability of a pre-synaptic event: "<<prob_f<<endl;
  cout<<"The average interval between two pre-synaptic events: "<<avg_intvl_f<<endl;
  cout<<"The max interval between two pre-synaptic events: "<<max_intvl_f<<endl;
  return;
#endif
  
  // train the readout layer
  networkmode = READOUT;
  _network->LSMSetNetworkMode(networkmode);
#ifdef CV
#if NUM_THREADS == 1
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
#ifdef _WRITE_STAT
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
#endif
  int correct = 0, wrong = 0, even = 0;
  for(int iii = 0; iii < NUM_ITERS; iii++){
    count = 0, correct = 0, wrong = 0, even = 0;
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
        _network->LSMNextTimeStep(++time,true,iii, 1, NULL,NULL);;
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
      Fp = NULL, Foutp = NULL;

#ifdef _WRITE_STAT
      sprintf(filename,"outputs/spikepattern%d.dat",info);
      Foutp = fopen(filename,"a");
      assert(Foutp != NULL);
#endif
      count++;
      int time = 0;
      int end_time = _network->SpeechEndTime();
      while(!_network->LSMEndOfSpeech(networkmode)){
        _network->LSMNextTimeStep(++time,false,1,end_time, Foutp,NULL);
      }
      /*
      if(file[info] != -1){
        if(file[info] != Tid){
        cout<<"Thread "<<Tid<<" tried to write file: "<<filename<<" but Thread "<<file[info]<<" is writing it!"<<endl;
        assert(0);
        }
      }
      file[info] = Tid;
      */

#ifdef _WRITE_STAT
      assert(Foutp != NULL);
      fprintf(Foutp,"%d\t%d\n",-1,-1);
      fclose(Foutp);
#endif

      ReadoutJudge(correct, wrong, even); // judge the readout output here
      _network->LSMClearSignals();
#ifdef CV
      info = _network->LoadNextSpeechTestCV(networkmode);
#else
      info = _network->LoadNextSpeech(false, networkmode);
#endif
    }
    _network->LSMPushResults(correct, wrong, even, iii); // collect the result in network

//    _network->SearchForNeuronGroup("output")->LSMPrintInputSyns();
  }
#ifdef CV
#if NUM_THREADS == 1
}
#endif
#endif
}

//* judge the readout result:
void Simulator::ReadoutJudge(int& correct, int& wrong, int& even){
  int res = _network->LSMJudge();

  if(res == 1) ++correct;
  else if(res == -1) ++wrong;
  else if(res == 0) ++even;
  else{
    cout<<"In Simulator::ReadoutJudge(int&, int&, int&)\n"
        <<"Undefined return type: "<<res<<" returned by Network::LSMJudge()"
        <<endl;
  }
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

/***************************************************************************** 
 * Collect the avg pre-synaptic event statistics of each neuron and compute avg
 * @param1-3 input vectors, each elem is the stat of each speech
 * @param4-6 return values
 ****************************************************************************/
void Simulator::CollectPAStat(vector<double>& prob, 
			      vector<double>& avg_intvl, 
			      vector<int>& max_intvl, 
			      double& prob_f, 
			      double& avg_intvl_f, 
			      int& max_intvl_f
			      )
{
  // initialize the return variables:
  prob_f = 0.0, avg_intvl_f = 0.0, max_intvl_f = 0;
  cout<<prob.size()<<"\t"<<avg_intvl.size()<<"\t"<<max_intvl.size()<<endl;

  assert(prob.size()== avg_intvl.size() 
      && prob.size()==max_intvl.size()
      && prob.size()==(size_t)_network->NumSpeech());

  for(size_t i = 0; i < prob.size(); ++i){
    prob_f += prob[i];
    avg_intvl_f += avg_intvl[i];
    max_intvl_f = max(max_intvl_f, max_intvl[i]);
  }

  prob_f = _network->NumSpeech() ? prob_f/(_network->NumSpeech()) : 0;
  avg_intvl_f = _network->NumSpeech() ? avg_intvl_f/(_network->NumSpeech()) : 0;
}
