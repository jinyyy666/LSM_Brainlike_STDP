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


class Neuron{
private:
  neuronmode_t _mode;
  char * _name;

  std::list<Synapse*> _outputSyns; 
  std::list<Synapse*> _inputSyns; 
  bool _excitatory; 
  int _teacherSignal;
  int _indexInGroup;
  bool _del;
  Network * _network;

/* FOR LIQUID STATE MACHINE */
  double _lsm_v_mem;
  double _lsm_v_mem_pre;
  double _lsm_calcium;
  double _lsm_calcium_pre;
  double _lsm_state_EP;
  double _lsm_state_EN;
  double _lsm_state_IP;
  double _lsm_state_IN;
  const double _lsm_tau_EP;
  const double _lsm_tau_EN;
  const double _lsm_tau_IP;
  const double _lsm_tau_IN;
  const double _lsm_tau_FO;
  const double _lsm_v_thresh;

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
  const int _D_lsm_v_thresh;
  const int _D_lsm_tau_EP;
  const int _D_lsm_tau_EN;
  const int _D_lsm_tau_IP;
  const int _D_lsm_tau_IN;
  const int _D_lsm_tau_FO;
  const int _Unit;

public:
  Neuron(char*, Network*);
  Neuron(char*, bool, Network*); // only for liquid state machine
  ~Neuron();
  char * Name();
  void AddPostSyn(Synapse*);
  void AddPreSyn(Synapse*);

  void LSMPrintInputSyns();

  void SetIndexInGroup(int index){_indexInGroup = index;}
  int IndexInGroup(){return _indexInGroup;}
  void SetTeacherSignal(int signal);
  int GetTeacherSignal(){return _teacherSignal;}
  void PrintTeacherSignal();
  void PrintMembranePotential();
  Network * GetNetwork(){return _network;}

  double LSMGetVMemPre(){return _lsm_v_mem_pre;}
  int DLSMGetVMemPre(){return _D_lsm_v_mem_pre;}
  double GetCalciumPre(){return _lsm_calcium_pre;}
  int DLSMGetCalciumPre(){return _D_lsm_calcium_pre;}
  void SetExcitatory(){_excitatory = true;}
  bool IsExcitatory(){return _excitatory;}
  void LSMClear();

  /** Wrappers for clean code: **/
  void ExpDecay(int& var, const int time_c);
  void ExpDecay(double& var, const int time_c);
  void AccumulateSynapticResponse(const int pos, double value);
  void DAccumulateSynapticResponse(const int pos, int value);
  double NOrderSynapticResponse();
  int DNOrderSynapticResponse();
  void SetPostNeuronSpikeT(int t);
  void HandleFiringActivity(bool isInput, int time, bool train);

  void LSMNextTimeStep(int t , FILE * Foutp, FILE * Fp, bool train);
  void LSMSetChannel(Channel*,channelmode_t);
  void LSMRemoveChannel();
  void LSMSetNeuronMode(neuronmode_t neuronmode){_mode = neuronmode;}
  void ResizeSyn();
  void LSMPrintOutputSyns(FILE*);
  void LSMPrintLiquidSyns(FILE*);
  int SizeOutputSyns(){return _outputSyns.size();} //Calculate # of output synapses for verfication
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

  void PrintTeacherSignal();
  void PrintMembranePotential(double);
  Network * GetNetwork(){return _network;}

  void LSMLoadSpeech(Speech*,int*,neuronmode_t,channelmode_t);
  void LSMRemoveSpeech();
  void LSMSetTeacherSignal(int);
  void LSMRemoveTeacherSignal(int);
  void LSMPrintInputSyns();

};

#endif

