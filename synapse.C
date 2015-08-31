#include"def.h"
#include"synapse.h"
#include"neuron.h"
#include"network.h"
#include<math.h>
#include<stdlib.h>
#include<iostream>
#include<assert.h>

using namespace std;

extern int COUNTER_LEARN;
#ifdef DIGITAL
extern const int one;
extern const int unit;
// the spike strength in STDP model:
const int UNIT_DELTA = 1<<NBT_STD_SYN;
#endif

Synapse::Synapse(Neuron * pre, Neuron * post, bool excitatory, bool fixed, int value){
  _pre = pre;
  _post = post;
  _excitatory = excitatory;
  _fixed = fixed;
//  _min = -1;
//  _max = -1;
  _ini_min = -1;
  _ini_max = -1;
  _weight = value;
  _state_internal = _weight;
  _factor = 1;

  _t_spike_pre = -1e8;
  _t_last_pre = 0;
  _t_spike_post = -1e8;
//  _t_last_post = 0;

  _mem_pos = 0;
  _mem_neg = 0;
#ifdef DIGITAL
  assert(0);
#endif
}

Synapse::Synapse(Neuron * pre, Neuron * post, bool excitatory, bool fixed, int factor, int ini_min, int ini_max){
  _pre = pre;
  _post = post;
  _excitatory = excitatory;
  _fixed = fixed;
//  _min = min;
//  _max = max;
  _ini_min = ini_min;
  _ini_max = ini_max;
  if(ini_max == -1){
    if(rand()%10000 < 10000*0.3) _weight = 1;
    else _weight = 0;
  }else if(ini_max == -2){
    if(rand()%10000 < 10000*0.5) _weight = 1;
    else _weight = 0;
  }else if((ini_max==1)&&(ini_min==0)){
    if(rand()%10000 < 10000*0.5) _weight = 1;
    else _weight = 0;
  }else _weight = (int) round( ((double)(rand()%10000))/10000*(ini_max-ini_min) + ini_min );
  _state_internal = _weight;
  _factor = factor;
  _t_spike_pre = -1e8;
  _t_last_pre = 0;
  _t_spike_post = -1e8;
//  _t_last_post = 0;

  _mem_pos = 0;
  _mem_neg = 0;
#ifdef DIGITAL
  assert(0);
#endif
}

Synapse::Synapse(Neuron * pre, Neuron * post, double lsm_weight, bool fixed, double lsm_weight_limit):
_pre(pre),
_post(post),
_lsm_weight(lsm_weight),
_lsm_state1(0),
_lsm_state2(0),
_lsm_spike(0),
_lsm_state(0),
_lsm_tau1(4),
_lsm_delay(0),
_fixed(fixed),
_lsm_weight_limit(fabs(lsm_weight_limit)),
_lsm_c(0),
_lsm_active(false){
  if(lsm_weight > 0) _lsm_tau2 = LSM_T_SYNE;
  else _lsm_tau2 = LSM_T_SYNI;
#ifdef DIGITAL
  assert(0);
#endif
}

Synapse::Synapse(Neuron * pre, Neuron * post, int D_lsm_weight, bool fixed, int D_lsm_weight_limit, bool excitatory, bool liquid):
_pre(pre),
_post(post),
_D_lsm_weight(D_lsm_weight),
_lsm_state1(0),
_lsm_state2(0),
_D_lsm_spike(0),
_lsm_state(0),
_lsm_tau1(4),
_lsm_delay(0),
_fixed(fixed),
_liquid(liquid),
_D_lsm_weight_limit(abs(D_lsm_weight_limit)),
_D_lsm_c(0),
_excitatory(excitatory),
_lsm_active(false){
 assert((_liquid == false)||_liquid ==true);
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
#ifndef DIGITAL
  assert(0);
#endif
  
  }
  else if(pre_name[0] == 'i'){
    _D_lsm_weight = _D_lsm_weight<<1;
    _D_lsm_weight_limit = _D_lsm_weight<<1;
  }
  else{
  assert(_liquid == true);
     _D_lsm_weight = (_D_lsm_weight<<(NUM_BIT_SYN-NBT_STD_SYN));
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
  Neuron * pre = this->PreNeuron();
  Neuron * post = this->PostNeuron();
  char * name_post = post->Name();
  // if the post neuron is the readout neuron:
  if(name_post[0] == 'o' && name_post[6] == '_'){
    //cout<<"Synapse: "<<pre->Name()<<" to "<<post->Name()<<endl;
    assert(_fixed == false);
    return true;
  }
  else{
    cout<<"Synapse: "<<pre->Name()<<" to "<<post->Name()<<endl;
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

void Synapse::SetPreSpikeT(int t){
  _t_spike_pre = t;
}

void Synapse::SetPostSpikeT(int t){
  _t_spike_post = t;
}

void Synapse::SendSpike(){
//  if(_fixed == false){
    if(_state_internal > THETA_X){
      _state_internal += (_t_spike_pre - _t_last_pre)*ALPHA;
      if(_state_internal > X_MAX) _state_internal = X_MAX;
    }else{
      _state_internal -= (_t_spike_pre - _t_last_pre)*BETA;
      if(_state_internal < 0) _state_internal = 0;
    }
//  }

  if(_excitatory == true) _post->ReceiveSpike(Weight()*_factor);
  else _post->ReceiveSpike(-Weight()*_factor);
//  if(( strcmp(_pre->Name(),"input_377") == 0)&&( strcmp(_post->Name(),"output_66") == 0)) cout<<"9876543210\t"<<_t_spike_pre<<"\t"<<_state_internal<<endl;
}

void Synapse::SetLearningSynapse(int syn){
  assert(_fixed == false);
  _weight = syn;
  _state_internal = (double)syn;
}

void Synapse::LSMLearn(int iteration){
  assert((_fixed==false)&&(_lsm_active==true));
  _lsm_active = false;
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
  CheckWeightOutBound();
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

void Synapse::Learn(int index){
  if(index == 1) return;
  if(_fixed == true) return;

double state = _state_internal;

  double c_pre = _post->GetCalciumPre();
  if(_post->GetVMemPre() >= THETA_V){
    if((c_pre>THETA_L_UP)&&(c_pre<THETA_H_UP)){
      _state_internal += A;

////////////////////////////////////////////////////////
      if((state<THETA_X)&&(_state_internal>THETA_X)){
        cout<<"----- potentiation\t"<<state<<"\t"<<_state_internal<<"\t"<<_pre->Name()<<"\t"<<_post->Name()<<"\t@ "<<_t_spike_pre<<endl;
        cout<<"9999999999\t\t"<<_t_spike_pre<<endl;
      }

      if(_state_internal > X_MAX) _state_internal = X_MAX;
    }
  }else{
    if(((c_pre>THETA_L_DN)&&(c_pre<THETA_H_DN))  ){// ||(c_pre>THETA_H_UP)){
      _state_internal -= B;

////////////////////////////////////////////////////////
      if((state>THETA_X)&&(_state_internal<THETA_X)){
        cout<<"----- depression\t"<<state<<"\t"<<_state_internal<<"\t"<<_pre->Name()<<"\t"<<_post->Name()<<"\t@ "<<_t_spike_pre<<endl;
        cout<<"1111111111\t\t"<<_t_spike_pre<<endl;
      }

      if(_state_internal < 0) _state_internal = 0;
    }
  }
}

int Synapse::Weight(){
  return _D_lsm_weight;
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
    _lsm_state1 += 1;
    _lsm_state2 += 1;
    _lsm_spike = 1;
    _D_lsm_spike = 1;
    _lsm_state += 1/LSM_T_FO;
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

double Synapse::LSMStaticCurrent(){
#ifdef DIGITAL
  assert(0);
#endif
  double current =  _lsm_spike*_lsm_weight;
  _lsm_spike = 0;
  return current;

}

void Synapse::DLSMStaticCurrent(int*pos,int*value){
  if(_excitatory) *pos = 1;
  else *pos = -1;
  *value = _D_lsm_spike*_D_lsm_weight;
  _D_lsm_spike = 0;
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
  // Remember to deactivate the synapses here:
  assert(_lsm_active == true);
  _lsm_active = false;
}

//* this function is used to update the trace x and trace y for STDP:
void Synapse::LSMUpdate(int t){
  // Remember the delay of deliverying the fired spike is one!!!
  //t--;
  if(t < 0)
    return;

  _y_i2_last = _y_i2;
  // update the trace x:
  if(t == _t_spike_pre)
    _x_j -= _x_j/TAU_X_TRACE + UNIT_DELTA;
  else
    _x_j -= _x_j/TAU_X_TRACE;

  // update the trace y_i1 and y_i2:
  _y_i2_last = _y_i2;	// need to keep track of the y_i2 before the update. y_i2_last is used in STDP learning
  if(t == _t_spike_post){
    _y_i1 -= _y_i1/TAU_Y1_TRACE + UNIT_DELTA;
    _y_i2 -= _y_i2/TAU_Y2_TRACE + UNIT_DELTA;
  }
  else{
    _y_i1 -= _y_i1/TAU_Y1_TRACE;
    _y_i2 -= _y_i2/TAU_Y2_TRACE;
  }
  
}

//* this function is the implementation of the STDP learning rule, please find more details in the paper:
//* "Phenomenological models of synaptic plasticity based on spike timing" 
void Synapse::LSMLiquidLearn(int t){
  // Remember that the delay of deliverying the fired spike is 1
  //t--;
  if( t < 0 || (t != _t_spike_pre && t != _t_spike_post))
    return; 

  // you need to make sure that this synapse is active!
  // F_pos/F_neg is the STDP function for LTP and LTD:
  // LAMBDA is the learning rate here
  int F_pos = LAMBDA*(_D_lsm_weight_limit - _D_lsm_weight);
  int F_neg = LAMBDA*ALPHA*_D_lsm_weight;
  int delta_w_pos = 0, delta_w_neg = 0; // delta weight resulted from LTP and LTD
  // 1. LTP (Long-Time Potientation i.e. w+):
  if(t == _t_spike_post){
    assert(_t_spike_pre <= _t_spike_post);
    delta_w_pos = F_pos*_x_j*_y_i2_last;
  }

  // 2. LTD (Long-Time Depression i.e. w-):
  if(t == _t_spike_pre){
    //cout<<"t: "<<t<<"\t_t_spike_pre: "<<_t_spike_pre<<"\t_t_spike_post: "<<_t_spike_post<<endl;
    assert(_t_spike_pre >= _t_spike_post);
    delta_w_neg = -1*F_neg*_y_i1;
  }

  // 3. Update the weight:
  _D_lsm_weight += delta_w_pos + delta_w_neg;
  
  // 4. Check whether or not the weight is out of bound:
  CheckWeightOutBound();

}

//* this function is to check whether or not the weight is out of bound:
void Synapse::CheckWeightOutBound(){
  if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit-1;
  if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
}

void Synapse::LSMClear(){
  _lsm_state1 = 0;
  _lsm_state2 = 0;
  _lsm_spike = 0;
  _D_lsm_spike = 0;
  _lsm_state = 0;
  _t_spike_pre = -1e8;
  _t_spike_post = -1e8;
  _lsm_delay = 0;
}

void Synapse::LSMClearLearningSynWeights(){
  if(_fixed == true) return;
  _D_lsm_weight = 0;
  _lsm_weight = 0;
}

//  Add the active synapses (reservoir/readout synapses) into the network
//  Add the active reservoir learning synapses (trained by STDP) into the network
//  Add the active readout synapses into the network
void Synapse::LSMActivate(Network * network, bool stdp_flag){
  // 1. Add the synapses for firing processing:
  network->LSMAddActiveSyn(this);
  
  if(_fixed == true){
    // 2. For the reservoir synapses which are assumed to be fixed: 
    if(stdp_flag == true && _lsm_active == false)
      network->LSMAddReservoirActiveSyn(this);
  }
  else{
    // 3. For the readout synapses, if we are using stdp in reservoir, just ignore the learning in the readout: 
    if(stdp_flag == false)
      network->LSMAddActiveLearnSyn(this);
  }
  // 4. Mark the synapse active state:
  _lsm_active = true;  
}


void Synapse::LSMDeactivate(){
  _lsm_active = false;
}

bool Synapse::LSMGetLiquid(){
  return _liquid;
}
