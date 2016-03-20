#include "def.h"
#include "neuron.h"
#include "synapse.h"
#include "speech.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

//#define _DEBUG_NETWORK_SEPARATE
//#define _DEBUG_VARBASE

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
  assert(num >= 1);
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);

  NeuronGroup * neuronGroup = new NeuronGroup(name, num, this);
  _groupNeurons.push_back(neuronGroup);
  for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
    _allNeurons.push_back(neuron);
    if(strcmp(name,"input") == 0)
	_inputNeurons.push_back(neuron);
    if(strcmp(name,"output") == 0)
	_outputNeurons.push_back(neuron);
  }
  if(excitatory == true) for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
    _allExcitatoryNeurons.push_back(neuron);
    neuron->SetExcitatory();
  }
  else for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()) _allInhibitoryNeurons.push_back(neuron);
}

//* At this stage, this function is only used for measuring the separation
//  this function is mainly used to delete the input neuron group
void Network::LSMDeleteNeuronGroup(char * name){
  NeuronGroup * to2del = SearchForNeuronGroup(name);
  if(to2del == NULL){
    cout<<"Cannot find the neurongroup named: "<<name<<endl;
    exit(-1);
  }
  // Two vector records the names of synapses-to-be-delelte
  vector<char*> pre_names;
  vector<char*> post_names;

  for(Neuron * neuron = to2del->First(); neuron != NULL; neuron = to2del->Next()){
    list<Synapse*>* outputSyns = neuron->LSMDisconnectNeuron(); // This is just to disconnect the neuron from the network. 
    
    // record those synapses:
    for(list<Synapse*>::iterator iter = (*outputSyns).begin(); iter != (*outputSyns).end(); iter++){
      Neuron * pre = (*iter)->PreNeuron();
      Neuron * post = (*iter)->PostNeuron();
      pre_names.push_back(pre->Name());
      post_names.push_back(post->Name());
    }
  }
  
  // Free the memory occupied those to-be-delete synapses.
  // Delete the pointers of those to-be-delete synapses from _synapses.
  for(int i = 0 ; i < pre_names.size(); ++i){
      char * pre_name = pre_names[i];
      char * post_name = post_names[i];
      // Find the pointer of that synapse to delete:
      for(list<Synapse*>::iterator iter_all = _synapses.begin(); iter_all != _synapses.end(); ){
        Neuron * pre_test = (*iter_all)->PreNeuron();
        Neuron * post_test = (*iter_all)->PostNeuron();
        char * pre_name_test = pre_test->Name();
        char * post_name_test = post_test->Name();
        if(strcmp(pre_name, pre_name_test) == 0 && strcmp(post_name, post_name_test) == 0){
#ifdef	_DEBUG_DEL
          cout<<"Delete the pointer of synapse : "<<pre_name_test<<" to "<<post_name_test<<" from _synapses!"<<endl;
#endif 
	  delete (*iter_all);
          iter_all = _synapses.erase(iter_all);
          break;
        }
        else
          iter_all++;
      }
  }

  for(list<Neuron*>::iterator iter = _allExcitatoryNeurons.begin(); iter != _allExcitatoryNeurons.end();){
    char neuron_name[64];
    strcpy(neuron_name, (*iter)->Name());
    char * token = strtok(neuron_name,"_");
    if(strcmp(token, name) == 0)
      iter = _allExcitatoryNeurons.erase(iter);
    else 
      iter++;
  }
  
  for(list<Neuron*>::iterator iter = _allInhibitoryNeurons.begin(); iter != _allInhibitoryNeurons.end();){
    char neuron_name[64];
    strcpy(neuron_name, (*iter)->Name());
    char * token = strtok(neuron_name,"_");
    if(strcmp(token, name) == 0)
      iter = _allInhibitoryNeurons.erase(iter);
    else 
      iter++;
  }
  
  for(list<NeuronGroup*>::iterator iter =  _groupNeurons.begin(); iter != _groupNeurons.end(); ){
     if(strcmp((*iter)->Name(), name) == 0)
       iter = _groupNeurons.erase(iter);
     else 
       iter++;
  }

    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); ){
    char neuron_name[64];
    strcpy(neuron_name, (*iter)->Name());
    char * token = strtok(neuron_name,"_");
    assert(token != NULL);
    if(strcmp(token, name) == 0){
#ifdef _DEBUG_DEL_1
      cout<<"Delete pointer of neuron: "<<(*iter)->Name()<<" from allNeurons!"<<endl;         
#endif
      delete (*iter);
      iter = _allNeurons.erase(iter);
    }
    else 
      iter++;
  }


  delete to2del; // Free the memory occupied by this neurongroup!
                 // Be careful to do so! Or you will have seg fault! 
  _lsm_input_layer = NULL;
}

//* THIS FUNCTION is only used for measuring the separation 
//  by running this function, the input layer is initialized
void Network::InitializeInputLayer(int num_inputs, int num_connections){
  assert(_lsm_input_layer == NULL);
  assert(SearchForNeuronGroup("input") == NULL);
  AddNeuronGroup("input", num_inputs, true);
 
  for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
    assert(SearchForNeuronGroup((*it)->Name()) != NULL);
    LSMAddSynapse("input", (*it)->Name(), 1, num_connections, 8, 1, true); 
              // from       to         fromN   toN         value  random  fixed
    
  }
}

//* add the speech labels to each individual reservoir
//  then apply the speeches whose labels are matched to the reservoir
//  each reservoir is like a specific filter to target at certain speech samples
void Network::LSMAddLabelToReservoirs(const char * name, set<int> labels){
    NeuronGroup * r = SearchForNeuronGroup(name);
    assert(r);
    r->SubSpeechLabel(labels); // pass the labels into neuron group
}

//* construct the reservoir given 3 dimensional grid information
void Network::LSMAddNeuronGroup(char * name, int dim1, int dim2, int dim3){
  assert((dim1>0)&&(dim2>0)&&(dim3>0));
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    assert(strcmp((*iter)->Name(),name) != 0);

  NeuronGroup * neuronGroup = new NeuronGroup(name, dim1, dim2, dim3, this);

  _groupNeurons.push_back(neuronGroup);
  _allReservoirs.push_back(neuronGroup); // push the individual liquid into the liquids

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
  _allReservoirs.push_back(neuronGroup); // push the individual liquid into the liquids

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
    weight_limit = value;
    D_weight_limit = value;
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
 
 assert(_allReservoirs.size() == 1);

 while(i < loss_neuron){
    int j = 0;
    int k = rand()%135; //Only select neuron in liquid
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	// for all reservoir neurons:
	for(Neuron * neuron = (*it)->First(); neuron != NULL; neuron = (*it)->Next()){
	    if(j == k){
		_name = neuron->Name();
		assert((_name[0] == 'r')&&(_name[1] == 'e')&&(_name[2] == 's'));
		if(neuron->LSMCheckNeuronMode(DEACTIVATED) == false){
		    cout<<"The reservoir neuron being removed has index: "<<k<<endl;
		    neuron->LSMSetNeuronMode(DEACTIVATED);
		    ++i;
		    (*it)->UnlockFirst();
		    break;
		}
	    }
	    else j++;
	}
    }
 }
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

//***********************************************************************************
//
// This function is to perform the adaptive gating.
// If the indegree of a reservoir neuron is <= INDEG_LIMIT
// and its outdegree is <= OUTDEC_DIGIT, we will turn off this neuron to save energy
//
//***********************************************************************************
void Network::LSMAdaptivePowerGating(){
    assert(!_allReservoirs.empty());

    int cnt = 0;
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	// for all reservoir neurons:
	for(Neuron * neuron = (*it)->First(); neuron != NULL; neuron = (*it)->Next()){
	    // do the power gating:
	    cnt += neuron->PowerGating();
	}
    }
    cout<<"Total number of reservoir neurons being power gated is "<<cnt<<endl;
}


//***********************************************************************************
//
// This function is to sum up the total number of neurons that are power gated.
// This function is only for the purpose to do double checking 
//
//***********************************************************************************
void Network::LSMSumGatedNeurons(){
    assert(!_allReservoirs.empty());

    int cnt = 0;
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	// for all reservoir neurons:
	for(Neuron * neuron = (*it)->First(); neuron != NULL; neuron = (*it)->Next()){
	    // do the power gating:
	    if(neuron->LSMCheckNeuronMode(DEACTIVATED)){
		cnt++;
		cout<<"The neuron gated: "<<neuron->Name()<<endl;
	    }
	}
    }
    cout<<"Actually, total number of reservoir neurons being power gated is "<<cnt<<endl;
}


//***********************************************************************************
//
// This function is to detect the total number of hubs in the reservoir
//
//***********************************************************************************
void Network::LSMHubDetection(){
    assert(!_allReservoirs.empty());

    int cnt = 0;
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	// for all reservoir neurons:
	for(Neuron * neuron = (*it)->First(); neuron != NULL; neuron = (*it)->Next()){
	    // do the hub detection
	    if(neuron->IsHubRoot()){
		cnt++;
		cout<<"The neuron which is the core of hub "<<neuron->Name()<<endl;
	    }
	}
    }
    cout<<"Total number of hubs in the reservoir is "<<cnt<<endl;
}



//* this is a function wrapper to train the reservoir. right now I am using STDP.
//* the readout layer is not trained during this stage
void Network::LSMReservoirTraining(networkmode_t networkmode){
    int info = -1;

    //1. Clear the signals
    LSMClearSignals();
    // note that no training of the readout synapses happens during this stage!
    
    // 2. Loop through reservoirs:
    int cnt_reservoir = 0;
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	info = LoadFirstSpeech(false, networkmode, (*it));
	while(info != -1){
#ifdef _SEPARATE_RESERVOIR
	    // separate the reservoir into several parts and stimulate with different sp:
	    if(!(*it)->InSet(info % CLS)){
		// speech label is not related to the reservoir, go on to next speech:
		info = LoadNextSpeech(false, networkmode, (*it));
		continue;
	    }		
#endif

#ifdef _DEBUG_NETWORK_SEPARATE
	    cout<<"Inputing speech: "<<info<<" for "<<cnt_reservoir<<"th reservoir"<<endl;
#endif
	    int time = 0;
	    while(!LSMEndOfSpeech(networkmode)){
		// the 5th parameter is the pointer to the individual reservoir,
		// if the 5th parameter is NULL meaning we are using all reservoirs.
		LSMNextTimeStep(++time, false, 1, NULL, NULL, (*it)); 
	    }

#ifdef _PRINT_SYN_ACT
	    char filename[128];
	    sprintf(filename, "activity/speech_%d.txt", info);
	    WriteSynActivityToFile("reservoir_1", "reservoir_15");
#endif
	    LSMClearSignals();
	    info = LoadNextSpeech(false, networkmode, (*it));
	}
	cnt_reservoir++;
    }

}

//* next simulation step; the last parameter should be the ptr to reservoir or NULL.
void Network::LSMNextTimeStep(int t, bool train,int iteration, FILE * Foutp, FILE * Fp, NeuronGroup * reservoir){
//  struct timeval val1,val2,val3;

  _lsm_t = t;


  for(list<Synapse*>::iterator iter = _lsmActiveSyns.begin(); iter != _lsmActiveSyns.end(); iter++) (*iter)->LSMNextTimeStep();
  _lsmActiveSyns.clear();

  // Please remember that the neuron firing activity will change the list: 
  // _lsmActiveSyns, _lsmActiveReservoirLearnSyns, and _lsmActiveLearnSyns:
  if(reservoir){
      // one individual reservoir is considered :
      // this case happens only when STDP training and we don't need to train readout!
      assert(!_inputNeurons.empty());
      for(list<Neuron*>::iterator it=_inputNeurons.begin(); it!=_inputNeurons.end(); ++it)
	  (*it)->LSMNextTimeStep(t, Foutp, Fp, train);
      for(Neuron* neuron=reservoir->First(); neuron != NULL;neuron = reservoir->Next()) 
	  neuron->LSMNextTimeStep(t, Foutp, Fp, train);
  }
  else{
      // all reservoirs are considered:
      for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->LSMNextTimeStep(t, Foutp, Fp, train);
  }
  // train the reservoir using STDP:
  if(_network_mode == TRAINRESERVOIR){
#ifndef _HARDWARE_CODE 
      // if not considering using simply hardware implementation
      if(reservoir){
	  // if the individual reservoir is specified:
	  for(Synapse * synapse = reservoir->FirstSynapse(); synapse != NULL; synapse = reservoir->NextSynapse()){
	      synapse->LSMUpdate(t);
	  }    
      }
      else{
	  // if simulating the whole network:
	  for(vector<Synapse*>::iterator iter = _rsynapses.begin(); iter != _rsynapses.end(); ++iter){
	      // Update the local variable implememnting STDP:
	      (*iter)->LSMUpdate(t);        
	  }
      }
#endif
      
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

//* function wrapper
//  1. Given a iter pointed to speech, load this speech to both input & reservoir layer.
//  if the pointer to reservoir layer is NULL, then load the speech to all reservoirs
//  2. In fact, we do not need to load the speech to the reservoir during STDP training,
//  so I will set ignore_reservoir to true to skip this part. 
//  Although we do not load the speech to the reservoir during STDP training, we still
//  need to set the reservoir neuron mode under this case !!!!
void Network::LoadSpeeches(Speech      * sp,
			   NeuronGroup * input,
			   NeuronGroup * reservoir,
			   NeuronGroup * output,
			   neuronmode_t neuronmode_input,
			   neuronmode_t neuronmode_reservoir,
			   neuronmode_t neuronmode_readout,
			   bool train,
			   bool ignore_reservoir
			   )
{
    assert(input);
    input->LSMLoadSpeech(sp,&_lsm_input,neuronmode_input,INPUTCHANNEL);

    // the reservoir will be ignore if we are doing STDP training.
    // this if loop is only accessed when networkmode is not TRAINRESERVOIR
    if(!ignore_reservoir){
	assert(_network_mode != TRAINRESERVOIR);
	if(reservoir)
	    reservoir->LSMLoadSpeech(sp,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
	else
	    LoadSpeechToAllReservoirs(sp,neuronmode_reservoir);
    }else{
	assert(reservoir && _lsm_reservoir_layer);
	// pass a NULL to ptr to speech, so that the function only set neuron mode!
	reservoir->LSMSetNeurons(NULL, neuronmode_reservoir, RESERVOIRCHANNEL, 0);
    }
    
    // load the speech to the output layer and setup its channels
    output->LSMLoadSpeech(sp,&_lsm_readout,neuronmode_readout,READOUTCHANNEL);

    // if train the readout 
    if(train == true){
	assert(output);
	output->LSMSetTeacherSignal(sp->Class());
    }
}

//**********************************************************************************//
//* the function to load each channel in the speech to each reservoir neuron and
//* set the neuronmode.
//* this function is doing two things: 
//*  1. Set the _lsm_reservoir to be the total numbers of reservoir neurons.
//*     _lsm_reservoir is used to indicate whether or not the speech is completely 
//*     played to the LSM. Please see the function of LSMEndofSpeech()
//*  2. Set the neuronmode according to the networkmode (TRANRESERVOIR or TRANSIENT) 
//**********************************************************************************//
void Network::LoadSpeechToAllReservoirs(Speech* sp, neuronmode_t neuronmode_reservoir){
    // the reservoir layer will be NULL if we load speech to every reservoir neuron
    // under the situation of separate reservoirs:
    assert(_lsm_reservoir_layer == NULL && _allReservoirs.size() > 1);
    
    // count total number of reservoir neurons first 
    // and set the total number of reservoir channel!
    int cnt = 0;
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	cnt += (*it)->Size();
    }
#ifdef _DEBUG_NETWORK_LOADSPEECH2ALL
    cout<<"Total number of neurons in the reservoir: "<<cnt<<endl;
#endif
    sp->SetNumReservoirChannel(cnt);
    
    // set the total of reservoir channels:
    _lsm_reservoir = cnt;

    // assign the channel ptr to each neuron according their index + offset
    // and set the neuronmode:
    int offset = 0;
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	(*it)->LSMSetNeurons(sp, neuronmode_reservoir, RESERVOIRCHANNEL, offset);
	offset += (*it)->Size();
    }	
}
  


//* function wrapper for removing speeches in the network 
void Network::LSMNetworkRemoveSpeech(){
    assert(_lsm_input_layer || _lsm_output_layer);

    if(_lsm_input_layer)
	_lsm_input_layer->LSMRemoveSpeech();
    _lsm_input_layer = NULL;

    if(_lsm_output_layer)
	_lsm_output_layer->LSMRemoveSpeech();
    _lsm_output_layer = NULL;

    assert(!_allReservoirs.empty());
    if(_lsm_reservoir_layer == NULL){ // if all invidual reservoirs are loaded with speeches:
	for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	    assert(*it);
	    (*it)->LSMRemoveSpeech();
	}
    }
    else
	_lsm_reservoir_layer->LSMRemoveSpeech();

    _lsm_reservoir_layer = NULL;
}


//* function wrapper for load the input, reservoir and output layers/neurongroups:
//* if the reservoir_group is NULL, then we are going to load all individual reservoirs
void Network::LSMLoadLayers(NeuronGroup* reservoir_group){
  assert(_lsm_input_layer == NULL);
  _lsm_input_layer = SearchForNeuronGroup("input");
  assert(_lsm_input_layer != NULL);

  assert(!_allReservoirs.empty());
  if(_allReservoirs.size() > 1)
      _lsm_reservoir_layer = reservoir_group;
  else{
      assert(_lsm_reservoir_layer == NULL);
      _lsm_reservoir_layer = SearchForNeuronGroup("reservoir");
      assert(_lsm_reservoir_layer != NULL);
  }

  _lsm_output_layer = SearchForNeuronGroup("output");
  assert(_lsm_output_layer != NULL);
}

void Network::DetermineNetworkNeuronMode(const networkmode_t & networkmode, neuronmode_t & neuronmode_input, neuronmode_t & neuronmode_reservoir, neuronmode_t & neuronmode_readout){
  if(networkmode == TRANSIENTSTATE){
    neuronmode_input = READCHANNEL;
    neuronmode_reservoir = WRITECHANNEL;
    neuronmode_readout = NORMAL;
  }else if(networkmode == TRAINRESERVOIR){
    neuronmode_input = READCHANNEL;
    neuronmode_reservoir = STDP;
    neuronmode_readout = NORMAL;
  }else if(networkmode == READOUT){
    neuronmode_input = DEACTIVATED;
    neuronmode_reservoir = READCHANNEL;
    neuronmode_readout = NORMAL;
  }else{
    cout<<"Unrecognized network mode!"<<endl;
    exit(EXIT_FAILURE);
  }

}

int Network::LoadFirstSpeech(bool train, networkmode_t networkmode, NeuronGroup * group){
    LSMLoadLayers(group);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 

    bool ignore_reservoir = networkmode == TRAINRESERVOIR ? true : false;

    _sp_iter = _speeches.begin();
    if(_sp_iter != _speeches.end()){
        LoadSpeeches(*_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer,neuronmode_input, neuronmode_reservoir, neuronmode_readout, train, ignore_reservoir);
	return (*_sp_iter)->Index();
    }else{
	return -1;
    }
}

// this function is similar to the original LoadFirstSpeech. But it is only used for separation measurement.
int Network::LoadFirstSpeech(bool train, networkmode_t networkmode, NeuronGroup * group, bool inputExist){
    // if the input layer has not been added to the network, then add it.
    // this can only happen for measuring the separation property:
    if(!inputExist){
      _sp_iter = _speeches.begin();
      
      if(_sp_iter != _speeches.end()){
	// Add the input layer here!!!
	InitializeInputLayer((*_sp_iter)->NumInputs(), (*_sp_iter)->NumConnections()); 
      }
      else{
	cout<<"No speech parsed into the network!"<<endl;
	return -1;
      }
    }

    LSMLoadLayers(group);
    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);
    
    bool ignore_reservoir = networkmode == TRAINRESERVOIR ? true : false;

    _sp_iter = _speeches.begin();
    if(_sp_iter != _speeches.end()){
        LoadSpeeches(*_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer,neuronmode_input, neuronmode_reservoir, neuronmode_readout,train, ignore_reservoir);
	return (*_sp_iter)->Index();
    }else{
	return -1;
    }
}


int Network::LoadNextSpeech(bool train, networkmode_t networkmode, NeuronGroup * group){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL || _allReservoirs.size() > 1);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    bool ignore_reservoir = networkmode == TRAINRESERVOIR ? true : false;

    if(train == true) _lsm_output_layer->LSMRemoveTeacherSignal((*_sp_iter)->Class());
    _sp_iter++;
    if(_sp_iter != _speeches.end()){
      LoadSpeeches(*_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir, neuronmode_readout, train, ignore_reservoir);
	return (*_sp_iter)->Index();
    }else{
	LSMNetworkRemoveSpeech();
	return -1;
    }
}

// This function is similar to the original LoadNextSpeech. 
// But it is only used for separation measurement.
int Network::LoadNextSpeech(bool train, networkmode_t networkmode, NeuronGroup * group, bool inputExist){
  //assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL || _allReservoirs.size() > 1);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    bool ignore_reservoir = networkmode == TRAINRESERVOIR ? true : false;

    if(train == true) _lsm_output_layer->LSMRemoveTeacherSignal((*_sp_iter)->Class());
    _sp_iter++;
    if(_sp_iter != _speeches.end()){
        if(!inputExist){
	  // if the input layer has not been added yet, added it here!
	  InitializeInputLayer((*_sp_iter)->NumInputs(), (*_sp_iter)->NumConnections());
	  assert(_lsm_input_layer == NULL);
	  _lsm_input_layer = SearchForNeuronGroup("input");
	  assert(_lsm_input_layer != NULL);
	}
	  
	LoadSpeeches(*_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir, neuronmode_readout,train, ignore_reservoir);
	return (*_sp_iter)->Index();
    }else{
	LSMNetworkRemoveSpeech();
	return -1;
    }
}


int Network::LoadFirstSpeechTrainCV(networkmode_t networkmode){
  assert((_fold_ind>=0)&&(_fold_ind<_fold));
 
  LSMLoadLayers(NULL);
  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);
   
    
  if(_fold_ind == 0) _train_fold_ind = 1;
  else _train_fold_ind = 0;
  _cv_train_sp_iter = _CVspeeches[_train_fold_ind]->begin();
  if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind]->end()){
    LoadSpeeches(*_cv_train_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir, neuronmode_readout,true, false); 
      return (*_cv_train_sp_iter)->Index();
  }else{
      cout<<"Warning, no training speech is specified!!"<<endl;
      return -1;
  }
}

int Network::LoadNextSpeechTrainCV(networkmode_t networkmode){
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer != NULL || _allReservoirs.size() > 1);
  assert(_lsm_output_layer != NULL);
  
  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 

  _lsm_output_layer->LSMRemoveTeacherSignal((*_cv_train_sp_iter)->Class());
  _cv_train_sp_iter++;
  if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind]->end()){
    LoadSpeeches(*_cv_train_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir, neuronmode_readout,true, false);
    return (*_cv_train_sp_iter)->Index();
  }else{
    ++_train_fold_ind;
    if(_train_fold_ind == _fold_ind)  ++_train_fold_ind;
    
    if(_train_fold_ind < _fold){
	_cv_train_sp_iter = _CVspeeches[_train_fold_ind]->begin();
	if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind]->end()){
      	    LoadSpeeches(*_cv_train_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir,neuronmode_readout, true, false);
	    return (*_cv_train_sp_iter)->Index();
	}else{
	    assert(0);
	    return -1;
	}
    }else{
	LSMNetworkRemoveSpeech();
	return -1;
    }
  }
}

int Network::LoadFirstSpeechTestCV(networkmode_t networkmode){
  assert((_fold_ind>=0)&&(_fold_ind<_fold));

  LSMLoadLayers(NULL);
  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 

  
  _cv_test_sp_iter = _CVspeeches[_fold_ind]->begin();
  if(_cv_test_sp_iter != _CVspeeches[_fold_ind]->end()){
    LoadSpeeches(*_cv_test_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir, neuronmode_readout, false, false);
    return (*_cv_test_sp_iter)->Index();
  }else{
    return -1;
  }
}

int Network::LoadNextSpeechTestCV(networkmode_t networkmode){
  assert(_lsm_input_layer != NULL);
  assert(_lsm_reservoir_layer != NULL || _allReservoirs.size() > 1);
  assert(_lsm_output_layer != NULL);

  neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
  // determine the neuron mode by the network mode:
  DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);
 
  _cv_test_sp_iter++;
  if(_cv_test_sp_iter != _CVspeeches[_fold_ind]->end()){
    LoadSpeeches(*_cv_test_sp_iter, _lsm_input_layer, _lsm_reservoir_layer, _lsm_output_layer, neuronmode_input, neuronmode_reservoir, neuronmode_readout, false, false);
    return (*_cv_test_sp_iter)->Index();
  }else{
      LSMNetworkRemoveSpeech();
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


void Network::RateToSpike(){
  int counter = 0;
  for(vector<Speech*>::iterator iter = _speeches.begin(); iter != _speeches.end(); iter++){
//    cout<<"Preprocessing speech "<<counter++<<"..."<<endl;
    counter++;
    (*iter)->RateToSpike();
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
    if(_sp_iter != _speeches.end())
	(*_sp_iter)->Info();
}

void Network::SpeechPrint(int info){
    if(_sp_iter != _speeches.end())
	(*_sp_iter)->PrintSpikes(info);
}

/***************************************************************************
* This function is to collect the pre-synaptic neuron firing actvities.
* It will gather the pre-synaptic neuron prob, avg interval and max interval
* between any two pre-synaptic spikes for each neuron.
* @param1: type (only resr is supported) @param2-4 prob, avg intv and max intv
***************************************************************************/
void Network::PreActivityStat(const char * type, vector<double>& prob, vector<double>& avg_intvl, vector<int>& max_intvl){
   synapsetype_t  syn_type = INVALID;
   DetermineSynType( type, syn_type, "PreActivityStat()");
   assert(syn_type == RESERVOIR_SYN);
   if(_allReservoirs.empty()){
     cout<<"No reservoir is in the list: _allReservoirs !"<<endl;
     assert(!_allReservoirs.empty());
   }
   
   double p = 0.0, avg_i = 0.0;
   int max_i = 0;
   // Collect the presynaptic firing activity from the neurongroup level:
   for(list<NeuronGroup*>::iterator it= _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
       double p_r = 0.0, avg_i_r = 0.0;
       int max_i_r = 0;
       assert(*it);
       (*it)->CollectPreSynAct(p_r, avg_i_r, max_i_r);
       p += p_r, avg_i += avg_i_r, max_i = max( max_i, max_i_r);
   }
   p = _allReservoirs.empty() ? 0 : p/((double)_allReservoirs.size());
   avg_i = _allReservoirs.empty() ? 0 : avg_i/((double)_allReservoirs.size());

   prob.push_back(p), avg_intvl.push_back(avg_i), max_intvl.push_back(max_i);
}

//* this function is to collect the firing fq from speech for each neuron:
void Network::CollectSpikeFreq(const char * type, vector<double>& fs, int end_t){
    synapsetype_t  syn_type = INVALID;
    DetermineSynType( type, syn_type, "CollectSpikeFreq()");
    
    if(_sp_iter == _speeches.end()){
        cout<<"In Network::CollectSpikeFreq(), you are trying to access the "
	    <<"current speech. But you have hit the _speeches.end()!"<<endl;
	cout<<"Abort the problem";
	assert(_sp_iter != _speeches.end());  
    }

    // collect the firing fq from the speech:
    assert(fs.empty());
    (*_sp_iter)->CollectFreq(syn_type, fs, end_t);

}

//* this function is to scatter the collected fq back to each neuron
//  now I only support the case of reservoir neurons:
void Network::ScatterSpikeFreq(const char * type, vector<double>& fs){
    synapsetype_t  syn_type = INVALID;
    DetermineSynType(type, syn_type, "ScatterSpikeFreq()");
    
    assert(syn_type == RESERVOIR_SYN);
    
    size_t bias = 0, cnt = 0;
    for(list<NeuronGroup*>::iterator it= _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
        assert(*it);
        (*it)->ScatterFreq(fs, bias, cnt);
    }
    assert(cnt == fs.size());
}

//* this function is to output the firing frequency of each neuron:
//* the first param: the type (input/reservoir neurons' firing frequency)
//* the second param: the output file stream for collecting the firing freq.
void Network::SpeechSpikeFreq(const char * type, ofstream & f_out, ofstream & f_label){
    if(_sp_iter != _speeches.end()){
	assert(f_out.is_open() && f_label.is_open());
	f_label<<(*_sp_iter)->PrintSpikeFreq(type, f_out)<<"\t";
    }
}

//* this function is to implement the variance based sparification:
void Network::VarBasedSparsify(const char * type){
    synapsetype_t  syn_type = INVALID;
    DetermineSynType(type, syn_type, "VarBasedSparsify()"); 
    
    // compute the variance of the firing rate for each neuron:
    multimap<double, Neuron*> my_map;
    for(list<NeuronGroup*>::iterator it= _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
        assert(*it);
        (*it)->CollectVariance(my_map);
    }
    
    // apply var-based sparisfication to the neurons with low variance :
    int cnt_limit = _TOP_PERCENT*my_map.size(), cnt = 0;
#ifdef _DEBUG_VARBASE
    cout<<"The lower "<<cnt_limit<<" neurons are applied with sparification."
	<<endl;
#endif
    for(multimap<double, Neuron*>::iterator it = my_map.begin(); it != my_map.end(); ++it){
        if(cnt < cnt_limit){
#ifdef _DEBUG_VARBASE
	    cout<<"Low "<<cnt+1<<"th neuron with var: "<<(*it).first<<endl;
#endif
	    // Apply the sparsification here:
	    (*it).second->DisableOutputSyn(syn_type);
        }
	++cnt;
    }
}

void Network::LSMChannelDecrement(channelmode_t channelmode){
  if(channelmode == INPUTCHANNEL) _lsm_input--;
  else if(channelmode == RESERVOIRCHANNEL) _lsm_reservoir--;
  else if(channelmode == READOUTCHANNEL) _lsm_readout--;
  else assert(0); // you code should never go here!
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


void Network::IndexSpeech(){
  for(int i = 0; i < _speeches.size(); i++) _speeches[i]->SetIndex(i);
}

//* This is a supporting function. Given a synapse type (r/o synapses)
//* determine the syn_type to be returned (ret_syn_type).
//* The third parameter is the function name used to handle exception.
void Network::DetermineSynType(const char * syn_type, synapsetype_t & ret_syn_type, const char * func_name){
  if(strcmp(syn_type, "reservoir") == 0){
    // Write reservoir syns into the file
    ret_syn_type = RESERVOIR_SYN;
  }
  else if(strcmp(syn_type, "readout") == 0){
    ret_syn_type = READOUT_SYN;
  }
  else{
    cout<<"In Network::"<<func_name<<", undefined synapse type: "<<syn_type<<endl;
    exit(EXIT_FAILURE);
  } 
}

/***********************************************************************************
 * This function is to implement the weight normalization used by Oja rule.
 * Beacause of the usage of Hebbian learning rule, the weight saturation is possible.
 * The normalization is helpful for Heb-learn. For more details, please see the wiki.
 * @param1: the synapse type.
 * Note: after normalization, the weight of each synapse should be within 0~1.
 * This function should be called only for continuous weight and the w is changed !!
 **********************************************************************************/
void Network::NormalizeContinuousWeights(const char * syn_type){
#ifdef DIGIAL
    cout<<"You are using the code for digital mode but you call the normalization for "
	<<"continuous weights!? Abort the problem"<<endl;
    assert(0);
#endif

    synapsetype_t  tar_syn_type = INVALID;
    DetermineSynType(syn_type, tar_syn_type, "NormalizeContinuousWeights()");
    assert(tar_syn_type != INVALID);
    vector<Synapse*> tmp;

    vector<Synapse*> & synapses = tar_syn_type == RESERVOIR_SYN ? _rsynapses : 
	(tar_syn_type == READOUT_SYN ? _rosynapses : tmp);
    assert(!synapses.empty()); 

    // right now the function should be run for reservoir synapses!
    double g_sum = 0.0;
    for(size_t i = 0; i < synapses.size(); ++i){
	assert(synapses[i]);
	double w = synapses[i]->Weight();
	g_sum += w*w;
    }

    // normalize the weight:
    assert(g_sum > 0);
    g_sum = sqrt(g_sum);
    for(size_t i = 0; i < synapses.size(); ++i){
	// compute the normalized weight:
	double new_w = synapses[i]->Weight()/g_sum;
	// set the new weight:
	synapses[i]->Weight(new_w);
    }
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
  DetermineSynType(syn_type, wrt_syn_type, "WriteSynWeightsToFile()");
  assert(wrt_syn_type != INVALID);

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
	/*
	 <<"\t"<<synapses[i]->PreNeuron()->IsExcitatory()
	 <<"\t"<<synapses[i]->PostNeuron()->IsExcitatory()
	*/
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
  DetermineSynType(syn_type, read_syn_type, "LoadSynWeightsFromFile()");
  assert(read_syn_type != INVALID);

  const vector<Synapse*> & synapses = read_syn_type == RESERVOIR_SYN ? _rsynapses : 
    (read_syn_type == READOUT_SYN ? _rosynapses : vector<Synapse*>());

  // load the synaptic weights from file:
  // the synaptic weight is stored in the file in the same order as the corresponding synapses store in the vector
  int index;
  string pre;
  string post;
  /*
  int pre_ext; // two variables to catch the excitatory information of syn
  int post_ext;
  */
#ifdef DIGITAL
  int weight;
#else
  double weight;
#endif
  while(f_in>>index>>pre>>post>>weight){//>>pre_ext>>post_ext){
    if(index < 0 || index >= synapses.size()){
	cout<<"In Network::LoadSynWeightsFromFile(), the index of the synapse you read : "<<index<<" is out of bound of the container stores the synapses!!"<<endl;
	exit(EXIT_FAILURE);
    }

    assert(strcmp(pre.c_str(), synapses[index]->PreNeuron()->Name()) == 0);
    assert(strcmp(post.c_str(), synapses[index]->PostNeuron()->Name()) == 0);
    synapses[index]->Weight(weight);
  }
  
}


//* This function writes the synaptic activity to the given file:
//* Right now I am only consider looking at reservoir synapses
void Network::WriteSynActivityToFile(char * pre_name, char * post_name, char * filename){
  assert(pre_name && post_name);

  // 1. Open the file:
  ofstream f_out(filename);
  if(!f_out.is_open()){
    cout<<"In function: Network::WriteSynActivityToFile()"
        <<" cannot open the file: "<<filename<<" to write!"<<endl;
    exit(EXIT_FAILURE);
  }

  // 2. Find out the target synapse (only consider reservoir synapse) :
  for(vector<Synapse*>::iterator it = _rsynapses.begin(); it != _rsynapses.end(); ++it){
    if(strcmp(pre_name, (*it)->PreNeuron()->Name()) == 0 &&
       strcmp(post_name, (*it)->PostNeuron()->Name()) == 0)
      (*it)->PrintActivity(f_out);
  }

  // 3. Close the file:
  f_out.close();

}

/*************************************************************************************
 * This function is to delete all the synapses in the reservoir and free the resources.
 * And leave the reservoir as only reservoir neurons.
 * I also delete the reservoir synapses from _rsynapses and _synapses.
 * The input param is the pointer to the reservoir. It is NULL means delete the synapses
 * for all reservoirs
 ************************************************************************************/
void Network::DestroyReservoirConn(NeuronGroup * reservoir){
    if(reservoir){
	reservoir->DestroyResConn();
    }
    else{
	assert(!_allReservoirs.empty());
	for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	    (*it)->DestroyResConn();
	}
    }

    // the following code is for debugging purpose:
    // check whether or not the synapses have been deleted:
    ofstream f_out("temp_check_reservoir.txt");
    if(reservoir){
	reservoir->PrintResSyn(f_out);
    }
    else{
	assert(!_allReservoirs.empty());
	for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	    (*it)->PrintResSyn(f_out);
	}
    }
    f_out.close();
    
}

/*************************************************************************************
 * This function is to truncate the intermediate weights of the reservoir synapses.
 * This is a handcrafted method for further reducing the design complexity and
 * lowing the energy consumption.
 **************************************************************************************/
void Network::TruncateIntermSyns(const char * syn_type){
    synapsetype_t trunc_syn_type = INVALID;
    DetermineSynType(syn_type, trunc_syn_type, "LoadSynWeightsFromFile()");

    // Right now only consider the reservoir synapses:
    assert(trunc_syn_type == RESERVOIR_SYN);
    
    const vector<Synapse*> & synapses = trunc_syn_type == RESERVOIR_SYN ? _rsynapses : 
    (trunc_syn_type == READOUT_SYN ? _rosynapses : vector<Synapse*>());
  
    assert(!synapses.empty());
    int cnt = 0;
    for(size_t i = 0; i < synapses.size(); ++i){
	// only consider excitatory synapses:
	if(synapses[i]->Excitatory()){
	    cnt += synapses[i]->TruncIntermWeight();
	}
    }

    cout<<"Total number of excitatory synapses being removed: "<<cnt<<endl;
}


/**************************************************************************************
*
* This function is to visualize the reservoir synapses.
* It will generate a reservoir_%d.gexf file, which %d is the indictor for the gen order
* The format of this type of file can be found in 
* http://graphml.graphdrawing.org/primer/graphml-primer.html#GraphNode
*
***************************************************************************************/
void Network::VisualizeReservoirSyns(int indictor){
    assert(!_rsynapses.empty());
    
    char filename[64];
    sprintf(filename, "reservoir_%d.gexf", indictor);

    ofstream f_out(filename);
    assert(f_out.is_open());

    // put the header first:
    f_out<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 <<"<gexf xmlns:viz=\"http:///www.gexf.net/1.1draft/viz\" version=\"1.1\" xmlns=\"http://www.gexf.net/1.1draft\">\n"
	 <<"<meta lastmodifieddate=\"2010-03-03+23:44\">\n"
	 <<"<creator>Gephi 0.7</creator>\n"
	 <<"</meta>"
	 <<endl;

    // define the graph type:
    f_out<<"<graph defaultedgetype=\"directed\" idtype=\"string\" type=\"static\">\n";

    // count the total number of reservoir nodes:
    int count_neuron = 0, offset = 0;
    assert(!_allReservoirs.empty());
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	(*it)->LSMIndexNeurons(offset);
	offset += (*it)->Size();
	for(Neuron * neuron = (*it)->First(); neuron != NULL; neuron = (*it)->Next()){
	    if(!neuron->LSMCheckNeuronMode(DEACTIVATED))
		count_neuron++;
	}
    }
    
    // count the total number of synapses:
    int count_syn = 0;
    for(vector<Synapse*>::iterator it = _rsynapses.begin(); it != _rsynapses.end(); ++it){
#ifdef DIGITAL
	if((*it)->IsValid() && (*it)->DWeight() >= 0)
#else
	if((*it)->IsValid() && (*it)->Weight() >= 0)
#endif
	    count_syn++;
    }
	
    cout<<"total neurons: "<<count_neuron<<"\t total_synapses: "<<count_syn<<endl;
    
    // output the reservoir neuron information:
    f_out<<"<nodes count=\""<<count_neuron<<"\">\n";
    for(list<NeuronGroup*>::iterator it = _allReservoirs.begin(); it != _allReservoirs.end(); ++it){
	for(Neuron * neuron = (*it)->First(); neuron != NULL; neuron = (*it)->Next()){
	    if(!neuron->LSMCheckNeuronMode(DEACTIVATED))
		// the label here is randomly assigned by me:
		f_out<<"<node id=\""<<neuron->Index()<<"\" label=\"OldMan\"/>\n";
	}
    }
    f_out<<"</nodes>"<<endl;

    // output the reservoir synapse information:
    f_out<<"<edges count=\""<<count_syn<<"\">\n";
    int index_syn = 0;
    for(vector<Synapse*>::iterator it = _rsynapses.begin(); it != _rsynapses.end(); ++it){
	// for valid and non-inhibitory synapses:
#ifdef DIGITAL
	if((*it)->IsValid() && (*it)->DWeight() >= 0){
#else
	if((*it)->IsValid() && (*it)->Weight() >= 0){
#endif
		int pre_ind = (*it)->PreNeuron()->Index();
		int post_ind = (*it)->PostNeuron()->Index();
		f_out<<"<edge id=\""<<index_syn<<"\" source=\""<<pre_ind
		     <<"\" target=\""<<post_ind<<"\" weight=\""
#ifdef DIGITAL
		     <<((*it)->DWeight() == 0 ? 0.00001 : (*it)->DWeight())<<"\"/>\n";
#else
		     <<(*it)->Weight()<<"\"/>\n";
#endif
		index_syn++;
	}		
    }
    
    f_out<<"</edges>\n"
	 <<"</graph>\n"
	 <<"</gexf>"<<endl;
	
    f_out.close();
}
