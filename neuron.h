#ifndef NEURON_H
#define NEURON_H

#include<list>
#include<vector>
#include"def.h"
#include<cstdio>

class Synapse;
class Network;
class Speech;
class Channel;
struct Probe;

class Neuron{
private:
  char * _name;
  int _v_mem;
  int _v_mem_pre;
  double _calcium;
  double _calcium_pre;
  int _all_inputs;
  int _t;
  std::list<Synapse*> _outputSyns; // for both Fusi network and LSM 
  std::list<Synapse*> _inputSyns; // for both Fusi network and LSM
  double _externalInput;
  bool _excitatory; // for both Fusi network and LSM

  Probe * _probe;
  double * _voteCounts;
  const char ** _namesInputs;
  int _numNames;
  int _fired;  // for both Fusi network and LSM
  int _label;

  int _indexInGroup;
  int _teacherSignal;

  Network * _network;

/* FOR LIQUID STATE MACHINE */
  double _lsm_v_mem;
  double _lsm_v_mem_pre;
  double _lsm_all_inputs;
  double _lsm_t;
  int _lsm_ref;
  Channel * _lsm_channel;
  int _t_next_spike;

  int _D_lsm_v_mem;
  int _D_lsm_v_mem_pre;
  int _D_lsm_calcium;
  int _D_lsm_calcium_pre;
  int _D_lsm_state_EP;
  int _D_lsm_state_EN;
  int _D_lsm_state_IP;
  int _D_lsm_state_IN;
  const int _D_lsm_tau_EP;
  const int _D_lsm_tau_EN;
  const int _D_lsm_tau_IP;
  const int _D_lsm_tau_IN;
  const int _D_lsm_tau_FO;
  bool _del;
  const int _Lsm_v_thresh;
  const int _Unit;

  neuronmode_t _mode;
public:
  Neuron(char*, Network*);
  Neuron(char*, bool, Network*); // only for liquid state machine
  ~Neuron();
  void SetInput(double);
  void ClearInput();
  char * Name();
  void AddPostSyn(Synapse*);
  void AddPreSyn(Synapse*);
  void Update(std::list<Synapse*>*, std::list<Synapse*>*, double, double);
  void ReceiveSpike(int);
  void PrintInputSyns();
  void LSMPrintInputSyns();
  void ReadInputSyns();
  int Active();
  void AddProbe(Probe*);
  void Fire();
  int Fired(){return _fired;}
  void SetIndexInGroup(int index){_indexInGroup = index;}
  int IndexInGroup(){return _indexInGroup;}
  void SetTeacherSignal(int signal);
  int GetTeacherSignal(){return _teacherSignal;}
  void PrintTeacherSignal();
  void PrintMembranePotential();
  void SetUpForUnsupervisedLearning(std::vector<const char*>*);
  void StatAccumulate();
  Network * GetNetwork(){return _network;}
  void SetLabel();
  void SetLabel(int label) {_label = label;}
  int GetLabel(){return _label;}
  int Vote();
  int PrintTrainingResponse();
  int GetMemV(){return _v_mem;}
  int GetVMemPre(){return _v_mem_pre;}
  double LSMGetVMemPre(){return _lsm_v_mem_pre;}
  int DLSMGetVMemPre(){return _D_lsm_v_mem_pre;}
  double GetCalciumPre();
  int DLSMGetCalciumPre();
  void SetExcitatory();
  bool IsExcitatory(){return _excitatory;}
  void ClearFiringStat(){_fired = 0;}
/* ONLY FOR LIQUID STATE MACHINE */
  void LSMClear();
  void LSMNextTimeStep(int,FILE *);
  void LSMSetChannel(Channel*,channelmode_t);
  void LSMRemoveChannel();
  void LSMSetNeuronMode(neuronmode_t neuronmode){_mode = neuronmode;}
  void ResizeSyn();
  void LSMPrintOutputSyns(FILE*);
  void LSMPrintLiquidSyns(FILE*);
  int SizeOutputSyns(); //Calculate # of output synapses for verfication
  void DeleteAllSyns();
  void DeleteBrokenSyns();
  bool GetStatus();
};

class NeuronGroup{
private:
  char * _name;
  std::vector<Neuron*> _neurons;
  std::vector<Neuron*>::iterator _iter;
  bool _firstCalled;
  int * _subIndices;
  int ** _lsm_coordinates;
  Network * _network;
public:
  NeuronGroup(char*,int,Network*);
  NeuronGroup(char*,int,int,int,Network*); // only for liquid state machine
  NeuronGroup(char*,char*,char*,Network*); // for brain-like structure
  ~NeuronGroup();
  char * Name();
  Neuron * First();
  Neuron * Next();
  Neuron * Order(int);
  void UnlockFirst();
  int Size(){return _neurons.size();}
  void AddProbe(Probe*);
  void SetTeacherSignal(int);
  void SetTeacherSignalGroup(int, int, int);
  void SetTeacherSignalOneInGroup(int, int, int);
  void ClearSubIndices();
  void PrintTeacherSignal();
  void PrintMembranePotential(double);
  void PrintFiringStat(Probe*);
  void ReadInputSyns();
  void SetLabel(int);
  void ClearNeuronFiringStat();
  Network * GetNetwork(){return _network;}

  void LSMLoadSpeech(Speech*,int*,neuronmode_t,channelmode_t);
  void LSMRemoveSpeech();
  void LSMSetTeacherSignal(int);
  void LSMRemoveTeacherSignal(int);
  void LSMPrintInputSyns();

};

#endif

