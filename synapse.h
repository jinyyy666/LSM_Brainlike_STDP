#ifndef SYNAPSE_H
#define SYNAPSE_H

#include<stdio.h>

class Neuron;
class Network;

// based on the model of an memrister synapse
class Synapse{
private:
  Neuron * _pre;
  Neuron * _post;
  bool _excitatory;
  bool _fixed;
  bool _liquid;
//  int _min;
//  int _max;
  int _ini_min;
  int _ini_max;
  int _weight;
  double _state_internal;
  int _factor;

  //double _t_spike_pre;
  //double _t_last_pre;
  //double _t_spike_post;
//  double _t_last_post;

  double _mem_pos;
  double _mem_neg;

/* ONLY FOR LIQUID STATE MACHINE */
  double _lsm_tau1;
  double _lsm_tau2;
  double _lsm_state1;
  double _lsm_state2;
  double _lsm_spike;
  int _D_lsm_spike;
  double _lsm_state;
  int _lsm_delay;
  // this variable is to indicate whether or not there is a fired spike or the synapse is activated by the pre/post-spike, which is a condition for STDP training: 
  bool _lsm_active;
  int _D_lsm_c;
  int _D_lsm_weight;
  int _D_lsm_weight_limit;

  double _lsm_weight;
  double _lsm_weight_limit;
  double _lsm_c;

/* ONLY FOR STDP Model */

  int _t_spike_pre;
  int _t_spike_post;
  int _t_last_pre;
  int _t_last_post;

  int _x_j;   // trace x_j induced by pre-spike
  int _y_i1; // fast trace y_i_1 induced by post-spike
  int _y_i2; // slow trace y_i_2 induced by post-spike
  int _y_i2_last ; // keep track of the _y_i_2 at the last time
public:
  Synapse(Neuron*,Neuron*,bool,bool,int);
  Synapse(Neuron*,Neuron*,bool,bool,int,int,int);
  Synapse(Neuron*,Neuron*,double,bool,double); // only for liquid state machine
//Synapse(Neuron*,Neuron*,int,bool,int,bool); // only for liquid state machine
  Synapse(Neuron*,Neuron*,int,bool,int,bool,bool); // only for liquid state machine (including initializing synapse in liquid)
  //void SetPreSpikeT(double);
  //void SetPostSpikeT(double);
  //* Get the _lsm_active of the synapse, which is the flag indicating whether or not the synapse is activated:
  bool GetActiveStatus(){return _lsm_active;}

  //* Set the _lsm_active of the synapse.  
  bool SetActiveStatus(bool status){_lsm_active = status;}

  //* Determine whether or not the synapse is the readout synapse:
  bool IsReadoutSyn();
  //* Determine whether or not the synapse is the input synapse:
  bool IsInputSyn();
  void SetPreSpikeT(int);
  void SetPostSpikeT(int);
  void SendSpike();
  void SetLearningSynapse(int);
  void Learn(int);
  int Weight();
  Neuron * PreNeuron();
  Neuron * PostNeuron();
  void Print();
  void Print(FILE*);
  void LSMPrint(FILE*);
  void LSMPrintSyns(FILE*);
  bool Excitatory();
  bool Fixed();
/* ONLY FOR LIQUID STATE MACHINE */
  void LSMPreSpike(int); // argument can only be 0 or 1
  void LSMNextTimeStep();
  double LSMCurrent();
  double LSMStaticCurrent();
  void DLSMStaticCurrent(int*,int*);
  double LSMFirstOrderCurrent();
  void LSMClear();
  void LSMClearLearningSynWeights();
  void LSMLearn(int);
  void LSMActivate(Network* network, bool stdp_flag = false);
  void LSMActivateReservoirSyns(Network*);
  void LSMDeactivate();
  bool LSMGetLiquid();

/* ONLY FOR STDP */
  void LSMUpdate(int);
  void LSMLiquidLearn(int);
  void CheckWeightOutBound();
};

#endif

