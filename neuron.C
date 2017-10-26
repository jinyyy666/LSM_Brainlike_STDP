#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "speech.h"
#include "channel.h"
#include "util.h"
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <string>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <utility>
#include <algorithm>
#include <climits>

//#define _DEBUG_NEURON
//#define _DEBUG_VARBASE
//#define _DEBUG_CORBASE
//#define _DEBUG_RM_ZERO_W
//#define _DEBUG_INPUT_TRAINING
//#define _DEBUG_BP
//#define _DEBUG_BP_HIDDEN

// NOTE: The time constants have been changed to 2*original settings 
//       optimized performance for letter recognition.
using namespace std;

extern int Current;
extern int Threshold;
extern double TSstrength;

/** unit defined for digital system but not used in continuous model **/
extern const int one = 1;
extern const int unit = (one<<NUM_DEC_DIGIT_CALCIUM);


// ONLY FOR RESERVOIR
Neuron::Neuron(char * name, bool excitatory, Network * network):
    _mode(NORMAL),
    _excitatory(excitatory),
    _EP_max(INT_MIN), _EP_min(INT_MAX), _EN_max(INT_MIN), _EN_min(INT_MAX), 
    _IP_max(INT_MIN), _IP_min(INT_MAX), _IN_max(INT_MIN), _IN_min(INT_MAX),
    _prev_delta_ep(0), _prev_delta_en(0), _prev_delta_ip(0), _prev_delta_in(0),
    _curr_delta_ep(0), _curr_delta_en(0), _curr_delta_ip(0), _curr_delta_in(0),
    _pre_fire_max(0),
    _lsm_v_mem(0),
    _lsm_v_mem_pre(0),
    _lsm_calcium(0),
    _lsm_calcium_pre(0),
    _lsm_state_EP(0),
    _lsm_state_EN(0),
    _lsm_state_IP(0),
    _lsm_state_IN(0),
    _lsm_tau_EP(4.0*2),
    _lsm_tau_EN(LSM_T_SYNE*2),
    _lsm_tau_IP(4.0*2),
    _lsm_tau_IN(LSM_T_SYNI*2),
    _lsm_tau_FO(LSM_T_FO),
    _lsm_v_thresh(LSM_V_THRESH),
    _ts_strength_p(0),
    _ts_strength_n(0),
    _ts_freq(DEFAULT_TS_FREQ),
    _lsm_ref(0),
    _lsm_channel(NULL),
    _inputsyn_sq_sum(-1),
    _D_lsm_v_mem(0),
    _D_lsm_v_mem_pre(0),
    _D_lsm_calcium(0),
    _D_lsm_calcium_pre(0),
    _D_lsm_state_EP(0),
    _D_lsm_state_EN(0),
    _D_lsm_state_IP(0),
    _D_lsm_state_IN(0),
    _D_lsm_tau_EP(4*2),
    _D_lsm_tau_EN(LSM_T_SYNE*2),
    _D_lsm_tau_IP(4*2),
    _D_lsm_tau_IN(LSM_T_SYNI*2),
    _D_lsm_tau_FO(LSM_T_FO),
    _D_lsm_v_reservoir_max((one<<(NUM_BIT_RESERVOIR_MEM-1)) - 1),
    _D_lsm_v_reservoir_min(-(one<<(NUM_BIT_RESERVOIR_MEM-1))),
    _D_lsm_v_readout_max(-1),
    _D_lsm_v_readout_min(-1),
    _t_next_spike(-1),
    _network(network),
    _Unit(one<<NUM_DEC_DIGIT_RESERVOIR_MEM),
    _teacherSignal(0),
    _ind(-1)
{
    _D_lsm_v_thresh = _Unit*LSM_V_THRESH;

    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _indexInGroup = -1;
    _del = false;
    _f_count = 0;
    _fired = false;
    _error = 0;
}

//FOR INPUT AND OUTPUT
Neuron::Neuron(char * name, bool excitatory, Network * network, double v_mem):
    _mode(NORMAL),
    _excitatory(excitatory),
    _EP_max(INT_MIN), _EP_min(INT_MAX), _EN_max(INT_MIN), _EN_min(INT_MAX), 
    _IP_max(INT_MIN), _IP_min(INT_MAX), _IN_max(INT_MIN), _IN_min(INT_MAX),
    _prev_delta_ep(0), _prev_delta_en(0), _prev_delta_ip(0), _prev_delta_in(0),
    _curr_delta_ep(0), _curr_delta_en(0), _curr_delta_ip(0), _curr_delta_in(0),
    _pre_fire_max(0),
    _lsm_v_mem(0),
    _lsm_v_mem_pre(0),
    _lsm_calcium(0),
    _lsm_calcium_pre(0),
    _lsm_state_EP(0),
    _lsm_state_EN(0),
    _lsm_state_IP(0),
    _lsm_state_IN(0),
    _lsm_tau_EP(4.0*2),
    _lsm_tau_EN(LSM_T_SYNE*2),
    _lsm_tau_IP(4.0*2),
    _lsm_tau_IN(LSM_T_SYNI*2),
    _lsm_tau_FO(LSM_T_FO),
    _lsm_v_thresh(v_mem),
    _ts_strength_p(0),
    _ts_strength_n(0),
    _ts_freq(DEFAULT_TS_FREQ),
    _lsm_ref(0),
    _lsm_channel(NULL),
    _inputsyn_sq_sum(-1),
    _D_lsm_v_mem(0),
    _D_lsm_v_mem_pre(0),
    _D_lsm_calcium(0),
    _D_lsm_calcium_pre(0),
    _D_lsm_state_EP(0),
    _D_lsm_state_EN(0),
    _D_lsm_state_IP(0),
    _D_lsm_state_IN(0),
    _D_lsm_tau_EP(4*2),
    _D_lsm_tau_EN(LSM_T_SYNE*2),
    _D_lsm_tau_IP(4*2),
    _D_lsm_tau_IN(LSM_T_SYNI*2),
    _D_lsm_tau_FO(LSM_T_FO),
    _D_lsm_v_reservoir_max(-1),
    _D_lsm_v_reservoir_min(-1),
    _D_lsm_v_readout_max((one<<(NUM_BIT_READOUT_MEM-1))-1),
    _D_lsm_v_readout_min(-(one<<(NUM_BIT_READOUT_MEM-1))),
    _t_next_spike(-1),
    _network(network),
    _Unit(one<<NUM_DEC_DIGIT_READOUT_MEM),
    _teacherSignal(-1),
    _ind(-1)
{
    _D_lsm_v_thresh = _Unit*v_mem;

    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _indexInGroup = -1;
    _del = false;
    _f_count = 0;
    _fired = false;
    _error = 0;
}

Neuron::~Neuron(){
    if(_name != NULL) delete [] _name;

}

char * Neuron::Name(){
    return _name;
}

void Neuron::AddPostSyn(Synapse * postSyn){
    _outputSyns.push_back(postSyn);
}

void Neuron::AddPreSyn(Synapse * preSyn){
    _inputSyns.push_back(preSyn);
}

void Neuron::LSMPrintInputSyns(ofstream& f_out){
    for(Synapse * synapse : _inputSyns){
#ifdef DIGITAL
        f_out<<synapse->PreNeuron()->Name()<<"\t"<<synapse->PostNeuron()->Name()<<"\t"<<synapse->DWeight()<<endl;
#else
        f_out<<synapse->PreNeuron()->Name()<<"\t"<<synapse->PostNeuron()->Name()<<"\t"<<synapse->Weight()<<endl;
#endif
    }
}

inline void Neuron::SetVth(double vth){
    _lsm_v_thresh = vth;
    _D_lsm_v_thresh = _Unit*vth;
}

inline void Neuron::SetTeacherSignal(int signal){
    assert((signal >= -1) && (signal <= 1));
    _teacherSignal = signal;
}

inline void Neuron::PrintTeacherSignal(){
    cout<<"Teacher signal of "<<_name<<": "<<_teacherSignal<<endl;
}

inline void Neuron::PrintMembranePotential(){
#ifdef DIGITAL
    cout<<"Membrane potential of "<<_name<<": "<<_D_lsm_v_mem<<endl;
#else
    cout<<"Membrane potential of "<<_name<<": "<<_lsm_v_mem<<endl;
#endif
}

//********************************************************************************
//
// This function is to perform power gating for each individual neuron
// If the total weight sum of in-degree reservoir-reservoir connection is <= INDEC_LIMIT
// and the total weight sum of out-degree reservoir-reservoir connection is <= OUTDEC_LIMIT
// then deactivate this neuron.
// @return boolean value indicate whether or not this neuron is deactivated
//
//*******************************************************************************
bool Neuron::PowerGating(){
    int sum_in = 0, sum_out = 0;
    // look at input synapses:
    for(Synapse * synapse : _inputSyns){
        if(synapse->IsLiquidSyn()){
#ifdef DIGITAL
            sum_in += abs(synapse->DWeight());
#else
            sum_in += fabs(synapse->Weight());
#endif
        }
    }

    // look at output synapses:
    for(Synapse * synapse : _outputSyns){
        if(synapse->IsLiquidSyn()){
#ifdef DIGITAL
            sum_out += abs(synapse->DWeight());
#else
            sum_out += fabs(synapse->Weight());
#endif
        }
    }

    assert(sum_in >= 0 && sum_out >= 0);

    if(sum_in <= INDEG_LIMIT && sum_out <= OUTDEG_LIMIT){
        _mode = DEACTIVATED;
        cout<<"The neuron gated: "<<_name<<endl;
        return true;
    }
    else
        return false;
}

//********************************************************************************
//
// This function is to determine whether or not this neuron is the core of the hub
// @return: a boolean value to indicate whether or not the node is the head of hub
// #example: 1->2 1->3 1->4 2->1 3->1 4->1 ------ this is a hub and 1 is the root
//           and 2, 3, 4 are the children
//
//*******************************************************************************
bool Neuron::IsHubRoot(){
    if(_mode == DEACTIVATED)  return false;
    // check all its outgoing synapses:
    int cnt = 0;
    for(Synapse * synapse : _outputSyns){
        if(synapse->IsLiquidSyn() && synapse->PostNeuron()->IsHubChild(_name)){
#ifdef DIGITAL
            if(synapse->DWeight() != 0)
#else
                if(synapse->Weight() != 0.0)
#endif
                    cnt++;
        }
    }
    if(cnt >= 3)
        return true;
    else
        return false;
}

//********************************************************************************
//
// This function is to determine whether or not this neuron is the child of the hub
// @return: a boolean value to indicate whether or not the node is the child of hub
// #example: 1->2 1->3 1->4 2->1 3->1 4->1 ------ this is a hub and 1 is the root
//           and 2, 3, 4 are the children
//
//*******************************************************************************
bool Neuron::IsHubChild(const char * name){
    if(_mode == DEACTIVATED)  return false;
    assert(name && name[0] == 'r' && name[9] == '_');
    // check all its outgoing synapses:
    for(Synapse * synapse : _outputSyns){
        // note that we need to ignore the self-connection when counting the hubs:
        if(synapse->IsLiquidSyn() && strcmp(name, synapse->PostNeuron()->Name()) == 0 && strcmp(name, _name) != 0)
#ifdef DIGITAL
            if(synapse->DWeight() != 0)
#else
                if(synapse->Weight() != 0.0)
#endif
                    return true;
    }
    return false;
}

void Neuron::LSMClear(){
    _f_count = 0;
    _fired = false;
    _error = 0;
    _vmems.clear();
    _calcium_stamp.clear();
    _fire_timings.clear();

    _prev_delta_ep = 0, _prev_delta_en = 0, _prev_delta_ip = 0, _prev_delta_in = 0;
    _curr_delta_ep = 0, _curr_delta_en = 0, _curr_delta_ip = 0, _curr_delta_in = 0;

    _lsm_ref = 0;

    _lsm_v_mem = 0;
    _lsm_v_mem_pre = 0;
    _lsm_calcium = 0;        // both of calicum might be LSM_CAL_MID-3;
    _lsm_calcium_pre = 0; 
    _lsm_state_EP = 0;
    _lsm_state_EN = 0;
    _lsm_state_IP = 0;
    _lsm_state_IN = 0;

    _t_next_spike = -1;
    _teacherSignal = 0;

    _inputsyn_sq_sum = -1;

    _D_lsm_v_mem = 0;
    _D_lsm_v_mem_pre = 0;
    _D_lsm_calcium = 0;
    _D_lsm_calcium_pre = 0;
    _D_lsm_state_EP = 0;
    _D_lsm_state_EN = 0;
    _D_lsm_state_IP = 0;
    _D_lsm_state_IN = 0;
}

//* function wrapper for ExpDecay:
//****** IMPORTANT: Under the low resolution settings, you should not do 
//******            decrease/increase if the abs(var) is less than time const!!
//******            Because if you do so, the changing rate of EP/IP and EN/IN
//******            will be the same and there will be no second order response.
inline void Neuron::ExpDecay(int& var, const int time_c){
    if(var == 0) return;
    if(_name[0] == 'r' && _name[9] == '_'){
#if NUM_DEC_DIGIT_RESERVOIR_MEM < 10
        var -= var/time_c;
#else
        var -= (var/time_c == 0) ? (var > 0 ? 1 : -1) : var/time_c;
#endif
    }
    else{
#if NUM_DEC_DIGIT_READOUT_MEM < 10
        var -= var/time_c;
#else
        var -= (var/time_c == 0) ? (var > 0 ? 1 : -1) : var/time_c;
#endif
    }
#ifndef DIGITAL
    assert(0);
#endif
}

//* function wrapper for ExpDecay under continuous case
//****** IMPORTANT: According to my experiment, the leaking terms seem to 
//******            too large for v_mem. And you need to reduce the leaking
//******            so that good performance is obtained under continuous case.
//******            But without leaking term, the continuous case will not work!
inline void Neuron::ExpDecay(double & var, const int time_c){
    var -= var/time_c;
#ifdef DIGITAL
    assert(0);
#endif
}

//* this is the function wrapper for bound the variable given certain resolution setting
inline void Neuron::BoundVariable(int& var, const int v_min, const int v_max){
    if(var <= v_min) var = v_min;
    if(var >= v_max) var = v_max;
}

/** collect the synaptic response and accumulate them **/
inline void Neuron::AccumulateSynapticResponse(const int pos, double value){
#ifdef DIGITAL
    assert(0);
#endif

#ifdef SYN_ORDER_2
    if(pos > 0){
        _lsm_state_EP += value;
        _lsm_state_EN += value;
    }
    else{
        _lsm_state_IP += value;
        _lsm_state_IN += value;
    }
#elif SYN_ORDER_1
    if(pos > 0) _lsm_state_EP += value;
    else _lsm_state_EP += value;
#else
    if(pos > 0) _lsm_state_EP += value;
    else _lsm_state_EP += value;
#endif

}

/** Digital version of collecting the synaptic response **/
/** Here I am considering if the synapse is the reservoir syn **/
/** For the digital system, I want w*2^(dec) =  w/(2^(n-1)-1)*2^(y) **/
/** That is how I transfer the weight in the discrete case **/
inline void Neuron::DAccumulateSynapticResponse(const int pos, int value, const int c_num_dec_digit_mem, const int c_nbt_std_syn, const int c_num_bit_syn){
#ifndef DIGITAL
    assert(0);
#endif

#ifdef SYN_ORDER_2
    if(pos > 0){
        _D_lsm_state_EP += value;
        _D_lsm_state_EN += value; 
    }
    else{
        _D_lsm_state_IP += value;
        _D_lsm_state_IN += value;
    }
#else 
    if(c_num_dec_digit_mem > c_num_bit_syn - c_nbt_std_syn)
        _D_lsm_state_EP += pos > 0 ? value : -1*value;    
    else _D_lsm_state_EP += pos > 0 ? value : -1*value;
#endif

    /*
    // adopt my earlier code for tuning the resolution:
#ifdef SYN_ORDER_2
if(pos > 0){
if(c_num_dec_digit_mem > c_num_bit_syn - c_nbt_std_syn){
_D_lsm_state_EP += value<<(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn);
_D_lsm_state_EN += value<<(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn);
}
else{
_D_lsm_state_EP += value>>(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem);
_D_lsm_state_EN += value>>(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem);
} 
}
else{
if(c_num_dec_digit_mem > c_num_bit_syn - c_nbt_std_syn){
_D_lsm_state_IP += value<<(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn);
_D_lsm_state_IN += value<<(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn);
}
else{
_D_lsm_state_IP += value>>(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem);
_D_lsm_state_IN += value>>(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem);
}
}
#else 
if(pos > 0){
if(c_num_dec_digit_mem > c_num_bit_syn - c_nbt_std_syn)
_D_lsm_state_EP += pos > 0 ? (value<<(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn)) : -1*(value>>(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn));    
else _D_lsm_state_EP += pos > 0 ? (value<<(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem)) : -1*(value>>(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem));
#endif
*/
}

/** Calculate the whole response together **/
inline double Neuron::NOrderSynapticResponse(){
#ifdef DIGITAL
    assert(0);
#endif
    //if(strcmp(_name, "reservoir_0") == 0)
    //cout<<_lsm_state_EP<<"\t"<<_lsm_state_EN<<"\t"<<_lsm_state_IP<<"\t"<<_lsm_state_IN<<endl;
#ifdef SYN_ORDER_2
    return (_lsm_state_EP-_lsm_state_EN)/(_lsm_tau_EP-_lsm_tau_EN)+(_lsm_state_IP-_lsm_state_IN)/(_lsm_tau_IP-_lsm_tau_IN);
#elif SYN_ORDER_1
    return _lsm_state_EP/_lsm_tau_EP;
#else
    return _lsm_state_EP;
#endif

}

/** Digital version of calculation of the whole response **/
inline int Neuron::DNOrderSynapticResponse(){
#ifndef DIGITAL
    assert(0);
#endif
    //if(strcmp(_name, "reservoir_11") == 0)
    //cout<<_D_lsm_state_EP<<"\t"<<_D_lsm_state_EN<<"\t"<<_D_lsm_state_IP<<"\t"<<_D_lsm_state_IN<<endl;

#ifdef SYN_ORDER_2
    return (_D_lsm_state_EP-_D_lsm_state_EN)/(_D_lsm_tau_EP-_D_lsm_tau_EN)+(_D_lsm_state_IP-_D_lsm_state_IN)/(_D_lsm_tau_IP-_D_lsm_tau_IN);
#elif SYN_ORDER_1
    return _D_lsm_state_EP/_D_lsm_tau_FO;
#else
    return  _D_lsm_state_EP;
#endif

}


//* this function calculate the vmem given the synaptic response
//  I adopt my earlier work to enable resolution tuning 
inline void Neuron::UpdateVmem(int& temp,const int c_num_dec_digit_mem, const int c_nbt_std_syn, const int c_num_bit_syn){
    if(c_num_dec_digit_mem > c_num_bit_syn - c_nbt_std_syn){
        temp = temp<<(c_num_dec_digit_mem+c_nbt_std_syn-c_num_bit_syn);
    }
    else{
        temp = temp>>(c_num_bit_syn-c_nbt_std_syn-c_num_dec_digit_mem);
    } 

    _D_lsm_v_mem += temp;

    if(_name[0] == 'r'){
        assert(_D_lsm_v_readout_min == -1 && _D_lsm_v_readout_max == -1);
        BoundVariable(_D_lsm_v_mem, _D_lsm_v_reservoir_min, _D_lsm_v_reservoir_max);  
    }

    if(_name[0] == 'o'){
        assert(_D_lsm_v_reservoir_min == -1 && _D_lsm_v_reservoir_max == -1);
        BoundVariable(_D_lsm_v_mem, _D_lsm_v_readout_min, _D_lsm_v_readout_max);
    }
}

/****************************************************************************
 * Set the t_post of _inputSyns when the neuron fires and activate the syns 
 * for STDP training.
 * Two cases for synapses: 1) reservoir-reservoir (laterial connection)
 *                         2) input-reservoir & reservoir-readout (inter-layer)
 ***************************************************************************/
inline void Neuron::SetPostNeuronSpikeT(int time){
    for(Synapse * synapse : _inputSyns){
        // indication of two case synapse
        bool betweenLayer = synapse->IsInputSyn() || synapse->IsReadoutSyn();
        if(!betweenLayer)  assert(synapse->IsLiquidSyn());

        if(_mode == STDP && !betweenLayer){
            // this synapse is in reservoir and we are training reservoir synaspes
            synapse->SetPostSpikeT(time);
            // push the synapse into STDP learning list
            if(synapse->GetActiveSTDPStatus() == false 
                    && synapse->PreNeuron()->LSMCheckNeuronMode(DEACTIVATED) == false
                    && synapse->WithinSTDPTable(time) )
                synapse->LSMActivateSTDPSyns(_network, "reservoir");	
        }
        else if((_mode == NORMALSTDP) && betweenLayer){
            // this synapse is in between two layers (input-reservoir or reservoir-readout)
            synapse->SetPostSpikeT(time);
            // push the synapse into STDP learning list
            if(synapse->GetActiveSTDPStatus() == false 
                    && synapse->PreNeuron()->LSMCheckNeuronMode(DEACTIVATED) == false
                    && synapse->WithinSTDPTable(time) )
                if(synapse->IsInputSyn())  synapse->LSMActivateSTDPSyns(_network, "input");
                else if(synapse->IsReadoutSyn())  synapse->LSMActivateSTDPSyns(_network, "readout");
        }
    }
}

/**********************************************************************************
 *  This is a function to handle the firing actvities with regard to postsynaptic-neurons
 *  @para1: is used as a neuron that only output spikes to drive the network
 *          both input and reservoir neurons can be in this case; 
 *  @para2: simulation time; @para3: in supervised training or not
 **********************************************************************************/      
inline void Neuron::HandleFiringActivity(bool isInput, int time, bool train){

    for(Synapse * synapse : _outputSyns){
        // need to get rid of the deactivated neuron !
        if(synapse->PostNeuron()->LSMCheckNeuronMode(DEACTIVATED) == true)  continue;

        // need to get rid of the deactivated synapse !
        if(synapse->DisableStatus())  continue;
    
        // no need to activate/learning the synapse if the post neuron is deactivated
        if(synapse->PostNeuron()->LSMCheckNeuronMode(DEACTIVATED) == true)  continue;
        
        synapse->LSMDeliverSpike();  
        synapse->SetPreSpikeT(time);

        if(_mode == STDP && !isInput){
            // for reservoir-reservoir synapse in TRAINRESERVOIR STATE:
            if(synapse->IsLiquidSyn() == true && synapse->GetActiveSTDPStatus() == false){
                // active this reservoir for STDP training
                synapse->LSMActivateSTDPSyns(_network, "reservoir");	  
            }
        }
        else if(isInput){
            if(_name[0] == 'i'){
                // for input synapses in TRANSIENT/TRAINRESERVOIR/TRAININPUT STATE:
                if(_mode == READCHANNELSTDP){
                    // TRAININPUT STATE (input->READCHANNELSTDP, reservoir->NORMALSTDP)
                    // add the input synapses for STDP training
                    if(synapse->GetActiveSTDPStatus() == false)
                        synapse->LSMActivateSTDPSyns(_network, "input");
                }
                // TRANSIENT/TRAINRESERVOIR STATE, just activate the input synapses
            }
            else{
                // for reservoir->reservoir/output synapse in TRAINREADOUT/READOUTSUPV STATE:
                if(_mode == READCHANNELSTDP){
                    // ignore the reservoir->reservoir synapses
                    if(synapse->PostNeuron()->LSMCheckNeuronMode(READCHANNELSTDP))  continue;
                    // TRAINREADOUT(r->READCHANNELSTDP, o->NORMALSTDP) train readout unsupervisedly
                    // READOUTSUPV(r->READCHANNELSTDP, o->NORMALSTDP) train supervisedly 
#if defined(_REWARD_MODULATE)
                    // ignore the training of the r->o synapses when the pre-synaptic neuron fires
                    if(_network->LSMGetNetworkMode() == READOUTSUPV)   continue;
#endif

                    // add the synapse for STDP training
                    if(synapse->GetActiveSTDPStatus() == false)
                        synapse->LSMActivateSTDPSyns(_network, "readout");
                }
                // for reservoir->reservoir/output synapse in READOUT STATE:
                else if(_mode == READCHANNEL){
                    if(synapse->IsReadoutSyn())
                        synapse->LSMActivate(_network, true, train);
                }    
                // for reservoir->reservoir/output synapse in READOUTBP STATE, just activate
            }
        }
        // For READOUTBP/ TRANSIENT STATE, just activate the synapse
    }

}

//* update the previous delta effect with current delta effect
void Neuron::UpdateDeltaEffect(){
    if(_mode == DEACTIVATED || _mode == READCHANNELSTDP || _mode == READCHANNELBP || _mode == READCHANNEL)
        return;
    
    _prev_delta_ep = _curr_delta_ep;
    _prev_delta_en = _curr_delta_en;
    _prev_delta_ip = _curr_delta_ip;
    _prev_delta_in = _curr_delta_in;
   
    _curr_delta_ep = 0; 
    _curr_delta_en = 0; 
    _curr_delta_ip = 0; 
    _curr_delta_in = 0; 
}

void Neuron::LSMNextTimeStep(int t, FILE * Foutp, bool train, int end_time){

    if(_mode == DEACTIVATED) return;
    if(_mode == READCHANNEL || _mode == READCHANNELSTDP || _mode == READCHANNELBP){
        if(_mode == READCHANNELBP){
            if(_name[0] == 'r' && (t % (2*TAU_C) == 0 || t == end_time))
#ifdef DIGITAL
                _calcium_stamp.push_back(_D_lsm_calcium_pre);
#else
                _calcium_stamp.push_back(_lsm_calcium_pre);
#endif
        }
        if(_mode != READCHANNEL){
            // if training the input-reservoir/reservoir-readout synapses, need to keep track the cal!
#ifdef DIGITAL
            _D_lsm_v_mem_pre = _D_lsm_v_mem;
            _D_lsm_calcium_pre = _D_lsm_calcium;
            _D_lsm_calcium -= _D_lsm_calcium/TAU_C;
#else
            _lsm_v_mem_pre = _lsm_v_mem;
            _lsm_calcium_pre = _lsm_calcium;
            _lsm_calcium -= _lsm_calcium/TAU_C;
#endif
        }
        if(_t_next_spike == -1) return;
        if(t < _t_next_spike) return;
        if(_mode != READCHANNEL){
#ifdef DIGITAL
            _D_lsm_calcium += unit;
#else
            _lsm_calcium += one;
#endif  
        }

#ifdef _DEBUG_INPUT_TRAINING
        if(!strcmp(_name, "reservoir_0"))
            cout<<_name<<" firing @ "<<t<<endl;
#endif

        /** Hand the firing behavior here for both input neurons and reservoir neurons */
        /** @param1: is the neuron only used to output spike? **/
        HandleFiringActivity(true, t, train);
        _fired = true;

        _t_next_spike = _lsm_channel->NextSpikeT();
        if(_t_next_spike == -1) {
            _network->LSMChannelDecrement(_lsm_channel->Mode()); 
        }
        return;
    }

    // the following code is to simulate the mode of NORMAL, WRITECHANNEL, and STDP
#ifdef DIGITAL
    _D_lsm_v_mem_pre = _D_lsm_v_mem;
    _D_lsm_calcium_pre = _D_lsm_calcium;
    _D_lsm_calcium -= _D_lsm_calcium/TAU_C;
#else
    _lsm_v_mem_pre = _lsm_v_mem;
    _lsm_calcium_pre = _lsm_calcium;
    _lsm_calcium -= _lsm_calcium/TAU_C;
#endif

#ifdef DIGITAL
    if((_teacherSignal==1)&&(_D_lsm_calcium < (LSM_CAL_MID+1)*unit)){
        _D_lsm_v_mem += t%_ts_freq == 0 ? _D_lsm_v_thresh*_ts_strength_p : 0;
    }else if((_teacherSignal==-1)&&(_D_lsm_calcium > (LSM_CAL_MID-1)*unit)){
        _D_lsm_v_mem -= t%_ts_freq == 0 ? _D_lsm_v_thresh*_ts_strength_n : 0;
    }
#else
    if((_teacherSignal==1)&&(_lsm_calcium < LSM_CAL_MID+1)){
        _lsm_v_mem += t%_ts_freq == 0 ? 20*_ts_strength_p : 0;
    }else if((_teacherSignal==-1)&&(_lsm_calcium > LSM_CAL_MID-1)){
        _lsm_v_mem -= t%_ts_freq == 0 ? 15*_ts_strength_n : 0;
    }

#endif

    list<Synapse*>::iterator iter;

#ifdef DIGITAL
    /* this is for digital case */
    int pos, value;
    ExpDecay(_D_lsm_v_mem, LSM_T_M);
#ifdef SYN_ORDER_2 
    ExpDecay(_D_lsm_state_EP, _D_lsm_tau_EP);
    ExpDecay(_D_lsm_state_EN, _D_lsm_tau_EN);
    ExpDecay(_D_lsm_state_IP, _D_lsm_tau_IP);
    ExpDecay(_D_lsm_state_IN, _D_lsm_tau_IN);
#elif SYN_ORDER_1
    ExpDecay(_D_lsm_state_EP, _D_lsm_tau_EP);
#elif SYN_ORDER_0
    _D_lsm_state_EP = 0;
#else
    assert(0);
#endif
#else
    /* this is for continuous case: */
    int pos;
    double value;
    ExpDecay(_lsm_v_mem, LSM_T_M_C);
#ifdef SYN_ORDER_2 
    ExpDecay(_lsm_state_EP, _lsm_tau_EP);
    ExpDecay(_lsm_state_EN, _lsm_tau_EN);
    ExpDecay(_lsm_state_IP, _lsm_tau_IP);
    ExpDecay(_lsm_state_IN, _lsm_tau_IN);
#elif SYN_ORDER_1
    ExpDecay(_lsm_state_EP, _lsm_tau_EP);
#elif SYN_ORDER_0
    _lsm_state_EP = 0;
#else
    assert(0);
#endif
#endif  

    // sum up the effect
    _lsm_state_EP += _prev_delta_ep;
    _lsm_state_EN += _prev_delta_en;
    _lsm_state_IP += _prev_delta_ip;
    _lsm_state_IN += _prev_delta_in;

#ifdef DIGITAL
    int temp = DNOrderSynapticResponse();
    // adopt my earlier work to enable resolution tuning.
    if(_name[0] == 'r' && _name[9] == '_'){
        UpdateVmem(temp,NUM_DEC_DIGIT_RESERVOIR_MEM,NBT_STD_SYN_R,NUM_BIT_SYN_R);
    }
    else{
        UpdateVmem(temp,NUM_DEC_DIGIT_READOUT_MEM,NBT_STD_SYN,NUM_BIT_SYN); 
    }
#ifdef _DUMP_RESPONSE
    _vmems.push_back(_lsm_ref > 0 ? 0 : _D_lsm_v_mem);
#endif

#else
    double temp = NOrderSynapticResponse();
    _lsm_v_mem += temp;
#ifdef _DUMP_RESPONSE
    _vmems.push_back(_lsm_ref > 0 ? 0 : _lsm_v_mem);
#endif

#endif

#ifdef _DEBUG_NEURON
    DebugFunc(t);
#endif

    if(_lsm_ref > 0){
        _lsm_ref--;
        _lsm_v_mem = LSM_V_REST;
        _D_lsm_v_mem = LSM_V_REST*_Unit;
        _fired = false;
        return;
    }

    _fired = false;

#ifdef DIGITAL
    if(_D_lsm_v_mem > _D_lsm_v_thresh){  
        _D_lsm_calcium += unit;
#else
    if(_lsm_v_mem > _lsm_v_thresh){
        _lsm_calcium += one;
#endif

#ifdef _DEBUG_INPUT_TRAINING
        if(!strcmp(_name, "output_0"))  cout<<_name<<" firing @: "<<t<<endl;
#endif

        if(_mode == STDP || _mode == NORMALSTDP){
            // keep track of the t_spike_post for the _inputSyns and activate this syn:
            SetPostNeuronSpikeT(t);
        }

        // 1. handle the _outputSyns after the neuron fires and activate the _outputSyns
        // 2. keep track of the t_spike_pre for the corresponding syns
        // 3. @para1: whether or not the current neuron is only used as a dummy input
        HandleFiringActivity(false, t, train);
        _fired = true;

#ifdef DIGITAL
        _D_lsm_v_mem = LSM_V_RESET*_Unit;
#else
        _lsm_v_mem = LSM_V_RESET;
#endif

        _lsm_ref = LSM_T_REFRAC;

#ifdef  _WRITE_STAT
        if(Foutp != NULL && _name[0] == 'o')
            fprintf(Foutp, "%d\t%d\n", _indexInGroup, t); 
#endif
        if(_name[0] == 'o' || _name[0] == 'h'){
            if(t <= end_time/2) _f_count += 1;
            else  ++_f_count;
            assert(t > (_fire_timings.empty() ? -1 : _fire_timings.back()));
            _fire_timings.push_back(t);
        }

        if(_mode == WRITECHANNEL){
            if(_lsm_channel == NULL){
                cout<<"Failure to assign a channel ptr to the neuron: "<<_name<<endl;
                assert(_lsm_channel);
            }
            _lsm_channel->AddSpike(t);
        }

    }
}


double Neuron::LSMSumAbsInputWeights(){
    double sum = 0;
    for(Synapse * synapse : _inputSyns){
        sum += fabs(synapse->Weight());
    }
    return sum;
}

int Neuron::DLSMSumAbsInputWeights(){
    int sum = 0;
    for(Synapse * synapse : _inputSyns){
        sum += abs(synapse->DWeight());
    }
    return sum;
}

void Neuron::LSMSetChannel(Channel * channel, channelmode_t channelmode){
    _lsm_channel = channel;
    _t_next_spike = _lsm_channel->FirstSpikeT();
    if(_t_next_spike == -1 || _mode == DEACTIVATED) _network->LSMChannelDecrement(channelmode);

}

void Neuron::LSMRemoveChannel(){
    _lsm_channel = NULL;
}


//* Get the timing of the spikes
void Neuron::GetSpikeTimes(vector<int>& times){
    if(_name[0] == 'o' || _name[0] == 'h'){
        times = _fire_timings;
    }
    else{
        assert(_lsm_channel);
        _lsm_channel->GetAllSpikes(times);
    }
}

//* Set the timing of the spikes, only valid for the readout neurons
void Neuron::SetSpikeTimes(const vector<int>& times){
    assert(_name[0] == 'o');
    _fire_timings = times;
}

//* Collect the presynaptic neuron firing activity:
void Neuron::CollectPreSynAct(double& p_n, double& avg_i_n, int& max_i_n){
    if(_presyn_act.empty()){
        cout<<"Do you forget to record the pre-synaptic firing activities??\n"
            <<"Or do you mistakenly clear the vector<bool> _presyn_act ?"<<endl;
        assert(!_presyn_act.empty());
    }

    int sum_intvl = 0, cnt_f = 0, max_i = 0;
    int start = -1;
    for(int i = 0; i < _presyn_act.size(); ++i){
        if(_presyn_act[i]){
            ++cnt_f;
            max_i = max(max_i, i - start - 1);
            sum_intvl += i - start - 1;
            start = i;
        }
    }
    max_i = max(max_i, (int)_presyn_act.size() - start - 1);
    sum_intvl += _presyn_act.size() - start - 1;

    p_n = ((double)cnt_f)/((double)_presyn_act.size());
    avg_i_n = ((double)sum_intvl)/(cnt_f+1);
    max_i_n = max_i;
    // clear the presynaptic neuron activity vector after visiting it!
    _presyn_act.clear();
}


//* compute the variance of the firing freq:
double Neuron::ComputeVariance(){
    double avg = 0;
    for(size_t i = 0; i < _fire_freq.size(); ++i)  avg += _fire_freq[i];
    avg = _fire_freq.empty() ? 0 : avg/_fire_freq.size();
    double var = 0;
    for(size_t i = 0; i < _fire_freq.size(); ++i) var += (_fire_freq[i] - avg)*(_fire_freq[i] - avg);

    return  _fire_freq.empty() ? 0 : var/_fire_freq.size();
}

// compute the correlation between neurons at each time step!
// only the paired neurons are considered
void Neuron::ComputeCorrelation(){
    for(Synapse * synapse : _outputSyns){
        assert(synapse);
        if(synapse->IsReadoutSyn()) continue;
        Neuron * post = synapse->PostNeuron();
        assert(post);
        char * name = post->Name();
        assert(name && name[0] == 'r');

        if(_correlation.find(post) == _correlation.end())  _correlation[post] = 0;
        _correlation[post] += (post->Fired() != _fired); // distance ++ is not correlated
    }
}

// sum the correlations for all neuron pairs (i.e., synapses)
void Neuron::CollectCorrelation(int& sum, int & cnt, int num_sample){
    for(auto & p : _correlation){
        char * name = p.first->Name(); 
        if(strcmp(_name, name) == 0)  continue; // ignore the self connections 
        sum += p.second;
        cnt++;
        p.second = p.second/num_sample;
#ifdef _DEBUG_CORBASE
        assert(name && name[0] == 'r');
        cout<<"Correlation between "<<_name<<" and "<<p.first->Name()<<" : "<<p.second<<endl;
#endif
    }
}

// group the correlated neurons using union find
void Neuron::GroupCorrelated(UnionFind & uf, int pre_ind, int thresh){
    uf.add(pre_ind);
    for(auto & p : _correlation){
        int post_ind = p.first->IndexInGroup();
        int correlation = p.second;
        uf.add(post_ind);
        // merge the two neurons if the correlation < thresh, ignore the self-loop
        if(correlation < thresh && post_ind != _indexInGroup){
#ifdef _DEBUG_CORBASE
            cout<<"Correlation between "<<_name<<" and "<<p.first->Name()<<": "<<correlation
                <<" < "<<thresh<<"\tUnion them together!"<<endl;
#endif
            uf.unite(pre_ind, post_ind);
        }
    }
}

// Merge two correlated neurons (this--> the target)
// only support for reservoir now:
void Neuron::MergeNeuron(Neuron * target){
    assert(target);
#ifdef _DEBUG_CORBASE
    cout<<"Start to merge "<<_name<<" into "<<target->Name()<<endl;
#endif

    // 1. Set the pre field of outputs to target neuron
    // merge the modified synapse to the target output field
    // Do not need to change the corresponding input because there is only one synapse!!
    for(Synapse * synapse : _outputSyns){
#ifdef _DEBUG_CORBASE
        cout<<"Merge "<<synapse->PreNeuron()->Name()<<" -> "<<synapse->PostNeuron()->Name()<<endl;
#endif
        synapse->SetPreNeuron(target);
        target->AddPostSyn(synapse);
#ifdef _DEBUG_CORBASE
        cout<<"Into "<<synapse->PreNeuron()->Name()<<" -> "<<synapse->PostNeuron()->Name()<<endl;
#endif
    }

    _outputSyns.clear();

    // 2. Deactivate the this neuron
    this->LSMSetNeuronMode(DEACTIVATED);

}

//* Gather the back-proped error
double Neuron::GatherError(){
    double error = 0;
    for(Synapse * synapse : _outputSyns){
        char * name = synapse->PostNeuron()->Name();
        assert(name[0] == 'o' || name[0] == 'h');
        error += synapse->LSMErrorBack();
    }
    return error;
}

//* Back prop the final error back
void Neuron::BpError(double error, int iteration){
#ifdef _TEST_
    // for testing the accumulative effect of each synapse:
    vector<double> A_k(this->FireCount(), 0.0);
    for(Synapse * synapse : _inputSyns){
        vector<int> post_times, pre_times;
        synapse->PreNeuron()->GetSpikeTimes(pre_times);
        synapse->PostNeuron()->GetSpikeTimes(post_times);
        vector<double> a_k = ComputeAccSRM(pre_times, post_times, 10*LSM_T_M_C, LSM_T_FO, LSM_T_M_C, LSM_T_REFRAC);
        A_k = A_k + synapse->Weight()*a_k;
    }
    for(double a : A_k){
        cout<<"For "<<_name<<", the sum of effects: "<<a<<endl; 
    }
#endif
    vector<int> post_times;
    GetSpikeTimes(post_times);
    for(Synapse * synapse : _inputSyns){
        assert(synapse);
        if(synapse->Fixed())  continue; 
#ifdef DIGITAL
        synapse->LSMBpSynError(error, _D_lsm_v_thresh, iteration, post_times);
#else
        synapse->LSMBpSynError(error, _lsm_v_thresh, iteration, post_times);
#endif
    }
}

//* update the input learning weight
void Neuron::UpdateLWeight(){
    for(Synapse * synapse : _inputSyns){
        synapse->LSMUpdateLearningWeight();
    }
}


//* The function for debug the neuron:
void Neuron::DebugFunc(int t){
    vector<pair<string, double> > pre_names;

    int fire_cnt = 0;
    _EP_max = max(_D_lsm_state_EP, _EP_max), _EN_max = max(_D_lsm_state_EN,_EN_max);
    _IP_max = max(_D_lsm_state_IP, _IP_max), _IN_max = max(_D_lsm_state_IN,_IN_max);
    _EP_min = min(_D_lsm_state_EP, _EP_min), _EN_min = min(_D_lsm_state_EN,_EN_min);
    _IP_min = min(_D_lsm_state_IP, _IP_min), _IN_min = min(_D_lsm_state_IN,_IN_min);

    for(Synapse * synapse : _inputSyns){
        if(synapse->PreNeuron()->Fired()){
#ifdef DIGITAL
            pre_names.push_back(make_pair(string(synapse->PreNeuron()->Name()), synapse->DWeight()));
#else
            pre_names.push_back(make_pair(string(synapse->PreNeuron()->Name()), synapse->Weight()));
#endif

            fire_cnt++;
        }
    }

    _presyn_act.push_back(pre_names.empty() ? false : true);

    _pre_fire_max = max(fire_cnt, _pre_fire_max);
    
    if(strcmp(_name,"output_0")==0 && (_mode == NORMALSTDP || _mode == NORMALBP)){
        cout<<"@time: "<<t<<endl;
        for(vector<pair<string, double> >::iterator it = pre_names.begin(); it != pre_names.end(); ++it){
            cout<<it->first<<" fires with weight "<<it->second<<endl;
        } 
        pre_names.clear();
#ifdef DIGITAL
        int temp = DNOrderSynapticResponse();
        cout<<_D_lsm_v_mem<<"\t"<<temp<<"\t"<<_D_lsm_state_EP<<"\t"<<_D_lsm_state_EN<<"\t"<<_D_lsm_state_IP<<"\t"<<_D_lsm_state_IN<<endl;
#else
        double temp = NOrderSynapticResponse();
        cout<<_lsm_v_mem<<"\t"<<temp<<"\t"<<_lsm_state_EP<<"\t"<<_lsm_state_EN<<"\t"<<_lsm_state_IP<<"\t"<<_lsm_state_IN<<endl;
        cout<<"Calicum level: "<< _lsm_calcium_pre<<endl;
#endif
    }

}


//* write the output synaptic weigths to the file
void Neuron::WriteOutputWeights(ofstream& f_out, int& index, const string& post_g){
    for(Synapse* synapse : _outputSyns){
        assert(synapse);
        string post_name(synapse->PostNeuron()->Name());

        if(post_name.substr(0, post_g.length()) != post_g)   continue;
        f_out<<index++<<"\t"<<_name<<"\t"<<synapse->PostNeuron()->Name()<<"\t"
#ifdef DIGITAL
             <<synapse->DWeight()<<endl;
#else
             <<synapse->Weight()<<endl;
#endif
    }
}


//* this function is to disable the output synapses whose type is syn_t
void Neuron::DisableOutputSyn(synapsetype_t syn_t){
    for(Synapse* synapse : _outputSyns){
        bool flag = syn_t == INPUT_SYN ? synapse->IsInputSyn() :
            syn_t == READOUT_SYN ? synapse->IsReadoutSyn() : 
            syn_t == RESERVOIR_SYN ? synapse->IsLiquidSyn() : false;
        if(flag){
            synapse->DisableStatus(true);
#ifdef _DEBUG_VARBASE
            cout<<"Disable synapse from "<<synapse->PreNeuron()->Name()<<" to "
                <<synapse->PostNeuron()->Name()<<endl;
#endif
        }
    }

}

vector<Synapse*>* Neuron::LSMDisconnectNeuron(){
    Neuron * pre;
    Neuron * post;
    if(_outputSyns.size() == 0){
        cout<<"Warming! The neuron: "<<_name<<" has no output synapses! "<<endl;
        return &_outputSyns;
    }
    for(Synapse* synapse : _outputSyns){
        post = synapse->PostNeuron();
        post->LSMDeleteInputSynapse(_name);  // Delete the pointer of targeted synapse from the inputSyns of the post-synaptic neuron!
        //delete (*iter); // Free the memory
    }
    return &_outputSyns; // Return the output synapse of the deleted neuron. Handle memory realease in network.
}

// erase the ptr of the input syns given the name of the presynaptic neuron
void Neuron::LSMDeleteInputSynapse(char * pre_name){
    Neuron * pre;
    for(auto iter = _inputSyns.begin(); iter != _inputSyns.end(); ){
        pre = (*iter)->PreNeuron();
        assert(pre != NULL);
        //cout<<pre->Name()<<endl;
        if(strcmp(pre->Name(),pre_name) == 0){
#ifdef DEBUG
            cout<<"The synapse from "<<pre_name<<" to "<<_name<<" has benn removed!"<<endl;
#endif
            iter = _inputSyns.erase(iter);
            break;
        }
        else
            iter++;	       
    }
}

// erase the ptr of the synapses whose weight is zero with the given type
// @param1: synapsetype_t , @param2: "in" or "out" synapses
int Neuron::RMZeroSyns(synapsetype_t syn_t, const char * t){
    bool f;
    int cnt = 0;
    if(strcmp(t, "in") == 0)  f = false;
    else if(strcmp(t, "out") == 0)  f = true;
    else assert(0);
    vector<Synapse*>& syns = !f ? _inputSyns : _outputSyns;

    for(auto iter = syns.begin(); iter != syns.end(); ){
        assert(*iter);
        bool flag = syn_t == INPUT_SYN ? (*iter)->IsInputSyn() :
            syn_t == READOUT_SYN ? (*iter)->IsReadoutSyn() : 
            syn_t == RESERVOIR_SYN ? (*iter)->IsLiquidSyn() : false;
        flag &= (*iter)->IsWeightZero();
        if(flag){
#ifdef _DEBUG_RM_ZERO_W
            if(!f)
                cout<<"The synapse from "<<(*iter)->PreNeuron()->Name()<<" to "<<_name<<" has been removed!"<<endl;
            else
                cout<<"The synapse from "<<_name<<" to "<<(*iter)->PostNeuron()->Name()<<" has been removed!"<<endl;
#endif
            (*iter)->DisableStatus(true); 
            iter = syns.erase(iter);
            cnt++;
        }
        else
            iter++;	       
    }
    return cnt;
}


//* This function is to delete all the in/out synapses starts with char 's'
// @param1: the type : "in" or "out"; @param2: the start character
void Neuron::DeleteSyn(const char * t, const char s){
    bool f;
    if(strcmp(t, "in") == 0)  f = false;
    else if(strcmp(t, "out") == 0)  f = true;
    else assert(0);
    vector<Synapse*>& syns = !f ? _inputSyns : _outputSyns;

    // delete all the input synapses starts with 's':
    for(auto it=syns.begin();it != syns.end(); ){
        Neuron * pre = (*it)->PreNeuron();  assert(pre);
        char * pre_name = pre->Name();  assert(pre_name);
        if(pre_name[0] == s)   it = syns.erase(it);
        else ++it;
    }
}

//* This function is to print all the in/out synapses starts with char 's'
//* It is mainly used for debugging purpose.
//* @param1: target file name @param2: "in" or "out"; @param3: the start character 
void Neuron::PrintSyn(ofstream& f_out, const char * t, const char s){
    bool f;
    if(strcmp(t, "in") == 0)  f = false;
    else if(strcmp(t, "out") == 0)  f = true;
    else assert(0);
    vector<Synapse*>& syns = !f ? _inputSyns : _outputSyns;

    // print all the input synapses starts with 's':
    for(auto it=syns.begin();it != syns.end(); ++it){
        Neuron * neu = !f ? (*it)->PreNeuron() : (*it)->PostNeuron();  assert(neu);
        char * name = neu->Name();  assert(name);
        if(name[0] == s){
            if(!f)
                f_out<<name<<"\t"<<_name;
            else
                f_out<<_name<<"\t"<<name;
        }   
    }
    f_out<<endl;
}

void Neuron::DeleteAllSyns(){
    _inputSyns.clear();
    _outputSyns.clear();
    _del = true;
}

void Neuron::DeleteBrokenSyns(){
    Neuron * pre;
    Neuron * post;

    for(auto iter = _inputSyns.begin(); iter != _inputSyns.end(); ){
        pre = (*iter)->PreNeuron();
        post = (*iter)->PostNeuron(); 
        assert(post->GetStatus() == false);
        if(pre->GetStatus() == true)   
            iter = _inputSyns.erase(iter);
        else
            iter++;
    }

    for(auto iter = _outputSyns.begin(); iter != _outputSyns.end(); ){
        pre = (*iter)->PreNeuron();
        post = (*iter)->PostNeuron(); 
        assert(pre->GetStatus() == false);
        if(post->GetStatus() == true)   
            iter = _outputSyns.erase(iter);
        else
            iter++;
    }
}


inline bool Neuron::GetStatus(){
    if(_del == true) return true;
    else return false;
}

//* Get the square sum of all its input synapses
double Neuron::GetInputSynSqSum(double weight_limit){
    if(_inputsyn_sq_sum == -1){
        double sum = 0;
        for(auto it = _inputSyns.begin(); it != _inputSyns.end(); ++it){
#ifdef DIGITAL
            int weight = (*it)->DWeight();
#else
            double weight = (*it)->Weight();
#endif
            sum += (weight/weight_limit)*(weight/weight_limit);
        }
        _inputsyn_sq_sum = _inputSyns.empty() ? 0 : sum/_inputSyns.size();
    }
    return _inputsyn_sq_sum;
}

///////////////////////////////////////////////////////////////////////////

NeuronGroup::NeuronGroup(char * name, int num, Network * network, bool excitatory, double v_mem):
    _firstCalled(false),
    _s_firstCalled(false),
    _lsm_coordinates(0),
    _network(network)
{
    _name = new char[strlen(name)+2];
    strcpy(_name,name);
    _neurons.resize(num);

    char * neuronName = new char[1024];
    for(int i = 0; i < num; i++){
        sprintf(neuronName,"%s_%d",name,i);
        Neuron * neuron = new Neuron(neuronName, excitatory, network, v_mem);
        neuron->SetIndexInGroup(i);
        _neurons[i] = neuron;
    }

    delete [] neuronName;
}

NeuronGroup::NeuronGroup(char * name, char * path_neuron, char * path_synapse, Network * network):
    _firstCalled(false),
    _s_firstCalled(false),
    _network(network)
{
    bool excitatory;
    char ** token = new char*[64];
    char linestring[8192];
    int i,weight_value,count;
    size_t pre_i,post_j;
    FILE * fp_neuron_path = fopen(path_neuron , "r");
    assert(fp_neuron_path != NULL);
    FILE * fp_synapse_path = fopen(path_synapse , "r");
    assert(fp_synapse_path != NULL);

    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _neurons.resize(0);

    while(fgets(linestring, 8191 , fp_neuron_path) != NULL){ //Parse the information of reservoir neurons
        if(strlen(linestring) <= 1)  continue;
        if(linestring[0] == '#')  continue;

        token[0] = strtok(linestring,"\t\n");
        assert(token[0] != NULL);
        token[1] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);
        assert((strcmp(token[1],"excitatory") == 0)||(strcmp(token[1],"inhibitory") == 0));

        if(strcmp(token[1],"excitatory") == 0) excitatory = true;
        else excitatory = false;
        Neuron * neuron = new Neuron(token[0],excitatory,network);
        _neurons.push_back(neuron);        
    }
    //  for(i = 0; i < _neurons.size(); ++i)
    //    cout<<"i:\t"<<i<<"\tname:"<<(*_neurons[i]).Name()<<endl;
    fclose(fp_neuron_path);

    count = 0;
    while(fgets(linestring, 8191, fp_synapse_path) != NULL){ //Parse the information of connectivity within the reservoir
        if(strlen(linestring) <= 1) continue;
        if(linestring[0] == '#')  continue;

        token[0] = strtok(linestring,"\t\n");
        assert(token[0] != NULL);
        token[1] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);
        token[2] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);
        token[3] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);

        pre_i = atoi(token[0]+10);
        post_j = atoi(token[1]+10);
        assert((pre_i <= _neurons.size())&&(post_j <= _neurons.size()));

        //    if(strcmp(token[2],"excitatory") == 0) excitatory = true;
        //    else excitatory = false;

        weight_value = atoi(token[3]);
        //    cout<<"pre_i:\t"<<pre_i<<"\tpost_j:\t"<<post_j<<"\tweight:\t"<<weight_value<<endl;
#ifdef DIGITAL
        _network->LSMAddSynapse(_neurons[pre_i],_neurons[post_j],weight_value,true,8,true, this);
#else
        _network->LSMAddSynapse(_neurons[pre_i], _neurons[post_j], (double)weight_value, true, 8.0, true, this);
#endif
        ++count;
    }
    fclose(fp_synapse_path);

    cout<<"# of Reservoir Synapses = "<<count<<endl;
    delete [] token;

}    




NeuronGroup::NeuronGroup(char * name, int dim1, int dim2, int dim3, Network * network):
    _firstCalled(false),
    _s_firstCalled(false),
    _network(network)
{
    int num = dim1*dim2*dim3;
    bool excitatory;
    int i, j, k, index;
    char * neuronName = new char[1024];
    srand(5);
    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _firstCalled = false;
    _neurons.resize(num);
    _lsm_coordinates = new int*[num];

    // initialization of neurons
    for(i = 0; i < num; i++){
        if(rand()%100 < 20) excitatory = false;
        else excitatory = true;
        sprintf(neuronName,"%s_%d",name,i);
        Neuron * neuron = new Neuron(neuronName,excitatory,network);
        neuron->SetIndexInGroup(i);
        _neurons[i] = neuron;
        _lsm_coordinates[i] = new int[3];
    }

    for(i = 0; i < dim1; i++)
        for(j = 0; j < dim2; j++)
            for(k = 0; k < dim3; k++){
                index = ((i*dim2+j)*dim3)+k;
                _lsm_coordinates[index][0] = i;
                _lsm_coordinates[index][1] = j;
                _lsm_coordinates[index][2] = k;
            }

    // initialization of synapses
    double c, a;
    double distsq, dist;
    const double factor = 10;
    const double factor2 = 1.5;
    int counter = 0;
    for(i = 0; i < num; i++)
        for(j = 0; j < num; j++){
            //      if(i==j) continue;
            if(_neurons[i]->IsExcitatory()){
                if(_neurons[j]->IsExcitatory()){
                    c = 0.3*factor2;
#ifdef LIQUID_SYN_MODIFICATION
                    a = 1;
#else
                    a = 0.3*factor;
#endif
                }else{
                    c = 0.2*factor2;
#ifdef LIQUID_SYN_MODIFICATION
                    a = 1;
#else
                    a = 0.6*factor;
#endif
                }
            }else{
                if(_neurons[j]->IsExcitatory()){
                    c = 0.4*factor2;
#ifdef LIQUID_SYN_MODIFICATION
                    a = -1;
#else
                    a= -0.19*factor;
#endif
                }else{
                    c = 0.1*factor2;
#ifdef LIQUID_SYN_MODIFICATION
                    a = -1;
#else
                    a = -0.19*factor;
#endif
                }
            }

            distsq = 0;
            dist= _lsm_coordinates[i][0]-_lsm_coordinates[j][0];
            distsq += dist*dist;
            dist= _lsm_coordinates[i][1]-_lsm_coordinates[j][1];
            distsq += dist*dist;
            dist= _lsm_coordinates[i][2]-_lsm_coordinates[j][2];
            distsq += dist*dist;
#ifdef PURE_RANDOM_RES_CONNECTION
            if(rand()%100000 < CONNECTION_PROB*100000){ // enable purely random connections
#else
            if(rand()%100000 < 100000*c*exp(-distsq/4)){
#endif
                counter++;
#ifdef DIGITAL
                // this is for digitial liquid state machine:
#ifdef DIGITAL_SYN_ORGINAL
                int stage = 0.6*factor/(one<<(NUM_BIT_SYN_R -1))+1;
                int val = round(a/stage)*1.00001;
                _network->LSMAddSynapse(_neurons[i],_neurons[j],val*stage,true,8,true, this);
#elif LIQUID_SYN_MODIFICATION
                int val = a;
                _network->LSMAddSynapse(_neurons[i],_neurons[j],val,true,8,true, this); 
#else
                assert(0);
#endif
#else
                // this is for the continuous weight case:
                _network->LSMAddSynapse(_neurons[i], _neurons[j],a, true, 8.0, true, this);
#endif
            }
    }

    cout<<"# of reservoir synapses = "<<counter<<endl;

    delete [] neuronName;
    neuronName = 0;
}

NeuronGroup::~NeuronGroup(){
    if(_name != NULL) delete [] _name;
}

// add the synapse (reservoir synapses) into the neurongroup:
void NeuronGroup::AddSynapse(Synapse * synapse){
    assert(synapse && !synapse->IsReadoutSyn()&& !synapse->IsInputSyn());
    _synapses.push_back(synapse);
}

Neuron * NeuronGroup::First(){
    assert(_firstCalled == false);
    _firstCalled = true;
    _iter = _neurons.begin();
    if(_iter != _neurons.end()) return (*_iter);
    else return NULL;
}

Neuron * NeuronGroup::Next(){
    assert(_firstCalled == true);
    if(_iter == _neurons.end()) return NULL;
    _iter++;
    if(_iter != _neurons.end()) return (*_iter);
    else{
        _firstCalled = false;
        return NULL;
    }
}

Synapse * NeuronGroup::FirstSynapse(){
    assert(_s_firstCalled == false);
    _s_firstCalled = true;
    _s_iter = _synapses.begin();
    if(_s_iter != _synapses.end()) return (*_s_iter);
    else return NULL;
}

Synapse * NeuronGroup::NextSynapse(){
    assert(_s_firstCalled == true);
    if(_s_iter == _synapses.end()) return NULL;
    _s_iter++;
    if(_s_iter != _synapses.end()) return (*_s_iter);
    else{
        _s_firstCalled = false;
        return NULL;
    }
}


Neuron * NeuronGroup::Order(int index){
    assert((index >= 0)&&(index < _neurons.size()));
    return _neurons[index];
}

void NeuronGroup::UnlockFirst(){
    _firstCalled = false;
}

void NeuronGroup::UnlockFirstSynapse(){
    _s_firstCalled = false;     
}

void NeuronGroup::PrintTeacherSignal(){
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintTeacherSignal();
}

void NeuronGroup::PrintMembranePotential(double t){
    cout<<t;
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintMembranePotential();
    cout<<endl;
}

//* this function is to load speech into the neuron group:
//* channel mode can be INPUT/RESERVOIR, which is a way to tell the types of the neuron:
void NeuronGroup::LSMLoadSpeech(Speech * speech, int * n_channel, neuronmode_t neuronmode, channelmode_t channelmode){
    assert(speech);
    if(_neurons.size() != speech->NumChannels(channelmode)){
        if(!(channelmode == RESERVOIRCHANNEL && (speech->NumChannels(channelmode) == 0))){
            cout<<"channelmode: "<<channelmode<<" has "<<speech->NumChannels(channelmode)<<" channels!"<<endl;
            assert(channelmode == RESERVOIRCHANNEL && speech->NumChannels(channelmode) == 0);
        }
        if(channelmode == RESERVOIRCHANNEL)
            speech->SetNumReservoirChannel(_neurons.size());
        else
            assert(0); // your code should never go here
    }
    *n_channel = speech->NumChannels(channelmode);
    // assign the channel ptr to each neuron:
    for(int i = 0; i < _neurons.size(); i++){
        _neurons[i]->LSMSetChannel(speech->GetChannel(i,channelmode),channelmode);
    }
    LSMSetNeurons(neuronmode);
}

//* Set the neuron mode for each neuron in the group
void NeuronGroup::LSMSetNeurons(neuronmode_t neuronmode){
    for(int i = 0; i < _neurons.size(); i++){
        _neurons[i]->LSMSetNeuronMode(neuronmode);
    }
}

//* Collect the max/min for E/I/P/N and max # of active pre-spikes for each neuron 
void NeuronGroup::Collect4State(int& ep_max, int& ep_min, int& ip_max, int& ip_min, 
        int& en_max, int& en_min, int& in_max, int& in_min, int& pre_active_max)
{
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        ep_max = max(ep_max, _neurons[i]->GetEPMax()), ep_min = min(ep_min, _neurons[i]->GetEPMin());
        en_max = max(en_max, _neurons[i]->GetENMax()), en_min = min(en_min, _neurons[i]->GetENMin());
        ip_max = max(ip_max, _neurons[i]->GetIPMax()), ip_min = min(ip_min, _neurons[i]->GetIPMin());
        in_max = max(in_max, _neurons[i]->GetINMax()), in_min = min(in_min, _neurons[i]->GetINMin());
        pre_active_max = max(pre_active_max, _neurons[i]->GetPreActiveMax());
    }
}

//* Scatter the collected frequency back to each neuron:
void NeuronGroup::ScatterFreq(vector<double>& fs, size_t& bias, size_t & cnt){
    for(size_t i = 0; i < _neurons.size(); ++i, ++bias){
        assert(_neurons[i]);
        assert(bias < cnt + fs.size());
        _neurons[i]->FireFreq(fs[bias]);
    }
    cnt += _neurons.size();
}


void NeuronGroup::CollectVariance(multimap<double, Neuron*>& my_map){
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        double var = _neurons[i]->ComputeVariance();
        my_map.insert(make_pair(var, _neurons[i]));
    }
}

//* Collect the presynaptic firing activity:
void NeuronGroup::CollectPreSynAct(double & p_r, double & avg_i_r, int & max_i_r){
    double p_n = 0.0, avg_i_n = 0.0;
    int max_i_n = 0;
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        _neurons[i]->CollectPreSynAct(p_n, avg_i_n, max_i_n);
        p_r += p_n,  avg_i_r += avg_i_n, max_i_r = max(max_i_r, max_i_n);
    }
    p_r = _neurons.empty() ? 0 : p_r/((double)_neurons.size());
    avg_i_r = _neurons.empty() ? 0 : avg_i_r/((double)_neurons.size());
}

//* Compute the correlation among neurons:
void NeuronGroup::ComputeCorrelation(){
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        _neurons[i]->ComputeCorrelation();
    }
}

//* merge the correlated neurons using union find, only support for reservoir:
//* @param1: the sample size
void NeuronGroup::MergeCorrelatedNeurons(int num_sample){
    // Step 0. Find the average correlation:
    int sum = 0, cnt = 0;
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->CollectCorrelation(sum, cnt, num_sample);
    }  

    int avg = sum/(cnt*num_sample);
    int thresh = avg*_LEVEL_PERCENT;

    cout<<"The average correlation: "<<avg
        <<"\nThe thresh for correlating: "<<thresh<<endl;

    // Step 1. Use UnionFind to form groups
    UnionFind uf(_neurons.size());
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->GroupCorrelated(uf, i, thresh);
    }

    // Step 2. For group size > 1, merge the output synapses of the grouped neurons
    // to their roots.
    vector<int> sizes = uf.getSizeVec(); // group size
    cnt = 0;
    for(size_t i = 0; i < _neurons.size(); ++i){
        int root = uf.root(i);
        assert(root < _neurons.size());
        if(root != i){

            _neurons[i]->MergeNeuron(_neurons[root]); // merge i into the root
            cnt++;
        }
    }
    cout<<"Correlation based sparsification: "<<cnt<<" neurons are grouped"<<endl;
}

//* judge the results of the readout layer after each speech is presented:
int NeuronGroup::Judge(int cls){
    vector<pair<int, int> > f_pairs;
    auto comp = [](const pair<int, int>& x, const pair<int, int>& y){ return x.first > y.first;};
    for(int i = 0; i < _neurons.size(); ++i){  
        assert(_neurons[i]);
        f_pairs.push_back(make_pair(_neurons[i]->FireCount(), i));
    }
    sort(f_pairs.begin(), f_pairs.end(), comp);
    if(f_pairs.size() < 2){
        cerr<<"Warning: only "<<f_pairs.size()<<" output neurons!!"<<endl;
        return 0;
    }

    int res = f_pairs[0].second;
    if(res == cls && f_pairs[0].first > f_pairs[1].first)  return 1; // correct case
    else if(res == cls && f_pairs[0].first == f_pairs[1].first)  return 0; // even case
    else return -1;  // wrong case
}


//* compute the softmax of the readout neurons:
double NeuronGroup::SoftMax(int max_count){
    double sum = 0;
    for(int i = 0; i < _neurons.size(); ++i){
        double f_cnt = _neurons[i]->FireCount();
        sum += exp(f_cnt - max_count);
    }
    return sum;
}

int NeuronGroup::MaxFireCount(){
    int max_count = -1;
    for(int i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        max_count = max(_neurons[i]->FireCount(), max_count);
    }
    return max_count;
}

//* compute and backprop the error in terms of the spike count/time-discounted reward
void NeuronGroup::BpOutputError(int cls, int iteration, int end_time, double sample_weight){
    int max_count = MaxFireCount();

    double error = 0;
    double sum_error = 0;
    double soft_max_sum = SoftMax(max_count);

#ifdef _DEBUG_BP
    cout<<"True class label: "<<cls<<endl;
#endif
    for(int i = 0; i < _neurons.size(); ++i){
        int f_cnt = _neurons[i]->FireCount();
        // simple ratio based obj
#if     defined(BP_OBJ_RATIO)
        if(max_count != 0)
            error = (i == cls) ? (f_cnt == max_count ? 0 : double(f_cnt)/max_count - 2): 
                //_neurons[i]->FireCount()/double(max_count);
                max(_neurons[i]->FireCount()/double(max_count) - 0.5, 0.0);
        else
            error = (i == cls) ? - 1: 0;
#elif   defined(BP_OBJ_COUNT)
        // simple count based obj
        if(i == cls)
            error = (f_cnt >= DESIRED_LEVEL - MARGIN && f_cnt <= DESIRED_LEVEL + MARGIN) ? 0 : f_cnt - DESIRED_LEVEL;
        else
            error = (f_cnt >= UNDESIRED_LEVEL - MARGIN && f_cnt <= UNDESIRED_LEVEL + MARGIN) ? 0 : f_cnt - UNDESIRED_LEVEL;
#elif   defined(BP_OBJ_SOFTMAX)
        // soft max based obj
        if(i == cls)
            error = exp(f_cnt - max_count)/soft_max_sum - 1;
        else
            error = exp(f_cnt - max_count)/soft_max_sum;
#endif
        error *= sample_weight;
        sum_error += error*error;
#ifdef _DEBUG_BP
        cout<<"The backprop error for neuron "<<i<<" : "<<error<<" with fc: "<<_neurons[i]->FireCount()<<endl;
#endif
        // build a dummmy firing timings for the targeted neuron when there is no spikes
        if(i == cls && f_cnt == 0){
            vector<int> dummy_f_seq = BuildDummyTimes(max_count, end_time);
            _neurons[i]->SetSpikeTimes(dummy_f_seq); 
        }
        // set the error for further bp to other layers
        _neurons[i]->SetError(error);

        if(error == 0)  continue;
        _neurons[i]->BpError(error, iteration);
    }
#ifdef _DEBUG_BP
    cout<<"Current accumulative error: "<<sum_error/2<<endl;
#endif
}

//* back-prop the error down to other layers:
void NeuronGroup::BpHiddenError(int iteration, int end_time, double sample_weight){
    for(int i = 0; i < _neurons.size(); ++i){
        // Gather the back-proped error from l+1 layer
        double error = sample_weight*_neurons[i]->GatherError();
#ifdef _DEBUG_BP_HIDDEN
        cout<<"The gather error for "<<_neurons[i]->Name()<<" : "<<error<<endl;
#endif
        // Set the error for further use
        _neurons[i]->SetError(error);
        // update the weight given the error
        _neurons[i]->BpError(error, iteration);
    }
}

//* compute the back-prop error and print out:
double NeuronGroup::ComputeRatioError(int cls){
    int max_count = MaxFireCount();
    double error = 0, sum_error = 0;
    for(int i = 0; i < _neurons.size(); ++i){
        int f_cnt = _neurons[i]->FireCount();
        if(max_count != 0)  error = (i == cls) - double(f_cnt)/max_count;
        else    error = (i == cls);
        sum_error += error*error;
    }
    return sum_error;
}

//* update the input synapses for each neuron
void NeuronGroup::UpdateLearningWeights(){
    for(int i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        _neurons[i]->UpdateLWeight();
    }
}


//* write the output synapse weights to the file
void NeuronGroup::WriteSynWeightsToFile(ofstream& f_out, int& index, const string& post_g){
    for(Neuron* neuron : _neurons){
        assert(neuron);
        neuron->WriteOutputWeights(f_out, index, post_g);
    }
}


//* dump the spike times for the neurons in the group:
void NeuronGroup::DumpSpikeTimes(const string& filename){
    ofstream f_out(filename.c_str());
    if(!f_out.is_open()){
        cout<<"Cannot file : "<<filename<<endl;
        assert(f_out.is_open());
    }
    f_out<<"-1\t-1"<<endl;
    for(int i = 0; i < _neurons.size(); ++i){
        vector<int> spike_times;
        _neurons[i]->GetSpikeTimes(spike_times);
        for(int time : spike_times) 
            f_out<<i<<"\t"<<time<<endl;    
    }
    f_out<<"-1\t-1"<<endl;
    f_out.close();
}


//* dump the calcium levels of each neuron in the group:
void NeuronGroup::DumpCalciumLevels(ofstream & f_out){
    for(size_t i = 0; i < _neurons.size(); ++i){
#ifdef DIGITAL
        f_out<<"Neuron Index: "<<i<<"\tCalcium level: "<<_neurons[i]->DLSMGetCalcium()<<endl;
#else
        f_out<<"Neuron Index: "<<i<<"\tCalcium level: "<<_neurons[i]->GetCalcium()<<endl;
#endif
    }
}

void NeuronGroup::DumpVMems(ofstream & f_out){
    size_t size = 0;
    for(int i = 0; i < _neurons.size(); ++i){
#ifdef DIGITAL
        vector<int> v;
#else
        vector<double> v;
#endif
        _neurons[i]->GetWaveForm(v);
        if(i == 0)  size = v.size();
        else   assert(size == v.size());
        for(auto e : v)  f_out<<e<<"\t";
        f_out<<endl;
    }
}

void NeuronGroup::PrintSpikeCount(int cls){
    cout<<"The current speech class: "<<cls<<endl;
    for(int i = 0; i < _neurons.size(); ++i){
        cout<<"Neuron "<<i<<" fires "<<_neurons[i]->FireCount()<<endl;
    }    
}

void NeuronGroup::LSMRemoveSpeech(){
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->LSMRemoveChannel();
}

// decrease the vth to enable more firings for the undesired neurons under reward based
// which might be helpful for depression
void NeuronGroup::LSMTuneVth(int index){
    assert((index >= 0)&&(index <_neurons.size()));
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetVth(VMEM_DAMPING_FACTOR*LSM_V_THRESH);
    _neurons[index]->SetVth(LSM_V_THRESH);
}

void NeuronGroup::LSMSetTeacherSignal(int index){
    assert((index >= 0)&&(index < _neurons.size()));
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(-1);
    _neurons[index]->SetTeacherSignal(1);
    //  cout<<"Teacher Signal Set!!!"<<endl;
}

void NeuronGroup::LSMSetTeacherSignalStrength(const double pos, const double neg, const int freq){
    for(int i = 0; i < _neurons.size(); i++){
        _neurons[i]->SetTeacherSignalStrength(pos, true);
        _neurons[i]->SetTeacherSignalStrength(neg, false); 
        _neurons[i]->SetTeacherSignalFreq(freq);
    }
}

void NeuronGroup::LSMRemoveTeacherSignal(int index){
    assert((index >= 0)&&(index < _neurons.size()));
    //  _neurons[index]->SetTeacherSignal(0);
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(0);
    //  cout<<"Teacher Sginal Removed..."<<endl;
}


void NeuronGroup::LSMPrintInputSyns(ofstream & f_out){
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->LSMPrintInputSyns(f_out);
    }
}

//* This function is to delete all the reservoir synapses in this neurongroup
void NeuronGroup::DestroyResConn(){
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->DeleteSyn("in", 'r');
        _neurons[i]->DeleteSyn("out", 'r');
    }
}

//* This function is to print out the reservoir syns:
void NeuronGroup::PrintResSyn(ofstream & f_out){
    assert(f_out.is_open());
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->PrintSyn(f_out,"in", 'r');
        _neurons[i]->PrintSyn(f_out,"out", 'r');
    }
}

//* Remove the outcoming synapses with weight zeros for each neuron
void NeuronGroup::RemoveZeroSyns(synapsetype_t syn_type){
    int cnt = 0;
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        cnt += _neurons[i]->RMZeroSyns(syn_type, "in");
        cnt += _neurons[i]->RMZeroSyns(syn_type, "out");
    }
    cout<<"The number of zero-out synapses: "<<cnt/2<<endl;
}
