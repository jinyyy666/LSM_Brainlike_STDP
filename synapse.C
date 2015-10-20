#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#define _DEBUG_SYN_UPDATE
#define _DEBUG_SYN_LEARN

using namespace std;

extern int COUNTER_LEARN;

/** unit system defined for digital lsm **/
extern const int one;
extern const int unit;
// the spike strength in STDP model:
const int UNIT_DELTA = 1<<(NUM_DEC_DIGIT);


Synapse::Synapse(Neuron * pre, Neuron * post, double lsm_weight, bool fixed, double lsm_weight_limit, bool excitatory, bool liquid):
_pre(pre),
_post(post),
_excitatory(excitatory),
_fixed(fixed),
_liquid(liquid),
_lsm_active(false),
_lsm_stdp_active(false),
_mem_pos(0),
_mem_neg(0),
_lsm_tau1(4),
_lsm_state1(0),
_lsm_state2(0),
_lsm_state(0),
_lsm_spike(0),
_D_lsm_spike(0),
_lsm_delay(0),
_lsm_c(0),
_lsm_weight(lsm_weight),
_lsm_weight_limit(fabs(lsm_weight_limit)),
_D_lsm_c(0),
_D_lsm_weight(0),
_D_lsm_weight_limit(0),
_t_spike_pre(-1e8),
_t_spike_post(-1e8),
_t_last_pre(0),
_t_last_post(0),
_x_j(0),
_y_i1(0),
_y_i2(0),
_y_i2_last(0),
_D_x_j(0),
_D_y_i1(0),
_D_y_i2(0),
_D_y_i2_last(0)
{
#ifdef DIGITAL
  assert(0);
#endif

  if(lsm_weight > 0) _lsm_tau2 = LSM_T_SYNE;
  else _lsm_tau2 = LSM_T_SYNI;

  cout<<pre->Name()<<"\t"<<post->Name()<<"\t"<<excitatory<<"\t"<<_lsm_weight<<endl;

}


Synapse::Synapse(Neuron * pre, Neuron * post, int D_lsm_weight, bool fixed, int D_lsm_weight_limit, bool excitatory, bool liquid):
_pre(pre),
_post(post),
_excitatory(excitatory),
_fixed(fixed),
_liquid(liquid),
_lsm_active(false),
_lsm_stdp_active(false),
_mem_pos(0),
_mem_neg(0),
_lsm_tau1(4),
_lsm_state1(0),
_lsm_state2(0),
_lsm_state(0),
_lsm_spike(0),
_D_lsm_spike(0),
_lsm_delay(0),
_lsm_c(0),
_lsm_weight(0),
_lsm_weight_limit(0),
_D_lsm_c(0),
_D_lsm_weight(D_lsm_weight),
_D_lsm_weight_limit(abs(D_lsm_weight_limit)),
_t_spike_pre(-1e8),
_t_spike_post(-1e8),
_t_last_pre(0),
_t_last_post(0),
_x_j(0),
_y_i1(0),
_y_i2(0),
_y_i2_last(0),
_D_x_j(0),
_D_y_i1(0),
_D_y_i2(0),
_D_y_i2_last(0)
{
#ifndef DIGITAL
  assert(0);
#endif

  if(D_lsm_weight > 0) _lsm_tau2 = LSM_T_SYNE;
  else _lsm_tau2 = LSM_T_SYNI;

  char * pre_name;
  pre_name = pre->Name();
  assert(pre_name != NULL);
  if((_liquid == false)&&(pre_name[0] != 'i')){
#if NUM_BIT_SYN > NBT_STD_SYN
    _D_lsm_weight = (_D_lsm_weight<<(NUM_BIT_SYN-NBT_STD_SYN));
    _D_lsm_weight_limit = (_D_lsm_weight_limit<<(NUM_BIT_SYN-NBT_STD_SYN));
#elif NUM_BIT_SYN < NBT_STD_SYN
    _D_lsm_weight = (_D_lsm_weight>>(NBT_STD_SYN-NUM_BIT_SYN));
    _D_lsm_weight_limit = (_D_lsm_weight_limit>>(NBT_STD_SYN-NUM_BIT_SYN));
#endif
  }
  else if(pre_name[0] == 'i'){
    _D_lsm_weight = (_D_lsm_weight<<(NUM_BIT_SYN - NBT_STD_SYN));
    _D_lsm_weight_limit = (_D_lsm_weight<<(NUM_BIT_SYN - NBT_STD_SYN));
  }
  else{
  assert(_liquid == true);
     _D_lsm_weight = (_D_lsm_weight<<(NUM_BIT_SYN-NBT_STD_SYN));
     // just to limit the total change range of the reservoir synapses to avoid the weights being too high:
     _D_lsm_weight_limit = (_D_lsm_weight_limit<<(NUM_BIT_SYN-NBT_STD_SYN));
  }
  
  cout<<pre->Name()<<"\t"<<post->Name()<<"\t"<<excitatory<<"\t"<<_D_lsm_weight<<endl;
}



/*
void Synapse::SetPreSpikeT(double t){
//  _mem_pos = _mem_pos*exp((_t_spike_pre - t)/160) + 1.5;
//  _mem_neg = _mem_neg*exp((_t_spike_pre - t)/40) - 3;
  _t_last_pre = _t_spike_pre;
  _t_spike_pre = t;
}

void Synapse::SetPostSpikeT(double t){
//  _t_last_post = _t_spike_post;
  _t_spike_post = t;
}*/

bool Synapse::IsReadoutSyn(){
  char * name_post = _post->Name();
  // if the post neuron is the readout neuron:
  if(name_post[0] == 'o' && name_post[6] == '_'){
    assert(_fixed == false);
    return true;
  }
  else{
    return false;
  }
}

bool Synapse::IsInputSyn(){
  char * name_pre = _pre->Name();

  if((name_pre[0] == 'i')&&(name_pre[5] == '_'))
    return true;
  else
    return false;
}


void Synapse::LSMLearn(int iteration){
  assert((_fixed==false)&&(_lsm_active==true));
  _lsm_active = false;
#ifdef DIGITAL
  int iter;

  if(iteration <= 10) iter = 1;
  else if(iteration <= 20) iter = 2;
  else if(iteration <= 30) iter = 4;
  else if(iteration <= 40) iter = 5;
  else if(iteration <= 50) iter = 8;
  else if(iteration <= 60) iter = 10;
  else iter = iteration; 

  const int temp1 = one<<18;
  const int temp2 = one<<(18+NUM_BIT_SYN-NBT_STD_SYN);

  _D_lsm_c = _post->DLSMGetCalciumPre();
  if(_D_lsm_c > LSM_CAL_MID*unit){
    if((_D_lsm_c < (LSM_CAL_MID+3)*unit)){
      _D_lsm_weight += (rand()%temp1<(LSM_DELTA_POT*temp2/(iter)))?1:0;
    }
  }else{
    if((_D_lsm_c > (LSM_CAL_MID-3)*unit)){
      _D_lsm_weight -= (rand()%temp1<(LSM_DELTA_DEP*temp2/(iter)))?1:0;
    }
  }

  // modify the weight if it is out of bound: 
  CheckReadoutWeightOutBound();
#else
  _lsm_c = _post->GetCalciumPre();
  if(_lsm_c > LSM_CAL_MID){
  if((_lsm_c < LSM_CAL_MID+3)){ // &&(_post->LSMGetVMemPre() > LSM_V_THRESH*0.2)){
      _lsm_weight += LSM_DELTA_POT;
    }
  }else{
  if((_lsm_c > LSM_CAL_MID-3)){ //&&(_post->LSMGetVMemPre() < LSM_V_THRESH*0.2)){
      _lsm_weight -= LSM_DELTA_DEP;
    }
  }

  CheckReadoutWeightOutBound();
//if(_lsm_c > 0) cout<<_lsm_c<<"\t"<<_pre->Name()<<"\t"<<_post->Name()<<"\t"<<_lsm_weight<<endl;
#endif
}


Neuron * Synapse::PreNeuron(){
  return _pre;
}

Neuron * Synapse::PostNeuron(){
  return _post;
}

bool Synapse::Excitatory(){
  return _excitatory;
}

bool Synapse::Fixed(){
  return _fixed;
}


void Synapse::LSMPrint(FILE * fp){
#ifdef DIGITAL
  fprintf(fp,"%d",_D_lsm_weight);
#else
  fprintf(fp,"%f",_lsm_weight);
#endif
}

void Synapse::LSMPrintSyns(FILE * fp){
//  if(_post->GetStatus() == true) return;
  if(fp != NULL) fprintf(fp,"%s\t%s\n",_pre->Name(),_post->Name());
}

// argument can only be 0 or 1
void Synapse::LSMPreSpike(int delay){
  if(delay <= 0){
#ifdef DIGITAL
    _D_lsm_spike = 1;
#else
    _lsm_spike = 1;
#endif
  }
  else{
//cout<<_pre->Name()<<" fired!"<<endl;
    _lsm_delay = delay;
  }
}

double Synapse::LSMCurrent(){
#ifdef DIGITAL
  assert(0);
#endif
  return (_lsm_state1-_lsm_state2)/(_lsm_tau1-_lsm_tau2)*_lsm_weight;
}


double Synapse::LSMFirstOrderCurrent(){
#ifdef DIGITAL
  assert(0);
#endif
  return _lsm_state*_lsm_weight;
}

//* this function is used to simulate the fired spike:
// the fired spike is simply passed down to the post-synaptic neuron with delay = 1
void Synapse::LSMNextTimeStep(){
  if(_lsm_delay > 0){
    _lsm_delay--;
    if(_lsm_delay == 0){
#ifdef DIGITAL
      _D_lsm_spike = 1;
#else
      _lsm_spike = 1;
#endif
    }
  }

}

//* this function is used to update the trace x and trace y for STDP:
void Synapse::LSMUpdate(int t){
  if(t < 0)
    return;

#ifdef _DEBUG_SYN_UPDATE
  char * pre_name = _pre->Name();
  char * post_name = _post->Name();
  assert(pre_name[0] == 'r' && post_name[0] == 'r');
#endif


#ifdef DIGITAL 
  _D_y_i2_last = _D_y_i2;   // need to keep track of the y_i2 before the update. y_i2_last is used in STDP learning
#else
  _y_i2_last = _y_i2;
#endif

#ifdef DIGITAL
  // update the trace x:
  ExpDecay(_D_x_j, TAU_X_TRACE);
#else
  ExpDecay(_x_j, TAU_X_TRACE);
#endif

  if(t == _t_spike_pre){
#ifdef DIGITAL
#ifdef NEAREST_NEIGHBOR
    _D_x_j = UNIT_DELTA;
#else
    _D_x_j += UNIT_DELTA;
#endif
#else
#ifdef NEAREST_NEIGHBOR
    _x_j = one;
#else
    _x_j += one;
#endif
#endif
  }

  // update the trace y_i1 and y_i2:
#ifdef DIGITAL	
  ExpDecay(_D_y_i1, TAU_Y1_TRACE);
  ExpDecay(_D_y_i2, TAU_Y2_TRACE);
#else
  ExpDecay(_y_i1, TAU_Y1_TRACE);
  ExpDecay(_y_i2, TAU_Y2_TRACE);
#endif

  if(t == _t_spike_post){
#ifdef DIGITAL
#ifdef NEAREST_NEIGHBOR
    _D_y_i1 = UNIT_DELTA;
    _D_y_i2 = UNIT_DELTA;
#else
    _D_y_i1 += UNIT_DELTA;
    _D_y_i2 += UNIT_DELTA;
#endif
#else
#ifdef NEAREST_NEIGHBOR
    _y_i1 = one;
    _y_i2 = one;
#else
    _y_i1 += one;
    _y_i2 += one;
#endif
#endif
  }
   
#ifdef _DEBUG_SYN_UPDATE 
  if(!strcmp(_pre->Name(), "reservoir_19") && !strcmp(_post->Name(), "reservoir_0"))
#ifdef DIGITAL
    cout<<"t: "<<t<<"\tx_j: "<<_D_x_j<<"\ty_i1: "<<_D_y_i1<<"\ty_i2: "<<_D_y_i2<<endl;
#else
    cout<<"t: "<<t<<"\tx_j: "<<_x_j<<"\ty_i1: "<<_y_i1<<"\ty_i2: "<<_y_i2<<endl;
#endif
#endif

}


//* this function is the implementation of the STDP learning rule, please find more details in the paper:
//* "Phenomenological models of synaptic plasticity based on spike timing" 
void Synapse::LSMLiquidLearn(int t){
  // you need to make sure that this synapse is active!
  assert(_lsm_stdp_active == true);
  _lsm_stdp_active = false;

#ifdef _DEBUG_SYN_LEARN
  char * pre_name = _pre->Name();
  char * post_name = _post->Name();
  assert(pre_name[0] == 'r' && post_name[0] == 'r');
#endif

  // Remember that the delay of deliverying the fired spike is 1
  //t--;
  if( t < 0 || (t != _t_spike_pre && t != _t_spike_post)) // || (_t_spike_pre == _t_spike_post))
    return; 
  /*
  if(_t_spike_pre == _t_spike_post)
    cout<<"Time: "<<t<<"\t"<<_pre->Name()<<"\t"<<_post->Name()<<endl;
  */
  
  // F_pos/F_neg is the STDP function for LTP and LTD:
  // LAMBDA is the learning rate here

#ifdef DIGITAL
#ifdef ADDITIVE_STDP
  int F_pos = _D_lsm_weight >= 0 ? D_A_POS : -1*D_A_POS;
  int F_neg = _D_lsm_weight >= 0 ? D_A_NEG : -1*D_A_NEG;
#else
  int F_pos = _D_lsm_weight >= 0 ? (_D_lsm_weight_limit - _D_lsm_weight) 
                                : (-_D_lsm_weight_limit -  _D_lsm_weight); // pos->polarized!
  int F_neg = _D_lsm_weight;
#endif
#else
#ifdef ADDITIVE_STDP
  double F_pos = _lsm_weight >= 0 ? A_POS : -1*A_POS;
  double F_neg = _lsm_weight >= 0 ? A_NEG : -1*A_NEG;
#else
  double F_pos = _lsm_weight >= 0 ? (_lsm_weight_limit - _lsm_weight) 
                                : (-_lsm_weight_limit -  _lsm_weight);
  double F_neg = _lsm_weight;
#endif
#endif
  
#ifdef DIGITAL
  int delta_w_pos = 0, delta_w_neg = 0; // delta weight resulted from LTP and LTD
#else
  double delta_w_pos = 0, delta_w_neg = 0;
#endif

  // 1. LTP (Long-Time Potientation i.e. w+):
  if(t == _t_spike_post){
    assert(_t_spike_pre <= _t_spike_post);
    CalcLTP(delta_w_pos, F_pos);

#ifdef _DEBUG_SYN_LEARN
    if(!strcmp(_pre->Name(), "reservoir_19") && !strcmp(_post->Name(), "reservoir_0"))
#ifdef DIGITAL
      cout<<"post firing time: "<<t<<"\tpre spike @"<<_t_spike_pre<<"\tF_pos: "<<D_F_pos<<"\tx_j: "<<_D_x_j<<"\ty_i2_last: "<<_D_y_i2_last<<"\tdelta_w_pos: "<<delta_w_pos<<endl;
#else
      cout<<"post firing time: "<<t<<"\tpre spike @"<<_t_spike_pre<<"\tF_pos: "<<F_pos<<"\tx_j: "<<_x_j<<"\ty_i2_last: "<<_y_i2_last<<"\tdelta_w_pos: "<<delta_w_pos<<endl;
#endif
#endif
  }

  // 2. LTD (Long-Time Depression i.e. w-):
  if(t == _t_spike_pre){
    assert(_t_spike_post <= _t_spike_pre);
    CalcLTD(delta_w_neg, F_neg);

#ifdef _DEBUG_SYN_LEARN
    if(!strcmp(_pre->Name(), "reservoir_19") && !strcmp(_post->Name(), "reservoir_0"))
#ifdef DIGITAL
      cout<<"pre firing time: "<<t<<"\tpost spike @"<<_t_spike_post<<"\tF_neg: "<<D_F_neg<<"\ty_i1: "<<_D_y_i1<<"\tdelta_w_neg: "<<delta_w_neg<<endl;
#else
      cout<<"pre firing time: "<<t<<"\tpost spike @"<<_t_spike_post<<"\tF_neg: "<<F_neg<<"\ty_i1: "<<_y_i1<<"\tdelta_w_neg: "<<delta_w_neg<<endl;
#endif
#endif
  }

  // 3. Update the weight:
#ifdef STOCHASTIC_STDP 
  //Here I adopt the abstract learning rule:
#ifdef DIGITAL
  _D_lsm_c = _post->DLSMGetCalciumPre();
  const int temp1 = one<<21;
  const int temp2 = one<<(18+NUM_BIT_SYN-NBT_STD_SYN);
  int temp_delta = delta_w_pos + delta_w_neg;
  if(_D_lsm_c > LSM_CAL_MID*unit){
    if((_D_lsm_c < (LSM_CAL_MID+3)*unit) &&  temp_delta > 0){
#ifdef ADDITIVE_STDP
      _D_lsm_weight += (rand()%temp1<(temp_delta))?1:0;
#else
      _D_lsm_weight += (rand()%temp1<(temp_delta*LSM_DELTA_POT))?1:0;
#endif
    }
  }else{
    if((_D_lsm_c > (LSM_CAL_MID- 3)*unit) && temp_delta < 0){
#ifdef ADDITIVE_STDP
      _D_lsm_weight -= (rand()%temp1<(-1*temp_delta))?1:0;
#else
      _D_lsm_weight -= (rand()%temp1<(-1*temp_delta*LSM_DELTA_DEP))?1:0;
#endif
    }
  }
#else
  _lsm_c = _post->GetCalciumPre();
  double temp_delta = delta_w_pos + delta_w_neg;
  if(_lsm_c > LSM_CAL_MID){
    if(_lsm_c < (LSM_CAL_MID+3) && temp_delta > 0){
      _lsm_weight += temp_delta;
    }
  }
  else{
      if(_lsm_c > (LSM_CAL_MID - 3) && temp_delta < 0)
        _lsm_weight += temp_delta;
  }
#endif
#else    // for non-stochastic update
#ifdef  DIGITAL
  _D_lsm_weight += delta_w_pos + delta_w_neg;
#else
  _lsm_weight += delta_w_pos + delta_w_neg;
#endif
#endif

#ifdef _DEBUG_SYN_LEARN
  if(!strcmp(_pre->Name(), "reservoir_19") && !strcmp(_post->Name(), "reservoir_0"))
#ifdef DIGITAL
    cout<<(delta_w_pos + delta_w_neg >= 0 ? "Weight increase: " : "Weight decrease: ")<<delta_w_pos + delta_w_neg<<" Calicium of the post: "<<_post->DLSMGetCalciumPre()<<endl;
#else
    cout<<(delta_w_pos + delta_w_neg >= 0 ? "Weight increase: " : "Weight decrease: ")<<delta_w_pos + delta_w_neg<<" Calicium of the post: "<<_post->GetCalciumPre()<<endl;
#endif
#endif


  // 4. Check whether or not the weight is out of bound:
  CheckReservoirWeightOutBound();

}

//* this function is to check whether or not the synaptic weights in the reservoir is out of bound:
void Synapse::CheckReservoirWeightOutBound(){
#ifdef DIGITAL
  // for excitatory synapses:
  if(_excitatory){
    if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit;
    if(_D_lsm_weight < 0) _D_lsm_weight = 0;
  }
  else{
    if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
    if(_D_lsm_weight > 0) _D_lsm_weight = 0;
  }
#else
  if(_excitatory){
    if(_lsm_weight >= _lsm_weight_limit) _lsm_weight = _lsm_weight_limit;
    if(_lsm_weight < 0) _lsm_weight = 0;
  }
  else{
    if(_lsm_weight < -_lsm_weight_limit) _lsm_weight = -_lsm_weight_limit;
    if(_lsm_weight > 0) _lsm_weight = 0;
#endif
  }
}

void Synapse::CheckReadoutWeightOutBound(){
#ifdef DIGITAL
  if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit;
  if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
#else
  if(_lsm_weight >= _lsm_weight_limit) _lsm_weight = _lsm_weight_limit;
  if(_lsm_weight < -_lsm_weight_limit) _lsm_weight = -_lsm_weight_limit;
#endif
}

void Synapse::LSMClear(){
  _lsm_active = false;
  _lsm_stdp_active = false;

  _mem_pos = 0;
  _mem_neg = 0;    

  _lsm_state1 = 0;
  _lsm_state2 = 0;
  _lsm_state = 0;
  _lsm_spike = 0;
  _D_lsm_spike = 0;
  _lsm_delay = 0;
  _lsm_c = 0;
  _D_lsm_c = 0;


  _t_spike_pre = -1e8;
  _t_spike_post = -1e8;
  _t_last_pre = 0;
  _t_last_post = 0;
  

  _x_j = 0;
  _y_i1 = 0;
  _y_i2 = 0;
  _y_i2_last = 0;

  _D_x_j = 0,
  _D_y_i1 = 0,
  _D_y_i2 = 0,
  _D_y_i2_last = 0;
}

void Synapse::LSMClearLearningSynWeights(){
  if(_fixed == true) return;
  _D_lsm_weight = 0;
  _lsm_weight = 0;
}

//**  Add the active firing synapses (reservoir/readout synapses) into the network
//**  Add the active readout synapses into the network
 void Synapse::LSMActivate(Network * network, bool stdp_flag, bool train){
    if(_lsm_active)
      cout<<_pre->Name()<<"\t"<<_post->Name()<<endl;

    assert(_lsm_active == false);
    // 1. Add the synapses for firing processing
  
    network->LSMAddActiveSyn(this);
  
    // 2. For the readout synapses, if we are using stdp in reservoir, just ignore the learning in the readout:
    //    Make sure that we are going to train this readout synapse:
    if(stdp_flag == false && _fixed == false && train == true){
      network->LSMAddActiveLearnSyn(this);
      // 3. Mark the readout synapse active state:
      _lsm_active = true;
    }
}

//** Activate the reservoir synapses to be trained by STDP
void Synapse::LSMActivateReservoirSyns(Network * network){
  assert(_lsm_stdp_active == false && !IsReadoutSyn() && !IsInputSyn());

  /** only train the excitatory reservoir synapses **/
  if(_excitatory == true)
    network->LSMAddReservoirActiveSyn(this);

  _lsm_stdp_active = true;
}


void Synapse::LSMDeactivate(){
  _lsm_active = false;
}

bool Synapse::LSMGetLiquid(){
  return _liquid;
}
