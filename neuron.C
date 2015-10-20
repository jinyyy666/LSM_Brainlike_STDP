#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "speech.h"
#include "channel.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//#define _DEBUG_NEURON

using namespace std;

extern int Current;
extern int Threshold;
extern double TSstrength;

/** unit defined for digital system but not used in continuous model **/
extern const int one = 1;
extern const int unit = (one<<NUM_DEC_DIGIT);


// ONLY FOR RESERVOIR
Neuron::Neuron(char * name, bool excitatory, Network * network):
_mode(NORMAL),
_excitatory(excitatory),
_lsm_v_mem(0),
_lsm_v_mem_pre(0),
_lsm_calcium(0),
_lsm_calcium_pre(0),
_lsm_state_EP(0),
_lsm_state_EN(0),
_lsm_state_IP(0),
_lsm_state_IN(0),
_lsm_tau_EP(4.0),
_lsm_tau_EN(LSM_T_SYNE),
_lsm_tau_IP(4.0),
_lsm_tau_IN(LSM_T_SYNI),
_lsm_tau_FO(LSM_T_FO),
_lsm_v_thresh(LSM_V_THRESH),
_lsm_ref(0),
_lsm_channel(NULL),
_D_lsm_v_mem(0),
_D_lsm_v_mem_pre(0),
_D_lsm_calcium(0),
_D_lsm_calcium_pre(0),
_D_lsm_state_EP(0),
_D_lsm_state_EN(0),
_D_lsm_state_IP(0),
_D_lsm_state_IN(0),
_D_lsm_tau_EP(4),
_D_lsm_tau_EN(LSM_T_SYNE),
_D_lsm_tau_IP(4),
_D_lsm_tau_IN(LSM_T_SYNI),
_D_lsm_tau_FO(LSM_T_FO),
_D_lsm_v_thresh(LSM_V_THRESH*unit),
_t_next_spike(-1),
_network(network),
_Unit(unit),
_teacherSignal(0)
{
  _name = new char[strlen(name)+2];
  strcpy(_name,name);

  _indexInGroup = -1;
  _del = false;
}

//FOR INPUT AND OUTPUT
Neuron::Neuron(char * name, Network * network):
_mode(NORMAL),
_excitatory(false),
_lsm_v_mem(0),
_lsm_v_mem_pre(0),
_lsm_calcium(0),
_lsm_calcium_pre(0),
_lsm_state_EP(0),
_lsm_state_EN(0),
_lsm_state_IP(0),
_lsm_state_IN(0),
_lsm_tau_EP(4.0),
_lsm_tau_EN(LSM_T_SYNE),
_lsm_tau_IP(4.0),
_lsm_tau_IN(LSM_T_SYNI),
_lsm_tau_FO(LSM_T_FO),
_lsm_v_thresh(LSM_V_THRESH),
_lsm_ref(0),
_lsm_channel(NULL),
_D_lsm_v_mem(0),
_D_lsm_v_mem_pre(0),
_D_lsm_calcium(0),
_D_lsm_calcium_pre(0),
_D_lsm_state_EP(0),
_D_lsm_state_EN(0),
_D_lsm_state_IP(0),
_D_lsm_state_IN(0),
_D_lsm_tau_EP(4),
_D_lsm_tau_EN(LSM_T_SYNE),
_D_lsm_tau_IP(4),
_D_lsm_tau_IN(LSM_T_SYNI),
_D_lsm_tau_FO(LSM_T_FO),
_D_lsm_v_thresh(LSM_V_THRESH*unit),
_t_next_spike(-1),
_network(network),
_Unit(unit),
_teacherSignal(-1)
{
  _name = new char[strlen(name)+2];
  strcpy(_name,name);

  _indexInGroup = -1;
  _del = false;
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

void Neuron::LSMPrintInputSyns(){
  int counter = 0;
  int counter2 = 0;
  FILE * fp = fopen(_name,"w");
  assert(_inputSyns.size() == 135);
  for(list<Synapse*>::iterator iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
    (*iter)->LSMPrint(fp);
    fprintf(fp,"\t");
    counter++;
    if(counter == 9){
      counter = 0;
      counter2++;
      fprintf(fp,"\n");
    }
    if(counter2 == 15) break;
  }
  fclose(fp);
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

void Neuron::LSMClear(){
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

  _D_lsm_v_mem = 0;
  _D_lsm_v_mem_pre = 0;
  _D_lsm_calcium = 0;
  _D_lsm_calcium_pre = 0;
  _D_lsm_state_EP = 0;
  _D_lsm_state_EN = 0;
  _D_lsm_state_IP = 0;
  _D_lsm_state_IN = 0;
}


inline void Neuron::ExpDecay(int& var, const int time_c){
  var -= (var/time_c == 0) ? 1 : var/time_c;
  if(var < 0) var = 0;
#ifndef DIGITAL
  assert(0);
#endif
}

inline void Neuron::ExpDecay(double & var, const int time_c){
  var -= var/time_c;
#ifdef DIGITAL
  assert(0);
#endif
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
  else _lsm_state_EP -= value;
#else
  if(pos > 0) _lsm_state_EP += value;
  else _lsm_state_EP -= value;
#endif

}

/** Digital version of collecting the synaptic response **/
inline void Neuron::DAccumulateSynapticResponse(const int pos, int value){
#ifndef DIGITAL
  assert(0);
#endif

#ifdef SYN_ORDER_2
  if(pos > 0){
    _D_lsm_state_EP += (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
    _D_lsm_state_EN += (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
  }
  else{
    _D_lsm_state_IP += (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
    _D_lsm_state_IN += (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
  }
#elif SYN_ORDER_1
  if(pos > 0) _D_lsm_state_EP += (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
  else _D_lsm_state_EP -= (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
#else
  if(pos > 0) _D_lsm_state_EP += (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
  else _D_lsm_state_EP -= (value<<(NUM_DEC_DIGIT+NBT_STD_SYN-NUM_BIT_SYN));
#endif

}

/** Calculate the whole response together **/
inline double Neuron::NOrderSynapticResponse(){
#ifdef DIGITAL
  assert(0);
#endif

#ifdef SYN_ORDER_2
  return (_lsm_state_EP-_lsm_state_EN)/(_lsm_tau_EP-_lsm_tau_EN)+(_lsm_state_IP-_lsm_state_IN)/(_lsm_tau_IP-_lsm_tau_IN);
#elif SYN_ORDER_1
  return _lsm_state_EP/_lsm_tau_FO;
#else
  return _lsm_state_EP;
#endif

}

/** Digital version of calculation of the whole response **/
inline int Neuron::DNOrderSynapticResponse(){
#ifndef DIGITAL
  assert(0);
#endif

#ifdef SYN_ORDER_2
  return (_D_lsm_state_EP-_D_lsm_state_EN)/(_D_lsm_tau_EP-_D_lsm_tau_EN)+(_D_lsm_state_IP-_D_lsm_state_IN)/(_D_lsm_tau_IP-_D_lsm_tau_IN);
#elif SYN_ORDER_1
  return _D_lsm_state_EP/_D_lsm_tau_FO;
#else
  return  _D_lsm_state_EP;
#endif

}


/** Set the t_post of _inputSyns when the neuron fires and activate the syns **/
inline void Neuron::SetPostNeuronSpikeT(int time){
  for(list<Synapse*>::iterator iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
    (*iter)->SetPostSpikeT(time);
    
    // if the synapse is not yet active for STDP training
    // push it into the reservoir learning synapse list. 
    // note that I do not train the input layer synapses using STDP at this stage
    // do not train readut synapses too
    if((*iter)->GetActiveSTDPStatus() == false && (*iter)->IsInputSyn() == false && (*iter)->IsReadoutSyn() == false)
      (*iter)->LSMActivateReservoirSyns(_network);
  }
}
      
inline void Neuron::HandleFiringActivity(bool isInput, int time, bool train){

  for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++){
    (*iter)->LSMPreSpike(1);  
    (*iter)->SetPreSpikeT(time);
    
    if(_mode == STDP && !isInput){
      // for excitatory reservoir synapse:
      if((*iter)->IsReadoutSyn() == false){
        // if the synapse is not yet active, push it into the to-be-processed list
	// the second parameter is a flag indicating stdp training in the reservoir
	// the third parameter is a flag indicating train or no
	(*iter)->LSMActivate(_network, true, train);

	// remember to active this reservoir for STDP training
	if((*iter)->GetActiveSTDPStatus() == false)
	  (*iter)->LSMActivateReservoirSyns(_network);	  
      }
    }
    else if(isInput){
      // for input synapses in TRANSIENT/TRAINRESERVOIR STATE:
      if(_name[0] == 'i')
        (*iter)->LSMActivate(_network, false, train);
      else{
        // for reservoir-output synapse in READOUT STATE:
        if((*iter)->IsReadoutSyn())
	  (*iter)->LSMActivate(_network, false, train);
      }
    }
    else{
      // for reservoir-reservoir and reservoir-output in TRANSIENT STATE
      // for possible future synapses coming out from readout in READOUT STATE:
      if((*iter)->GetActiveStatus() == false && (*iter)->IsReadoutSyn() == false)
	(*iter)->LSMActivate(_network, false, train);
    }
	
  }

}
void Neuron::LSMNextTimeStep(int t, FILE * Foutp, FILE * Fp, bool train){

  if(_mode == DEACTIVATED) return;
  if(_mode == READCHANNEL){
    if(_t_next_spike == -1) return;
    if(t < _t_next_spike) return;

    //cout<<_name<<" firing @ "<<t<<"\tinfluencing "<<_outputSyns.size()<<" neurons: "<<(*_outputSyns.begin())->PostNeuron()->Name()<<" ..."<<endl;
    
    /** Hand the firing behavior here for both input neurons and reservoir neurons */
    /** The 1st parameter taken:  isInput **/
    HandleFiringActivity(true, t, train);

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
    _D_lsm_v_mem += 20*unit;
  }else if((_teacherSignal==-1)&&(_D_lsm_calcium > (LSM_CAL_MID-1)*unit)){
    _D_lsm_v_mem -= 15*unit;
  }
#else
  if((_teacherSignal==1)&&(_lsm_calcium < LSM_CAL_MID+1)){
    _lsm_v_mem += 20;
  }else if((_teacherSignal==-1)&&(_lsm_calcium > LSM_CAL_MID-1)){
    _lsm_v_mem -= 15;
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
  ExpDecay(_lsm_v_mem, LSM_T_M);
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
  for(iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
#ifdef DIGITAL
    (*iter)->DLSMStaticCurrent(&pos, &value);
#else
    (*iter)->LSMStaticCurrent(&pos, &value);
#endif
    /*** this is for both reservoir, readout and input synaptic response : ***/
    if(value != 0){
#ifdef DIGITAL
      DAccumulateSynapticResponse(pos, value);
#else
      AccumulateSynapticResponse(pos, value);
#endif
    }
  }
  
#ifdef DIGITAL
  int temp = DNOrderSynapticResponse();
  _D_lsm_v_mem += temp;
#else
  double temp = NOrderSynapticResponse();
  _lsm_v_mem += temp;
#endif


#ifdef _DEBUG_NEURON
  if(strcmp(_name,"reservoir_0")==0) //cout<<_Unit<<"\t"<<_D_lsm_v_thresh<<endl;
#ifdef DIGITAL
     cout<<_D_lsm_v_mem<<"\t"<<temp<<"\t"<<_D_lsm_state_EP<<"\t"<<_D_lsm_state_EN<<"\t"<<_D_lsm_state_IP<<"\t"<<_D_lsm_state_IN<<endl;
#else
    cout<<_lsm_v_mem<<"\t"<<temp<<"\t"<<_lsm_state_EP<<"\t"<<_lsm_state_EN<<"\t"<<_lsm_state_IP<<"\t"<<_lsm_state_IN<<endl;
#endif
#endif

  if(_lsm_ref > 0){
    _lsm_ref--;
    _lsm_v_mem = LSM_V_REST;
    _D_lsm_v_mem = LSM_V_REST*_Unit;
    return;
  }

#ifdef DIGITAL
  if(_D_lsm_v_mem > _D_lsm_v_thresh){  
    _D_lsm_calcium += _Unit;
#else
  if(_lsm_v_mem > _lsm_v_thresh){
    _lsm_calcium += one;
#endif

    if(_mode == STDP){
      // keep track of the t_spike_post for the _inputSyns and activate this syn:
      SetPostNeuronSpikeT(t);
    }

    // 1. handle the _outputSyns after the neuron fires and activate the _outputSyns
    // 2. keep track of the t_spike_pre for the corresponding syns
    // 3. the 1st parameter taken-in means whether or not the current neuron is input
    HandleFiringActivity(false, t, train);

#ifdef DIGITAL
    _D_lsm_v_mem = LSM_V_RESET*_Unit;
#else
    _lsm_v_mem = LSM_V_RESET;
#endif

    _lsm_ref = LSM_T_REFRAC;

    if((_name[0]=='r')&&(_name[9]=='_')){
      if(Fp != NULL) fprintf(Fp,"%d\t%d\n",atoi(_name+10),t);
    }else if(Foutp != NULL) 
	fprintf(Foutp,"%d\t%d\n",atoi(_name+7),t);

    if(_mode == WRITECHANNEL){
      _lsm_channel->AddSpike(t);
    }

  }
}

void Neuron::LSMSetChannel(Channel * channel, channelmode_t channelmode){
  _lsm_channel = channel;
  _t_next_spike = _lsm_channel->FirstSpikeT();
  if(_t_next_spike == -1) _network->LSMChannelDecrement(channelmode);
}

void Neuron::LSMRemoveChannel(){
  _lsm_channel = NULL;
}

/* Remove/Resize the outputs synapse */
void Neuron::ResizeSyn(){
  int n,m,i;
  Neuron * pre;
  Neuron * post;
  char * _name_pre;
  char * _name_post;
  n = _outputSyns.size();
  if((n-10) == 0) return;
  if((n-10) == 1) {
    list<Synapse*>::iterator iter = _outputSyns.begin();
    post = (*iter)->PostNeuron(); 
    _name_post = post->Name();
    assert((_name_post[0] == 'r')&&(_name_post[1] == 'e')&&(_name_post[2] == 's'));
    assert(post->GetStatus() == false);
    iter = _outputSyns.erase(iter);
    return;
  }
  assert(n>= 2);

  m = rand()%(n-10);
  i = 0;

  for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++){
    if(i == m){
      pre = (*iter)->PreNeuron();
      post = (*iter)->PostNeuron(); 
      _name_pre = pre->Name();
      _name_post = post->Name();
      assert((_name_pre[0] == 'r')&&(_name_pre[9] == '_'));
    //  if((_name_post[0] == 'o')&&(_name_post[1] == 't')) continue;
      assert((_name_post[0] == 'r')&&(_name_post[9] == '_'));
      assert(post->GetStatus() == false);   
      iter = _outputSyns.erase(iter);   //Remove the selected synapse.
      
      break;
    }
    else i++;
  }
}


void Neuron::LSMPrintOutputSyns(FILE * fp){
  for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++) (*iter)->LSMPrintSyns(fp);

}

void Neuron::LSMPrintLiquidSyns(FILE * fp){
  Neuron * post;
  char * _name_post;
 
  if((_name[0] == 'r')&&(_name[9] == '_'))
    for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++){
      post = (*iter)->PostNeuron();
      _name_post = post->Name();
      if((_name_post[0] == 'r')&&(_name_post[9] == '_')){  
	assert(fp != NULL);
        fprintf(fp,"%d\t%d\n",atoi(_name+10),atoi(_name_post+10));
      }
      else
	continue;
    }
  return;
}

void Neuron::DeleteAllSyns(){
  _inputSyns.clear();
  _outputSyns.clear();
  _del = true;
}

void Neuron::DeleteBrokenSyns(){
  Neuron * pre;
  Neuron * post;
 
  for(list<Synapse*>::iterator iter = _inputSyns.begin(); iter != _inputSyns.end(); ){
    pre = (*iter)->PreNeuron();
    post = (*iter)->PostNeuron(); 
    assert(post->GetStatus() == false);
    if(pre->GetStatus() == true)   
      iter = _inputSyns.erase(iter);
    else
      iter++;
  }
 
  for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); ){
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


///////////////////////////////////////////////////////////////////////////

NeuronGroup::NeuronGroup(char * name, int num, Network * network):
_lsm_coordinates(0)
{
  _firstCalled = false;
  _name = new char[strlen(name)+2];
  strcpy(_name,name);
  _neurons.resize(num);

  char * neuronName = new char[1024];
  for(int i = 0; i < num; i++){
    sprintf(neuronName,"%s_%d",name,i);
    Neuron * neuron = new Neuron(neuronName,network);
    neuron->SetIndexInGroup(i);
    _neurons[i] = neuron;
  }

  delete [] neuronName;
  neuronName = 0;
}

NeuronGroup::NeuronGroup(char * name, char * path_neuron, char * path_synapse, Network * network):
_network(network),
_firstCalled(false)
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
    _network->LSMAddSynapse(_neurons[pre_i],_neurons[post_j],weight_value,true,8,true);
#else
    _network->LSMAddSynapse(_neurons[pre_i], _neurons[post_j], (double)weight_value, true, 8.0, true);
#endif
    ++count;
  }
  fclose(fp_synapse_path);

  cout<<"# of Reservoir Synapses = "<<count<<endl;
  delete [] token;

}    


    

NeuronGroup::NeuronGroup(char * name, int dim1, int dim2, int dim3, Network * network):
_network(network),
_firstCalled(false)
{
  int num = dim1*dim2*dim3;
  bool excitatory;
  int i, j, k, index;
  char * neuronName = new char[1024];
  srand(9);
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
      if(rand()%100000 < 100000*c*exp(-distsq/4)){
        counter++;
#ifdef DIGITAL
	// this is for digitial liquid state machine:
#ifdef DIGITAL_SYN_ORGINAL
        int stage = 0.6*factor/(one<<(NUM_BIT_RESERVOIR-1))+1;
        int val = round(a/stage)*1.00001;
        _network->LSMAddSynapse(_neurons[i],_neurons[j],val*stage,true,8,false);
#elif LIQUID_SYN_MODIFICATION
	int val = a;
	_network->LSMAddSynapse(_neurons[i],_neurons[j],val,true,8,true); 
#else
	assert(0);
#endif
#else
	// this is for the continuous weight case:
	_network->LSMAddSynapse(_neurons[i], _neurons[j],a, true, 8.0, true);
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

Neuron * NeuronGroup::Order(int index){
  assert((index >= 0)&&(index < _neurons.size()));
  return _neurons[index];
}

void NeuronGroup::UnlockFirst(){
  _firstCalled = false;
}

char * NeuronGroup::Name(){
  return _name;
}

void NeuronGroup::PrintTeacherSignal(){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintTeacherSignal();
}

void NeuronGroup::PrintMembranePotential(double t){
  cout<<t;
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintMembranePotential();
  cout<<endl;
}


//* channel mode can be INPUTCHANNEL or RESERVOIRCHANNEL, which is a way to tell the types of the neuron:
void NeuronGroup::LSMLoadSpeech(Speech * speech, int * n_channel, neuronmode_t neuronmode, channelmode_t channelmode){
  if(_neurons.size() != speech->NumChannels(channelmode)){
    assert((channelmode == RESERVOIRCHANNEL) && (speech->NumChannels(channelmode) == 0));
    speech->SetNumReservoirChannel(_neurons.size());
  }
  *n_channel = speech->NumChannels(channelmode);
  for(int i = 0; i < _neurons.size(); i++){
    _neurons[i]->LSMSetChannel(speech->GetChannel(i,channelmode),channelmode);
    _neurons[i]->LSMSetNeuronMode(neuronmode);
  }
}

void NeuronGroup::LSMRemoveSpeech(){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->LSMRemoveChannel();
}

void NeuronGroup::LSMSetTeacherSignal(int index){
  assert((index >= 0)&&(index < _neurons.size()));
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(-1);
  _neurons[index]->SetTeacherSignal(1);
//  cout<<"Teacher Signal Set!!!"<<endl;
}

void NeuronGroup::LSMRemoveTeacherSignal(int index){
  assert((index >= 0)&&(index < _neurons.size()));
//  _neurons[index]->SetTeacherSignal(0);
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(0);
//  cout<<"Teacher Sginal Removed..."<<endl;
}

void NeuronGroup::LSMPrintInputSyns(){
  for(vector<Neuron*>::iterator iter = _neurons.begin(); iter != _neurons.end(); iter++) (*iter)->LSMPrintInputSyns();
}

