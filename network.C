#include "def.h"
#include "neuron.h"
#include "synapse.h"
#include "speech.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

using namespace std;
FILE * Fp_syn;
FILE * Fp_liquid;
FILE * Fp_neuron;
extern double Rate;
int COUNTER_LEARN = 0;

Network::Network():
_fold(0),
_fold_ind(-1),
_CVspeeches(NULL),
_lsm_input_layer(NULL),
_lsm_reservoir_layer(NULL),
_lsm_output_layer(NULL),
_lsm_input(0),
_lsm_reservoir(0),
_lsm_t(0),
_network_mode(VOID)
{
}

Network::~Network(){
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    delete (*iter);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    delete (*iter);
}

bool Network::CheckExistence(char * name){
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),name) == 0) return true;
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),name) == 0) return true;
  return false;
}

void Network::AddNeuron(char * name, bool excitatory){
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);

  Neuron * neuron = new Neuron(name, this);
  _individualNeurons.push_back(neuron);
  _allNeurons.push_back(neuron);
  if(excitatory == true){
    _allExcitatoryNeurons.push_back(neuron);
    neuron->SetExcitatory();
  }else _allInhibitoryNeurons.push_back(neuron);
}

void Network::AddNeuronGroup(char * name, int num, bool excitatory){
  assert(num > 1);
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);

  NeuronGroup * neuronGroup = new NeuronGroup(name, num, this);
  _groupNeurons.push_back(neuronGroup);
  for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next())
    _allNeurons.push_back(neuron);
  if(excitatory == true) for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
    _allExcitatoryNeurons.push_back(neuron);
    neuron->SetExcitatory();
  }
  else for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()) _allInhibitoryNeurons.push_back(neuron);
}

void Network::LSMAddNeuronGroup(char * name, int dim1, int dim2, int dim3){
  assert((dim1>0)&&(dim2>0)&&(dim3>0));
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);

  NeuronGroup * neuronGroup = new NeuronGroup(name, dim1, dim2, dim3, this);

  _groupNeurons.push_back(neuronGroup);
  for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
    _allNeurons.push_back(neuron);
    if(neuron->IsExcitatory()) _allExcitatoryNeurons.push_back(neuron);
    else _allInhibitoryNeurons.push_back(neuron);
  }
}
void Network::LSMAddNeuronGroup(char * name, char * path_info_neuron, char * path_info_synapse){
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);

  NeuronGroup * neuronGroup = new NeuronGroup(name, path_info_neuron, path_info_synapse,this); //This function helps to generate the reservoir with brain-like connectivity
  _groupNeurons.push_back(neuronGroup);

  for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
    _allNeurons.push_back(neuron);
    if(neuron->IsExcitatory()) _allExcitatoryNeurons.push_back(neuron);
    else _allInhibitoryNeurons.push_back(neuron);
  }
}

  
// Please find the fuction to to add the synapses for LSM in the header file: network.h. 
//Function: template<T>  void Network::LSMAddSynapse(Neuron * pre, Neuron * post, T weight, bool fixed, T weight_limit,bool liquid)
// This function is written as a template function.


void Network::LSMAddSynapse(Neuron * pre, NeuronGroup * post, int npost, int value, int random, bool fixed){
  Neuron * neuron;
  double factor;
  double weight_limit;
  bool liquid = false;
  int D_weight_limit;
  if(fixed == true){
    /***** this is for the case of input synapses: *****/
    weight_limit = 0;
    D_weight_limit = 0;
  }else{
    weight_limit = value;
    D_weight_limit = value;
  }

  if(npost < 0){
    assert(npost == -1);
    for(neuron = post->First(); neuron != NULL; neuron = post->Next()){
#ifdef DIGITAL
      if(random == 2){
	int D_weight = -8 + rand()%15;
	LSMAddSynapse(pre,neuron,D_weight,fixed,D_weight_limit,liquid);
      }
      else if(random == 0){
	int D_weight = 0;
	LSMAddSynapse(pre,neuron,D_weight,fixed,D_weight_limit,liquid);
      }else assert(0);
	
#else
      if(random == 2){
	/*** This is for the case of continuous readout weights: ***/
	/*** Remember to change the netlist to                   ***/
	/*** lsmsynapse reservoir output -1 -1 8 2, 2 is random  ***/
	/*** The initial weights are W_max*(-1 to 1)             ***/
        factor = rand()%100000;
        factor = factor*2/99999-1;
      }else if(random == 0) factor = 0;
      else assert(0);
      LSMAddSynapse(pre,neuron,value*factor,fixed,weight_limit,liquid);
#endif
    }
  }else{
    /*** This is for adding input synapses!! ***/
    assert(random == 1);
    for(int i = 0; i < npost; i++){
      neuron = post->Order(rand()%post->Size());
#ifdef DIGITAL
      if(rand()%2==1) LSMAddSynapse(pre,neuron,value,fixed,D_weight_limit,liquid);
      else LSMAddSynapse(pre,neuron,-value,fixed,D_weight_limit,liquid);
#else
      factor = rand()%2;
      factor = factor*2-1;
      LSMAddSynapse(pre,neuron,value*factor,fixed,weight_limit,liquid);
#endif
    }
  }
}

void Network::LSMAddSynapse(NeuronGroup * pre, NeuronGroup * post, int npre, int npost, int value, int random, bool fixed){
  assert((npre == 1)||(npre == -1));
  for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next()){
    LSMAddSynapse(neuron,post,npost,value, random, fixed);
  }
}

void Network::LSMAddSynapse(char * pre, char * post, int npre, int npost, int value, int random, bool fixed){
  NeuronGroup * preNeuronGroup = SearchForNeuronGroup(pre);
  NeuronGroup * postNeuronGroup = SearchForNeuronGroup(post);
  assert((preNeuronGroup != NULL)&&(postNeuronGroup != NULL));

  // add synapses
  LSMAddSynapse(preNeuronGroup, postNeuronGroup, npre, npost, value, random, fixed);
}


void Network::PrintSize(){
  cout<<"Total Number of neurons: "<<_allNeurons.size()<<endl;
  cout<<"Total Number of synapses: "<<_synapses.size()<<endl;
  int numExitatory = 0;
  int numLearning = 0;
  for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++){
    if((*iter)->Excitatory()==true) numExitatory++;
    if((*iter)->Fixed()==false) numLearning++;
  }
  cout<<"\tNumber of excitatory synapses: "<<numExitatory<<endl;
  cout<<"\tNumber of learning synapses: "<<numLearning<<endl;
}

int Network::LSMSumAllOutputSyns(){
  int sum = 0;
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) sum += (*iter)->SizeOutputSyns();
  return sum;
}

void Network::LSMTruncSyns(int loss_synapse){
  int i,j,k;
  char *_name;
  
  LSMPrintAllSyns(0);
  for(i = 0; i < loss_synapse; i++){
    j = 0;
    cout<<"# of all neurons:"<<_allNeurons.size()<<endl;
    k = rand()%(_allNeurons.size()-109) + 83; //Only select synapse in liquid
    cout<<"# of Neurons in liquid: "<< (_allNeurons.size()-87)<<endl;
    cout<<"k = "<<k<<endl;
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter !=_allNeurons.end(); iter++){
      if(j == k){
         _name = (*iter)->Name();
         assert((_name[0] == 'r')&&(_name[1] == 'e')&&(_name[2] == 's'));
         (*iter)->ResizeSyn();
         break;
      }
      else j++;
    }
  }

  //Print all synapses to double check
  LSMPrintAllSyns(1);

} 

void Network::LSMTruncNeurons(int loss_neuron){
 int i,j,k;
 char *_name;
 
 LSMPrintAllLiquidSyns(0);
 LSMPrintAllSyns(0);
 LSMPrintAllNeurons(0);
  for(i = 0; i < loss_neuron; i++){
    j = 0;
//    cout<<"# of all neurons:"<<_allNeurons.size()<<endl;
    k = rand()%(_allNeurons.size()-109) + 83; //Only select neuron in liquid

    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter !=_allNeurons.end(); iter++){
      if(j == k){
         _name = (*iter)->Name();
         assert((_name[0] == 'r')&&(_name[1] == 'e')&&(_name[2] == 's'));
         (*iter)->DeleteAllSyns();
         iter = _allNeurons.erase(iter);
         break;
      }
      else j++;
    }
  }
  
  //Delete the synapses associated with removed neurons
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter !=_allNeurons.end(); iter++) (*iter)->DeleteBrokenSyns();

//Print all synapses to double check
  LSMPrintAllLiquidSyns(1);
  LSMPrintAllSyns(1);
  LSMPrintAllNeurons(1);

}

void Network::LSMPrintAllLiquidSyns(int count){
  char filename[64];
  sprintf(filename,"reservoir_synapse_%d.txt",count);
  Fp_liquid = fopen(filename,"w");
  assert(Fp_liquid != NULL);
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter !=_allNeurons.end(); iter++) (*iter)->LSMPrintLiquidSyns(Fp_liquid);
  fclose(Fp_liquid);
}


int Network::LSMSizeAllNeurons(){
  return _allNeurons.size();
}

void Network::LSMPrintAllSyns(int count){
  char filename[64]; 
  sprintf(filename,"all_synapse_%d.txt",count);
  Fp_syn = fopen(filename,"w");
  assert(Fp_syn != NULL);
  
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter !=_allNeurons.end(); iter++) (*iter)->LSMPrintOutputSyns(Fp_syn);
  fclose(Fp_syn);
}

void Network::LSMPrintAllNeurons(int count){
  char filename[64];
  char * _name; 
  sprintf(filename,"l_neuron_%d.txt",count);
  Fp_neuron = fopen(filename,"w");
  assert(Fp_neuron != NULL);
  
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter !=_allNeurons.end(); iter++){
   _name = (*iter)->Name();
   if(Fp_neuron != NULL) fprintf(Fp_neuron,"%s\n",_name);
  }
  fclose(Fp_neuron);
}


void Network::LSMNextTimeStep(int t, bool train,int iteration, FILE * Foutp, FILE * Fp){
//  struct timeval val1,val2,val3;

  _lsm_t = t;


  for(list<Synapse*>::iterator iter = _lsmActiveSyns.begin(); iter != _lsmActiveSyns.end(); iter++) (*iter)->LSMNextTimeStep();
  _lsmActiveSyns.clear();

  // Please remember that the neuron firing activity will change the list: 
  // _lsmActiveSyns, _lsmActiveReservoirLearnSyns, and _lsmActiveLearnSyns:
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->LSMNextTimeStep(t, Foutp, Fp, train);
 
  // train the reservoir using STDP:
  if(_network_mode == TRAINRESERVOIR){
    for(vector<Synapse*>::iterator iter = _rsynapses.begin(); iter != _rsynapses.end(); ++iter){
      // Update the local variable implememnting STDP with triplet pairing scheme:
      (*iter)->LSMUpdate(t);        
    }
      
    for(list<Synapse*>::iterator iter = _lsmActiveReservoirLearnSyns.begin(); iter != _lsmActiveReservoirLearnSyns.end(); iter++){   
      // train the reservoir synapse with STDP rule:
      (*iter)->LSMLiquidLearn(t);
    }
    _lsmActiveReservoirLearnSyns.clear();
  }
    
//  cout<<t<<"\t"<<((val2.tv_sec-val1.tv_sec)+double(val2.tv_usec-val1.tv_usec)*1e-6)<<"\t"<<((val3.tv_sec-val2.tv_sec)+double(val3.tv_usec-val2.tv_usec)*1e-6)<<endl;
  if(train == true) {
    for(list<Synapse*>::iterator iter = _lsmActiveLearnSyns.begin(); iter != _lsmActiveLearnSyns.end(); iter++) (*iter)->LSMLearn(iteration);
//    cout<<"learning..."<<endl;
  }
  _lsmActiveLearnSyns.clear();
}


Neuron * Network::SearchForNeuron(const char * name){
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),name) == 0){
      return (*iter);
    }

  return NULL;
}

Neuron * Network::SearchForNeuron(const char * name1, const char * name2){
  char * name = new char[strlen(name1)+strlen(name2)+2];
  sprintf(name,"%s_%s",name1,name2);

  list<NeuronGroup*>::iterator iter;
  Neuron * neuron;

  for(iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),name1) == 0) break;

  assert(iter != _groupNeurons.end());
  for(neuron = (*iter)->First(); neuron != NULL; neuron = (*iter)->Next())
    if(strcmp(name,neuron->Name()) == 0){
      (*iter)->UnlockFirst();
      delete [] name;
      return neuron;
    }

  assert(neuron != NULL);
}


NeuronGroup * Network::SearchForNeuronGroup(const char * name){
  list<NeuronGroup*>::iterator iter;

  for(iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),name) == 0) break;

  if(iter == _groupNeurons.end()) return NULL;
  return (*iter);
}


void Network::AddSpeech(Speech * speech){
  _speeches.push_back(speech);
}

int Network::LoadFirstSpeech(bool train, networkmode_t networkmode){
  assert(_lsm_input_layer == NULL);
  _lsm_input_layer = SearchForNeuronGroup("input");
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer == NULL);
  _lsm_reservoir_layer = SearchForNeuronGroup("reservoir");
  assert(_lsm_reservoir_layer != NULL);
  _lsm_output_layer = SearchForNeuronGroup("output");
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input, neuronmode_reservoir;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir); 
  

  _sp_iter = _speeches.begin();
  if(_sp_iter != _speeches.end()){
    _lsm_input_layer->LSMLoadSpeech(*_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
    _lsm_reservoir_layer->LSMLoadSpeech(*_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    if(train == true) _lsm_output_layer->LSMSetTeacherSignal((*_sp_iter)->Class());
    return (*_sp_iter)->Index();
  }else{
    return -1;
  }
}

int Network::LoadNextSpeech(bool train, networkmode_t networkmode_type){
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer != NULL);
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input, neuronmode_reservoir;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode_type, neuronmode_input, neuronmode_reservoir); 
  

  if(train == true) _lsm_output_layer->LSMRemoveTeacherSignal((*_sp_iter)->Class());
  _sp_iter++;
  if(_sp_iter != _speeches.end()){
    _lsm_input_layer->LSMLoadSpeech(*_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
    _lsm_reservoir_layer->LSMLoadSpeech(*_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    if(train == true) _lsm_output_layer->LSMSetTeacherSignal((*_sp_iter)->Class());
//    cout<<"Target cls:"<<(*_sp_iter)->Class()<<endl; 
//    _lsm_output_layer->LSMTeacherSignalPrint(); //Print TS
    return (*_sp_iter)->Index();
  }else{
    _lsm_input_layer->LSMRemoveSpeech();
    _lsm_input_layer = NULL;
    _lsm_reservoir_layer->LSMRemoveSpeech();
    _lsm_reservoir_layer = NULL;
    return -1;
  }
}


void Network::PrintAllNeuronName(){
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) cout<<(*iter)->Name()<<endl;
}



void Network::AnalogToSpike(){
  int counter = 0;
  for(vector<Speech*>::iterator iter = _speeches.begin(); iter != _speeches.end(); iter++){
//    cout<<"Preprocessing speech "<<counter++<<"..."<<endl;
    counter++;
    (*iter)->AnalogToSpike();
  }
}

void Network::LSMClearSignals(){
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->LSMClear();
  for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClear();
  // Remember to clear the list to store the synapses:      
  _lsmActiveLearnSyns.clear();                              
  _lsmActiveSyns.clear();
  _lsmActiveReservoirLearnSyns.clear(); 
}
void Network::LSMClearWeights(){
  for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClearLearningSynWeights();
}
bool Network::LSMEndOfSpeech(networkmode_t networkmode){
/*
  if(_lsm_t < 1000) return false;
  else{
    _lsm_t = 0;
    return true;
  }
*/
  if((networkmode == TRAINRESERVOIR)&&(_lsm_input>0)){
    return false;
  }

  if((networkmode == TRANSIENTSTATE)&&(_lsm_input>0)){
    return false;
  }
  if((networkmode == READOUT)&&(_lsm_reservoir>0)){
    return false;
  }
  return true;
}

void Network::SpeechInfo(){
  (*_sp_iter)->Info();
}

void Network::SpeechPrint(int info){
  (*_sp_iter)->PrintSpikes(info);
}

void Network::LSMChannelDecrement(channelmode_t channelmode){
  if(channelmode == INPUTCHANNEL) _lsm_input--;
  else _lsm_reservoir--;
}


//* add the active reservoir synapse about to be trained by STDP
void Network::LSMAddReservoirActiveSyn(Synapse * synapse){
  assert(synapse->GetActiveStatus() == false);
  _lsmActiveReservoirLearnSyns.push_back(synapse);
}

void Network::CrossValidation(int fold){
  int i, j, k, cls, sample;
  std::srand(0);

  assert(fold>1);
  _fold = fold;
  _CVspeeches = new vector<Speech*>*[fold];
  for(k = 0; k < fold; k++) _CVspeeches[k] = new vector<Speech*>;
  cls = CLS;
  sample = _speeches.size();
  cout<<"sample: "<<sample<<endl;
  vector<int> ** index;
  index = new vector<int>*[cls];
  for(i = 0; i < cls; i++){
    index[i] = new vector<int>;
    for(j = 0; j < sample/cls; j++) index[i]->push_back(i+j*cls);
    random_shuffle(index[i]->begin(),index[i]->end());
  }
  
  for(k = 0; k < fold; k++)
    for(j = (k*sample)/(cls*fold); j < ((k+1)*sample)/(cls*fold); j++)
      for(i = 0; i < cls; i++){
        _CVspeeches[k]->push_back(_speeches[(*index[i])[j]]);
      }

  for(i = 0; i < cls; i++) delete index[i];
  delete [] index;
}

int Network::LoadFirstSpeechTrainCV(networkmode_t networkmode){
  assert((_fold_ind>=0)&&(_fold_ind<_fold));

  assert(_lsm_input_layer == NULL);
  _lsm_input_layer = SearchForNeuronGroup("input");
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer == NULL);
  _lsm_reservoir_layer = SearchForNeuronGroup("reservoir");
  assert(_lsm_reservoir_layer != NULL);
  _lsm_output_layer = SearchForNeuronGroup("output");
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir); 
    
  if(_fold_ind == 0) _train_fold_ind = 1;
  else _train_fold_ind = 0;
  _cv_train_sp_iter = _CVspeeches[_train_fold_ind]->begin();
  if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind]->end()){
    _lsm_input_layer->LSMLoadSpeech(*_cv_train_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
    _lsm_reservoir_layer->LSMLoadSpeech(*_cv_train_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    _lsm_output_layer->LSMSetTeacherSignal((*_cv_train_sp_iter)->Class());
    return (*_cv_train_sp_iter)->Index();
  }else{
    assert(0);
    return -1;
  }
}

int Network::LoadNextSpeechTrainCV(networkmode_t networkmode){
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer != NULL);
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir); 

  _lsm_output_layer->LSMRemoveTeacherSignal((*_cv_train_sp_iter)->Class());
  _cv_train_sp_iter++;
  if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind]->end()){
    _lsm_input_layer->LSMLoadSpeech(*_cv_train_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
    _lsm_reservoir_layer->LSMLoadSpeech(*_cv_train_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    _lsm_output_layer->LSMSetTeacherSignal((*_cv_train_sp_iter)->Class());
    return (*_cv_train_sp_iter)->Index();
  }else{
    ++_train_fold_ind;
    if(_train_fold_ind == _fold_ind) {  ++_train_fold_ind;
//     cout<<"Opp! Sorry about that! LEt's change _train_fold_ind!"<<endl;
    }
    if(_train_fold_ind < _fold){
      _cv_train_sp_iter = _CVspeeches[_train_fold_ind]->begin();
      if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind]->end()){
        _lsm_input_layer->LSMLoadSpeech(*_cv_train_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
        _lsm_reservoir_layer->LSMLoadSpeech(*_cv_train_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
        _lsm_output_layer->LSMSetTeacherSignal((*_cv_train_sp_iter)->Class());
        return (*_cv_train_sp_iter)->Index();
      }else{
        assert(0);
        return -1;
      }
    }else{
      _lsm_input_layer->LSMRemoveSpeech();
      _lsm_input_layer = NULL;
      _lsm_reservoir_layer->LSMRemoveSpeech();
      _lsm_reservoir_layer = NULL;
      _lsm_output_layer = NULL;
      return -1;
    }
  }
}

int Network::LoadFirstSpeechTestCV(networkmode_t networkmode){
  assert((_fold_ind>=0)&&(_fold_ind<_fold));

  assert(_lsm_input_layer == NULL);
  _lsm_input_layer = SearchForNeuronGroup("input");
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer == NULL);
  _lsm_reservoir_layer = SearchForNeuronGroup("reservoir");
  assert(_lsm_reservoir_layer != NULL);
  _lsm_output_layer = SearchForNeuronGroup("output");
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL;
  // determine the neuron operation mode by network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir);
  
  _cv_test_sp_iter = _CVspeeches[_fold_ind]->begin();
  if(_cv_test_sp_iter != _CVspeeches[_fold_ind]->end()){
    _lsm_input_layer->LSMLoadSpeech(*_cv_test_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
    _lsm_reservoir_layer->LSMLoadSpeech(*_cv_test_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    return (*_cv_test_sp_iter)->Index();
  }else{
    return -1;
  }
}

int Network::LoadNextSpeechTestCV(networkmode_t networkmode){
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer != NULL);
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL;
  // determine the neuron operation mode by networkmode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir);
 
  _cv_test_sp_iter++;
  if(_cv_test_sp_iter != _CVspeeches[_fold_ind]->end()){
    _lsm_input_layer->LSMLoadSpeech(*_cv_test_sp_iter,&_lsm_input,neuronmode_input,INPUTCHANNEL);
    _lsm_reservoir_layer->LSMLoadSpeech(*_cv_test_sp_iter,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    return (*_cv_test_sp_iter)->Index();
  }else{
    _lsm_input_layer->LSMRemoveSpeech();
    _lsm_input_layer = NULL;
    _lsm_reservoir_layer->LSMRemoveSpeech();
    _lsm_reservoir_layer = NULL;
    _lsm_output_layer = NULL;
    return -1;
  }
}

void Network::DetermineNetworkNeuronMode(const networkmode_t & networkmode, neuronmode_t & neuronmode_input, neuronmode_t & neuronmode_reservoir){
  if(networkmode == TRANSIENTSTATE){
    neuronmode_input = READCHANNEL;
    neuronmode_reservoir = WRITECHANNEL;
  }else if(networkmode == TRAINRESERVOIR){
    neuronmode_input = READCHANNEL;
    neuronmode_reservoir = STDP;
  }else if(networkmode == READOUT){
    neuronmode_input = DEACTIVATED;
    neuronmode_reservoir = READCHANNEL;
  }else{
    cout<<"Unrecognized network mode!"<<endl;
    exit(EXIT_FAILURE);
  }

}

void Network::IndexSpeech(){
  for(int i = 0; i < _speeches.size(); i++) _speeches[i]->SetIndex(i);
}


//* This function is to write the synaptic weights back into a file:
//* "syn_type" can be reservoir or readout
void Network::WriteSynWeightsToFile(const char * syn_type, char * filename){
  ofstream f_out(filename);
  if(!f_out.is_open()){
    cout<<"In Network::WriteSynWeightsToFile(), cannot open the file : "<<filename<<endl;
    exit(EXIT_FAILURE);
  }
  
  synapsetype_t  wrt_syn_type = INVALID;
  if(strcmp(syn_type, "reservoir") == 0){
    // Write reservoir syns into the file
    wrt_syn_type = RESERVOIR_SYN;
  }
  else if(strcmp(syn_type, "readout") == 0){
    wrt_syn_type = READOUT_SYN;
  }
  else{
    cout<<"In Network::WriteSynWeightsToFile(), undefined synapse type: "<<syn_type<<endl;
    exit(EXIT_FAILURE);
  }
  
  const vector<Synapse*> & synapses = wrt_syn_type == RESERVOIR_SYN ? _rsynapses : 
    (wrt_syn_type == READOUT_SYN ? _rosynapses : vector<Synapse*>());
  
  // write the synapse back to the file:
  
  assert(!synapses.empty());
    
  for(size_t i = 0; i < synapses.size(); ++i)
    f_out<<i<<"\t"<<synapses[i]->PreNeuron()->Name()
	 <<"\t"<<synapses[i]->PostNeuron()->Name()
#ifdef DIGITAL
	 <<"\t"<<synapses[i]->DWeight()
#else
	 <<"\t"<<synapses[i]->Weight()
#endif
	 <<endl;

}

//* This function is to load the synaptic weights from file and assign it to the synapses:
//* "syn_type" can be reservoir or readout
void Network::LoadSynWeightsFromFile(const char * syn_type, char * filename){
  ifstream f_in(filename);
  if(!f_in.is_open()){
    cout<<"In Network::LoadSynWeightsFromFile(), cannot open the file: "<<filename<<endl;
    exit(EXIT_FAILURE);
  }

  synapsetype_t read_syn_type = INVALID;
  if(strcmp(syn_type, "reservoir") == 0){
    read_syn_type = RESERVOIR_SYN;
  }
  else if(strcmp(syn_type, "readout") == 0){
    read_syn_type = READOUT_SYN;
  }
  else{
    cout<<"In Network::LoadSynWeightsFromFile(), undefined synapse type: "<<syn_type<<endl;
    exit(EXIT_FAILURE);
  }

  const vector<Synapse*> & synapses = read_syn_type == RESERVOIR_SYN ? _rsynapses : 
    (read_syn_type == READOUT_SYN ? _rosynapses : vector<Synapse*>());

  // load the synaptic weights from file:
  // the synaptic weight is stored in the file in the same order as the corresponding synapses store in the vector
  int index;
  string pre;
  string post;
#ifdef DIGITAL
  int weight;
#else
  double weight;
#endif
  while(f_in>>index>>pre>>post>>weight){
    if(index < 0 || index >= synapses.size()){
	cout<<"In Network::LoadSynWeightsFromFile(), the index of the synapse you read : "<<index<<" is out of bound of the container stores the synapses!!"<<endl;
	exit(EXIT_FAILURE);
    }

    assert(strcmp(pre.c_str(), synapses[index]->PreNeuron()->Name()) == 0);
    assert(strcmp(post.c_str(), synapses[index]->PostNeuron()->Name()) == 0);
    synapses[index]->Weight(weight);
  }
  
}
