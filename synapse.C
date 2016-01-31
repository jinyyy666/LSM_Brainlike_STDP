#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>


//#define _DEBUG_SYN_UPDATE
//#define _DEBUG_SYN_LEARN
//#define _DEBUG_LOOKUP_TABLE

using namespace std;

extern int COUNTER_LEARN;

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
  assert(pre && post);

  if(lsm_weight > 0) _lsm_tau2 = LSM_T_SYNE;
  else _lsm_tau2 = LSM_T_SYNI;
  
  cout<<pre->Name()<<"\t"<<post->Name()<<"\t"<<excitatory<<"\t"<<post->IsExcitatory()<<"\t"<<_lsm_weight<<"\t"<<_lsm_weight_limit<<endl;

  // initialize the look-up table:
  _init_lookup_table();
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
  assert(pre && post);

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
    _D_lsm_weight = (_D_lsm_weight<<(NUM_BIT_SYN_R - NBT_STD_SYN_R));
    _D_lsm_weight = _D_lsm_weight/4;
    _D_lsm_weight_limit = (_D_lsm_weight<<(NUM_BIT_SYN_R - NBT_STD_SYN_R));
    _D_lsm_weight_limit = _D_lsm_weight_limit/4;
  }
  else{
     assert(_liquid == true);
#if NUM_BIT_SYN_R > NBT_STD_SYN_R
     _D_lsm_weight = (_D_lsm_weight<<(NUM_BIT_SYN_R-NBT_STD_SYN_R));
     _D_lsm_weight_limit = (_D_lsm_weight_limit<<(NUM_BIT_SYN_R-NBT_STD_SYN_R));
#else
     _D_lsm_weight = (_D_lsm_weight>>(NBT_STD_SYN_R - NUM_BIT_SYN_R));
     _D_lsm_weight_limit = (_D_lsm_weight_limit>>(NBT_STD_SYN_R - NUM_BIT_SYN_R));
#endif
  }
  
  cout<<pre->Name()<<"\t"<<post->Name()<<"\t"<<excitatory<<"\t"<<post->IsExcitatory()<<"\t"<<_D_lsm_weight<<endl;

  // initialize the look-up table:
  _init_lookup_table();
}


void Synapse::_init_lookup_table(){
#ifdef _DEBUG_LOOKUP_TABLE
  cout<<"\nPrinting look-up table for LTP: "<<endl;
#endif

    // try to differentiate the synapses
    // use a boolean value to indicate the synapse type:
    // typeFlag = true -> E-E  if false -> E-I (or I-E / I-I which I don't care)
    bool typeFlag = _pre->IsExcitatory()&&_post->IsExcitatory() ? true : false;
    int tau_x_trace = typeFlag ? TAU_X_TRACE_E : TAU_X_TRACE_I;
    int tau_y_trace = typeFlag ? TAU_Y1_TRACE_E : TAU_Y1_TRACE_I;
    int a_pos = typeFlag ? D_A_POS_E : D_A_POS_I;
    int a_neg = typeFlag ? D_A_NEG_E : D_A_NEG_I;
    double double_a_pos = typeFlag ? A_POS_E : A_POS_I;
    double double_a_neg = typeFlag ? A_NEG_E : A_NEG_I;

  for(int i = 0; i < 3*tau_x_trace; ++i){
#ifdef DIGITAL
      if(i == 0)
	  _TABLE_LTP.push_back(UNIT_DELTA*a_pos);
      else
	  _TABLE_LTP.push_back( _TABLE_LTP[i-1] - _TABLE_LTP[i-1]/tau_x_trace);
#else
      if(i == 0)
	  _TABLE_LTP.push_back(double_a_pos);
      else
	  _TABLE_LTP.push_back(_TABLE_LTP[i-1] - _TABLE_LTP[i-1]/tau_x_trace);
#endif
#ifdef _DEBUG_LOOKUP_TABLE 
      cout<<_TABLE_LTP[i]<<"\t";
#endif
  }

#ifdef _DEBUG_LOOKUP_TABLE
  cout<<"\nPrinting look-up table for LTD: "<<endl;
#endif

  for(int i = 0; i < 3*tau_y_trace; ++i){
#ifdef DIGITAL
      if(i == 0)
	  _TABLE_LTD.push_back(-1*UNIT_DELTA*a_neg);
      else
	  _TABLE_LTD.push_back( _TABLE_LTD[i-1] - _TABLE_LTD[i-1]/tau_y_trace);
#else
      if(i == 0)
	  _TABLE_LTD.push_back(-1*double_a_neg);
      else
	  _TABLE_LTD.push_back(_TABLE_LTD[i-1] - _TABLE_LTD[i-1]/tau_y_trace);
#endif		
#ifdef _DEBUG_LOOKUP_TABLE 
      cout<<_TABLE_LTD[i]<<"\t";
#endif
  }
#ifdef _DEBUG_LOOKUP_TABLE 
  cout<<endl;
#endif

      /* // close to exp version:
#ifdef DIGITAL
      _TABLE_LTP[i] = (int)UNIT_DELTA*D_A_POS_E*exp(-1.0*i/TAU_X_TRACE_E);
#else
      _TABLE_LTP[i] = A_POS_E*exp(-1.0*i/TAU_X_TRACE_E);
#endif
  }
  for(int i = 0; i < 3*TAU_Y1_TRACE_E; ++i){
#ifdef DIGITAL
      _TABLE_LTD[i] = (int)-1*UNIT_DELTA*D_A_NEG_E*exp(-1.0*i/TAU_Y1_TRACE_E);
#else
      _TABLE_LTD[i] = -1*A_NEG_E*exp(-1.0*i/TAU_Y1_TRACE_E);
#endif	
  }
      */
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


bool Synapse::IsValid(){
    assert(_pre && _post); 
    return !(_pre->LSMCheckNeuronMode(DEACTIVATED) ||
	     _post->LSMCheckNeuronMode(DEACTIVATED));
}



void Synapse::LSMLearn(int iteration){
  assert((_fixed==false)&&(_lsm_active==true));
  _lsm_active = false;
#ifdef DIGITAL
  int iter = iteration + 1;

  if(iteration <= 50) iter = 1;
  else if(iteration <= 100) iter = 2;
  else if(iteration <= 150) iter = 4;
  else if(iteration <= 200) iter = 8;
  else if(iteration <= 250) iter = 16;
  else if(iteration <= 300) iter = 32;
  else iter = iteration/4; 

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
      _lsm_weight += LSM_DELTA_POT/( 1 + iteration/ITER_SEARCH_CONV);
    }
  }else{
  if((_lsm_c > LSM_CAL_MID-3)){ //&&(_post->LSMGetVMemPre() < LSM_V_THRESH*0.2)){
      _lsm_weight -= LSM_DELTA_DEP/( 1+ iteration/ITER_SEARCH_CONV);
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

// update the trace x:
  if(_post->IsExcitatory()){
#ifdef DIGITAL
      ExpDecay(_D_x_j, TAU_X_TRACE_E);
#else
      ExpDecay(_x_j, TAU_X_TRACE_E);
#endif
  }
  else{
#ifdef DIGITAL
      ExpDecay(_D_x_j, TAU_X_TRACE_I);
#else
      ExpDecay(_x_j, TAU_X_TRACE_I);
#endif
  }

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
  if(_post->IsExcitatory()){
#ifdef DIGITAL
      ExpDecay(_D_y_i1, TAU_Y1_TRACE_E);
      ExpDecay(_D_y_i2, TAU_Y2_TRACE_E);
#else
      ExpDecay(_y_i1, TAU_Y1_TRACE_E);
      ExpDecay(_y_i2, TAU_Y2_TRACE_E);
#endif
  }
  else{
#ifdef DIGITAL
      ExpDecay(_D_y_i1, TAU_Y1_TRACE_I);
      ExpDecay(_D_y_i2, TAU_Y2_TRACE_I);
  
#else
      ExpDecay(_y_i1, TAU_Y1_TRACE_I);
      ExpDecay(_y_i2, TAU_Y2_TRACE_I);
#endif
  }

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
  if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_15"))
#ifdef DIGITAL
      cout<<"t: "<<t<<"\tx_j: "<<_D_x_j<<"\ty_i1: "<<_D_y_i1<<"\ty_i2: "<<_D_y_i2<<endl;
#else
      cout<<"t: "<<t<<"\tx_j: "<<_x_j<<"\ty_i1: "<<_y_i1<<"\ty_i2: "<<_y_i2<<endl;
#endif
#endif

  /* Keep track of the synaptic activities here */
#ifdef _PRINT_SYN_ACT
  _simulation_time.push_back(t); 
#ifdef DIGITAL
  _x_collection.push_back(_D_x_j);
  _y_collection.push_back(_D_y_i1);
#else
  _x_collection.push_back(_x_j);
  _y_collection.push_back(_y_i1);
#endif

#ifdef DIGITAL
  // if the synpase has not been activated by STDP rule, record its w here:
  if(!_lsm_stdp_active)
      _weights_collection.push_back(_D_lsm_weight);
#else
  if(!_lsm_stdp_active)
      _weights_collection.push_back(_lsm_weight);
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

  // if the close to hareware implementation is considered,
  // apply the look-up table appoarch
#ifdef _HARDWARE_CODE
  LSMLiquidHarewareLearn(t);
  return;
#endif

  /*
  if(_t_spike_pre == _t_spike_post)
    cout<<"Time: "<<t<<"\t"<<_pre->Name()<<"\t"<<_post->Name()<<endl;
  */
  
  // F_pos/F_neg is the STDP function for LTP and LTD:
  // LAMBDA is the learning rate here

#ifdef DIGITAL
#ifdef ADDITIVE_STDP
  int F_pos = _D_lsm_weight >= 0 ? 
      (_post->IsExcitatory() ? D_A_POS_E : D_A_POS_I) :
      (_post->IsExcitatory() ? -1*D_A_POS_E : -1*D_A_POS_I);

  int F_neg = _D_lsm_weight >= 0 ?
      (_post->IsExcitatory() ? D_A_NEG_E : D_A_NEG_I) :
      (_post->IsExcitatory() ? -1*D_A_NEG_E : -1*D_A_NEG_I);
#else
  int F_pos = _D_lsm_weight >= 0 ? (_D_lsm_weight_limit - _D_lsm_weight) 
                                : (-_D_lsm_weight_limit -  _D_lsm_weight); // pos->polarized!
  int F_neg = _D_lsm_weight;
#endif
#else
#ifdef ADDITIVE_STDP
  double F_pos = _lsm_weight >= 0 ? 
      (_post->IsExcitatory() ? A_POS_E : A_POS_I) :
      (_post->IsExcitatory() ? -1*A_POS_E : -1*A_POS_I);

  double F_neg = _lsm_weight >= 0 ? 
      (_post->IsExcitatory() ? A_NEG_E : A_NEG_I) :
      (_post->IsExcitatory() ? -1*A_NEG_E : -1*A_NEG_I);
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
    if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_15"))
#ifdef DIGITAL
      cout<<"post firing time: "<<t<<"\tpre spike @"<<_t_spike_pre<<"\tF_pos: "<<F_pos<<"\tx_j: "<<_D_x_j<<"\ty_i2_last: "<<_D_y_i2_last<<"\tdelta_w_pos: "<<delta_w_pos<<endl;
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
    if(!strcmp(_pre->Name(), "reservoir_1") && !strcmp(_post->Name(), "reservoir_15"))
#ifdef DIGITAL
      cout<<"pre firing time: "<<t<<"\tpost spike @"<<_t_spike_post<<"\tF_neg: "<<F_neg<<"\ty_i1: "<<_D_y_i1<<"\tdelta_w_neg: "<<delta_w_neg<<endl;
#else
      cout<<"pre firing time: "<<t<<"\tpost spike @"<<_t_spike_post<<"\tF_neg: "<<F_neg<<"\ty_i1: "<<_y_i1<<"\tdelta_w_neg: "<<delta_w_neg<<endl;
#endif
#endif
  }

  // 3. Update the weight:
#ifdef STOCHASTIC_STDP 
  //Here I adopt the abstract learning rule:
  _D_lsm_c = _post->DLSMGetCalciumPre();
  _lsm_c = _post->GetCalciumPre();
  StochasticSTDP(delta_w_pos, delta_w_neg);
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


  // 5. Record the weight information if needed:
#ifdef _PRINT_SYN_ACT
#ifdef DIGITAL
  _weights_collection.push_back(_D_lsm_weight);
#else
  _weights_collection.push_back(_lsm_weight);
#endif
#endif

}

//* implement the look-up table approach for simply hardware implementation
void Synapse::LSMLiquidHarewareLearn(int t){
#ifdef DIGITAL
  int delta_w_pos = 0, delta_w_neg = 0;
#else
  double delta_w_pos = 0, delta_w_neg = 0;
#endif
  if(t == _t_spike_post){  // LTP:
      assert(_t_spike_pre <= _t_spike_post);
      size_t ind = _t_spike_post - _t_spike_pre;
      if(ind < _TABLE_LTP.size()) // look-up table:
#ifdef DIGITAL
#ifdef STOCHASTIC_STDP // I will consider the stochastic sdtp here for both all-pairing 
	               // and nearest neighbor-pairing
         delta_w_pos = (_TABLE_LTP[ind]);
#else
         delta_w_pos = (_TABLE_LTP[ind])>>(LAMBDA_BIT);
#endif
#else
         delta_w_pos = _TABLE_LTP[ind];
#endif
  }

  if(t == _t_spike_pre){ // LTD:
      assert(_t_spike_post <= _t_spike_pre);
      size_t ind = _t_spike_pre - _t_spike_post;
      if(ind < _TABLE_LTD.size())
#ifdef DIGITAL
#ifdef STOCHASTIC_STDP
	  delta_w_neg = (_TABLE_LTD[ind]);
#else
          delta_w_neg = (_TABLE_LTD[ind])>>(LAMBDA_BIT + ALPHA_BIT);
#endif
#else
	  delta_w_neg = (_TABLE_LTD[ind]);
#endif
  }

  // Update the weight:
#ifdef STOCHASTIC_STDP 
  // Here I adopt the abstract learning rule:
  // need to get the calcium level of post neuron in advance
  _D_lsm_c = _post->DLSMGetCalciumPre();
  _lsm_c = _post->GetCalciumPre();
  StochasticSTDP(delta_w_pos, delta_w_neg);
#else    
#ifdef  DIGITAL
  _D_lsm_weight += delta_w_pos + delta_w_neg;
#else
  _lsm_weight += delta_w_pos + delta_w_neg;
#endif
#endif

  CheckReservoirWeightOutBound();
  return;

}

//* this function is to check whether or not the synaptic weights in the reservoir is out of bound:
inline
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
  }
#endif
}


inline
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

#ifdef _PRINT_SYN_ACT
  // clear the local memory used for print synaptic activities
  _t_pre_collection.clear();
  _t_post_collection.clear();
  _simulation_time.clear();
  _x_collection.clear();
  _y_collection.clear();
  _weights_collection.clear();
#endif

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

//* Print the synapitc activity into the file.
//* Follow the order: pre&post firing time -> local variable -> weight
void Synapse::PrintActivity(ofstream & f_out){
  assert(_weights_collection.size() == _simulation_time.size());

  // 0. Print the simulation time into file
  //    Use "-1" to do the separation between different information:
  f_out<<"-1\n";
  PrintToFile(_simulation_time, f_out);
  f_out<<"-1\n";

  // 1. Print the pre & post-synaptic neuron activity:
  PrintToFile(_t_pre_collection, f_out);
  f_out<<"-1\n";
  PrintToFile(_t_post_collection, f_out);
  f_out<<"-1\n";


  // 2. Print the local variable:
  PrintToFile(_x_collection, f_out);
  f_out<<"-1\n";
  PrintToFile(_y_collection, f_out);
  f_out<<"-1\n";

  // 3. Print the weight:
  PrintToFile(_weights_collection, f_out);
  f_out<<"-1\n"<<endl;
  
}
