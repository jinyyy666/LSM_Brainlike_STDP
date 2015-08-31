#include "def.h"
#include "network.h"
#include "neuron.h"
#include "synapse.h"
#include "pattern.h"
#include "speech.h"
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
  _iter_started = false;
  _patterns = NULL;
  _learning = 1;
  _probe._probeFlag = 0;
  _probe._probeStats = NULL;
  _currentPattern = NULL;
  _voteCounts = NULL;
  _hasInputs = false;
}

Network::~Network(){
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    delete (*iter);
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    delete (*iter);
  for(list<Pattern*>::iterator iter = _patternsTrain.begin(); iter != _patternsTrain.end(); iter++)
    delete (*iter);
  for(list<Pattern*>::iterator iter = _patternsTest.begin(); iter != _patternsTest.end(); iter++)
    delete (*iter);
  if(_probe._probeStats != NULL){
    for(int i = 0; i <= _probe._nameInputs.size(); i++) delete [] _probe._probeStats[i];
    delete [] _probe._probeStats;
  }
  if(_voteCounts != NULL) delete _voteCounts;
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

  cout<<"after constructing the reservoir, _rsynapses has size: "<<_rsynapses.size()<<endl;

  for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
    _allNeurons.push_back(neuron);
    if(neuron->IsExcitatory()) _allExcitatoryNeurons.push_back(neuron);
    else _allInhibitoryNeurons.push_back(neuron);
  }
}

  
// the following five functions are for liquid state machine only
void Network::LSMAddSynapse(Neuron * pre, Neuron * post, double value, bool fixed, double weight_limit){
  Synapse * synapse = new Synapse(pre, post, value, fixed, weight_limit);
  _synapses.push_back(synapse);
  pre->AddPostSyn(synapse);
  post->AddPreSyn(synapse);
}

void Network::LSMAddSynapse(Neuron * pre, NeuronGroup * post, double value, bool fixed, double weight_limit){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    LSMAddSynapse(pre,neuron,value, fixed, weight_limit);
}

void Network::LSMAddSynapse(NeuronGroup * pre,Neuron * post, double value, bool fixed, double weight_limit){
  for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next())
    LSMAddSynapse(neuron,post,value, fixed, weight_limit);
}

void Network::LSMAddSynapse(NeuronGroup * pre, NeuronGroup * post, double value, bool fixed, double weight_limit){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    LSMAddSynapse(pre,neuron,value, fixed, weight_limit);
}

void Network::LSMAddSynapse(char * pre, char * post, double value, bool fixed, double weight_limit){
  Neuron * preNeuron, * postNeuron;
  NeuronGroup * preNeuronGroup, * postNeuronGroup;
  int preIndex = 0;
  int postIndex = 0;

  // find the presynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 1;
      preNeuron = (*iter);
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 2;
      preNeuronGroup = (*iter);
    }

  // find the postsynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 1;
      postNeuron = (*iter);
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 2;
      postNeuronGroup = (*iter);
    }

  // add synapses
  if(preIndex == 1){
    if(postIndex == 1){
      LSMAddSynapse(preNeuron, postNeuron, value, fixed, weight_limit);
    }else if(postIndex == 2){
      LSMAddSynapse(preNeuron, postNeuronGroup, value, fixed, weight_limit);
    }else assert(0);
  }else if(preIndex == 2){
    if(postIndex == 1){
      LSMAddSynapse(preNeuronGroup, postNeuron, value, fixed, weight_limit);
    }else if(postIndex == 2){
      LSMAddSynapse(preNeuronGroup, postNeuronGroup, value, fixed, weight_limit);
    }else assert(0);
  }else assert(0);
}

// for LSM with detailed information
void Network::LSMAddSynapse(Neuron * pre, Neuron * post, int value, bool fixed, int D_weight_limit,bool liquid){
    assert((liquid == false)||(liquid == true));
    Synapse * synapse = new Synapse(pre, post, value, fixed, D_weight_limit, pre->IsExcitatory(),liquid);
    _synapses.push_back(synapse);
    // push back the reservoir synapses into the list:
    if(!synapse->IsReadoutSyn() && !synapse->IsInputSyn())
      _rsynapses.push_back(synapse);
    pre->AddPostSyn(synapse);
    post->AddPreSyn(synapse);	
}

void Network::LSMAddSynapse(Neuron * pre, NeuronGroup * post, int npost, int value, int random, bool fixed){
  Neuron * neuron;
  double factor;
  double weight_limit;
  bool liquid = false;
  int D_weight_limit;
  if(fixed == true){
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
      assert(random == 0);
//      int D_weight = 0;
      int D_weight = -8 + rand()%15;
      LSMAddSynapse(pre,neuron,D_weight,fixed,D_weight_limit,liquid);
#else
      if(random == 2){
        factor = rand()%100000;
        factor = factor*2/99999-1;
      }else if(random == 0) factor = 0;
      else assert(0);
      LSMAddSynapse(pre,neuron,value*factor,fixed,weight_limit,liquid);
#endif
    }
  }else{
    assert(random == 1);
//cout<<npost<<endl;
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

// the following five functions are for fixed synapses
void Network::AddSynapse(Neuron * pre, Neuron * post, bool excitatory, bool fixed, int value){
  Synapse * synapse = new Synapse(pre, post, excitatory, fixed, value);
  _synapses.push_back(synapse);
  pre->AddPostSyn(synapse);
  post->AddPreSyn(synapse);
}

void Network::AddSynapse(Neuron * pre, NeuronGroup * post, bool excitatory, bool fixed, int value){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    AddSynapse(pre,neuron,excitatory,fixed,value);
}

void Network::AddSynapse(NeuronGroup * pre, Neuron * post, bool excitatory, bool fixed, int value){
  for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next())
    AddSynapse(neuron,post,excitatory,fixed,value);
}

void Network::AddSynapse(NeuronGroup * pre, NeuronGroup * post, bool excitatory, bool fixed, int value){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    AddSynapse(pre,neuron,excitatory,fixed,value);
}

void Network::AddSynapse(char * pre, char * post, bool excitatory, bool fixed, int value){
  Neuron * preNeuron, * postNeuron;
  NeuronGroup * preNeuronGroup, * postNeuronGroup;
  int preIndex = 0;
  int postIndex = 0;

  // find the presynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 1;
      preNeuron = (*iter);
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 2;
      preNeuronGroup = (*iter);
    }

  // find the postsynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 1;
      postNeuron = (*iter);
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 2;
      postNeuronGroup = (*iter);
    }

  // add synapses
  if(preIndex == 1){
    if(postIndex == 1){
      AddSynapse(preNeuron, postNeuron, excitatory, fixed, value);
    }else if(postIndex == 2){
      AddSynapse(preNeuron, postNeuronGroup, excitatory, fixed, value);
    }else assert(0);
  }else if(preIndex == 2){
    if(postIndex == 1){
      AddSynapse(preNeuronGroup, postNeuron, excitatory, fixed, value);
    }else if(postIndex == 2){
      AddSynapse(preNeuronGroup, postNeuronGroup, excitatory, fixed, value);
    }else assert(0);
  }else assert(0);
}


// the following five functions are for learning synapses
void Network::AddSynapse(Neuron * pre, Neuron * post, bool excitatory, bool fixed, int factor, int ini_min, int ini_max){
  Synapse * synapse = new Synapse(pre, post, excitatory, fixed, factor, ini_min, ini_max);
  _synapses.push_back(synapse);
  pre->AddPostSyn(synapse);
  post->AddPreSyn(synapse);
}

void Network::AddSynapse(Neuron * pre, NeuronGroup * post, bool excitatory, bool fixed, int factor, int ini_min, int ini_max){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    AddSynapse(pre,neuron,excitatory,fixed, factor, ini_min, ini_max);
}

void Network::AddSynapse(NeuronGroup * pre, Neuron * post, bool excitatory, bool fixed, int factor, int ini_min, int ini_max){
  for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next())
    AddSynapse(neuron,post,excitatory,fixed, factor, ini_min, ini_max);
}

void Network::AddSynapse(NeuronGroup * pre, NeuronGroup * post, bool excitatory, bool fixed, int factor, int ini_min, int ini_max){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    AddSynapse(pre,neuron,excitatory,fixed, factor, ini_min, ini_max);
}

void Network::AddSynapse(char * pre, char * post, bool excitatory, bool fixed, int factor, int ini_min, int ini_max){
  Neuron * preNeuron, * postNeuron;
  NeuronGroup * preNeuronGroup, * postNeuronGroup;
  int preIndex = 0;
  int postIndex = 0;

  // find the presynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 1;
      preNeuron = (*iter);
      break;
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 2;
      preNeuronGroup = (*iter);
      break;
    }

  // find the postsynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 1;
      postNeuron = (*iter);
      break;
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 2;
      postNeuronGroup = (*iter);
      break;
    }

  // add synapses
  if(preIndex == 1){
    if(postIndex == 1){
      AddSynapse(preNeuron, postNeuron, excitatory, fixed, factor, ini_min, ini_max);
    }else if(postIndex == 2){
      AddSynapse(preNeuron, postNeuronGroup, excitatory, fixed, factor, ini_min, ini_max);
    }else assert(0);
  }else if(preIndex == 2){
    if(postIndex == 1){
      AddSynapse(preNeuronGroup, postNeuron, excitatory, fixed, factor, ini_min, ini_max);
    }else if(postIndex == 2){
      AddSynapse(preNeuronGroup, postNeuronGroup, excitatory, fixed, factor, ini_min, ini_max);
    }else assert(0);
  }else assert(0);
}

/*
// the following five functions are for learning synapses
void Network::AddSynapse(Neuron * pre, Neuron * post, bool excitatory, bool fixed, int min, int max, int ini_min, int ini_max){
  Synapse * synapse = new Synapse(pre, post, excitatory, fixed, min, max, ini_min, ini_max);
  _synapses.push_back(synapse);
  pre->AddPostSyn(synapse);
  post->AddPreSyn(synapse);
}

void Network::AddSynapse(Neuron * pre, NeuronGroup * post, bool excitatory, bool fixed, int min, int max, int ini_min, int ini_max){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    AddSynapse(pre,neuron,excitatory,fixed, min, max, ini_min, ini_max);
}

void Network::AddSynapse(NeuronGroup * pre, Neuron * post, bool excitatory, bool fixed, int min, int max, int ini_min, int ini_max){
  for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next())
    AddSynapse(neuron,post,excitatory,fixed, min, max, ini_min, ini_max);
}

void Network::AddSynapse(NeuronGroup * pre, NeuronGroup * post, bool excitatory, bool fixed, int min, int max, int ini_min, int ini_max){
  for(Neuron * neuron = post->First(); neuron != NULL; neuron = post->Next())
    AddSynapse(pre,neuron,excitatory,fixed, min, max, ini_min, ini_max);
}

void Network::AddSynapse(char * pre, char * post, bool excitatory, bool fixed, int min, int max, int ini_min, int ini_max){
  Neuron * preNeuron, * postNeuron;
  NeuronGroup * preNeuronGroup, * postNeuronGroup;
  int preIndex = 0;
  int postIndex = 0;

  // find the presynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 1;
      preNeuron = (*iter);
      break;
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),pre) == 0){
      preIndex = 2;
      preNeuronGroup = (*iter);
      break;
    }

  // find the postsynaptic part
  for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 1;
      postNeuron = (*iter);
      break;
    }
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++)
    if(strcmp((*iter)->Name(),post) == 0){
      postIndex = 2;
      postNeuronGroup = (*iter);
      break;
    }

  // add synapses
  if(preIndex == 1){
    if(postIndex == 1){
      AddSynapse(preNeuron, postNeuron, excitatory, fixed, min, max, ini_min, ini_max);
    }else if(postIndex == 2){
      AddSynapse(preNeuron, postNeuronGroup, excitatory, fixed, min, max, ini_min, ini_max);
    }else assert(0);
  }else if(preIndex == 2){
    if(postIndex == 1){
      AddSynapse(preNeuronGroup, postNeuron, excitatory, fixed, min, max, ini_min, ini_max);
    }else if(postIndex == 2){
      AddSynapse(preNeuronGroup, postNeuronGroup, excitatory, fixed, min, max, ini_min, ini_max);
    }else assert(0);
  }else assert(0);
}
*/

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
//  assert(0);
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
//    cout<<"# of Neurons in liquid: "<< (_allNeurons.size()-112)<<endl;
//    cout<<"k = "<<k<<endl;
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
//  assert(0);
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
void Network::LSMNextTimeStep(int t, bool train,int iteration, FILE * Foutp){
//  struct timeval val1,val2,val3;

  _lsm_t = t;

//  gettimeofday(&val1,&zone);
  for(list<Synapse*>::iterator iter = _lsmActiveSyns.begin(); iter != _lsmActiveSyns.end(); iter++) (*iter)->LSMNextTimeStep();
  _lsmActiveSyns.clear();
//  gettimeofday(&val2,&zone);
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->LSMNextTimeStep(t,Foutp);
 
  // train the reservoir using STDP:
  if(_network_mode == TRAINRESERVOIR){
    for(list<Synapse*>::iterator iter = _lsmActiveReservoirLearnSyns.begin(); iter != _lsmActiveReservoirLearnSyns.end(); iter++){
      // Update the local variable implememnting STDP with triplet pairing scheme:
      (*iter)->LSMUpdate(t);      
      // train the reservoir synapse with STDP rule:
      (*iter)->LSMLiquidLearn(t);
    }
    _lsmActiveReservoirLearnSyns.clear();
  }
    
//cout<<t<<"\t"<<_lsmActiveLearnSyns.size()<<endl;
//  cout<<t<<"\t"<<((val2.tv_sec-val1.tv_sec)+double(val2.tv_usec-val1.tv_usec)*1e-6)<<"\t"<<((val3.tv_sec-val2.tv_sec)+double(val3.tv_usec-val2.tv_usec)*1e-6)<<endl;
//if(_lsmActiveLearnSyns.size()>0)cout<<t<<"\t"<<_lsmActiveLearnSyns.size()<<endl;
  if(train == true) {
    for(list<Synapse*>::iterator iter = _lsmActiveLearnSyns.begin(); iter != _lsmActiveLearnSyns.end(); iter++) (*iter)->LSMLearn(iteration);
//    cout<<"learning..."<<endl;
  }
  _lsmActiveLearnSyns.clear();
}

void Network::UpdateNeurons(double t_step, double t){
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->Update(&_activeInputSyns,&_activeOutputSyns,t_step,t);
}

void Network::UpdateExcitatoryNeurons(double t_step, double t){
  for(list<Neuron*>::iterator iter = _allExcitatoryNeurons.begin(); iter != _allExcitatoryNeurons.end(); iter++) (*iter)->Update(&_activeInputSyns,&_activeOutputSyns,t_step,t);
}

void Network::UpdateInhibitoryNeurons(double t_step, double t){
  for(list<Neuron*>::iterator iter = _allInhibitoryNeurons.begin(); iter != _allInhibitoryNeurons.end(); iter++) (*iter)->Update(&_activeInputSyns,&_activeOutputSyns,t_step,t);
}

void Network::UpdateSynapses(){
  list<Synapse*>::iterator iter;
  for(iter = _activeOutputSyns.begin(); iter != _activeOutputSyns.end(); iter++) (*iter)->SendSpike();
  if(_learning != 0){
    for(iter = _activeOutputSyns.begin(); iter != _activeOutputSyns.end(); iter++) (*iter)->Learn(0);
    for(iter = _activeInputSyns.begin(); iter != _activeInputSyns.end(); iter++) (*iter)->Learn(1);
  }

  _activeInputSyns.clear();
  _activeOutputSyns.clear();
/*
  if(COUNTER_LEARN != 0){
    cout<<"number of the synapses learning: "<<COUNTER_LEARN<<endl;
    COUNTER_LEARN = 0;
  }
*/
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

void Network::ActivateInputs(Pattern * pattern){
  pattern->Activate();
  _hasInputs = true;
}

void Network::ClearInputs(){
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++)
    (*iter)->ClearInput();
  _hasInputs = false;
}

void Network::LoadPatternsTrain(){
  _patterns = &_patternsTrain;
}

void Network::LoadPatternsTest(){
  _patterns = &_patternsTest;
}

void Network::AddPattern(Pattern * pattern){
  _patterns->push_back(pattern);
}

Pattern * Network::FirstPattern(){
  _iter = _patterns->begin();
  _iter_started = true;
  return (*_iter);
}

Pattern * Network::NextPattern(){
  assert(_iter_started == true);
  _iter++;
  if(_iter != _patterns->end()) return (*_iter);
  else{
    _iter_started = false;
    return NULL;
  }
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

void Network::ClearProbe(){
  _probe._probeSpiking.clear();
}

int Network::ReadProbe(){
  return _probe._probeSpiking.size();
}

void Network::SetProbe(const char * name){
  NeuronGroup * neurongroup = SearchForNeuronGroup(name);
  if(neurongroup != NULL){
    neurongroup->AddProbe(&_probe);
    return;
  }
  Neuron * neuron = SearchForNeuron(name);
  if(neuron != 0){
    neuron->AddProbe(&_probe);
    return;
  }
  assert(0);
}

// if flag==1, build data structure for collecting testing results
void Network::ProbeStat(int flag){
  bool found;
  int i, j, temp;

  if(_probe._nameInputs.size() == 0){
    for(list<Pattern*>::iterator iter = _patterns->begin(); iter != _patterns->end(); iter++){
      found = false;
      for(i = 0; i < _probe._nameInputs.size(); i++){
        if(strcmp((*iter)->Name(), _probe._nameInputs[i]) == 0){
          found = true;
          break;
        }
      }
      if(found == false) _probe._nameInputs.push_back((*iter)->Name());
    }
  }

  if(flag == 0){
    _numNames = (_probe._nameInputs).size();
    _voteCounts = new int[_numNames];
    for(i = 0; i < _numNames; i++){
      _voteCounts[i] = 0;
    }

    for(std::list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++){
      (*iter)->SetUpForUnsupervisedLearning(&(_probe._nameInputs));
    }
    _probe._probeFlag = 0;
    return;
  }

  assert(flag == 1);
  _probe._probeFlag = 1;
  temp = _probe._nameInputs.size()+1;//SearchForNeuronGroup("output")->Size()+1;
  _probe._probeStats = new int*[temp];
  for(i = 0; i < temp; i++){
    _probe._probeStats[i] = new int[_probe._nameInputs.size()+1];
    for(j = 0; j < _probe._nameInputs.size()+1; j++)
      _probe._probeStats[i][j] = 0;
  }
}

void Network::PatternStat(){
  vector<const char*> names;
  vector<int> counts;
  bool found;
  int i;
  for(list<Pattern*>::iterator iter = _patterns->begin(); iter != _patterns->end(); iter++){
    found = false;
    for(i = 0; i < names.size(); i++){
      if(strcmp((*iter)->Name(), names[i]) == 0){
        counts[i]++;
        found = true;
        break;
      }
    }
    if(found == false){
      names.push_back((*iter)->Name());
      counts.push_back(1);
    }
  }

  for(i = 0; i < names.size(); i++) cout<<"counts of "<<names[i]<<":\t"<<counts[i]<<endl;
}

void Network::SetLearning(int learning){
  _learning = learning;
}

void Network::SetTeacherSignal(const char * name1, const char * name2, int signal){
  Neuron * neuron = SearchForNeuron(name1,name2);
  assert(neuron != NULL);
  neuron->SetTeacherSignal(signal);
}

void Network::SetTeacherSignalGroup(const char * name1, const char * name2, int signal){
  int index, total;
  NeuronGroup * neurongroup = SearchForNeuronGroup(name1);
  assert(neurongroup != NULL);

  total = _probe._nameInputs.size();
  for(index = 0; index < total; index++) if(strcmp(_probe._nameInputs[index], name2)== 0) break;
  assert(index < total);
  neurongroup->SetTeacherSignalGroup(index, total, signal);
}

void Network::SetTeacherSignalOneInGroup(char * name1, char * name2, int signal){
  int index, total;
  NeuronGroup * neurongroup = SearchForNeuronGroup(name1);
  assert(neurongroup != NULL);

  total = _probe._nameInputs.size();
  for(index = 0; index < total; index++) if(strcmp(_probe._nameInputs[index], name2)== 0) break;
  assert(index < total);
  neurongroup->SetTeacherSignalOneInGroup(index, total, signal);
}

void Network::PrintAllNeuronName(){
  for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) cout<<(*iter)->Name()<<endl;
}

void Network::Print(){
/*
for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++){
  if(strcmp("inhi",(*iter)->Name()) == 0) (*iter)->PrintInputSyns();
}
*/

/*
  NeuronGroup * output = NULL;
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++){
    if(strcmp("output",(*iter)->Name()) == 0) output = *iter;
  }
  assert(output != NULL);

  Neuron * neuron;
  for(neuron = output->First(); neuron != 0; neuron = output->Next()){
    neuron->PrintInputSyns();
    cout<<endl<<endl;
  }
*/

cout<<"printing to file"<<endl;

  NeuronGroup * output = NULL;
  for(list<NeuronGroup*>::iterator iter = _groupNeurons.begin(); iter != _groupNeurons.end(); iter++){
    if(strcmp("output",(*iter)->Name()) == 0) output = *iter;
  }
  assert(output != NULL);

  Neuron * neuron;
  for(neuron = output->First(); neuron != NULL; neuron = output->Next()){
    neuron->PrintInputSyns();
//    cout<<endl<<endl;
  }
}

// count the pattern (put it into the Stat data structure) by fired neuron
void Network::AddPatternStat(int index, Pattern * pattern){
  assert(_probe._probeFlag == 1);

  int i, vote;
  for(i = 0; i < _probe._nameInputs.size(); i++){
    if(strcmp(pattern->Name(), _probe._nameInputs[i]) == 0) break;
  }
  vote = Vote();
  if(vote == -1){
    _probe._probeStats[_numNames][i]++;
  }else{
    _probe._probeStats[vote][i]++;
  }
/*
  if(_probe._probeSpiking.size() > 0){
    for(list<int>::iterator iter = _probe._probeSpiking.begin(); iter != _probe._probeSpiking.end(); iter++)
      _probe._probeStats[*iter][i]++;
  }else{
    _probe._probeStats[SearchForNeuronGroup("output")->Size()][i]++;
  }
*/
  if(vote == -1) cout<<"vote = "<<"N/A"<<endl;
  else cout<<"vote = "<<_probe._nameInputs[vote]<<endl;
  cout<<"name = "<<_probe._nameInputs[i]<<endl;

  if(vote == -1) cout<<"error! #"<<index<<"\t"<<_probe._nameInputs[i]<<" -> N/A"<<endl;
  else if(strcmp(_probe._nameInputs[vote],_probe._nameInputs[i])!=0) cout<<"error! #"<<index<<"\t"<<_probe._nameInputs[i]<<" -> "<<_probe._nameInputs[vote]<<endl;
}

void Network::NeuronStatAccumulate(){
  assert(_probe._probeFlag == 0);
  for(std::list<Neuron*>::iterator iter =  _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->StatAccumulate();
}

void Network::SetCurrentPattern(Pattern * pattern){
  _currentPattern = pattern;
}

void Network::SetLabel(){
  assert(_probe._probeFlag == 0);
  for(std::list<Neuron*>::iterator iter =  _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->SetLabel();
}

void Network::SetLabel(const char * name){
  NeuronGroup * neurongroup = SearchForNeuronGroup(name);
  assert(neurongroup != NULL);
  int size = _probe._nameInputs.size();
  assert(size != 0);
  neurongroup->SetLabel(size);
}

void Network::SaveLabel(){
  NeuronGroup * neurons;
  Neuron * neuron;
  FILE * fp;
  neurons = SearchForNeuronGroup("output");

  fp = fopen("label.txt","w");
  for(neuron = neurons->First(); neuron != NULL; neuron = neurons->Next()){
    fprintf(fp,"%d\n",neuron->GetLabel());
  }
  fclose(fp);
}

void Network::ReadLabel(){
  NeuronGroup * neurons;
  Neuron * neuron;
  FILE * fp;
  char line[64];
  neurons = SearchForNeuronGroup("output");

  fp = fopen("label.txt","r");
  for(neuron = neurons->First(); neuron != NULL; neuron = neurons->Next()){
    assert(fgets(line, 64, fp) != NULL);
    neuron->SetLabel(atoi(line));
  }
  fclose(fp);
}

int Network::Vote(){
  assert(_probe._probeFlag == 1);
  int i, vote, max;
  for(i = 0; i < _numNames; i++) _voteCounts[i] = 0;
  for(std::list<Neuron*>::iterator iter =  _allNeurons.begin(); iter != _allNeurons.end(); iter++){
    vote = (*iter)->Vote();
    if(vote != -1) _voteCounts[vote]++;
  }
  cout<<"Name";
  for(i = 0; i < _numNames; i++) cout<<"\t\""<<_probe._nameInputs[i]<<"\"";
  cout<<endl;
  max = -1;
  vote = -1;
  cout<<"Vote";
  for(i = 0; i < _numNames; i++){
    cout<<"\t"<<_voteCounts[i];
    if((max < _voteCounts[i])&&(_voteCounts > 0)){
      max = _voteCounts[i];
      vote = i;
    }
  }
  cout<<endl;
  if(max == 0) return (-1);
  return vote;
}

void Network::PrintProbeStats(){
  assert(_probe._probeFlag == 1);
  int i, j;
//  int numOutput = SearchForNeuronGroup("output")->Size();
//  int numInput = _probe._nameInputs.size();
  int numNames = _probe._nameInputs.size();
  int numInput = numNames;
  int numOutput = numNames;
  int * map = new int[numInput+1];
//  int * max = new int[numInput];
  int * cls = new int[numInput];
  int * total = new int[numInput];
  char * tempName;
  int tempIndex;
  double ratio;
  int totalCls, totalSpike;
  for(i = 0; i < numInput; i++){
    map[i] = i;
//    max[i] = 0;
    cls[i] = 0;
    total[i] = 0;
  }
  map[numInput] = numInput;

  for(i = numInput-1; i > 0; i--)
    for(j = 0; j < i; j++)
      if(strcmp(_probe._nameInputs[map[j]],_probe._nameInputs[map[j+1]]) > 0){
        tempIndex = map[j];
        map[j] = map[j+1];
        map[j+1] = tempIndex;
      }
/*
      if(strcmp(_probe._nameInputs[j],_probe._nameInputs[j+1]) > 0){
        tempName = _probe._nameInputs[j];
        _probe._nameInputs[j] = _probe._nameInputs[j+1];
        _probe._nameInputs[j+1] = tempName;
        tempIndex = map[j];
        map[j] = map[j+1];
        map[j+1] = tempIndex;
      }
*/
  for(j = 0; j < numInput; j++)
    for(i = 0; i <= numOutput; i++){
      total[j] += _probe._probeStats[i][j];
//      if(max[j] < _probe._probeStats[i][j]) max[j] = _probe._probeStats[i][j];
    }

  for(i = 0; i < numInput; i++) cls[i] = _probe._probeStats[i][i];

/*
  for(i = 0; i < numOutput; i++){
    ratio = 0;
    tempIndex = -1;
    for(j = 0; j < numInput; j++){
      if(_probe._probeStats[i][j] == 0) continue;
      if(ratio < ((double)_probe._probeStats[i][j])/((double)total[j])){
        ratio = ((double)_probe._probeStats[i][j])/((double)total[j]);
        tempIndex = j;
      }
    }
    if(tempIndex >= 0) cls[tempIndex] += _probe._probeStats[i][tempIndex];
  }
*/
  cout<<endl;
  for(j = 0; j < numInput; j++) cout<<"\t\""<<_probe._nameInputs[map[j]]<<"\"";
  cout<<"\tunexpected"<<endl;

  for(i = 0; i <= numOutput; i++){
    if(i == numOutput) cout<<"miss";
    else cout<<"O_"<<_probe._nameInputs[map[i]];
    for(j = 0; j <= numInput; j++)cout<<'\t'<<_probe._probeStats[map[i]][map[j]];
    cout<<endl;
  }

//  cout<<"max";
//  for(i = 0; i < numInput; i++)cout<<'\t'<<max[map[i]];
//  cout<<endl;
  cout<<"cls";
  for(i = 0; i < numInput; i++)cout<<'\t'<<cls[map[i]];
  cout<<endl;
  cout<<"tot";
  for(i = 0; i < numInput; i++)cout<<'\t'<<total[map[i]];
  cout<<endl;

  totalCls = 0;
  totalSpike = 0;
  for(i = 0; i < numInput; i++){
    totalCls += cls[i];
    totalSpike += total[i];
  }
  cout<<"classification rate = "<<totalCls<<"/"<<totalSpike<<" = "<<((double)totalCls)/((double)totalSpike)<<endl;

//  Rate = ((double)totalCls)/((double)totalSpike);

  delete [] total;
  delete [] cls;
//  delete [] max;
  delete [] map;
}

void Network::PatternToFile(char * name){
  int index = 0;
  FILE * fp;
  char filename[128];
  for(list<Pattern*>::iterator iter = _patterns->begin(); iter != _patterns->end(); iter++){
    if(strcmp((*iter)->Name(), name) == 0){
      sprintf(filename,"pattern_%d",index);
      fp = fopen(filename,"w");
      (*iter)->PrintFile(fp);
      fclose(fp);
      index++;
    }
  }
}

void Network::PatternToFile(){
  int index = 0;
  FILE * fp;
  char filename[128];

  for(list<Pattern*>::iterator iter = _patterns->begin(); iter != _patterns->end(); iter++){
    sprintf(filename,"pattern_%d",index);
    fp = fopen(filename,"w");
    (*iter)->PrintFile(fp);
    fclose(fp);
    index++;
  }
/*
  Pattern * pattern;
  for(pattern = FirstPattern(); pattern != NULL; pattern = NextPattern()){
    sprintf(filename,"pattern_%d\0",index);
    fp = fopen(filename,"w");
    pattern->PrintFile(fp);
    fclose(fp);
    index++;
  }
*/
}

void Network::PrintTrainingResponse(){
  int i, vote;
  int * votes = new int[_probe._nameInputs.size()];
  for(i = 0; i < _probe._nameInputs.size(); i++) votes[i] = 0;
  cout<<endl<<"\t";
  for(i = 0; i < _numNames; i++) cout<<"\t\'"<<_probe._nameInputs[i]<<"\'";
  cout<<"\tvote"<<endl;
  for(std::list<Neuron*>::iterator iter =  _allNeurons.begin(); iter != _allNeurons.end(); iter++){
    vote = (*iter)->PrintTrainingResponse();
    if(vote != -1) votes[vote]++;
  }
  cout<<"votes\t";
  for(i = 0; i < _probe._nameInputs.size(); i++) cout<<'\t'<<votes[i];
  cout<<endl;
  delete [] votes;
}

void Network::PatternsNormalization(){
  for(list<Pattern*>::iterator iter = _patternsTrain.begin(); iter != _patternsTrain.end(); iter++) (*iter)->Normalization();
  for(list<Pattern*>::iterator iter = _patternsTest.begin(); iter != _patternsTest.end(); iter++) (*iter)->Normalization();
}

void Network::CodingLevel(){
  for(list<Pattern*>::iterator iter = _patterns->begin(); iter != _patterns->end(); iter++) cout<<(*iter)->NonZeroPixel()<<"\t"<<(*iter)->TotalPixelStrength()<<endl;
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

void Network::LSMChannelDecrement(channelmode_t channelmode){
  if(channelmode == INPUTCHANNEL) _lsm_input--;
  else _lsm_reservoir--;
}

int Network::myrandom(int i){
  srand(0);
  return rand()%i;
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
    neuronmode_reservoir == WRITECHANNEL;
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


void Network::WriteSynWeightsToFile(const char * syn_type, char * filename){
  ofstream f_out(filename);
  if(!f_out.is_open()){
    cout<<"In Network::WriteSynWeightsToFile(), cannot open the file :"<<endl;
    exit(EXIT_FAILURE);
  }
  
  synapsetype_t  wrt_syn_type = INVALID;
  if(strcmp(syn_type, "reservoir") == 0){
    // Write reservoir syns into the file
    wrt_syn_type = RESERVOIR_SYN;
  }
  else{
    cout<<"In Network::WriteSynWeightsToFile(), undefined synapse type: "<<syn_type<<endl;
    exit(EXIT_FAILURE);
  }
  
  // write the reservoir synapse back to the file:
  if(wrt_syn_type == RESERVOIR_SYN){
    assert(!_rsynapses.empty());
    
    cout<<"_rsynapse has size: "<<_rsynapses.size()<<endl;
    for(size_t i = 0; i < _rsynapses.size(); ++i)
      f_out<<i<<"\t"<<_rsynapses[i]->PreNeuron()->Name()
	   <<"\t"<<_rsynapses[i]->PostNeuron()->Name()
	   <<"\t"<<_rsynapses[i]->Weight()
	   <<endl;
  }
  
}

