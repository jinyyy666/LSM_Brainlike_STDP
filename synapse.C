#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "util.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <numeric>


//#define _DEBUG_SYN_UPDATE
//#define _DEBUG_SYN_LEARN
//#define _DEBUG_LOOKUP_TABLE
//#define _DEBUG_REWARD_TABLE
//#define _DEBUG_TRUNC
//#define _DEBUG_YONG_RULE
//#define _DEBUG_SIMPLE_STDP
#define _DEBUG_AP_STDP
//#define _DEBUG_UNSUPERV_TRAINING
//#define _DEBUG_SUPV_STDP
//#define _DEBUG_UPDATE_AT_LAST
//#define _DEBUG_BP

using namespace std;

extern int COUNTER_LEARN;

// for continuous model:
Synapse::Synapse(Neuron * pre, Neuron * post, double lsm_weight, bool fixed, double lsm_weight_limit, bool excitatory, bool liquid):
    _pre(pre),
    _post(post),
    _excitatory(excitatory),
    _fixed(fixed),
    _liquid(liquid),
    _disable(false),
    _lsm_active(false),
    _lsm_stdp_active(false),
    _mem_pos(0),
    _mem_neg(0),
    _lsm_tau1(4*2),
    _lsm_state1(0),
    _lsm_state2(0),
    _lsm_state(0),
    _lsm_spike(0),
    _D_lsm_spike(0),
    _lsm_delay(0),
    _lsm_c(0),
    _lsm_weight(lsm_weight),
    _lsm_v_w(0),
    _lsm_weight_old(_lsm_weight),
    _lsm_weight_limit(fabs(lsm_weight_limit)),
    _lsm_effect_ratio(1.0),
    _lsm_g1(0.0f),
    _lsm_g2(0.0f),
    _b1_t(0.9),
    _b2_t(0.999),
    _Unit(1),
    _D_lsm_c(0),
    _D_lsm_weight(0),
    _D_lsm_weight_limit(0),
    _D_lsm_tau1(4*2),
    _D_lsm_state1(0),
    _D_lsm_state2(0),
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

    _lsm_tau2 = _excitatory ? LSM_T_SYNE*2 : LSM_T_SYNI*2;
    _D_lsm_tau2 = _excitatory ? LSM_T_SYNE*2 : LSM_T_SYNI*2;
    char * name = post->Name();
    if(name[0] == 'o')  _excitatory = _lsm_weight >= 0; 

    cout<<pre->Name()<<"\t"<<post->Name()<<"\t"<<_excitatory<<"\t"<<post->IsExcitatory()<<"\t"<<_lsm_weight<<"\t"<<_lsm_weight_limit<<endl;

    // initialize the look-up table:
    _init_lookup_table(false);
    _init_lookup_table(true);
}

// for digital implementation model:
Synapse::Synapse(Neuron * pre, Neuron * post, int D_lsm_weight, bool fixed, int D_lsm_weight_limit, bool excitatory, bool liquid):
    _pre(pre),
    _post(post),
    _excitatory(excitatory),
    _fixed(fixed),
    _liquid(liquid),
    _disable(false),
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
    _lsm_v_w(0),
    _lsm_weight_old(_lsm_weight),
    _lsm_weight_limit(0),
    _lsm_effect_ratio(1.0),
    _lsm_g1(0.0f),
    _lsm_g2(0.0f),
    _b1_t(0.9),
    _b2_t(0.999),
    _D_lsm_c(0),
    _D_lsm_weight(D_lsm_weight),
    _D_lsm_weight_limit(abs(D_lsm_weight_limit)),
    _D_lsm_tau1(4*2),
    _D_lsm_state1(0),
    _D_lsm_state2(0),
    _Unit(1),
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

    _lsm_tau2 = _excitatory ? LSM_T_SYNE*2 : LSM_T_SYNI*2;
    _D_lsm_tau2 = _excitatory ? LSM_T_SYNE*2 : LSM_T_SYNI*2;

    char * pre_name = pre->Name();
    assert(pre_name != NULL);
    // be careful.. that might hamper the performance : ( 
    // if(pre_name[0] == 'i')  _excitatory = _D_lsm_weight >= 0; 

    if(IsFeedbackSyn()){
        _Unit = 1;
    }
    else if((_liquid == false)&&(pre_name[0] != 'i')){
#if NUM_BIT_SYN > NBT_STD_SYN
        _Unit = _Unit<<(NUM_BIT_SYN-NBT_STD_SYN);
        _D_lsm_weight = _D_lsm_weight<<(NUM_BIT_SYN-NBT_STD_SYN);
        _D_lsm_weight_limit = _D_lsm_weight_limit<<(NUM_BIT_SYN-NBT_STD_SYN);
#elif NUM_BIT_SYN < NBT_STD_SYN
        _Unit = _Unit>>(NBT_STD_SYN-NUM_BIT_SYN);
        _D_lsm_weight = _D_lsm_weight>>(NBT_STD_SYN-NUM_BIT_SYN);
        _D_lsm_weight_limit = _D_lsm_weight_limit>>(NBT_STD_SYN-NUM_BIT_SYN);
#endif
    }
    else if(pre_name[0] == 'i'){
        _Unit = _Unit<<(NUM_BIT_SYN_R - NBT_STD_SYN_R);
        _D_lsm_weight = _D_lsm_weight<<(NUM_BIT_SYN_R - NBT_STD_SYN_R);
        _D_lsm_weight = _D_lsm_weight/4;
        _D_lsm_weight_limit = _D_lsm_weight_limit<<(NUM_BIT_SYN_R - NBT_STD_SYN_R);
        _D_lsm_weight_limit = _D_lsm_weight_limit;
    }
    else{
        assert(_liquid == true);
#if NUM_BIT_SYN_R > NBT_STD_SYN_R
        _Unit = _Unit<<(NUM_BIT_SYN_R-NBT_STD_SYN_R);
        _D_lsm_weight = _D_lsm_weight<<(NUM_BIT_SYN_R-NBT_STD_SYN_R);
        _D_lsm_weight_limit = _D_lsm_weight_limit<<(NUM_BIT_SYN_R-NBT_STD_SYN_R);
#else
        _Unit = _Unit>>(NUM_BIT_SYN_R-NBT_STD_SYN_R);
        _D_lsm_weight = _D_lsm_weight>>(NBT_STD_SYN_R - NUM_BIT_SYN_R);
        _D_lsm_weight_limit = _D_lsm_weight_limit>>(NBT_STD_SYN_R - NUM_BIT_SYN_R);
#endif
    }

    cout<<pre->Name()<<"\t"<<post->Name()<<"\tPreExcit: "<<_excitatory<<"\tPostExcit: "<<post->IsExcitatory()<<"\t"<<_D_lsm_weight<<endl;

    // initialize the look-up table:
    _init_lookup_table(false);
    _init_lookup_table(true);

}


void Synapse::_init_lookup_table(bool isSupv){
#ifdef _DEBUG_LOOKUP_TABLE
    if(isSupv)
        cout<<"\nPrinting look-up table for LTP (SUPV): "<<endl;    
    else
        cout<<"\nPrinting look-up table for LTP: "<<endl;
#endif
    // try to differentiate the synapses
    // typeFlag = true -> E-E  if false -> E-I (or I-E / I-I)
    const int c_tau_x_e = isSupv ? TAU_X_TRACE_E_S : TAU_X_TRACE_E;
    const int c_tau_y1_e = isSupv ? TAU_Y1_TRACE_E_S : TAU_Y1_TRACE_E;
    const int c_tau_y2_e = isSupv ? TAU_Y2_TRACE_E_S : TAU_Y2_TRACE_E;
    const int c_tau_x_i = isSupv ? TAU_X_TRACE_I_S : TAU_X_TRACE_I;
    const int c_tau_y1_i = isSupv ? TAU_Y1_TRACE_I_S : TAU_Y1_TRACE_I;
    const int c_tau_y2_i = isSupv ? TAU_Y2_TRACE_I_S : TAU_Y2_TRACE_I;

    const double c_a_pos_e = isSupv ? A_POS_E_S : A_POS_E;
    const double c_a_neg_e = isSupv ? A_NEG_E_S : A_NEG_E;
    const double c_a_pos_i = isSupv ? A_POS_I_S : A_POS_I;
    const double c_a_neg_i = isSupv ? A_NEG_I_S : A_NEG_I;

    const int c_digital_a_pos_e = isSupv ? D_A_POS_E_S : D_A_POS_E;
    const int c_digital_a_neg_e = isSupv ? D_A_NEG_E_S : D_A_NEG_E;
    const int c_digital_a_pos_i = isSupv ? D_A_POS_I_S : D_A_POS_I;
    const int c_digital_a_neg_i = isSupv ? D_A_NEG_I_S : D_A_NEG_I;

    // time consts for the reward modulated rule
    /*
    const int c_tau_syn_pos_e = _pre->GetTauEP()/2;
    const int c_tau_syn_neg_e = _pre->GetTauEN()/2;
    const int c_tau_syn_pos_i = _pre->GetTauIP()/2;
    const int c_tau_syn_neg_i = _pre->GetTauIN()/2;
    */
    const int c_tau_syn_pos_e = TAU_REWARD_POS_E;
    const int c_tau_syn_neg_e = TAU_REWARD_NEG_E;
    const int c_tau_syn_pos_i = TAU_REWARD_POS_I;
    const int c_tau_syn_neg_i = TAU_REWARD_NEG_I;

    bool typeFlag = _pre->IsExcitatory()&&_post->IsExcitatory() ? true : false;
    int tau_x_trace = typeFlag ? c_tau_x_e : c_tau_x_i;
    int tau_y_trace = typeFlag ? c_tau_y1_e : c_tau_y1_i;

    int a_pos = typeFlag ? c_digital_a_pos_e : c_digital_a_pos_i;
    int a_neg = typeFlag ? c_digital_a_neg_e : c_digital_a_neg_i;

    double double_a_pos = typeFlag ? c_a_pos_e : c_a_pos_i;
    double double_a_neg = typeFlag ? c_a_neg_e : c_a_neg_i;

    int tau_reward_pos = _pre->IsExcitatory() ? c_tau_syn_pos_e : c_tau_syn_pos_i;
    int tau_reward_neg = _pre->IsExcitatory() ? c_tau_syn_neg_e : c_tau_syn_neg_i;

#ifdef DIGITAL
    vector<int> & table_ltp = isSupv ? _TABLE_LTP_S : _TABLE_LTP;
    vector<int> & table_ltd = isSupv ? _TABLE_LTD_S : _TABLE_LTD;
    vector<int> & table_reward = _TABLE_REWARD;
#else
    vector<double> & table_ltp = isSupv ? _TABLE_LTP_S : _TABLE_LTP;
    vector<double> & table_ltd = isSupv ? _TABLE_LTD_S : _TABLE_LTD;
    vector<double> & table_reward = _TABLE_REWARD;
#endif

    for(int i = 0; i < 3*tau_x_trace; ++i){
#ifdef DIGITAL
        if(i == 0)
            table_ltp.push_back(UNIT_DELTA*a_pos);
        else{
            table_ltp[i-1]/tau_x_trace == 0 && table_ltp[i-1] > 0 ?
                table_ltp.push_back( table_ltp[i-1] - 1) : 
                table_ltp.push_back( table_ltp[i-1] - table_ltp[i-1]/tau_x_trace);
        }
#else
        if(i == 0)
            table_ltp.push_back(double_a_pos);
        else
            table_ltp.push_back(table_ltp[i-1] - table_ltp[i-1]/tau_x_trace);
#endif

#ifdef _DEBUG_LOOKUP_TABLE 
        cout<<table_ltp[i]<<"\t";
#endif
    }

#ifdef _DEBUG_LOOKUP_TABLE
    cout<<"\nPrinting look-up table for LTD: "<<endl;
#endif

    for(int i = 0; i < 3*tau_y_trace; ++i){
#ifdef DIGITAL
        if(i == 0)
            table_ltd.push_back(-1*UNIT_DELTA*a_neg);
        else{
            table_ltd[i-1]/tau_y_trace == 0 && table_ltd[i-1] < 0 ? 
                table_ltd.push_back(table_ltd[i-1] + 1) :
                table_ltd.push_back(table_ltd[i-1] - table_ltd[i-1]/tau_y_trace);
        }
#else
        if(i == 0)
            table_ltd.push_back(-1*double_a_neg);
        else
            table_ltd.push_back(table_ltd[i-1] - table_ltd[i-1]/tau_y_trace);
#endif		
#ifdef _DEBUG_LOOKUP_TABLE 
        cout<<table_ltd[i]<<"\t";
#endif
    }
#ifdef _DEBUG_LOOKUP_TABLE 
    cout<<endl;
#endif
    int size = !table_reward.empty() ? 0 : 
        (tau_reward_pos > tau_reward_neg ? tau_reward_pos : tau_reward_neg);
    int ds_pos = 0, ds_neg = 0;
    double s_pos = 0, s_neg = 0;
    for(int i = 0; i < 3*size; ++i){
#ifdef DIGITAL
        if(i == 0){
            ds_pos = IsLiquidSyn() ? 1 : D_A_REWARD_POS; // note for reservoir syns here!!
            ds_neg = IsLiquidSyn() ? 1 : D_A_REWARD_NEG;
        }
        else{
            ds_pos = ds_pos <= 0 ? 0 : ds_pos - ds_pos/tau_reward_pos;
            ds_neg = ds_neg <= 0 ? 0 : ds_neg - ds_neg/tau_reward_neg;
        }
        table_reward.push_back((ds_pos - ds_neg)/(tau_reward_pos - tau_reward_neg));
#else
        if(i == 0){
            s_pos = A_REWARD_POS;
            s_neg = A_REWARD_NEG;
        }
        else{
            s_pos = s_pos <= 0 ? 0 : s_pos - s_pos/tau_reward_pos;
            s_neg = s_neg <= 0 ? 0 : s_neg - s_neg/tau_reward_neg;
        }
        table_reward.push_back((s_pos - s_neg)/(tau_reward_pos - tau_reward_neg)); 
#endif      
#ifdef _DEBUG_REWARD_TABLE
        cout<<table_reward[i]<<"\t";
#endif
    }
#ifdef _DEBUG_REWARD_TABLE
    cout<<endl;
#endif
}

// Check need to perform the update or not
bool Synapse::WithinSTDPTable(int time){
    if(time < 0 || (time != _t_spike_pre && time != _t_spike_post))
        return false;
    int delta_t = time == _t_spike_pre ? _t_spike_pre - _t_spike_post : _t_spike_post - _t_spike_pre; // LTD:LTP
    assert(delta_t >= 0);

#ifdef _REWARD_MODULATE
    if(delta_t >= _TABLE_REWARD.size())  return false;
#else
    if(delta_t >= _TABLE_LTD_S.size())  return false;
#endif
    return true;
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

void Synapse::SetPreNeuron(Neuron * pre){
    assert(pre);
    _pre = pre;
}

void Synapse::SetPostNeuron(Neuron * post){
    assert(post);
    _post = post;
}

bool Synapse::IsReadoutSyn(){
    char * name_post = _post->Name();
    // if the post neuron is the readout/hidden neuron:
    if((name_post[0] == 'o' && name_post[6] == '_') || (name_post[0] == 'h' && name_post[6] == '_')){
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

bool Synapse::IsFeedbackSyn(){
    char * name_post = _post->Name();
    // if the post neuron is the reservoir neuron:
    if((name_post[0] == 'r' && name_post[9] == '_')){
        return true;
    }
    else{
        return false;
    }
}

bool Synapse::IsValid(){
    assert(_pre && _post); 
    return !(_pre->LSMCheckNeuronMode(DEACTIVATED) ||
            _post->LSMCheckNeuronMode(DEACTIVATED) ||
            _disable);
}


void Synapse::LSMLearn(int t, int iteration){
    assert((_fixed==false)&&(_lsm_active==true));
    _lsm_active = false;

    int iter = iteration + 1;

    if(iteration <= 50) iter = 1;
    else if(iteration <= 100) iter = 2;
    else if(iteration <= 150) iter = 4;
    else if(iteration <= 200) iter = 8;
    else if(iteration <= 250) iter = 16;
    else if(iteration <= 300) iter = 32;
    else iter = iteration/4; 

#ifdef _DEBUG_YONG_RULE
    //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0")){
    if(!strcmp(_post->Name(), "output_0")){
        cout<<"Synapse from "<<_pre->Name()<<"\tto\t"<<_post->Name()<<endl;
#ifdef DIGITAL
        cout<<"The weight before : "<<_D_lsm_weight<<endl;
#else
        cout<<"The weight before : "<<_lsm_weight<<endl;
#endif
        cout<<"Firing @"<<t<<"\t t_pre: "<<_t_spike_pre<<"\t t_post: "<<_t_spike_post<<" with TS: "<<_post->GetTeacherSignal()<<endl;
    }
#endif

#ifdef DIGITAL
    const int temp1 = one<<18;
    const int temp2 = one<<(18+NUM_BIT_SYN-NBT_STD_SYN);
    int weight_old = _D_lsm_weight;

    _D_lsm_c = _post->DLSMGetCalciumPre();
    if(_D_lsm_c > LSM_CAL_MID*unit){
        if((_D_lsm_c < (LSM_CAL_MID+3)*unit)){
            _D_lsm_weight += (rand()%temp1) < (LSM_DELTA_POT*temp2/iter) ? 1 : 0;
        }
    }else{
        if((_D_lsm_c > (LSM_CAL_MID-3)*unit)){
            _D_lsm_weight -= (rand()%temp1) < (LSM_DELTA_DEP*temp2/iter) ? 1 : 0;
        }
    }
#else
    double weight_old = _lsm_weight;
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
#endif

    // keep track the weight update, and perform one update at the end
#ifdef _UPDATE_AT_LAST
#ifdef DIGITAL
    if(weight_old != _D_lsm_weight){
        _firing_stamp.push_back({t, _D_lsm_weight - weight_old});
        _D_lsm_weight = weight_old;
    }
#else
    if(fabs(weight_old - _lsm_weight) > 1e-8){
        _firing_stamp.push_back({t, _lsm_weight - weight_old});
        _lsm_weight = weight_old;
    }
#endif
#endif


#ifdef _DEBUG_YONG_RULE
    //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0"))
    if(!strcmp(_post->Name(), "output_0"))
#ifdef DIGITAL
        cout<<"After Weight: "<<_D_lsm_weight<<(_D_lsm_weight >= weight_old ? " Weight increase: " : " Weight decrease: ")<<_D_lsm_weight - weight_old<<" Calicium of the post: "<<_post->DLSMGetCalciumPre()<<endl;
#else
        cout<<"After Weight: "<<_lsm_weight<<(_lsm_weight >= weight_old ? " Weight increase: " : " Weight decrease: ")<<_lsm_weight - weight_old<<" Calicium of the post: "<<_post->GetCalciumPre()<<endl;
#endif
#endif

    CheckReadoutWeightOutBound();
}

//* Truncate the intermediate weights by setting the synaptic weight to zero.
//* This is a handcrafting way to reduce design complexity and save energy
//* for the hardware.
//* @return : true-> a synapse is removed; false->otherwise
bool Synapse::TruncIntermWeight(){
    bool ret = false; 
    // Only call this function for excitatory synapses:
    assert(_excitatory);
#ifdef DIGITAL
    assert(_D_lsm_weight >= 0);
#else
    assert(_lsm_weight >= 0);
#endif
    assert(!IsInputSyn());

    // only consider the _D_lsm_weight now:
    // this levels here is the predefined w_max in the netlist/
#if NUM_BIT_SYN_R > NBT_STD_SYN_R
    int levels = _D_lsm_weight_limit>>(NUM_BIT_SYN_R-NBT_STD_SYN_R);
#else
    int levels = _D_lsm_weight_limit<<(NBT_STD_SYN_R - NUM_BIT_SYN_R);
#endif

#ifdef _DEBUG_TRUNC
    cout<<"levels: "<<levels<<"\t"<<_D_lsm_weight_limit/levels<<endl;
#endif
    int gran = _D_lsm_weight_limit/levels;  // granularity
    if(_D_lsm_weight > gran && _D_lsm_weight < _D_lsm_weight_limit - gran){
        ret = true;
        _D_lsm_weight = 0;
    }

    return ret;
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


//* delivery the spike response
//* set the curr_inputs_pos/curr_inputs_neg field of the post neuron
void Synapse::LSMDeliverSpike(){
#ifdef DIGITAL
    int effect = _D_lsm_weight;
#else
    double effect = _lsm_weight;
#endif

#ifdef SYN_ORDER_2
    if(_excitatory){
        _post->IncreaseEP(effect);
        _post->IncreaseEN(effect);
    }
    else{
        _post->IncreaseIP(effect);
        _post->IncreaseIN(effect);
    }
#else
    _post->IncreaseEP(effect);
#endif

}

//* keep track of the firing timing of the synapses and do the weight update at the end
void Synapse::LSMFiringStamp(int time){
    assert(time >= 0);
    double resp = LSMSecondOrderResp();
#ifdef DIGITAL
    _D_lsm_c = _post->DLSMGetCalciumPre();
    //if(_D_lsm_c > (LSM_CAL_MID - 3)*unit && _D_lsm_c < (LSM_CAL_MID + 3))
    _firing_stamp.push_back(make_pair(time, int(resp) ) );
#else
    _lsm_c = _post->GetCalciumPre();
    //if(_lsm_c > LSM_CAL_MID - 3 && _lsm_c < LSM_CAL_MID + 3)  
    _firing_stamp.push_back(make_pair(time, double(resp) ) );
#endif
}

//* Prop back the weighted error from l+1 layer
double Synapse::LSMErrorBack(){
   return _post->GetError() * _lsm_weight_old * _lsm_effect_ratio;
}

//* back prop the error w.r.t each synapse:
void Synapse::LSMBpSynError(double error, double vth, double s_effect, int iteration, const vector<int>& post_times){
#ifdef DIGITAL
    assert(0);
#endif
    _lsm_weight_old = _lsm_weight;
    const float b1 = 0.9f;
    const float b2 = 0.999f;
    const float eps = 1.e-8f;
    double beta = BP_BETA_REG;
    double lambda = BP_LAMBDA_REG;
    double presyn_sq_sum = _post->GetInputSynSqSum(_lsm_weight_limit);
    double lr = error < 0 ? BP_DELTA_POT : BP_DELTA_DEP;
    lr = lr/sqrt(1 + iteration);//lr/( 1 + iteration/BP_ITER_SEARCH_CONV);
    //auto pre_calcium_stamp = _pre->GetCalciumStamp();
    vector<int> pre_times;
    _pre->GetSpikeTimes(pre_times);
    auto synaptic_effects = ComputeAccSRM(pre_times, post_times, 4*LSM_T_M_C, LSM_T_FO, LSM_T_M_C, LSM_T_REFRAC);
    double size = synaptic_effects.size();
#ifdef _DEBUG_BP
    if(strcmp(_pre->Name(), "input_0") == 0 && strcmp(_post->Name(), "hidden_0_0") == 0){
        cout<<"Pre fire times for "<<_pre->Name()<<" : "<<pre_times<<endl;
        cout<<"Post fire times for "<<_post->Name()<<" : "<<post_times<<endl;
    }
#endif
    double acc_response = 0;
    for(auto & c : synaptic_effects){
        acc_response += c;
    }
    if(post_times.empty() && !pre_times.empty())
        acc_response = 0.1;

    _lsm_effect_ratio = pre_times.empty() || post_times.empty() ? 1 : acc_response / pre_times.size();
#ifdef _DEBUG_BP
    if(strcmp(_pre->Name(), "input_0") == 0 && strcmp(_post->Name(), "hidden_0_0") == 0)
        cout<<"error part: "<<error*lr*acc_response<<" regulation part: "<<lambda*beta * _lsm_weight/_lsm_weight_limit * exp(beta*( presyn_sq_sum - 1))<<endl;
#endif

    double weight_grad = error * acc_response * (1 + s_effect) + lambda*beta * (_lsm_weight/_lsm_weight_limit) * exp(beta*( presyn_sq_sum - 1));

#ifdef OPT_ADAM
    _lsm_g1 = b1 * _lsm_g1 + (1 - b1) * weight_grad;
    _lsm_g2 = b2 * _lsm_g2 + (1 - b2) * weight_grad * weight_grad;
#ifdef _DEBUG_BP
    if(strcmp(_pre->Name(), "input_0") == 0 && strcmp(_post->Name(), "hidden_0_0") == 0)
        cout<<"Wgrad: "<<weight_grad<<" Adam weight update: "<<lr* (_lsm_g1/(1.f-_b1_t)) / ((float)sqrt(_lsm_g2/(1.-_b2_t)) + eps)<<" g1: "<<_lsm_g1<<" g2: "<<_lsm_g2<<endl;
#endif
    _lsm_weight -= lr* (_lsm_g1/(1.f-_b1_t)) / ((float)sqrt(_lsm_g2/(1.-_b2_t)) + eps);
    _b1_t *= b1;
    _b2_t *= b2; 
#else
    _lsm_v_w = _lsm_v_w * BP_MOMENTUM + weight_grad * lr;
    _lsm_weight -= _lsm_v_w;   
#endif
    CheckReadoutWeightOutBound();
}

//* Update the learning weight in the end
void Synapse::LSMUpdateLearningWeight(){
    double sum = 0;
    for(auto & p : _firing_stamp){
        sum += p.second;
    }
#ifdef DIGITAL
    int weight_old = _D_lsm_weight;
    if(_D_lsm_weight_limit/_Unit > 0){
        sum /= _D_lsm_weight_limit/_Unit; 
        _D_lsm_weight += sum;
    }
    int update = _D_lsm_weight - weight_old;
#else
    double weight_old = _lsm_weight;
    if(_lsm_weight_limit > 0){
        sum /= _lsm_weight_limit;
        _lsm_weight += sum;
    }
    double update = _lsm_weight - weight_old;
#endif

#ifdef _DEBUG_UPDATE_AT_LAST
    //if(strcmp(_pre->Name(), "reservoir_0") == 0 && strcmp(_post->Name(), "output_0") == 0)
    cout<<"Update: "<<update<<endl;
#endif
}

double Synapse::LSMSecondOrderResp(){
#ifdef DIGITAL
    return (_D_lsm_state1 - _D_lsm_state2)/(_D_lsm_tau1 - _D_lsm_tau2);
#endif
    return (_lsm_state1-_lsm_state2)/(_lsm_tau1-_lsm_tau2);
}

double Synapse::LSMFirstOrderCurrent(){
#ifdef DIGITAL
    assert(0);
#endif
    return _lsm_state*_lsm_weight;
}

//* two 2nd state variable decay
void Synapse::LSMRespDecay(int time, int end_time){
#ifdef DIGITAL
    ExpDecay(_D_lsm_state1, _D_lsm_tau1);
    ExpDecay(_D_lsm_state2, _D_lsm_tau2);
#else
    ExpDecay(_lsm_state1, _lsm_tau1);
    ExpDecay(_lsm_state2, _lsm_tau2);
#endif
    //if(strcmp(_pre->Name(), "reservoir_0") == 0 && strcmp(_post->Name(), "output_0") == 0)
    //cout<<"Response for r_0 -> o_0 : "<<LSMSecondOrderResp()<<" @ "<<time<<endl;
    if(_post->LSMCheckNeuronMode(NORMALBP) && (time % (2*LSM_T_SYNE) == 0 || time == end_time ))
        _firing_stamp.push_back(make_pair(time, LSMSecondOrderResp()));
}

//* this function is used to simulate the fired spike:
// the fired spike is simply passed down to the post-synaptic neuron with delay = 1
void Synapse::LSMNextTimeStep(){
    if(_lsm_delay > 0){
        _lsm_delay--;
        if(_lsm_delay == 0){
#ifdef DIGITAL
            _D_lsm_spike = 1;
            _D_lsm_state1 += _D_lsm_weight;
            _D_lsm_state2 += _D_lsm_weight;
#else
            _lsm_spike = 1;
            _lsm_state1 += 1;
            _lsm_state2 += 1;     
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
    assert(pre_name[0] == 'r' && post_name[0] == 'r' ||
            pre_name[0] == 'i' && post_name[0] == 'r');
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

//* TODO simple STDP rule for readout supervised STDP learning
void Synapse::LSMSTDPSupvSimpleLearn(int t){
    return;
}

//* implementation of Supervised STDP
//* only consider the implementation for LUT-liked manner for simpilicity
void Synapse::LSMSTDPSupvLearn(int t, int iteration){
#ifndef _LUT_HARDWARE_APPROACH
    cout<<"Warning! You are using the supervised STDP training without enabling the LUT approach!"<<endl;
    assert(0);
#endif

    assert(_lsm_stdp_active == true);
    _lsm_stdp_active = false;

#ifdef _DEBUG_SYN_LEARN
    char * pre_name = _pre->Name();
    char * post_name = _post->Name();
    assert(pre_name[0] == 'r' && post_name[0] == 'o'); 
#endif


    if(t < 0 || (t != _t_spike_pre && t != _t_spike_post))
        return;
    int delta_t = t == _t_spike_pre ? _t_spike_pre - _t_spike_post : _t_spike_post - _t_spike_pre; // LTD:LTP
    assert(delta_t >= 0);

#if defined(_REWARD_MODULATE) && defined(_DEBUG_SUPV_STDP)
    if((t == _t_spike_post|| t == _t_spike_pre) 
            && !strcmp(_post->Name(), "output_0") 
            && delta_t >= _TABLE_REWARD.size()){
        //cout<<"Ignore synapse from "<<_pre->Name()<<"\tto\t"<<_post->Name()<<endl;
    }
#endif

#ifdef _REWARD_MODULATE
    if(t == _t_spike_pre && delta_t >= _TABLE_REWARD.size())  return;
    if(t == _t_spike_post && delta_t >= _TABLE_REWARD.size()) return;
#else
    if(t == _t_spike_pre && delta_t >= _TABLE_LTD_S.size())  return;
    if(t == _t_spike_post && delta_t >= _TABLE_LTP_S.size()) return;
#endif

#if defined(DIGITAL) && defined(_SIMPLE_STDP) 
    // if the simple hardware efficiency STDP rule is considered,
    // apply the simple approach:
    LSMSTDPSupvSimpleLearn(t);
#else
    // apply the look-up table appoarch
    LSMSTDPSupvHardwareLearn(t, iteration);
#endif

}

//* LUT approach of supervised STDP
void Synapse::LSMSTDPSupvHardwareLearn(int t, int iteration){
#ifdef DIGITAL
    int delta_w_pos = 0, delta_w_neg = 0, weight_old = _D_lsm_weight;
#else
    double delta_w_pos = 0, delta_w_neg = 0, weight_old = _lsm_weight;
#endif

#ifdef _REWARD_MODULATE
    if(t == _t_spike_pre && _t_spike_pre != _t_spike_post)  return;


#ifdef _DEBUG_SUPV_REWARD
    if(strcmp(_pre->Name(), "reservoir_0") == 0 && strcmp(_post->Name(), "output_0") == 0)
        cout<<"Current: "<<resp<<" @"<<time<<endl;
#endif

    // Read the reward modulated rule @param4: isSupv?
    LSMReadRewardLUT(t, delta_w_pos, delta_w_neg, true);
#else
    // Read the look-up table for the supervised STDP @param4: isSupv?
    LSMReadLUT(t, delta_w_pos, delta_w_neg, true);
#endif

    // Update the weight:
    assert(_post && (_post->GetTeacherSignal() == 1 || _post->GetTeacherSignal() == -1));
    if(_post->GetTeacherSignal() == -1){
        assert(delta_w_pos >= 0);
        delta_w_pos = -1*delta_w_pos; // penalize the wrong update
    }

#ifdef DIGITAL
    int l1_norm = 0;
    int regular = 0;
#else
    double l1_norm = 0;
    double regular = 0;
#endif

#ifdef STOCHASTIC_STDP_SUPV
    // similar idea as ad-stdp and Yong's training
    _D_lsm_c = _post->DLSMGetCalciumPre();
    _lsm_c = _post->GetCalciumPre();

#ifdef _REGULARIZATION_STDP_SUPV
#ifdef DIGITAL
    assert(NUM_BIT_SYN >= NBT_STD_SYN);
    const int c_weight_omega = WEIGHT_OMEGA*_Unit;
    l1_norm = _post->DLSMSumAbsInputWeights();
    regular = l1_norm < c_weight_omega ? 0 : ((l1_norm - c_weight_omega)*_D_lsm_weight)/(_Unit*_Unit);
#else
    l1_norm = _post->LSMSumAbsInputWeights();
    regular = l1_norm < WEIGHT_OMEGA ? 0 : GAMMA_REG*(l1_norm - WEIGHT_OMEGA)*_lsm_weight;
#endif
#endif

    StochasticSTDPSupv(delta_w_pos, delta_w_neg, l1_norm, t, iteration);

#else 
    // no stochastic case:
#ifdef  DIGITAL
    // digital case:
#ifdef _REGULARIZATION_STDP_SUPV
    int l1_norm = _post->DLSMSumAbsInputWeights();
    _D_lsm_weight += delta_w_pos + delta_w_neg - (l1_norm < WEIGHT_OMEGA ? 0 : (l1_norm - WEIGHT_OMEGA));
#else
    _D_lsm_weight += delta_w_pos + delta_w_neg;
#endif

#else
    // continuous case:
#ifdef _REGULARIZATION_STDP_SUPV
    double l1_norm  = _post->LSMSumAbsInputWeights();
    _lsm_weight += delta_w_pos + delta_w_neg - (l1_norm < WEIGHT_OMEGA ? 0 : (l1_norm - WEIGHT_OMEGA));
#else
    _lsm_weight += delta_w_pos + delta_w_neg;
#endif

#endif
#endif

#ifdef _DEBUG_SUPV_STDP
    //if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0"))
    if(!strcmp(_post->Name(), "output_0")){
#ifdef DIGITAL
        if(_D_lsm_weight != weight_old){
            cout<<"Synapse from "<<_pre->Name()<<"\tto\t"<<_post->Name()<<endl;
            cout<<"The weight before : "<<weight_old<<"\n"<<"Firing @"<<t<<"\t t_pre: "<<_t_spike_pre<<"\t t_post: "<<_t_spike_post<<" with TS: "<<_post->GetTeacherSignal()<<endl;
            cout<<"Weight: "<<_D_lsm_weight<<(_D_lsm_weight > weight_old ? " Weight increase: " : " Weight decrease: ")<<_D_lsm_weight - weight_old<<" Calicium of the post: "<<_post->DLSMGetCalciumPre()<<endl;
        }
#else
        if(fabs(_lsm_weight - weight_old) > 1e-8){
            cout<<"Synapse from "<<_pre->Name()<<"\tto\t"<<_post->Name()<<endl;
            cout<<"The weight before : "<<weight_old<<"\n"<<"Firing @"<<t<<"\t t_pre: "<<_t_spike_pre<<"\t t_post: "<<_t_spike_post<<" with TS: "<<_post->GetTeacherSignal()<<endl;
            cout<<"Weight: "<<_lsm_weight<<(_lsm_weight > weight_old ? " Weight increase: " : " Weight decrease: ")<<_lsm_weight - weight_old<<" Calicium of the post: "<<_post->GetCalciumPre()<<endl;
        }
#endif
    }
#endif

    // check if weight out of bound
    CheckReadoutWeightOutBound();
}


//* this function is the implementation of the STDP learning rule, please find more details in the paper:
//* "Phenomenological models of synaptic plasticity based on spike timing" 
void Synapse::LSMSTDPLearn(int t){
    // you need to make sure that this synapse is active!
    assert(_lsm_stdp_active == true);
    _lsm_stdp_active = false;

    if( t < 0 || (t != _t_spike_pre && t != _t_spike_post)) 
        return; 


    // if the LUT approach of implementation is considered,
#ifdef _LUT_HARDWARE_APPROACH
#if defined(DIGITAL) && defined(_SIMPLE_STDP) 
    // if the simple hardware efficiency STDP rule is considered,
    // apply the simple approach:
    LSMSTDPSimpleLearn(t);
#else
    // apply the look-up table appoarch
    LSMSTDPHardwareLearn(t);
#endif
    return;
#endif


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
    CheckPlasticWeightOutBound();


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
void Synapse::LSMSTDPHardwareLearn(int t){
#ifdef DIGITAL
    int delta_w_pos = 0, delta_w_neg = 0;
#else
    double delta_w_pos = 0, delta_w_neg = 0;
#endif

#ifdef _DEBUG_AP_STDP
    if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0")){
#ifdef DIGITAL
        cout<<"The weight before : "<<_D_lsm_weight<<endl;
#else
        cout<<"The weight before : "<<_lsm_weight<<endl;
#endif
        cout<<"Firing @"<<t<<"\t t_pre: "<<_t_spike_pre<<"\t t_post: "<<_t_spike_post<<endl;
    }
#endif

    // Read the look-up table (in-line function): 
    LSMReadLUT(t, delta_w_pos, delta_w_neg, false);

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

#ifdef _DEBUG_AP_STDP
    if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0"))
#ifdef DIGITAL
        cout<<"LTP: "<<delta_w_pos<<"\tLTD: "<<delta_w_neg<<"\tThe weight after : "<<_D_lsm_weight<<endl;
#else
        cout<<"LTP: "<<delta_w_pos<<"\tLTD: "<<delta_w_neg<<"\tThe weight after : " <<_lsm_weight<<endl;
#endif
#endif

    CheckPlasticWeightOutBound();
    return;

}

//* implement the very simple hardware efficient STDP learning
void Synapse::LSMSTDPSimpleLearn(int t){
#ifdef DIGITAL
    int delta_w_pos = 0, delta_w_neg = 0;
#else
    cout<<"We are simluating the case close to hardware, please enable 'DIGITAL in def.h'"<<endl;
    assert(0);
#endif

    // typeFlag = true -> E-E  if false -> E-I (or I-E / I-I which I don't care)
    bool typeFlag = _pre->IsExcitatory()&&_post->IsExcitatory() ? true : false;
    // i design this rule by myself:
    int tau_x = typeFlag ? 4 : 2;
    int tau_y = typeFlag ? 6 : 3;

#ifdef _DEBUG_SIMPLE_STDP
    if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0")){
        cout<<"The weight before : "<<_D_lsm_weight<<endl;
        cout<<"Firing @"<<t<<"\t t_pre: "<<_t_spike_pre<<"\t t_post: "<<_t_spike_post<<endl;
    }
#endif

    if(t == _t_spike_post){  // LTP:
        assert(_t_spike_pre <= _t_spike_post);
        size_t d = _t_spike_post - _t_spike_pre;
        if(d == 0){ // special case, match with previous one!
            assert(_t_last_pre != 0 && _t_last_pre < _t_spike_post);
            d = _t_spike_post - _t_last_pre;
        }
        // implement the simple STDP rule here:
        // delta_w == 0 ---> w += 2  delta_w == 1 or 2 ---> w += 1 otherwises delta_w = 0 
        if(d == 0)  _D_lsm_weight += 1*_Unit;
        else if(d <= tau_x/2)  _D_lsm_weight += 3*_Unit;
        else if(d <= tau_x)  _D_lsm_weight += 1*_Unit;
    }

    if(t == _t_spike_pre){ // LTD:
        assert(_t_spike_post <= _t_spike_pre);
        size_t d = _t_spike_pre - _t_spike_post;
        if(d == 0){ // special case, match with previous one!
            assert(_t_last_post != 0 && _t_last_post < _t_spike_pre);
            d = _t_spike_pre - _t_last_post;
        }
        // implement the simple STDP rule here:
        // there are two cases you can try: 1. reset w = 0; 2. decrease w by 1
        // the modification for the training input synapses where the weight can be < 0
        // but all the input synapses are of excitatory type
        if(_t_spike_post != _t_spike_pre && d <= tau_y)
            if(d <= tau_y/3)
                _D_lsm_weight >= 0 ? _D_lsm_weight = 0 : _D_lsm_weight -= 3*_Unit;
            else if(d <= 2*tau_y/3)
                _D_lsm_weight >= 0 ? _D_lsm_weight = 1*_Unit : _D_lsm_weight -= 1*_Unit;
            else
                _D_lsm_weight >= 0 ? (_D_lsm_weight == 8*_Unit ? _D_lsm_weight = 2*_Unit :
                        _D_lsm_weight == 0 ? 0 : _D_lsm_weight -= 1*_Unit)
                    : _D_lsm_weight;
    }
#ifdef _DEBUG_SIMPLE_STDP
    if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0"))
        cout<<"The weight after : "<<_D_lsm_weight<<endl;
#endif

    if(IsLiquidSyn()) // only remap the weights for reservoir synapse because of low resolution
        RemapReservoirWeight();

#ifdef _DEBUG_SIMPLE_STDP
    if(!strcmp(_pre->Name(), "reservoir_0") && !strcmp(_post->Name(), "output_0"))
        cout<<"Remapped weight to 2-bit: "<<_D_lsm_weight<<endl;
#endif

    return;

}

//* check the synaptic weights of the unsupervisedly trained one is out of bound:
inline void Synapse::CheckPlasticWeightOutBound(){
    assert(_pre);
    char * pre_name = _pre->Name(); assert(pre_name);
#ifdef DIGITAL
    // for excitatory synapses:
    if(_excitatory){
        if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit;
        if(_D_lsm_weight <0 && pre_name[0]!='i') _D_lsm_weight = 0; // no put 0 for input
    }
    else{
        if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
        if(_D_lsm_weight >0 && pre_name[0]!='i') _D_lsm_weight = 0; 
    }
#else
    if(_excitatory){
        if(_lsm_weight >= _lsm_weight_limit) _lsm_weight = _lsm_weight_limit;
        if(_lsm_weight < 0&&pre_name[0]!='i') _lsm_weight = 0; // no put 0 for input 
    }
    else{
        if(_lsm_weight < -_lsm_weight_limit) _lsm_weight = -_lsm_weight_limit;
        if(_lsm_weight > 0 && pre_name[0]!='i') _lsm_weight = 0;
    }
#endif
}

inline void Synapse::CheckReadoutWeightOutBound(){
#ifdef DIGITAL
    if(_D_lsm_weight >= _D_lsm_weight_limit) _D_lsm_weight = _D_lsm_weight_limit;
    if(_D_lsm_weight < -_D_lsm_weight_limit) _D_lsm_weight = -_D_lsm_weight_limit;
#else
    if(_lsm_weight >= _lsm_weight_limit) _lsm_weight = _lsm_weight_limit;
    if(_lsm_weight < -_lsm_weight_limit) _lsm_weight = -_lsm_weight_limit;
#endif
}

//* this function is to remap the reservoir weights value to 2-bit resolution:
//* w <= -1 -> w = -1, w >= 2 -> 8 then under this mapping, the weights can be represented by 2 bits
inline void Synapse::RemapReservoirWeight(){
    // only consider for excitatory synapses:
    // I am doing the rounding here. I will round the 5/-5 to 8/-8.
    if(_excitatory){
        if(_D_lsm_weight <= -5*_Unit) _D_lsm_weight = -8*_Unit;
        else if(_D_lsm_weight <= -3*_Unit) _D_lsm_weight = -2*_Unit;
        else if(_D_lsm_weight <= 2*_Unit) _D_lsm_weight = _D_lsm_weight;
        else if(_D_lsm_weight <  5*_Unit) _D_lsm_weight = 2*_Unit;
        else _D_lsm_weight = 8*_Unit;
    }
}

void Synapse::LSMClear(){
    _lsm_active = false;
    _lsm_stdp_active = false;

    _mem_pos = 0;
    _mem_neg = 0;    

    _lsm_state1 = 0;
    _lsm_state2 = 0;
    _D_lsm_state1 = 0;
    _D_lsm_state2 = 0;
    _firing_stamp.clear();

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
//**  Add the active synapses into the network for learning if needed
//**  @param2: need to tune the synapse at the current step;   
//**  _lsm_active is only used for indication of the active learning synapses
void Synapse::LSMActivate(Network * network, bool need_learning, bool train){
    if(_lsm_active)
        cout<<_pre->Name()<<"\t"<<_post->Name()<<endl;

    assert(_lsm_active == false);
    // 1. Add the synapses for firing processing

    //network->LSMAddActiveSyn(this);

    // 2. For the readout synapses, if we are using stdp training in reservoir/input,
    //  just ignore the learning in the readout:
    //  But Make sure that we are going to train this readout synapse if not:
    if(need_learning == true && _fixed == false && train == true){
        network->LSMAddActiveLearnSyn(this);
        // 3. Mark the readout synapse active state:
        _lsm_active = true;
    }
}

/******************************************************
 * Activate the target synapses to be trained by STDP
 * The synapses can be input, reservoir or readout
 * @param2: synapse type indicator
 ******************************************************/
void Synapse::LSMActivateSTDPSyns(Network * network, const char * type){
    if(strcmp(type, "input") == 0)
        assert(_lsm_stdp_active == false && !IsReadoutSyn() && !IsLiquidSyn());
    else if(strcmp(type, "reservoir") == 0)
        assert(_lsm_stdp_active == false && !IsReadoutSyn() && !IsInputSyn());
    else if(strcmp(type, "readout") == 0)
        assert(_lsm_stdp_active == false && !IsInputSyn() && !IsLiquidSyn());
    else
        assert(0);

    if(_t_spike_pre == -1e8 || _t_spike_post == -1e8)  return;

    /** only train the excitatory target synapses for reservoir synapses     **/
    /** But for input synapses, since they are all excitatory, we dont care  **/
    /** Currently, for readout synapses, I am training it regardless of type **/
    /** Because the type of the readout synapses is independent of the pre-  **/
    /** neuron under the current settings (supervised learning)              **/
    if(_excitatory == true || (strcmp(type, "readout") == 0 && network->LSMGetNetworkMode() == READOUTSUPV)){
#ifdef _DEBUG_UNSUPERV_TRAINING
        cout<<"Put synapse from "<<_pre->Name()<<" to "<<_post->Name()<<" for STDP training"<<endl;
#endif
        network->LSMAddSTDPActiveSyn(this);
    }

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
