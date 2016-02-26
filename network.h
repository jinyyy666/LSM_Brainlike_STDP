#ifndef NETWORK_H
#define NETWORK_H

#include <list>
#include <vector>
#include <set>
#include <stdlib.h>
#include "def.h"
#include <cstdio>
#include <fstream>

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
  std::list<Neuron*> _inputNeurons;
  std::list<Neuron*> _outputNeurons;
  std::list<NeuronGroup*> _allReservoirs;
  std::list<Synapse*> _synapses;
  std::vector<Synapse*> _rsynapses;       
  std::vector<Synapse*> _rosynapses;;
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
  int _lsm_readout;
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
  void LSMDeleteNeuronGroup(char * name);
  void InitializeInputLayer(int num_inputs, int num_connections);
  void LSMAddLabelToReservoirs(const char * name, std::set<int> labels);
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

  // Supporting functions for enhanced analysis
  void LSMAdaptivePowerGating();
  void LSMSumGatedNeurons();
  void LSMHubDetection();

  void LSMReservoirTraining(networkmode_t networkmode);
  void LSMNextTimeStep(int t, bool train, int iteration, FILE* Foutp, FILE* fp, NeuronGroup* reservoir = NULL);

  Neuron * SearchForNeuron(const char*);
  Neuron * SearchForNeuron(const char*, const char*);
  NeuronGroup * SearchForNeuronGroup(const char*);

  void IndexSpeech();

  // for LSM
  void AddSpeech(Speech*);
  void LoadSpeeches(Speech      * sp_iter, 
		    NeuronGroup * input,
		    NeuronGroup * reservoir,
		    NeuronGroup * output,
		    neuronmode_t neuronmode_input,
		    neuronmode_t neuronmode_reservoir,
		    neuronmode_t neuronmode_readout,
		    bool train,
		    bool ignore_reservoir
		    );
  void LoadSpeechToAllReservoirs(Speech *sp, neuronmode_t neuronmode_reservoir);
  void LSMNetworkRemoveSpeech();
  void LSMLoadLayers(NeuronGroup * reservoir_group);
  void DetermineNetworkNeuronMode(const networkmode_t &, neuronmode_t &, neuronmode_t &, neuronmode_t&);

  int LoadFirstSpeech(bool train, networkmode_t networkmode, NeuronGroup* group = NULL);
  int LoadFirstSpeech(bool train, networkmode_t networkmode, NeuronGroup* group, bool inputExist);
  int LoadNextSpeech(bool train, networkmode_t networkmode, NeuronGroup* group = NULL);
  int LoadNextSpeech(bool train, networkmode_t networkmode, NeuronGroup* group, bool inputExist);
  int LoadFirstSpeechTrainCV(networkmode_t);
  int LoadNextSpeechTrainCV(networkmode_t);
  int LoadFirstSpeechTestCV(networkmode_t);
  int LoadNextSpeechTestCV(networkmode_t);

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

  void SpeechInfo();
  // print the spikes into the file
  void SpeechPrint(int info);
  void CollectSpikeFreq(const char * type, std::vector<double>& fs, int end_t);
  void ScatterSpikeFreq(const char * type, std::vector<double>& fs);
  // print the spiking frequency into the file:
  void SpeechSpikeFreq(const char * type, std::ofstream & f_out, std::ofstream & f_label);
  void VarBasedSparsify(const char * type);
  
  // supporting functions:
  // this first parameter is used to indicate which neurongroup does the syn belongs to.
  void DetermineSynType(const char * syn_type, synapsetype_t & ret_syn_type, const char * func_name);
  void NormalizeContinuousWeights(const char * syn_type);
  void WriteSynWeightsToFile(const char * syn_type, char * filename);
  void LoadSynWeightsFromFile(const char * syn_type, char * filename);
  void WriteSynActivityToFile(char * pre_name, char * post_name, char * filename);
  void DestroyReservoirConn(NeuronGroup * reservoir);
  void TruncateIntermSyns(const char * syn_type);
  
  void VisualizeReservoirSyns(int indicator);


  template<class T>
  void LSMAddSynapse(Neuron * pre, Neuron * post, T weight, bool fixed, T weight_limit,bool liquid, NeuronGroup * group = NULL){
    Synapse * synapse = new Synapse(pre, post, weight, fixed, weight_limit, pre->IsExcitatory(),liquid);
    _synapses.push_back(synapse);
    // push back the reservoir and readout synapses into the vector:
    if(!synapse->IsReadoutSyn() && !synapse->IsInputSyn())
      _rsynapses.push_back(synapse);
    if(synapse->IsReadoutSyn())
      _rosynapses.push_back(synapse);

    if(group)  group->AddSynapse(synapse);
    pre->AddPostSyn(synapse);
    post->AddPreSyn(synapse);	
  } 

};

#endif
