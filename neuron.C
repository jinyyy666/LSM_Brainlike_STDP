#include"def.h"
#include"neuron.h"
#include"synapse.h"
#include"network.h"
#include"pattern.h"
#include"speech.h"
#include"channel.h"
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<assert.h>
#include<string.h>
#include<stdio.h>
#include<math.h>

using namespace std;

extern int Current;
extern int Threshold;
extern double TSstrength;
//extern FILE * Fp;
//extern FILE * Foutp;
extern std::fstream Fs;
extern std::fstream Fouts;
//extern FILE * Fp_syn;

#ifdef DIGITAL
extern const int one = 1;
extern const int unit = (one<<NUM_DEC_DIGIT);
//int unit = 1;
//int Unit = 1 << NUM_DEC_DIGIT;

#endif

// ONLY FOR RESERVOIR
Neuron::Neuron(char * name, bool excitatory, Network * network):
_mode(NORMAL),
_excitatory(excitatory),
_lsm_v_mem(0),
_lsm_v_mem_pre(0),
_lsm_all_inputs(0),
_lsm_t(0),
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
_D_lsm_tau_EP(4*2),
_D_lsm_tau_EN(LSM_T_SYNE*2),
_D_lsm_tau_IP(4*2),
_D_lsm_tau_IN(LSM_T_SYNI*2),
_D_lsm_tau_FO(LSM_T_FO),
_t_next_spike(-1),
_network(network),
_calcium(LSM_CAL_MID-3),
_calcium_pre(LSM_CAL_MID-3),
_Unit(one),
_Lsm_v_thresh(25),
_teacherSignal(0){
  _name = new char[strlen(name)+2];
  strcpy(_name,name);
  _del = false;
}

//FOR INPUT AND OUTPUT
Neuron::Neuron(char * name, Network * network):
_mode(NORMAL),
_lsm_v_mem(0),
_lsm_v_mem_pre(0),
_lsm_all_inputs(0),
_lsm_t(0),
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
_D_lsm_tau_EP(4*2),
_D_lsm_tau_EN(LSM_T_SYNE*2),
_D_lsm_tau_IP(4*2),
_D_lsm_tau_IN(LSM_T_SYNI*2),
_D_lsm_tau_FO(LSM_T_FO),
_t_next_spike(-1),
_network(network),
_calcium(LSM_CAL_MID-3),
_calcium_pre(LSM_CAL_MID-3),
_Unit(unit),
_Lsm_v_thresh(LSM_V_THRESH*unit){
  _name = new char[strlen(name)+2];
  strcpy(_name,name);

  _v_mem = VREST + rand()%(VTHRE-VREST);
  _v_mem_pre = _v_mem;
  _calcium = 0;
  _calcium_pre = _calcium;
  _t = -1000000;
  _externalInput = 0;
  _probe = NULL;
  _voteCounts = NULL;
  _namesInputs = NULL;
  _numNames = -1;
  _indexInGroup = -1;
  _teacherSignal = -1;
  _fired = 0;
  _label = -1;
  _excitatory = false;
  _del = false;
}

Neuron::~Neuron(){
  if(_name != NULL) delete [] _name;
  if(_voteCounts != NULL) delete _voteCounts;
  if(_namesInputs != NULL) delete _namesInputs;
}

void Neuron::SetInput(double input){
  _externalInput = input;
}

void Neuron::ClearInput(){
  _externalInput = 0;
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

int counter = 0;

void Neuron::Update(list<Synapse*> * inputSyns, list<Synapse*> * outputSyns, double t_step, double t){
  if(_excitatory == true){
    _v_mem_pre = _v_mem;
    _calcium_pre = _calcium;

    _calcium *= (1-t_step/TAU_C);

    if(_externalInput > 1e-6){
      if(rand()%10000 < 500*_externalInput) _v_mem = 10000;
      else _v_mem = 0;
    }
    if(_network->HasInputs() == false) _all_inputs = 0;
    _v_mem += _all_inputs;
    _v_mem -= LAMBDA;
    if(_v_mem < VREST) _v_mem = VREST;
    _all_inputs = 0;

#ifdef SUPERVISED
    if(_teacherSignal == 1){
      _v_mem += rand()%81;
    }
    //else if(_probe != NULL) _v_mem = 0;
#endif

    if(_v_mem >= VTHRE){
//if(strcmp(_name, "input_546") == 0)cout<<"firing\t"<<t<<"\t"<<++counter<<"\t"<<500*_externalInput<<"\t"<<_v_mem<<"\t"<<RAND_MAX<<endl;
      _calcium += J_C;
      if(_probe != NULL){
        (_probe->_probeSpiking).push_back(_indexInGroup);
        _fired++;
      }
      _v_mem = VH;
      list<Synapse*>::iterator iter;
      for(iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
        (*iter)->SetPostSpikeT(t);
        inputSyns->push_back(*iter);
      }
      for(iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++){
        (*iter)->SetPreSpikeT(t);
        outputSyns->push_back(*iter);
      }
    }

//if((_name[0]=='o')&&(_name[1]=='u')&&(_name[2]=='t')) cout<<"1234567890"<<(_name+7)<<"\t\t"<<_calcium<<endl;
//if(strcmp(_name, "output_68")==0) cout<<"1234567890"<<"\t\t"<<_calcium<<endl;

  }else{
    _v_mem += _all_inputs;
    _all_inputs = 0;
    if(_v_mem >= VTHRE){
      _v_mem = VH;
      for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++) outputSyns->push_back(*iter);
    }
  }
}

void Neuron::ReceiveSpike(int weight){
//  if(strcmp(_name, "output_3") == 0) cout<<"increase"<<weight<<endl;
  _all_inputs += weight;
}

void Neuron::PrintInputSyns(){
  int counter = 0;
  int counter2 = 0;
  FILE * fp = fopen(_name,"w");
  for(list<Synapse*>::iterator iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
    (*iter)->Print(fp);
    fprintf(fp,"\t"); // cout<<'\t';
    counter++;
    if(counter == 28){
      counter = 0;
      counter2++;
      fprintf(fp,"\n"); // cout<<endl;
    }
    if(counter2 == 28) break;
  }
  fclose(fp);
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

void Neuron::ReadInputSyns(){
  int total_input, i, syn;
  list<Synapse*>::iterator iter;
  ifstream filesyns;
  total_input = 28*28;

  filesyns.open(_name);
  iter = _inputSyns.begin();
  for(int i = 0; i < total_input; i++){
    assert(iter != _inputSyns.end());
    filesyns >> syn;
    (*iter)->SetLearningSynapse(syn);
    iter++;
  }
  filesyns.close();
}

int Neuron::Active(){
  if(_externalInput > 0) return 1;
  else return 0;
}

void Neuron::AddProbe(Probe * probe){
  _probe = probe;
}

void Neuron::Fire(){
  _v_mem = 100000000;
}

void Neuron::SetTeacherSignal(int signal){
  assert((signal >= -1)&&(signal <= 1));
  _teacherSignal = signal;
}

void Neuron::PrintTeacherSignal(){
  cout<<"Teacher signal of "<<_name<<": "<<_teacherSignal<<endl;
}

void Neuron::PrintMembranePotential(){
  //cout<<"Membrane potential of "<<_name<<": "<<_v_mem<<endl;
  cout<<'\t'<<_v_mem;
}

void Neuron::SetUpForUnsupervisedLearning(std::vector<const char*> * nameInputs){
  _numNames = nameInputs->size();
  _voteCounts = new double[_numNames];
  _namesInputs = new const char*[_numNames];

  for(int i = 0; i < _numNames; i++){
    _namesInputs[i] = (*nameInputs)[i];
    _voteCounts[i] = 0;
  }
}

void Neuron::StatAccumulate(){
  if(_probe == NULL) return;
  if(_fired > 0){
    int i;
//    for(i = 0; i < _numNames; i++) _voteCounts[i] *= 0.9;
    for(i = 0; i < _numNames; i++){
      if(strcmp(_network->GetCurrentPattern()->Name(), _namesInputs[i]) == 0) break;
    }
    assert(i < _numNames);
    _voteCounts[i] += 1;
    _fired = 0;
//COUNTER++;
//cout<<"......"<<COUNTER<<endl;
  }
}

int Neuron::PrintTrainingResponse(){
  if(_probe == NULL) return -1;
  cout<<_name;
  for(int i = 0; i < _numNames; i++) cout<<"\t"<<(int)_voteCounts[i];
  if(_label == -1) cout<<"\t\t"<<_label<<endl;
  else cout<<'\t'<<_namesInputs[_label]<<endl;
  return _label;
}

void Neuron::SetLabel(){
  if(_probe == NULL) return;
  int i, maxindex;
  double maxcount = -1;
  maxindex = -1;
  for(i = 0; i < _numNames; i++){
    if((maxcount < _voteCounts[i])&&(_voteCounts[i] > 0)){
      maxcount = _voteCounts[i];
      maxindex = i;
    }
  }
  _label = maxindex;
}

int Neuron::Vote(){
  if((_probe == NULL)||(_fired < 7)) return (-1);
  _fired = 0;
  return _label;
}

double Neuron::GetCalciumPre(){
  return _calcium_pre;
}

int Neuron::DLSMGetCalciumPre(){
  return _D_lsm_calcium_pre;
}

void Neuron::SetExcitatory(){
  _excitatory = true;
  _v_mem = 0;
}

void Neuron::LSMClear(){
  _lsm_v_mem = 0;
  _lsm_all_inputs = 0;
  _lsm_ref = 0;
  _lsm_t = 0;
  _calcium = LSM_CAL_MID-3;
  _lsm_v_mem_pre = 0;
  _calcium_pre = LSM_CAL_MID-3;
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


int Neuron::SizeOutputSyns(){
  return _outputSyns.size();
}

void Neuron::DeleteAllSyns(){
  _inputSyns.clear();
  _outputSyns.clear();
  _del = true;
}

void Neuron::DeleteBrokenSyns(){
  Neuron * pre;
  Neuron * post;
//  char * _name_pre;
//  char * _name_post;
 
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

bool Neuron::GetStatus(){
  if(_del == true) return true;
  else return false;
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
//  cout<<"m = "<<m<<endl;
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


void Neuron::LSMNextTimeStep(int t, FILE * Foutp){
//if(t>=5) assert(0);
//  if(_lsm_channel){
//  int extra=0;
  if(_mode == DEACTIVATED) return;
  if(_mode == READCHANNEL){
    if(_t_next_spike == -1) return;
    if(t < _t_next_spike) return;

//cout<<_name<<" firing @ "<<t<<"\tinfluencing "<<_outputSyns.size()<<" neurons: "<<(*_outputSyns.begin())->PostNeuron()->Name()<<" ..."<<endl;
    for(list<Synapse*>::iterator iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++){
      (*iter)->LSMPreSpike(1);
      (*iter)->LSMActivate(_network);
      (*iter)->SetPreSpikeT(t);
    }
    _t_next_spike = _lsm_channel->NextSpikeT();
    if(_t_next_spike == -1) {
      _network->LSMChannelDecrement(_lsm_channel->Mode()); 
//      fprintf(Fp,"%d\t%d\n",-1,t);
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
  _calcium_pre = _calcium;
  _calcium -= _calcium/TAU_C;
#endif

#ifdef DIGITAL
  if((_teacherSignal==1)&&(_D_lsm_calcium < (LSM_CAL_MID+1)*unit)){
    _D_lsm_v_mem += 20*unit;
  }else if((_teacherSignal==-1)&&(_D_lsm_calcium > (LSM_CAL_MID-1)*unit)){
    _D_lsm_v_mem -= 15*unit;
  }
#else
  if((_teacherSignal==1)&&(_calcium < LSM_CAL_MID+1)){
//cout<<"positive!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
    _lsm_v_mem += 20;
//    _lsm_v_mem += (double(rand()%10000)/5000)*LSM_V_THRESH/7 + 1.6;
  }else if((_teacherSignal==-1)&&(_calcium > LSM_CAL_MID-1)){
//cout<<"negative!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
    _lsm_v_mem -= 10;
  }
//  if((_teacherSignal == 1)&&(rand()%15000 < 1000)){
//    _lsm_v_mem += 1e6;
//  }

//if(_name[0]=='o') cout<<t<<"\t"<<_name<<"\t"<<_teacherSignal<<"\t"<<_calcium<<endl;
#endif

  list<Synapse*>::iterator iter;

#ifdef DIGITAL
  int pos, value;
  bool liquid;
  _D_lsm_v_mem -= ceil(_D_lsm_v_mem/LSM_T_M);
#ifdef SYN_ORDER_2 
  _D_lsm_state_EP -= ceil(_D_lsm_state_EP/_D_lsm_tau_EP);
  _D_lsm_state_EN -= ceil(_D_lsm_state_EN/_D_lsm_tau_EN);
  _D_lsm_state_IP -= ceil(_D_lsm_state_IP/_D_lsm_tau_IP);
  _D_lsm_state_IN -= ceil(_D_lsm_state_IN/_D_lsm_tau_IN);
#elif SYN_ORDER_1
  _D_lsm_state_EP -= _D_lsm_state_EP/_D_lsm_tau_FO;
#elif SYN_ORDER_0
  _D_lsm_state_EP = 0;
#else
  assert(0);
#endif
  for(iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
    (*iter)->DLSMStaticCurrent(&pos,&value);
    // this is for both reservoir and readout:
    if(value != 0){
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
    }
  
#ifdef SYN_ORDER_2
  int temp = (_D_lsm_state_EP-_D_lsm_state_EN)/(_D_lsm_tau_EP-_D_lsm_tau_EN)+(_D_lsm_state_IP-_D_lsm_state_IN)/(_D_lsm_tau_IP-_D_lsm_tau_IN);
#elif SYN_ORDER_1
  int temp = _D_lsm_state_EP/_D_lsm_tau_FO;
#else
  int temp = _D_lsm_state_EP;
#endif
  _D_lsm_v_mem += temp;
//  if((_name[0] == 'r')&&(_name[9] == '_'))
//    if(_D_lsm_v_mem <= -32) _D_lsm_v_mem = -32;
//  if(strcmp(_name,"reservoir_15")==0) //cout<<_Unit<<"\t"<<_Lsm_v_thresh<<endl;
//     cout<<_D_lsm_v_mem<<"\t"<<temp<<"\t"<<_D_lsm_state_EP<<"\t"<<_D_lsm_state_EN<<"\t"<<_D_lsm_state_IP<<"\t"<<_D_lsm_state_IN<<endl;
#else
  _lsm_v_mem -= ceil( _lsm_v_mem/LSM_T_M);
  for(iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++) _lsm_v_mem += (*iter)->LSMCurrent();
#endif
  if(_lsm_ref > 0){
    _lsm_ref--;
    _lsm_v_mem = 0;
    _D_lsm_v_mem = 0;
    return;
  }

#ifdef DIGITAL
  if(_D_lsm_v_mem > (_Lsm_v_thresh)){
      _D_lsm_calcium += _Unit;
#else
  if(_lsm_v_mem > LSM_V_THRESH*_Unit){
    _calcium += 1;
#endif
    if(_mode == STDP){
      // keep track of the t_spike_pre and t_spike_post:
      for(iter = _inputSyns.begin(); iter != _inputSyns.end(); iter++){
	(*iter)->SetPostSpikeT(t);

	// if the synapse is not yet active, push it into the to-be-processed list
	if((*iter)->GetActiveStatus() == false)
	  (*iter)->LSMActivate(_network, true);
      }
    }

    for(iter = _outputSyns.begin(); iter != _outputSyns.end(); iter++){
      (*iter)->LSMPreSpike(1);  
      (*iter)->SetPreSpikeT(t);
    
      if(_mode == STDP){
	// if the synapse is not yet active, push it into the to-be-processed list
	// do not consider the synapses connected from the reservoir to readout:
	if((*iter)->GetActiveStatus() == false && (*iter)->IsReadoutSyn() == false){
	  // the second parameter passed into the function is a flag indicating stdp training in the reservoir:
	  (*iter)->LSMActivate(_network, true);
	}
      }
      else{
	// if the synapse is not yet active, push it into the to-be-processed list:
	if((*iter)->GetActiveStatus() == false)
	  (*iter)->LSMActivate(_network, false);
      }
	
    }

#ifdef DIGITAL
    _D_lsm_v_mem = LSM_V_RESET*_Unit;
#else
    _lsm_v_mem = LSM_V_RESET;
#endif

    _lsm_ref = LSM_T_REFRAC;
    if((_name[0]=='r')&&(_name[9]=='_')){
//      cout<<atoi(_name+10)<<"\t"<<t<<endl;
//     if(Fp != NULL) fprintf(Fp,"%d\t%d\n",atoi(_name+10),t);
    }else if(Foutp != NULL) 
	fprintf(Foutp,"%d\t%d\n",atoi(_name+7),t);

    if(_mode == WRITECHANNEL){
//cout<<"write"<<endl;
      _lsm_channel->AddSpike(t);
    }
//cout<<_name<<"\t"<<t<<endl;
//if(strcmp(_name,"reservoir_5")==0) cout<<"5\t"<<t<<endl;
//if(strcmp(_name,"reservoir_15")==0) cout<<"15\t"<<t<<endl;
//if(strcmp(_name,"reservoir_25")==0) cout<<"25\t"<<t<<endl;
//if(strcmp(_name,"reservoir_35")==0) cout<<"35\t"<<t<<endl;
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
  _subIndices = NULL;
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
    
    _network->LSMAddSynapse(_neurons[pre_i],_neurons[post_j],weight_value,true,8,true);
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
#ifdef DIGITAL_SYN_ORGINAL
        int stage = 0.6*factor/(one<<(NUM_BIT_RESERVOIR-1))+1;
        int val = round(a/stage)*1.00001;
        _network->LSMAddSynapse(_neurons[i],_neurons[j],val*stage,true,8,false);
//#else
//        _network->LSMAddSynapse(_neurons[i],_neurons[j],a,true,0);
#endif

#ifdef LIQUID_SYN_MODIFICATION
	int val = a;
	_network->LSMAddSynapse(_neurons[i],_neurons[j],a,true,8,true); 
#endif

      }
    }

cout<<"# of reservoir synapses = "<<counter<<endl;

  _subIndices = NULL;
  delete [] neuronName;
  neuronName = 0;
}

NeuronGroup::~NeuronGroup(){
  if(_name != NULL) delete [] _name;
  if(_subIndices) delete [] _subIndices;
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

void NeuronGroup::AddProbe(Probe * probe){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->AddProbe(probe);
}

void NeuronGroup::SetTeacherSignal(int signal){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(signal);
}

void NeuronGroup::SetTeacherSignalGroup(int index, int total, int signal){
  for(int i = index; i < _neurons.size(); i += total) _neurons[i]->SetTeacherSignal(signal);
}

void NeuronGroup::SetTeacherSignalOneInGroup(int index, int total, int signal){
  if(_subIndices == NULL){
    _subIndices = new int[total];
    for(int i = 0; i < total; i++) _subIndices[i] = -1;
  }

  assert(signal == 1);
  for(int i = index; i < _neurons.size(); i += total) _neurons[i]->SetTeacherSignal(signal);
  _subIndices[index]++;
  if((index+total*_subIndices[index]) >= _neurons.size()) _subIndices[index] = 0;
  _neurons[(index+total*_subIndices[index])]->SetTeacherSignal(signal);
}

void NeuronGroup::ClearSubIndices(){
  if(_subIndices){
    delete [] _subIndices;
    _subIndices = NULL;
  }
}

void NeuronGroup::PrintTeacherSignal(){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintTeacherSignal();
}

void NeuronGroup::PrintMembranePotential(double t){
  cout<<t;
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintMembranePotential();
  cout<<endl;
}

void NeuronGroup::PrintFiringStat(Probe * probe){
  int i, j;
  for(i = 0; i < probe->_nameInputs.size(); i++){
    cout<<"\""<<probe->_nameInputs[i]<<"\":";
    for(j = 0; j < _neurons.size(); j++) if(_neurons[j]->GetLabel() == i) cout<<"\t"<<_neurons[j]->Fired();
    cout<<endl;
  }
}

void NeuronGroup::ReadInputSyns(){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->ReadInputSyns();
}

void NeuronGroup::SetLabel(int size){
  for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetLabel(i%size);
}

void NeuronGroup::ClearNeuronFiringStat(){
  for(std::vector<Neuron*>::iterator iter =  _neurons.begin(); iter != _neurons.end(); iter++) (*iter)->ClearFiringStat();
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

