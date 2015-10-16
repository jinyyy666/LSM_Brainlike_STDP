#ifndef SYNAPSE_H
#define SYNAPSE_H

#include <stdio.h>
#include <assert.h>

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

  // _lsm_active is to indicate whether or not 
  // there is readout synapse to be trained
  bool _lsm_active;

  // _lsm_stdp_active is to indicate whether or not 
  // the synapse is being trained by STDP rule
  bool _lsm_stdp_active;

  double _mem_pos;  // those two variables are current not used
  double _mem_neg;

/* ONLY FOR LIQUID STATE MACHINE */
  double _lsm_tau1;
  double _lsm_tau2;
  double _lsm_state1;
  double _lsm_state2;
  double _lsm_state;

  double _lsm_spike;
  int _D_lsm_spike;

  int _lsm_delay;

  double _lsm_c;
  double _lsm_weight;
  double _lsm_weight_limit;

  int _D_lsm_c;
  int _D_lsm_weight;
  int _D_lsm_weight_limit;

/* ONLY FOR STDP Model */

  int _t_spike_pre;
  int _t_spike_post;
  int _t_last_pre;
  int _t_last_post;

  double _x_j;   // trace x_j induced by pre-spike
  double _y_i1;  // fast trace y_i_1 induced by post-spike
  double _y_i2;  // slow trace y_i_2 induced by post-spike
  double _y_i2_last; // keep track of the _y_i_2 at the last time

  int _D_x_j;     // digital version
  int _D_y_i1; 
  int _D_y_i2; 
  int _D_y_i2_last; 
public:
  Synapse(Neuron*,Neuron*,double,bool,double,bool,bool); // only for non-digital liquid state machine
  Synapse(Neuron*,Neuron*,int,bool,int,bool,bool); // only for digital liquid state machine (including initializing synapse in liquid)

  //* Get the _lsm_active of the synapse, which is the flag indicating whether or not the synapse is activated:
  bool GetActiveStatus(){return _lsm_active;}

  bool GetActiveSTDPStatus(){return _lsm_stdp_active;}

  //* Set the _lsm_active of the synapse.  
  bool SetActiveStatus(bool status){_lsm_active = status;}

  //* Determine whether or not the synapse is the readout synapse:
  bool IsReadoutSyn();
  //* Determine whether or not the synapse is the input synapse:
  bool IsInputSyn();
  void SetPreSpikeT(int t){_t_spike_pre = t;}
  void SetPostSpikeT(int t){_t_spike_post = t;}

  //* Get the synaptic weights:
  int Weight(){return _D_lsm_weight;}

  //* Set the synaptic weights:
  void Weight(int weight){_D_lsm_weight = weight;}

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

  double LSMFirstOrderCurrent();
  void LSMClear();
  void LSMClearLearningSynWeights();
  void LSMLearn(int);
  void LSMActivate(Network * network, bool stdp_flag = false);
  void LSMActivateReservoirSyns(Network*);
  void LSMDeactivate();
  bool LSMGetLiquid();

/* ONLY FOR STDP */
  void LSMUpdate(int);
  void LSMLiquidLearn(int);
  void CheckReservoirWeightOutBound();
  void CheckReadoutWeightOutBound();


  /** Definition for inline functions: **/
  inline 
  void LSMStaticCurrent(int * pos, double * value){
#ifdef DIGITAL
    assert(0);
#endif
    *pos = _excitatory ? 1 : -1;
    *value = _lsm_spike == 0 ? 0 : _lsm_spike*_lsm_weight;
    _lsm_spike = 0;
  }


  inline 
  void DLSMStaticCurrent(int* pos,int * value){
#ifndef DIGITAL
    assert(0);
#endif
    *pos = _excitatory ? 1 : -1;  
    *value = _D_lsm_spike*_D_lsm_weight;
    _D_lsm_spike = 0;
  }


};

#endif

