
#ifndef SYNAPSE_H
#define SYNAPSE_H

#include <cmath>
#include <stdio.h>
#include <iomanip>
#include <fstream>
#include <vector>
#include <assert.h>
#include "def.h"


/** unit system defined for digital lsm **/
extern const int one;
extern const int unit;

// the spike strength in STDP model:
const int UNIT_DELTA = 1<<(NUM_DEC_DIGIT_RESERVOIR_MEM);

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

  // _disable is the switch to disable this synapse:
  bool _disable;

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
  double _lsm_weight_last;
  double _lsm_weight;
  double _lsm_weight_limit;

  int _D_lsm_c;
  int _D_lsm_weight;
  int _D_lsm_weight_limit;

  int _Unit;
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

  /* ONLY FOR PRINT PURPOSE */
  /* keep track of the weights and firing activity */
  std::vector<int> _t_pre_collection;
  std::vector<int> _t_post_collection;
  std::vector<int> _simulation_time;
#ifdef DIGITAL
  std::vector<int> _x_collection;
  std::vector<int> _y_collection;
  std::vector<int> _weights_collection;
#else
  std::vector<double> _x_collection;
  std::vector<double> _y_collection;
  std::vector<double> _weights_collection;
#endif

// look-up table for LTD and LTP, 
#ifdef DIGITAL
  // look up table for unsupervised STDP curve:
  std::vector<int> _TABLE_LTP;
  std::vector<int> _TABLE_LTD;
  // look up table for unsupervised STDP curve:
  std::vector<int> _TABLE_LTP_S;
  std::vector<int> _TABLE_LTD_S;
#else
  std::vector<double> _TABLE_LTP;
  std::vector<double> _TABLE_LTD;
  std::vector<double> _TABLE_LTP_S;
  std::vector<double> _TABLE_LTD_S;
#endif
    void _init_lookup_table(bool isSupv);
public:
  Synapse(Neuron*,Neuron*,double,bool,double,bool,bool); // only for non-digital liquid state machine
  Synapse(Neuron*,Neuron*,int,bool,int,bool,bool); // only for digital liquid state machine (including initializing synapse in liquid)

  void DisableStatus(bool disable){ _disable = disable; }
  bool DisableStatus(){ return _disable; }

  //* Get the _lsm_active of the synapse, which is the flag indicating whether or not the synapse is activated:
  bool GetActiveStatus(){return _lsm_active;}

  bool GetActiveSTDPStatus(){return _lsm_stdp_active;}

  //* Set the _lsm_active of the synapse.  
  void SetActiveStatus(bool status){_lsm_active = status;}

  //* Determine whether or not the synapse is the readout synapse:
  bool IsReadoutSyn();
  //* Determine whether or not the synapse is the input synapse:
  bool IsInputSyn();
  //* Determine whether or not the synapse is valid (connected to no deactivated neurons)
  bool IsValid();

  //* Get the synaptic weights:
  double Weight(){ return _lsm_weight;}
  int DWeight(){return _D_lsm_weight;}

  //* Set the synaptic weights:
  void Weight(int weight){_D_lsm_weight = weight;}
  void Weight(double weight){_lsm_weight = weight;}

  bool IsWeightZero(){
#ifdef DIGITAL
    return _D_lsm_weight == 0;
#else
    return _lsm_weight == 0;
#endif
  }

  // * Truncate the intermediate weights
  bool TruncIntermWeight();

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
  void LSMLearn(int iteration);
  void LSMActivate(Network * network, bool stdp_flag, bool train);
  void LSMActivateSTDPSyns(Network * network, const char * type);
  void LSMDeactivate(){ _lsm_active = false;}

  // determine whether or not this synapse is in the liquid?
  bool IsLiquidSyn(){ return _liquid;};

  void CheckPlasticWeightOutBound();
  void CheckReadoutWeightOutBound();
  void RemapReservoirWeight();

/* ONLY FOR STDP */

  void ExpDecay(double& var, const int time_c){var -= var/time_c;}
  void LSMUpdate(int t);
  void LSMSTDPLearn(int t);
  void LSMSTDPHardwareLearn(int t);
  void LSMSTDPSimpleLearn(int t);
  void LSMSTDPSupvLearn(int t, int iteration);
  void LSMSTDPSupvSimpleLearn(int t);
  void LSMSTDPSupvHardwareLearn(int t, int iteration);
  void PrintActivity(std::ofstream& f_out);
    
  void ExpDecay(int& var, const int time_c){
    if(var == 0) return;
#if NUM_DEC_DIGIT_RESERVOIR_MEM < 10
    var -= var/time_c;
#else
    var -= (var/time_c == 0) ? (var > 0 ? 1 : -1) : var/time_c; 
#endif 
  }

  void SetPreSpikeT(int t){
    _t_spike_pre = t; 
#ifdef _PRINT_SYN_ACT
    _t_pre_collection.push_back(t);
#endif
  }

  void SetPostSpikeT(int t){
    _t_spike_post = t;
#ifdef _PRINT_SYN_ACT
    _t_post_collection.push_back(t);
#endif
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

  //* read the look-up table and determine the delta
  template<class T>
  void LSMReadLUT(int t, T& delta_w_pos, T& delta_w_neg, bool isSupv){
#ifdef DIGITAL
    std::vector<int> & table_ltp = isSupv ? _TABLE_LTP_S : _TABLE_LTP;
    std::vector<int> & table_ltd = isSupv ? _TABLE_LTD_S : _TABLE_LTD;
#else
    std::vector<double> & table_ltp = isSupv ? _TABLE_LTP_S : _TABLE_LTP;
    std::vector<double> & table_ltd = isSupv ? _TABLE_LTD_S : _TABLE_LTD;
#endif


    if(t == _t_spike_post){  // LTP:
      assert(_t_spike_pre <= _t_spike_post);
      size_t ind = _t_spike_post - _t_spike_pre;
      if(ind < table_ltp.size()) // look-up table:
#ifdef DIGITAL
#if defined(STOCHASTIC_STDP) || defined(STOCHASTIC_STDP_SUPV)
	// I will consider the stochastic sdtp here for both all-pairing 
	// and nearest neighbor-pairing
         delta_w_pos = (table_ltp[ind]);
#else
         delta_w_pos = (table_ltp[ind])>>(LAMBDA_BIT);
#endif
#else
         delta_w_pos = table_ltp[ind];
#endif
    }

    if(t == _t_spike_pre){ // LTD:
      assert(_t_spike_post <= _t_spike_pre);
      size_t ind = _t_spike_pre - _t_spike_post;
      if(ind < table_ltd.size())
#ifdef DIGITAL
#if defined(STOCHASTIC_STDP) || defined(STOCHASTIC_STDP_SUPV)
	  delta_w_neg = (table_ltd[ind]);
#else
          delta_w_neg = (table_ltd[ind])>>(LAMBDA_BIT + ALPHA_BIT);
#endif
#else
	  delta_w_neg = (table_ltd[ind]);
#endif
    }
}

  // Calculate the LTP:
  // Pot/Polarization will dirve weights to w_limit, 
  // so delta_w_pos > 0 for w > 0 & F_pos > 0
  //    delta_w_pos < 0 for w < 0 & F_pos < 0
  template<class T>
  void CalcLTP(T & delta_w_pos, T F_pos){
#ifdef DIGITAL // the digital case should be stochastic!
#ifdef STOCHASTIC_STDP
#ifdef PAIR_BASED
    delta_w_pos = (F_pos*_D_x_j);
#elif NEAREST_NEIGHBOR
    delta_w_pos = (F_pos*_D_x_j);
#else
    delta_w_pos = (F_pos*_D_x_j*_D_y_i2_last);
#endif
#else
#ifdef PAIR_BASED
    delta_w_pos = (F_pos*_D_x_j)>>(LAMBDA_BIT);
#elif NEAREST_NEIGHBOR
    delta_w_pos = (F_pos*_D_x_j)>>(LAMBDA_BIT + NN_BIT_P);
#else
    delta_w_pos = (F_pos*_D_x_j*_D_y_i2_last)>>(LAMBDA_BIT + DAMPING_BIT);
#endif
#endif
#else
#ifdef PAIR_BASED
    delta_w_pos = (F_pos*_x_j)*LAMBDA;
#elif NEAREST_NEIGHBOR
    delta_w_pos = (F_pos*_x_j)*LAMBDA;
#else
    delta_w_pos = (F_pos*_x_j*_y_i2_last)*LAMBDA;
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
#ifdef STOCHASTIC_STDP
    delta_w_neg = -1*(F_neg*_D_y_i1);
#else
#ifdef PAIR_BASED
    delta_w_neg = -1*(F_neg*_D_y_i1)>>(LAMBDA_BIT + ALPHA_BIT);
#elif NEAREST_NEIGHBOR
    delta_w_neg = -1*(F_neg*_D_y_i1)>>(LAMBDA_BIT + ALPHA_BIT + NN_BIT_N);
#else
    delta_w_neg = -1*(F_neg*_D_y_i1)>>(LAMBDA_BIT + ALPHA_BIT);
#endif
#endif
#else
    delta_w_neg = -1*LAMBDA*ALPHA*(F_neg*_y_i1);
#endif
  }

  // Update the reservoir weights using STDP in a stochastic way:
  template<class T>
  void StochasticSTDPSupv(const T delta_w_pos, const T delta_w_neg, const T l1_norm, int iteration = 1){
      int iter = iteration + 1;
      if(iteration <= 50) iter = 1;
      else if(iteration <= 100) iter = 2;
      else if(iteration <= 150) iter = 4;
      else if(iteration <= 200) iter = 8;
      else if(iteration <= 250) iter = 16;
      else if(iteration <= 300) iter = 32;
      else iter = iteration/4; 

#ifdef DIGITAL
      const int c_dec_digit = IsReadoutSyn() ? NUM_DEC_DIGIT_READOUT_MEM : NUM_DEC_DIGIT_RESERVOIR_MEM;
      const int c_syn_bit_diff = IsReadoutSyn() ? NUM_BIT_SYN - NBT_STD_SYN : NUM_BIT_SYN_R - NBT_STD_SYN_R;
      const int temp1 = one<<(c_dec_digit + 14);
      const int temp2 = one<<(c_syn_bit_diff + 14); // learning rate can also be tuned here!
      T temp_delta = delta_w_pos + delta_w_neg;
      const int c_weight_omega = WEIGHT_OMEGA<<(c_syn_bit_diff);
      int regulization = l1_norm < c_weight_omega ? 0 : ((l1_norm - c_weight_omega)*_D_lsm_weight)/(1<<c_syn_bit_diff); // left shift cause overflow, so I use div here!

      if(_D_lsm_c > LSM_CAL_MID*unit){
	  if((_D_lsm_c < (LSM_CAL_MID+3)*unit) &&  temp_delta > 0)
#ifdef ADDITIVE_STDP
	      _D_lsm_weight += (rand()%temp1<(temp_delta*temp2*LAMBDA/iter))? 1 : 0;
#else
	      _D_lsm_weight += (rand()%temp1<(temp_delta*LSM_DELTA_POT/iter))? 1 : 0;
#endif 
      }else{
	  if((_D_lsm_c > (LSM_CAL_MID-3)*unit) && temp_delta < 0)
#ifdef ADDITIVE_STDP
	      _D_lsm_weight -= (rand()%temp1<(-1*temp_delta*temp2*LAMBDA/iter))? 1 : 0;
#else
              _D_lsm_weight -= (rand()%temp1<(-1*temp_delta*LSM_DELTA_DEP/iter))? 1 : 0;
#endif
      }
      _D_lsm_weight -= regulization;
#else
      T temp_delta = delta_w_pos + delta_w_neg;
      double regulization = l1_norm < WEIGHT_OMEGA ? 0 : GAMMA_REG*(l1_norm - WEIGHT_OMEGA)*_lsm_weight;
      if(_lsm_c > LSM_CAL_MID){
	  if(_lsm_c < (LSM_CAL_MID+3) && temp_delta > 0)
	      _lsm_weight += temp_delta/(1 + iter/ITER_SEARCH_CONV);
      }
      else{
	  if(_lsm_c > (LSM_CAL_MID-3) && temp_delta < 0)
	      _lsm_weight += temp_delta/(1 + iter/ITER_SEARCH_CONV);
      }
      _lsm_weight -= regulization;
#endif
  }
  
  // Update the reservoir weights using STDP (unsupvervised) in a stochastic way:
  template<class T>
  void StochasticSTDP(const T delta_w_pos, const T delta_w_neg){
#ifdef DIGITAL
      const int c_dec_digit = IsReadoutSyn() ? NUM_DEC_DIGIT_READOUT_MEM : NUM_DEC_DIGIT_RESERVOIR_MEM;
      const int c_syn_bit_diff = IsReadoutSyn() ? NUM_BIT_SYN - NBT_STD_SYN : NUM_BIT_SYN_R - NBT_STD_SYN_R;
      const int temp1 = one<<(c_dec_digit + 14);
      const int temp2 = one<<(c_syn_bit_diff + 8);
      T temp_delta = delta_w_pos + delta_w_neg;
      if(_D_lsm_c > LSM_CAL_MID*unit){
	  if((_D_lsm_c < (LSM_CAL_MID+3)*unit) &&  temp_delta > 0)
#ifdef ADDITIVE_STDP
	      _D_lsm_weight += (rand()%temp1<(temp_delta*temp2*LAMBDA))? 1 : 0;
#else
	      _D_lsm_weight += (rand()%temp1<(temp_delta*LSM_DELTA_POT))? 1 : 0;
#endif 
      }else{
	  if((_D_lsm_c > (LSM_CAL_MID-3)*unit) && temp_delta < 0)
#ifdef ADDITIVE_STDP
	      _D_lsm_weight -= (rand()%temp1<(-1*temp_delta*temp2*LAMBDA))? 1 : 0;
#else
              _D_lsm_weight -= (rand()%temp1<(-1*temp_delta*LSM_DELTA_DEP))? 1 : 0;
#endif
      }
#else
      T temp_delta = delta_w_pos + delta_w_neg;
      if(_lsm_c > LSM_CAL_MID){
	  if(_lsm_c < (LSM_CAL_MID+3) && temp_delta > 0)
	      _lsm_weight += temp_delta;
      }
      else{
	  if(_lsm_c > (LSM_CAL_MID-3) && temp_delta < 0)
	      _lsm_weight += temp_delta;
      }
#endif
  }


  // PrintToFile: Given a vector, print the information 
  //              in the vector to file:
  template<class T>
  void PrintToFile(std::vector<T>& collection, std::ofstream& f_out){
    for(typename std::vector<T>::iterator it = collection.begin(); it != collection.end(); ++it)
	f_out<<std::setprecision(8)<<(*it)<<std::endl;
  }
};


#endif

