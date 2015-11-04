#ifndef NETWORK_H
#define NETWORK_H

#include <list>
#include <vector>
#include <stdlib.h>
#include "def.h"
#include <cstdio>

class Neuron;
class Synapse;
class NeuronGroup;
class Speech;

class Network{
private:
  std::list<Neuron*> _individualNeurons;
  std::list<NeuronGroup*> _groupNeurons;
  std::list<Neuron*> _allNeurons;
  std::list<Neuron*> _allExcitatoryNeurons;
  std::list<Neuron*> _allInhibitoryNeurons;
  std::list<Synapse*> _synapses;
  std::vector<Synapse*> _rsynapses;       
  std::vector<Synapse*> _rosynapses;
  std::list<Synapse*> _activeInputSyns;
  std::list<Synapse*> _activeOutputSyns;
  std::list<Synapse*> _lsmActiveLearnSyns;
  std::list<Synapse*> _lsmActiveSyns;
  std::list<Synapse*> _lsmActiveReservoirLearnSyns;

  // for LSM
  std::vector<Speech*> _speeches;
  std::vector<Speech*>::iterator _sp_iter;
  int _fold;
  int _fold_ind;
  int _train_fold_ind;
  std::vector<Speech*> ** _CVspeeches;
  std::vector<Speech*>::iterator _cv_train_sp_iter, _cv_test_sp_iter;
  void RemoveCVSpeeches();
  NeuronGroup * _lsm_input_layer;
  NeuronGroup * _lsm_reservoir_layer;
  NeuronGroup * _lsm_output_layer;
  int _lsm_input;
  int _lsm_reservoir;
  int _lsm_t;
  networkmode_t _network_mode;
public:
  Network();
  ~Network();
  void LSMSetNetworkMode(networkmode_t networkmode){_network_mode = networkmode;}
  networkmode_t LSMGetNetworkMode(){return _network_mode;}

  bool CheckExistence(char *);
  void AddNeuron(char*,bool);
  void AddNeuronGroup(char*,int,bool);
  void LSMAddNeuronGroup(char*,int,int,int);
  void LSMAddNeuronGroup(char*,char*,char*); //reservoir, path_info_neuron, path_info_synapse
  
  void LSMAddSynapse(char*,char*,int,int,int,int,bool);
  void LSMAddSynapse(Neuron*,NeuronGroup*,int,int,int,bool);
  void LSMAddSynapse(NeuronGroup*,NeuronGroup*,int,int,int,int,bool);

  void PrintSize();
  void PrintAllNeuronName();

  int LSMSumAllOutputSyns();
  int LSMSizeAllNeurons();
  void LSMTruncSyns(int);
  void LSMTruncNeurons(int);
  void LSMPrintAllSyns(int);
  void LSMPrintAllLiquidSyns(int);
  void LSMPrintAllNeurons(int);
  void LSMNextTimeStep(int,bool,int,FILE *,FILE *);

  Neuron * SearchForNeuron(const char*);
  Neuron * SearchForNeuron(const char*, const char*);
  NeuronGroup * SearchForNeuronGroup(const char*);

  void IndexSpeech();

  // for LSM
  void AddSpeech(Speech*);
  int LoadFirstSpeech(bool,networkmode_t);
  int LoadNextSpeech(bool,networkmode_t);
  int NumSpeech(){return _speeches.size();}
  void AnalogToSpike();
  void RateToSpike();
  void LSMClearSignals();
  void LSMClearWeights();
  bool LSMEndOfSpeech(networkmode_t);
  void LSMChannelDecrement(channelmode_t);
  //* this function is to add the active learning readout synapses:
  void LSMAddActiveLearnSyn(Synapse*synapse){_lsmActiveLearnSyns.push_back(synapse);}
  void LSMAddActiveSyn(Synapse*synapse){_lsmActiveSyns.push_back(synapse);}

  //* this function is to add the reservoir synapses that should be trained by STDP:
  void LSMAddReservoirActiveSyn(Synapse * synapse);

  void CrossValidation(int);
  void Fold(int fold_ind){_fold_ind = fold_ind;}
  int LoadFirstSpeechTrainCV(networkmode_t);
  int LoadNextSpeechTrainCV(networkmode_t);
  int LoadFirstSpeechTestCV(networkmode_t);
  int LoadNextSpeechTestCV(networkmode_t);
  
  void SpeechInfo();
  // print the spikes into the file
  void SpeechPrint(int info);
  void DetermineNetworkNeuronMode(const networkmode_t &, neuronmode_t &, neuronmode_t &);
  void WriteSynWeightsToFile(const char * syn_type, char * filename);
  void LoadSynWeightsFromFile(const char * syn_type, char * filename);
  void WriteSynActivityToFile(char * pre_name, char * post_name, char * filename);

 template<class T>
 void LSMAddSynapse(Neuron * pre, Neuron * post, T weight, bool fixed, T weight_limit,bool liquid){
    Synapse * synapse = new Synapse(pre, post, weight, fixed, weight_limit, pre->IsExcitatory(),liquid);
    _synapses.push_back(synapse);
    // push back the reservoir and readout synapses into the vector:
    if(!synapse->IsReadoutSyn() && !synapse->IsInputSyn())
      _rsynapses.push_back(synapse);
    if(synapse->IsReadoutSyn())
      _rosynapses.push_back(synapse);

    pre->AddPostSyn(synapse);
    post->AddPreSyn(synapse);	
  } 

};

#endif
