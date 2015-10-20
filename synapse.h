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

  int _lsm_spike;
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
  double Weight(){ return _lsm_weight;}
  int DWeight(){return _D_lsm_weight;}

  //* Set the synaptic weights:
  void Weight(int weight){_D_lsm_weight = weight;}
  void Weight(double weight){_lsm_weight = weight;}

  Neuron * PreNeuron();
  Neuron * PostNeuron();

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
  void LSMActivate(Network * network, bool stdp_flag, bool train);
  void LSMActivateReservoirSyns(Network*);
  void LSMDeactivate();
  bool LSMGetLiquid();

/* ONLY FOR STDP */

  void ExpDecay(double& var, const int time_c){var -= var/time_c;}
  void LSMUpdate(int);
  void LSMLiquidLearn(int);
  void CheckReservoirWeightOutBound();
  void CheckReadoutWeightOutBound();

    
  void ExpDecay(int& var, const int time_c){
    var -= (var/time_c == 0) ? 1 : var/time_c; 
    if(var < 0) 
    var = 0;
  }

  /** Definition for inline functions: **/
  inline 
  void LSMStaticCurrent(int * pos, double * value){
#ifdef DIGITAL
    assert(0);
#endif
    *pos = _excitatory ? 1 : -1;
    *value = _lsm_spike == 0 ? 0 : _lsm_weight;
    _lsm_spike = 0;
  }


  inline 
  void DLSMStaticCurrent(int* pos,int * value){
#ifndef DIGITAL
    assert(0);
#endif
    *pos = _excitatory ? 1 : -1;  
    *value = _D_lsm_spike == 0 ? 0: _D_lsm_weight;
    _D_lsm_spike = 0;
  }


  /** Definition for template functions: **/

  // Calculate the LTP:
  // Pot/Polarization will dirve weights to w_limit, 
  // so delta_w_pos > 0 for w > 0 & F_pos > 0
  //    delta_w_pos < 0 for w < 0 & F_pos < 0
  template<class T>
  void CalcLTP(T & delta_w_pos, T F_pos){
#ifdef DIGITAL
#ifdef PAIR_BASED
    delta_w_pos = (F_pos*_D_x_j);//>>(LAMBDA_BIT);
#elif NEAREST_NEIGHBOR
    delta_w_pos = (F_pos*_D_x_j);//>>(LAMBDA_BIT + NN_BIT_P);
#else
    delta_w_pos = (F_pos*_D_x_j*_D_y_i2_last);//>>(LAMBDA_BIT + DAMPING_BIT);
#endif
#else
#ifdef PAIR_BASED
    delta_w_pos = (F_pos*_x_j)*LAMBDA;//>>(LAMBDA_BIT);
#elif NEAREST_NEIGHBOR
    delta_w_pos = (F_pos*_x_j)*LAMBDA;//>>(LAMBDA_BIT + NN_BIT_P);
#else
    delta_w_pos = (F_pos*_x_j*_y_i2_last)*LAMBDA;//>>(LAMBDA_BIT + DAMPING_BIT);
#endif  
#endif
  }

  // Calculate the LTD:
  // Dep/Depolarization will drive weights to zero, 
  // so delta_w_neg < 0 for w > 0 & F_neg > 0
  //    delta_w_neg > 0 for w < 0 & F_neg < 0
  template<class T>
  void CalcLTD(T& delta_w_neg, T F_neg){
#ifdef DIGITAL
    delta_w_neg = -1*(F_neg*_D_y_i1);
#else
    delta_w_neg = -1*LAMBDA*ALPHA*(F_neg*_y_i1);
#endif
  }

};

#endif

