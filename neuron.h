#ifndef NEURON_H
#define NEURON_H

#include <list>
#include <vector>
#include <map>
#include <set>
#include "def.h"
#include "util.h"
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
  std::map<Neuron*, int> _correlation;
#ifdef DIGITAL
  std::vector<int> _vmems;
#else
  std::vector<double> _vmems;
#endif
  bool _excitatory; 
  int _teacherSignal;
  int _indexInGroup;
  bool _del;
  Network * _network;
  int _ind;
  int _f_count; // counter for firing activity
  bool _fired;

  // collect ep/en/ip/in stat for resolution
  int _EP_max, _EP_min, _EN_max, _EN_min, _IP_max, _IP_min, _IN_max, _IN_min;
  // collect max pre-spike count at each time point
  int _pre_fire_max;

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
  double _lsm_v_thresh;
  double _ts_strength_p; // for both continuous and digital model
  double _ts_strength_n;
  int _ts_freq;

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
  void LSMPrintInputSyns(std::ofstream& f_out);

  // return EP/EN/IP/IN max/min
  int GetEPMax(){return _EP_max;};
  int GetENMax(){return _EN_max;};
  int GetIPMax(){return _IP_max;};
  int GetINMax(){return _IN_max;};
  int GetEPMin(){return _EP_min;};
  int GetENMin(){return _EN_min;};
  int GetIPMin(){return _IP_min;};
  int GetINMin(){return _IN_min;};
  int GetPreActiveMax(){return _pre_fire_max;}

  // return the tau for EP/EN/IP/IN
  int GetTauEP(){return _lsm_tau_EP;}
  int GetTauEN(){return _lsm_tau_EN;}
  int GetTauIP(){return _lsm_tau_IP;}
  int GetTauIN(){return _lsm_tau_IN;}

  // record the firing frequency
  void FireFreq(double f){_fire_freq.push_back(f);}
  double FireFreq(){return _fire_freq.empty() ? 0 : _fire_freq.back();}

  // record the firing count:
  int FireCount(){return _f_count;}
  void FireCount(int count){_f_count = count;}

  bool Fired(){return _fired;}

  template <typename T> void GetWaveForm(std::vector<T>& v){v = _vmems;}
  // set the neuron index under the separated reservoir cases:
  void Index(int ind){_ind = ind;}
  
  // return the index of the neuron under the separated reservoir
  int Index(){return _ind;}

  void SetIndexInGroup(int index){_indexInGroup = index;}
  int IndexInGroup(){return _indexInGroup;}
  void SetVth(double vth);
  void SetTeacherSignal(int signal);
  void SetTeacherSignalStrength(double val, bool isPos){isPos? _ts_strength_p = val : _ts_strength_n = val;}
  void SetTeacherSignalFreq(int freq){_ts_freq = freq;}
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

  void LSMNextTimeStep(int t , FILE * Foutp, FILE * Fp, bool train, int end_time);
  double LSMSumAbsInputWeights();
  int  DLSMSumAbsInputWeights();
  void LSMSetChannel(Channel*,channelmode_t);
  void LSMRemoveChannel();
  void LSMSetNeuronMode(neuronmode_t neuronmode){_mode = neuronmode;}
  bool LSMCheckNeuronMode(neuronmode_t neuronmode){return _mode == neuronmode;}

  void CollectPreSynAct(double& p_n, double& avg_i_n, int& max_i_n);
  double ComputeVariance();

  void ComputeCorrelation();
  void CollectCorrelation(int& sum, int& cnt, int num_sample);
  void GroupCorrelated(UnionFind &uf, int pre_ind, int nsamples);
  void MergeNeuron(Neuron * target); // merge the this into target neuron
  //* Bp the error for each neuron
  void BpError(double error);
  void UpdateLWeight();

  void ResizeSyn();
  void LSMPrintOutputSyns(FILE*);
  void LSMPrintLiquidSyns(FILE*);
  int SizeOutputSyns(){return _outputSyns.size();} //Calculate # of output synapses for verfication
  void DisableOutputSyn(synapsetype_t syn_t);
  std::list<Synapse*> * LSMDisconnectNeuron();
  void LSMDeleteInputSynapse(char* pre_name);
  int RMZeroSyns(synapsetype_t syn_t, const char * t);
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

  void Collect4State(int& ep_max, int& ep_min, int& ip_max, int& ip_min, 
                     int& en_max, int& en_min, int& in_max, int& in_min, int& pre_active_max);
  void ScatterFreq(std::vector<double>& fs, std::size_t & bias, std::size_t & cnt);
  void CollectVariance(std::multimap<double, Neuron*>& my_map);
  void CollectPreSynAct(double & p_r, double & avg_i_r, int & max_i_r);
  void ComputeCorrelation();
  void MergeCorrelatedNeurons(int num_sample);

  int Judge(int cls);
  int MaxFireCount();
  void BpError(int cls);
  double ComputeRatioError(int cls);
  void UpdateLearningWeights();

  void DumpWaveFormGroup(std::ofstream & f_out);
  void LSMRemoveSpeech();
  void LSMTuneVth(int index);
  void LSMSetTeacherSignal(int);
  void LSMSetTeacherSignalStrength(const double pos, const double neg, int freq);
  void LSMRemoveTeacherSignal(int);
  void LSMPrintInputSyns();
  void LSMPrintInputSyns(std::ofstream& f_out);

  void DestroyResConn();
  void PrintResSyn(std::ofstream& f_out);
  void RemoveZeroSyns(synapsetype_t syn_type);
};

#endif

