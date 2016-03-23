#ifndef NEURON_H
#define NEURON_H

#include <list>
#include <vector>
#include <map>
#include <set>
#include "def.h"
#include <cstdio>
#include <fstream>

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
  std::vector<double> _fire_freq;
  std::vector<bool> _presyn_act;
  bool _excitatory; 
  int _teacherSignal;
  int _indexInGroup;
  bool _del;
  Network * _network;
  int _ind;

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
  int _D_lsm_v_thresh;
  const int _D_lsm_tau_EP;
  const int _D_lsm_tau_EN;
  const int _D_lsm_tau_IP;
  const int _D_lsm_tau_IN;
  const int _D_lsm_tau_FO;
  const int _D_lsm_v_reservoir_max;
  const int _D_lsm_v_reservoir_min;
  const int _D_lsm_v_readout_max;
  const int _D_lsm_v_readout_min;
  const int _Unit;

public:
  Neuron(char*, Network*);
  Neuron(char*, bool, Network*); // only for liquid state machine
  ~Neuron();
  char * Name();
  void AddPostSyn(Synapse*);
  void AddPreSyn(Synapse*);

  void LSMPrintInputSyns();

  // record the firing frequency
  void FireFreq(double f){_fire_freq.push_back(f);}
  double FireFreq(){return _fire_freq.empty() ? 0 : _fire_freq.back();}

  // set the neuron index under the separated reservoir cases:
  void Index(int ind){_ind = ind;}
  
  // return the index of the neuron under the separated reservoir
  int Index(){return _ind;}

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

  bool PowerGating();
  bool IsHubRoot();
  bool IsHubChild(const char * name);
  void LSMClear();

  /** Wrappers for clean code: **/
  void ExpDecay(int& var, const int time_c);
  void ExpDecay(double& var, const int time_c);
  void BoundVariable(int& var, const int v_min, const int v_max);
  void AccumulateSynapticResponse(const int pos, double value);
  void DAccumulateSynapticResponse(const int pos, int value, const int c_num_dec_digit_mem,const int c_nbt_std_syn, const int c_num_bit_syn);
  double NOrderSynapticResponse();
  int DNOrderSynapticResponse();
  void UpdateVmem(int& temp, const int c_num_dec_digit_mem, const int c_nbt_std_syn, const int c_num_bit_syn);
  void SetPostNeuronSpikeT(int t);
  void HandleFiringActivity(bool isInput, int time, bool train);

  void LSMNextTimeStep(int t , FILE * Foutp, FILE * Fp, bool train);
  void LSMSetChannel(Channel*,channelmode_t);
  void LSMRemoveChannel();
  void LSMSetNeuronMode(neuronmode_t neuronmode){_mode = neuronmode;}
  bool LSMCheckNeuronMode(neuronmode_t neuronmode){return _mode == neuronmode;}
  void CollectPreSynAct(double& p_n, double& avg_i_n, int& max_i_n);
  double ComputeVariance();
  void ResizeSyn();
  void LSMPrintOutputSyns(FILE*);
  void LSMPrintLiquidSyns(FILE*);
  int SizeOutputSyns(){return _outputSyns.size();} //Calculate # of output synapses for verfication
  void DisableOutputSyn(synapsetype_t syn_t);
  std::list<Synapse*> * LSMDisconnectNeuron();
  void LSMDeleteInputSynapse(char* pre_name);
  void RMZeroSyns(synapsetype_t syn_t, const char * t);
  void DeleteSyn(const char * t, const char s);
  void PrintSyn(std::ofstream& f_out, const char * t, const char s);

  void DeleteAllSyns();
  void DeleteBrokenSyns();
  bool GetStatus();
};

class NeuronGroup{
private:
  char * _name;
  std::vector<Synapse*> _synapses;
  std::vector<Synapse*>::iterator _s_iter;
  std::vector<Neuron*> _neurons;
  std::vector<Neuron*>::iterator _iter;
  std::set<int> _s_labels;
  bool _firstCalled;
  bool _s_firstCalled;

  int ** _lsm_coordinates;
  Network * _network;
public:
  NeuronGroup(char*,int,Network*);
  NeuronGroup(char*,int,int,int,Network*); // only for liquid state machine
  NeuronGroup(char*,char*,char*,Network*); // for brain-like structure
  ~NeuronGroup();

  char * Name(){return _name;};

  void AddSynapse(Synapse * synapse);
  Neuron * First();
  Neuron * Next();
  Synapse * FirstSynapse();
  Synapse * NextSynapse();

  Neuron * Order(int);
  void UnlockFirst();
  void UnlockFirstSynapse();

  int Size(){return _neurons.size();}
  void SubSpeechLabel(std::set<int> labels){_s_labels = labels;}
  std::set<int> SubSpeechLabel(){return _s_labels;}
  bool InSet(int num){return _s_labels.empty()? true: _s_labels.count(num)!=0;}

  void PrintTeacherSignal();
  void PrintMembranePotential(double);
  Network * GetNetwork(){return _network;}

  void LSMLoadSpeech(Speech*,int*,neuronmode_t,channelmode_t);
  void LSMSetNeurons(Speech* speech, neuronmode_t neuronmode, channelmode_t channelmode, int offset);
  void LSMIndexNeurons(int offset);

  void ScatterFreq(std::vector<double>& fs, std::size_t & bias, std::size_t & cnt);
  void CollectVariance(std::multimap<double, Neuron*>& my_map);
  void CollectPreSynAct(double & p_r, double & avg_i_r, int & max_i_r);
  void LSMRemoveSpeech();
  void LSMSetTeacherSignal(int);
  void LSMRemoveTeacherSignal(int);
  void LSMPrintInputSyns();
  
  void DestroyResConn();
  void PrintResSyn(std::ofstream& f_out);
  void RemoveZeroSyns(synapsetype_t syn_type);
};

#endif

