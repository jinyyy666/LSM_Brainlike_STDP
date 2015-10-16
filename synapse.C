#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

//#define _DEBUG_SYN_UPDATE
//#define _DEBUG_SYN_LEARN

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
  Neuron * post = this->PostNeuron();
  char * name_post = post->Name();
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
  char * name_pre = this->PreNeuron()->Name();

  if((name_pre[0] == 'i')&&(name_pre[5] == '_'))
    return true;
  else
    return false;
}

void Synapse::LSMLearn(int iteration){
  assert((_fixed==false)&&(_lsm_active==true));
  int iter;

  if(iteration <= 10) iter = 1;
  else if(iteration <= 20) iter = 2;
  else if(iteration <= 30) iter = 4;
  else if(iteration <= 40) iter = 5;
  else if(iteration <= 50) iter = 8;
  else if(iteration <= 60) iter = 10;
  else iter = iteration; 
//  if(iteration < 1) iter = 1;
//  else iter = iteration;

//else iter = iteration;
  const int temp1 = one<<18;
  const int temp2 = one<<(18+NUM_BIT_SYN-NBT_STD_SYN);

#ifdef DIGITAL
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
    if((_lsm_c < LSM_CAL_MID+3)&&(_post->LSMGetVMemPre() > LSM_V_THRESH*0.2)){
      _lsm_weight += LSM_DELTA_POT;
//      cout<<"!!!"<<endl;
    }
  }else{
    if((_lsm_c > LSM_CAL_MID-3)&&(_post->LSMGetVMemPre() < LSM_V_THRESH*0.2)){
      _lsm_weight -= LSM_DELTA_DEP;
//      cout<<"***"<<endl;
    }
  }
/*
  if(_lsm_c > 2.5){
//    if((_lsm_c < 12)&&(_post->LSMGetVMemPre() > 0.01)) _lsm_weight += 0.001;
    if((_lsm_c < 12)&&(_post->LSMGetVMemPre() > LSM_V_THRESH*0.7)) _lsm_weight += 0.003;
//    if(_lsm_c < 12) _lsm_weight += 0.001;
  }else{
    if((_lsm_c > 1.5)&&(_post->LSMGetVMemPre() < LSM_V_THRESH*0.15)) _lsm_weight -= 0.0003;
  }
*/
/*
  if((_lsm_c < 16)&&(_lsm_c > 12)){
//cout<<_pre->Name()<<": "<<_t_spike_pre<<"\t"<<_post->Name()<<": "<<_t_spike_post<<endl;
    if(_t_spike_post > _t_spike_pre){
      _lsm_weight += 0.004*exp((_t_spike_pre-_t_spike_post)/5);
//      cout<<"potentiation!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
    }else if(_t_spike_pre > _t_spike_post){
      _lsm_weight -= 0.001*exp((_t_spike_post-_t_spike_pre)/20);
//      cout<<"depression~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl;
    }
  }
*/
  if(_lsm_weight > _lsm_weight_limit) _lsm_weight = _lsm_weight_limit;
  if(_lsm_weight < -_lsm_weight_limit) _lsm_weight = -_lsm_weight_limit;
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

void Synapse::Print(){
  cout<<Weight();
}

void Synapse::Print(FILE * fp){
  fprintf(fp,"%d",Weight());
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
//  cout<<_pre->Name()<<"\t"<<_post->Name()<<"\tstate1 = "<<_lsm_state1<<"\tstate2 = "<<_lsm_state2<<"\ttau1 = "<<_lsm_tau1<<"\ttau2 = "<<_lsm_tau2<<"\t_lsm_weight = "<<_lsm_weight<<endl;
  return (_lsm_state1-_lsm_state2)/(_lsm_tau1-_lsm_tau2)*_lsm_weight;
#ifdef DIGITAL
  assert(0);
#endif
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
  if(t < 0 || _t_spike_pre == _t_spike_post)
    return;

  _D_y_i2_last = _D_y_i2;
  // update the trace x:
  if(_D_x_j > 0)
    _D_x_j -= (_D_x_j/TAU_X_TRACE == 0) ? 1 : _D_x_j/TAU_X_TRACE;

  if(t == _t_spike_pre)
#ifdef NEAREST_NEIGHBOR
    _D_x_j = UNIT_DELTA;
#else
    _D_x_j += UNIT_DELTA;
#endif

  // update the trace y_i1 and y_i2:
  _D_y_i2_last = _D_y_i2;	// need to keep track of the y_i2 before the update. y_i2_last is used in STDP learning
  
  if(_D_y_i1 > 0)
    _D_y_i1 -= _y_i1/TAU_Y1_TRACE == 0 ? 1 : _D_y_i1/TAU_Y1_TRACE;
  if(_y_i2 > 0)
    _D_y_i2 -= _y_i2/TAU_Y2_TRACE == 0 ? 1 : _D_y_i2/TAU_Y2_TRACE;
  
  if(t == _t_spike_post){
#ifdef NEAREST_NEIGHBOR
    _D_y_i1 = UNIT_DELTA;
    _D_y_i2 = UNIT_DELTA;
#else
    _D_y_i1 += UNIT_DELTA;
    _D_y_i2 += UNIT_DELTA;
#endif
  }
   
#ifdef _DEBUG_SYN_UPDATE
  //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "reservoir_9") && _x_j > 0)    
  if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_0"))
    cout<<"t: "<<t<<"\tx_j: "<<_D_x_j<<"\ty_i1: "<<_D_y_i1<<"\ty_i2: "<<_D_y_i2<<endl;
#endif
}

//* this function is the implementation of the STDP learning rule, please find more details in the paper:
//* "Phenomenological models of synaptic plasticity based on spike timing" 
void Synapse::LSMLiquidLearn(int t){
  assert(_lsm_stdp_active == true);
  _lsm_stdp_active = false;
  
  // Remember that the delay of deliverying the fired spike is 1
  //t--;
  if( t < 0 || (t != _t_spike_pre && t != _t_spike_post) || (_t_spike_pre == _t_spike_post))
    return; 

  // you need to make sure that this synapse is active!
  // F_pos/F_neg is the STDP function for LTP and LTD:
  // LAMBDA is the learning rate here

#ifdef ADDITIVE_STDP
  int F_pos = A_POS;
  int F_neg = A_NEG;
#else
  int F_pos = _D_lsm_weight > 0 ? (_D_lsm_weight_limit - _D_lsm_weight) 
                                : (_D_lsm_weight_limit +  _D_lsm_weight);
  int F_neg = _D_lsm_weight;
#endif
  int delta_w_pos = 0, delta_w_neg = 0; // delta weight resulted from LTP and LTD
  // 1. LTP (Long-Time Potientation i.e. w+):
  if(t == _t_spike_post){
    assert(_t_spike_pre < _t_spike_post);
#ifdef PAIR_BASED
    delta_w_pos = (F_pos*_x_j);//>>(LAMBDA_BIT);
#elif NEAREST_NEIGHBOR
    delta_w_pos = (F_pos*_x_j);//>>(LAMBDA_BIT + NN_BIT_P);
#else
    delta_w_pos = (F_pos*_x_j*_y_i2_last);//>>(LAMBDA_BIT + DAMPING_BIT);
#endif

#ifdef _DEBUG_SYN_LEARN
    //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "reservoir_9"))
    if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_0"))
      cout<<"post firing time: "<<t<<"\tF_pos: "<<F_pos<<"\tx_j: "<<_x_j<<"\ty_i2_last: "<<_y_i2_last<<"\tdelta_w_pos: "<<delta_w_pos<<endl;
#endif
  }

  // 2. LTD (Long-Time Depression i.e. w-):
  if(t == _t_spike_pre){
    assert(_t_spike_pre > _t_spike_post);
#ifdef PAIR_BASED
    delta_w_neg = _D_lsm_weight >= 0 ? -1*((F_neg*_y_i1))
				        : (F_neg*_y_i1);
#elif NEAREST_NEIGHBOR
     delta_w_neg = _D_lsm_weight >= 0 ? -1*((F_neg*_y_i1)) 
					: (F_neg*_y_i1);
#else
    delta_w_neg = _D_lsm_weight >= 0 ? -1*((F_neg*_y_i1))
                                     : (F_neg*_y_i1));
#endif

#ifdef _DEBUG_SYN_LEARN
    //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "reservoir_9"))
    if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_0"))
      cout<<"pre firing time: "<<t<<"\tpost spike @"<<_t_spike_post<<"\tF_neg: "<<F_neg<<"\ty_i1: "<<_y_i1<<"\tdelta_w_neg: "<<delta_w_neg<<endl;
#endif
  }

  // 3. Update the weight:
#ifdef _DEBUG_SYN_LEARN
  //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "reservoir_9"))
  if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_0"))
    cout<<(delta_w_pos + delta_w_neg >= 0 ? "Weight increase: " : "Weight decrease: ")<<delta_w_pos + delta_w_neg<<" Calicium of the post: "<<_post->DLSMGetCalciumPre()<<endl;
#endif

#ifdef STOCHASTIC_STDP 
  //Here I adopt the abstract learning rule:
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
  _D_lsm_weight += delta_w_pos + delta_w_neg;
#endif

  // 4. Check whether or not the weight is out of bound:
  CheckReservoirWeightOutBound();

}

//* this function is to check whether or not the synaptic weights in the reservoir is out of bound:
void Synapse::CheckReservoirWeightOutBound(){
  // for excitatory synapses:
  if(_excitatory){
    if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit;
    if(_D_lsm_weight < 0) _D_lsm_weight = 0;
  }
  else{
    if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
    if(_D_lsm_weight > 0) _D_lsm_weight = 0;
  }
}

void Synapse::CheckReadoutWeightOutBound(){
  if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit;
  if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
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
void Synapse::LSMActivate(Network * network, bool stdp_flag){
    assert(_lsm_active == false);
    // 1. Add the synapses for firing processing
  
    network->LSMAddActiveSyn(this);
  
    // 2. For the readout synapses, if we are using stdp in reservoir, just ignore the learning in the readout: 
    if(stdp_flag == false && _fixed == false){
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
