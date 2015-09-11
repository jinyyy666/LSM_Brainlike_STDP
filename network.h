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
class Pattern;
class Speech;

struct Probe{
  int _probeFlag;
  std::list<int> _probeSpiking;
  std::vector<const char*> _nameInputs;
  int **_probeStats;
};

class Network{
private:
  std::list<Neuron*> _individualNeurons;
  std::list<NeuronGroup*> _groupNeurons;
  std::list<Neuron*> _allNeurons;
  std::list<Neuron*> _allExcitatoryNeurons;
  std::list<Neuron*> _allInhibitoryNeurons;
  std::list<Synapse*> _synapses;
  std::vector<Synapse*> _rsynapses;       
  std::list<Synapse*> _activeInputSyns;
  std::list<Synapse*> _activeOutputSyns;
  std::list<Synapse*> _lsmActiveLearnSyns;
  std::list<Synapse*> _lsmActiveSyns;
  std::list<Synapse*> _lsmActiveReservoirLearnSyns;
  std::list<Pattern*> _patternsTrain;
  std::list<Pattern*> _patternsTest;
  std::list<Pattern*> * _patterns;
  std::list<Pattern*>::iterator _iter;
  bool _iter_started;
  int _learning;
  Pattern * _currentPattern;
  Probe _probe;
  int _numNames;
  int * _voteCounts;
  bool _hasInputs;

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

  void AddSynapse(Neuron*,Neuron*,bool,bool,int);
  void AddSynapse(Neuron*,NeuronGroup*,bool,bool,int);
  void AddSynapse(NeuronGroup*,Neuron*,bool,bool,int);
  void AddSynapse(NeuronGroup*,NeuronGroup*,bool,bool,int);
  void AddSynapse(char*,char*,bool,bool,int);

  void LSMAddSynapse(Neuron*,Neuron*,double,bool,double);
  void LSMAddSynapse(Neuron*,NeuronGroup*,double,bool,double);
  void LSMAddSynapse(NeuronGroup*,Neuron*,double,bool,double);
  void LSMAddSynapse(NeuronGroup*,NeuronGroup*,double,bool,double);
  void LSMAddSynapse(char*,char*,double,bool,double);

  void LSMAddSynapse(Neuron*,Neuron*,int,bool,int,bool);
  void LSMAddSynapse(char*,char*,int,int,int,int,bool);
  void LSMAddSynapse(Neuron*,NeuronGroup*,int,int,int,bool);
  void LSMAddSynapse(NeuronGroup*,NeuronGroup*,int,int,int,int,bool);

/*
  void AddSynapse(Neuron*,Neuron*,bool,bool,int,int,int,int);
  void AddSynapse(Neuron*,NeuronGroup*,bool,bool,int,int,int,int);
  void AddSynapse(NeuronGroup*,Neuron*,bool,bool,int,int,int,int);
  void AddSynapse(NeuronGroup*,NeuronGroup*,bool,bool,int,int,int,int);
  void AddSynapse(char*,char*,bool,bool,int,int,int,int);
*/
  void AddSynapse(Neuron*,Neuron*,bool,bool,int,int,int);
  void AddSynapse(Neuron*,NeuronGroup*,bool,bool,int,int,int);
  void AddSynapse(NeuronGroup*,Neuron*,bool,bool,int,int,int);
  void AddSynapse(NeuronGroup*,NeuronGroup*,bool,bool,int,int,int);
  void AddSynapse(char*,char*,bool,bool,int,int,int);

  void PrintSize();
  void PrintAllNeuronName();

  void UpdateNeurons(double,double);
  void UpdateExcitatoryNeurons(double,double);
  void UpdateInhibitoryNeurons(double,double);
  void UpdateSynapses();
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
  void ActivateInputs(Pattern*);
  void ClearInputs();
  bool HasInputs(){return _hasInputs;}
  void LoadPatternsTrain();
  void LoadPatternsTest();
  void AddPattern(Pattern*);
  Pattern * FirstPattern();
  Pattern * NextPattern();

  void ClearProbe();
  int ReadProbe();
  void SetProbe(const char*);
  void ProbeStat(int);
  Probe * GetProbe(){return &_probe;}
  void SetTeacherSignal(const char*,const char*,int);
  void SetTeacherSignalGroup(const char*,const char*,int);
  void SetTeacherSignalOneInGroup(char*,char*,int);
  void PatternStat();
  void SetLearning(int);
  void AddPatternStat(int,Pattern*);
  void PrintProbeStats();
  void NeuronStatAccumulate();
  void SetCurrentPattern(Pattern*);
  Pattern * GetCurrentPattern(){return _currentPattern;}
  int PatternNum(){return (_patterns == NULL) ? 0 : _patterns->size();}
  void SetLabel();
  void SetLabel(const char*);
  void SaveLabel();
  void ReadLabel();
  int Vote();
  void PatternToFile(char*);
  void PatternToFile();
  void Print();
  void PrintTrainingResponse();
  void PatternsNormalization();
  void CodingLevel();
  void IndexSpeech();

  // for LSM
  void AddSpeech(Speech*);
  int LoadFirstSpeech(bool,networkmode_t);
  int LoadNextSpeech(bool,networkmode_t);
  int NumSpeech(){return _speeches.size();}
  void AnalogToSpike();
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
  
  int myrandom(int);
  void SpeechInfo();
  void DetermineNetworkNeuronMode(const networkmode_t &, neuronmode_t &, neuronmode_t &);
  void WriteSynWeightsToFile(const char * syn_type, char * filename);
};

#endif
