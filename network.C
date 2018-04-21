#include "def.h"
#include "neuron.h"
#include "synapse.h"
#include "speech.h"
#include "network.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <climits>
#include <cmath>
#include <string>
#include <unistd.h>

//#define _DEBUG_VARBASE
//#define _DEBUG_CORBASE
//#define _SHOW_SPEECH_ORDER
//#define _PRINT_SPIKE_COUNT

using namespace std;

extern double Rate;
extern int file[500];
int COUNTER_LEARN = 0;

Network::Network():
_readout_correct(vector<int>(NUM_ITERS, 0)),
_readout_wrong(vector<int>(NUM_ITERS, 0)),
_readout_even(vector<int>(NUM_ITERS, 0)),
_readout_correct_breakdown(vector<vector<int> >(NUM_ITERS, vector<int>(CLS, 0))),
_readout_test_error(vector<double>(NUM_ITERS, 0.0)),
_sp(NULL),
_fold(0),
_fold_ind(-1),
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
    for(auto & b_neuron : _allBiasNeurons)
        delete b_neuron;
    for(auto & p : _groupNeurons)
        delete (p.second);
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++)
        delete (*iter);
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end();++iter)
        delete (*iter);
    for(vector<Speech*>::iterator iter = _speeches.begin(); iter != _speeches.end();++iter)
        delete (*iter);
}

bool Network::CheckExistence(char * name){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        if(strcmp((*iter)->Name(),name) == 0) return true;

    if(_groupNeurons.find(string(name)) != _groupNeurons.end()) return true;

    return false;
}

void Network::AddNeuron(char * name, bool excitatory, double v_mem){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);

    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    Neuron * neuron = new Neuron(name, excitatory, this, v_mem);
    _individualNeurons.push_back(neuron);
    _allNeurons.push_back(neuron);
    if(excitatory == true){
        _allExcitatoryNeurons.push_back(neuron);
        neuron->SetExcitatory();
    }else _allInhibitoryNeurons.push_back(neuron);
}


void Network::AddBiasNeuron(char * ng_name, int dummy_freq)
{
    NeuronGroup * ng = SearchForNeuronGroup(ng_name);
    assert(ng);
    string name = string(ng_name) + "_" + to_string(ng->Size()) + "bias";
    double vmem = ng->Order(0)->GetVth();
    BiasNeuron* b_neuron = new BiasNeuron((char*)name.c_str(), true, this, vmem, dummy_freq);
    
    ng->SetBiasNeuron(b_neuron);
    _allBiasNeurons.push_back(b_neuron);
}


void Network::AddNeuronGroup(char * name, int num, bool excitatory, double v_mem){
    assert(num >= 1);
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);

    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    NeuronGroup * neuronGroup = new NeuronGroup(name, num, this, excitatory, v_mem);
    _groupNeurons[string(name)] = neuronGroup;

    string name_str(name);
    if(name_str.substr(0, 6) == "hidden")   _hidden_layer_names.push_back(name_str);
   
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
        vector<Synapse*>* outputSyns = neuron->LSMDisconnectNeuron(); // This is just to disconnect the neuron from the network. 

        // record those synapses:
        for(auto iter = (*outputSyns).begin(); iter != (*outputSyns).end(); iter++){
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

    _groupNeurons.erase(string(name));

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
    AddNeuronGroup(const_cast<char*>("input"), num_inputs, true, LSM_V_THRESH);
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    LSMAddSynapse(const_cast<char*>("input"), reservoir->Name(), 1, num_connections, 8, 1, true); 
    // from       to         fromN   toN         value  random  fixed

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

    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    NeuronGroup * neuronGroup = new NeuronGroup(name, dim1, dim2, dim3, this);

    _groupNeurons[string(name)] = neuronGroup;

    for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
        _allNeurons.push_back(neuron);
        if(neuron->IsExcitatory()) _allExcitatoryNeurons.push_back(neuron);
        else _allInhibitoryNeurons.push_back(neuron);
    }
}

//* construct the brain like topology of the neuron group. Not used now!
void Network::LSMAddNeuronGroup(char * name, char * path_info_neuron, char * path_info_synapse){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);
    
    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    NeuronGroup * neuronGroup = new NeuronGroup(name, path_info_neuron, path_info_synapse,this); //This function helps to generate the reservoir with brain-like connectivity
    _groupNeurons[string(name)] = neuronGroup;

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
                /* 
                   int D_weight = 1;
                   if(!(pre->IsExcitatory()) D_weight = -8 + rand()%9; // best for depressive STDP
                   */
                int D_weight = -8 + rand()%15;
                // set the sign of the readout weight the same as pre-neuron E/I might be hampful for the performance
                // if((!pre->IsExcitatory() && D_weight > 0) ||(pre->IsExcitatory() && D_weight < 0))  D_weight = -1*D_weight;
                //D_weight = 0;
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
                /*** The initial weights are in W_max*(-1 to 1)          ***/
                factor = rand()%100000;
                factor = factor*2/99999-1;
            }else if(random == 0) factor = 0;
            else assert(0);
            double weight = value*factor/weight_limit;
            // set the sign of the readout weight the same as pre-neuron E/I might be hampful for the performance
            // if((!pre->IsExcitatory() && weight > 0) ||(pre->IsExcitatory() && weight < 0))  weight = -1*weight; 
            //if(weight > 0)  weight = weight*0.5;
            LSMAddSynapse(pre,neuron,weight,fixed,weight_limit,liquid);
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

//* add the lateral inhibition inside the layer
void Network::LSMAddSynapse(NeuronGroup * pre, int npre, int npost, int value, int random, bool fixed){
    assert(npre == -1 && npost == -1);
    assert(fixed);
    assert(value > 0);
    pre->SetLateral();
    pre->SetLateralWeight(double(value));
    for(Neuron * pre_neuron = pre->First(); pre_neuron != NULL; pre_neuron = pre->Next()){
        for(int i = 0; i < pre->Size(); ++i){
            Neuron * post_neuron = pre->Order(i);
            if(pre_neuron == post_neuron)   continue;
            // weight_value, fixed, weight_limit, is_in_liquid
            LSMAddSynapse(pre_neuron, post_neuron, int(-value), fixed, int(value), false);
        }
    }
}

void Network::LSMAddSynapse(NeuronGroup * pre, NeuronGroup * post, int npre, int npost, int value, int random, bool fixed){
    assert((npre == 1)||(npre == -1));
    for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next()){
        LSMAddSynapse(neuron,post,npost,value, random, fixed);
    }
    if(pre->GetBiasNeuron() != NULL){
        BiasNeuron * b_neuron = pre->GetBiasNeuron();
        LSMAddSynapse(b_neuron, post, npost, value, 0, fixed);
    }
}

void Network::LSMAddSynapse(char * pre, char * post, int npre, int npost, int value, int random, bool fixed){
    NeuronGroup * preNeuronGroup = SearchForNeuronGroup(pre);
    NeuronGroup * postNeuronGroup = SearchForNeuronGroup(post);
    assert((preNeuronGroup != NULL)&&(postNeuronGroup != NULL));
    
    // add synapses
    if(strcmp(pre, post) != 0) // between layer connections
        LSMAddSynapse(preNeuronGroup, postNeuronGroup, npre, npost, value, random, fixed);
    else{ // laterial inhibition
        assert(preNeuronGroup == postNeuronGroup);
        LSMAddSynapse(preNeuronGroup, npre, npost, value, random, fixed);
    } 
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


void Network::LSMTruncNeurons(int loss_neuron){
    int i,j,k;
    char *_name;

    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);
    while(i < loss_neuron){
        int j = 0;
        int k = rand()%135; //Only select neuron in liquid
        // for all reservoir neurons:
        for(Neuron * neuron = reservoir->First(); neuron != NULL; neuron = reservoir->Next()){
            if(j == k){
                _name = neuron->Name();
                assert((_name[0] == 'r')&&(_name[1] == 'e')&&(_name[2] == 's'));
                if(neuron->LSMCheckNeuronMode(DEACTIVATED) == false){
                    cout<<"The reservoir neuron being removed has index: "<<k<<endl;
                    neuron->LSMSetNeuronMode(DEACTIVATED);
                    ++i;
                    reservoir->UnlockFirst();
                    break;
                }
            }
            else j++;
        }
    }
}


int Network::LSMSizeAllNeurons(){
    return _allNeurons.size();
}


//***********************************************************************************
//
// This function is to perform the adaptive gating.
// If the indegree of a reservoir neuron is <= INDEG_LIMIT
// and its outdegree is <= OUTDEC_DIGIT, we will turn off this neuron to save energy
//
//***********************************************************************************
void Network::LSMAdaptivePowerGating(){
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);
    int cnt = 0;
    // for all reservoir neurons:
    for(Neuron * neuron = reservoir->First(); neuron != NULL; neuron = reservoir->Next()){
        // do the power gating:
        cnt += neuron->PowerGating();
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
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);

    int cnt = 0;
    // for all reservoir neurons:
    for(Neuron * neuron = reservoir->First(); neuron != NULL; neuron = reservoir->Next()){
        // do the power gating:
        if(neuron->LSMCheckNeuronMode(DEACTIVATED)){
            cnt++;
            cout<<"The neuron gated: "<<neuron->Name()<<endl;
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
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);

    int cnt = 0;
    for(Neuron * neuron = reservoir->First(); neuron != NULL; neuron = reservoir->Next()){
        // do the hub detection
        if(neuron->IsHubRoot()){
            cnt++;
            cout<<"The neuron which is the core of hub "<<neuron->Name()<<endl;
        }
    }
    cout<<"Total number of hubs in the reservoir is "<<cnt<<endl;
}

//****************************************************************************************
//
// This is a function wrapper for running the transient simulation
// @param2: can be "train_sample", samples for training and cv, 
//                 "test_sample", samples for purely testing, need to enable "USE_TEST_SAMPLE"
//****************************************************************************************
void Network::LSMTransientSim(networkmode_t networkmode, int  tid, const string sample_type){
    int count = 0;
    LSMClearSignals();
    int info = this->LoadFirstSpeech(false, networkmode, sample_type);
    while(info != -1){
        count++;
        int time = 0, end_time = this->SpeechEndTime();
        while(!LSMEndOfSpeech(networkmode, end_time)){
            LSMNextTimeStep(++time,false,1, end_time, NULL);
        }
#ifdef QUICK_RESPONSE   //Only for NMNIST
        SpeechPrint(info+(GetTid()%10)*(NumSpeech()>400?600:100), "reservoir"); // dump the reservoir response for quick simulation
#else

#ifdef _DUMP_RESPONSE
        if(tid == 0 && sample_type == "train_sample"){
			SpeechPrint(info, "all");
            DumpVMems("Waveform/transient", info, "reservoir");
        }
#endif
#endif

#ifdef _VARBASED_SPARSE
        // Collect the firing activity of each reservoir neurons from sp
        // and scatter the frequency back to the network:
        cout<<"Begin readout sparsification...."<<endl;
        vector<double> fs;
        this->CollectSpikeFreq("reservoir", fs, time);
        this->ScatterSpikeFreq("reservoir", fs);
        cout<<"Done with the readout sparsification!"<<endl;
#endif
        LSMClearSignals();
        info = LoadNextSpeech(false, networkmode, sample_type);
    }

}


//****************************************************************************************
//
// This is a function wrapper for training the readout synapses supervisedly! 
// Two approaches: 1. Yong's old rule, 2. Supervised STDP.
// Note that the readout training is implemented through changing network stat.
// @param3: the # of iterations
//****************************************************************************************
void Network::LSMSupervisedTraining(networkmode_t networkmode, int tid, int iteration){
    int info = -1;
    int count = 0;
    vector<pair<int, int> > correct, wrong, even;
    vector<pair<Speech*, bool> > predictions; // the predictions for each speech
    FILE * Foutp = NULL;
    char filename[64];
    vector<double> each_sample_errors;    

    // 1. Clear the signals
    LSMClearSignals();
   
    // 2. Load the first speech:
#ifdef CV
    info = LoadFirstSpeechTrainCV(networkmode);
#else
    info = LoadFirstSpeech(true, networkmode);
#endif
    double cost = 0;
    // 3. Load the speech for training
    while(info != -1){
        count++;
        int time = 0, end_time = this->SpeechEndTime();
        Foutp = NULL;
#ifdef _SHOW_SPEECH_ORDER
        cout<<"************* Speech : "<<info<<" ****************"<<endl;
#endif
        while(!LSMEndOfSpeech(networkmode, end_time)){
            LSMNextTimeStep(++time, true, iteration, end_time, NULL); 
        }

#ifdef _DUMP_RESPONSE
        if(tid == 0 && iteration == 0){
            DumpVMems("Waveform/train", info, "output");
            DumpVMems("Waveform/train", info, "hidden_0");
            DumpHiddenOutputResponse("train", info);
        }
#endif
        // compute the boost error
#ifdef _BOOSTING_METHOD
        ComputeBoostError(predictions);  
#endif

       // Apply the error bp when the speech is played
        if(_network_mode == READOUTBP){ 
            BackPropError(iteration, end_time);
            cost += GetBpCost();
        }

#if defined(_UPDATE_AT_LAST)
        UpdateLearningWeights();
#endif

        LSMClearSignals();

#ifdef CV
        info = LoadNextSpeechTrainCV(networkmode);
#else
        info = LoadNextSpeech(true, networkmode);
#endif

    }
    // update the training weight of each sample
#ifdef _BOOSTING_METHOD
    BoostWeightUpdate(predictions);
#endif
    if(_network_mode == READOUTBP)
        cout<<"@"<<iteration<<"th iteration, the cost on the training sample: "<<cost/count<<endl;

    LSMClearSignals();

    // 4. Load the speech for testing
#ifdef CV
    info = LoadFirstSpeechTestCV(networkmode);
#elif defined(USE_TEST_SAMPLE)
    info = LoadFirstSpeech(false, networkmode, "test_sample");
#else
    info = LoadFirstSpeech(false, networkmode);
#endif
    cost = 0;
    count = 0;
    while(info != -1){
        Foutp = NULL;
#if defined(_WRITE_STAT)
        sprintf(filename,"outputs/spikepattern%d.dat",info);
        Foutp = fopen(filename,"a");
        assert(Foutp != NULL);
#endif

        count++;
        int time = 0, end_time = this->SpeechEndTime();
        while(!LSMEndOfSpeech(networkmode, end_time)){
            LSMNextTimeStep(++time, false, 1, end_time, Foutp);
        }
#if defined(_WRITE_STAT)
        assert(Foutp != NULL);
        fprintf(Foutp,"%d\t%d\n",-1,-1);
        fclose(Foutp);
#endif

        ReadoutJudge(correct, wrong, even); // judge the readout output here
        CollectErrorPerSample(each_sample_errors); // collect the test error for the sample
        if(_network_mode == READOUTBP)  cost += GetBpCost();

#ifdef _PRINT_SPIKE_COUNT
        if(tid == 0 && (iteration == 0 || (iteration+1) % 5 == 0))
            PrintSpikeCount("output"); // print the readout firing counts
#endif


#ifdef _DUMP_RESPONSE 
        if(tid == 0 && iteration == 0){
            DumpVMems("Waveform/test", info, "output");
            DumpVMems("Waveform/test", info, "hidden_0");
            DumpHiddenOutputResponse("test", info);
        }
#endif

#ifdef _DUMP_CALCIUM
        if(tid == 0 && iteration == 49){
            DumpCalciumLevels("readout", "Calcium", "speech_"+ to_string(info));
        }
#endif
        LSMClearSignals();

#ifdef CV
        info = LoadNextSpeechTestCV(networkmode);
#elif defined(USE_TEST_SAMPLE)
        info = LoadNextSpeech(false, networkmode, "test_sample");
#else
        info = LoadNextSpeech(false, networkmode);
#endif
    }
    // 5. Push the recognition result:
    LSMPushResults(correct, wrong, even, iteration);

    // 6. Log the test error
    LogTestError(each_sample_errors, iteration);

    // 7. Output the cost on the test samples
    if(_network_mode == READOUTBP)
        cout<<"@"<<iteration<<"th iteration, the cost on the testing sample: "<<cost/count<<endl;

}


//****************************************************************************************
//
// This is a function wrapper for training the input or readout synapses. 
// Now I am using STDP.
// Note that the input/readout STDP training is implemented through changing network stat.
//
//****************************************************************************************
void Network::LSMUnsupervisedTraining(networkmode_t networkmode, int tid){
    int info = -1;
    // 1. Clear the signals
    LSMClearSignals();

    // 2. Load the first speech:
    info = LoadFirstSpeech(false, networkmode);
    FILE * Foutp = NULL;
    char filename[64];

    // 3. Simulate
    while(info != -1){
        int time = 0, end_time = this->SpeechEndTime();
#ifdef _SHOW_SPEECH_ORDER
        cout<<"========================\n"<<"Loading speech: "<<info<<"\n======================\n"<<endl;
#endif
        Foutp = NULL;
#ifdef _DUMP_RESPONSE
        if(tid == 0){
            sprintf(filename,"spikes/Readout_Response_Unsupv/readout_spikes_%d.dat",info);
            Foutp = fopen(filename,"a");
            assert(Foutp != NULL);
        }
#endif
        while(!LSMEndOfSpeech(networkmode, end_time)){
            LSMNextTimeStep(++time, false, 1, end_time, Foutp); 
        }
#ifdef _DUMP_RESPONSE
        if(tid == 0){
            assert(Foutp != NULL);
            fprintf(Foutp,"%d\t%d\n",-1,-1);
            fclose(Foutp);
        }
#endif
        LSMClearSignals();
        info = LoadNextSpeech(false, networkmode);
    }
    LSMClearSignals();

}


//* this is a function wrapper to train the reservoir. right now I am using STDP.
//* the readout layer is not trained during this stage
void Network::LSMReservoirTraining(networkmode_t networkmode){
    int info = -1;

    //1. Clear the signals
    LSMClearSignals();
    // note that no training of the readout synapses happens during this stage!

    // 2. Loop through reservoir:
    info = LoadFirstSpeech(false, networkmode);
    while(info != -1){
        int time = 0, end_time = this->SpeechEndTime();
#ifdef _SHOW_SPEECH_ORDER
        cout<<"================================================\n"
            <<"Loading speech: "<<info
            <<"\n===============================================\n"<<endl;
#endif
        while(!LSMEndOfSpeech(networkmode, end_time)){
            // the 6th parameter is the pointer to the individual reservoir,
            // if the 6th parameter is NULL meaning we are using all reservoirs.
            LSMNextTimeStep(++time, false, 1, end_time, NULL); 
        }

#ifdef _PRINT_SYN_ACT
        char filename[128];
        sprintf(filename, "activity/speech_%d.txt", info);
        WriteSynActivityToFile("reservoir_1", "reservoir_15");
#endif
        LSMClearSignals();
        info = LoadNextSpeech(false, networkmode);
    }

}

//* next simulation step; the last parameter should be the ptr to reservoir or NULL.
void Network::LSMNextTimeStep(int t, bool train,int iteration,int end_time, FILE * Foutp){
    _lsm_t = t;

/*
#if defined(_EACH_SYNAPSE_RESPONSE)
    for(vector<Synapse*>::iterator iter = _rosynapses.begin(); iter != _rosynapses.end(); ++iter) (*iter)->LSMRespDecay(t, end_time);
#endif

    for(list<Synapse*>::iterator iter = _lsmActiveSyns.begin(); iter != _lsmActiveSyns.end(); iter++) (*iter)->LSMNextTimeStep();
*/
    _lsmActiveSyns.clear();

    // Please remember that the neuron firing activity will change the list: 
    // _lsmActiveSyns, _lsmActiveSTDPLearnSyns, and _lsmActiveLearnSyns:
    for(Neuron * neuron : _allNeurons)  neuron->LSMNextTimeStep(t, Foutp, train, end_time);

    for(BiasNeuron * b_neuron : _allBiasNeurons) b_neuron->LSMNextTimeStep(t, Foutp, train, end_time);

    // Update the previous delta effect with the current delta effect
    for(Neuron * neuron : _allNeurons)  neuron->UpdateDeltaEffect();
 
    // train the reservoir/input/readout using STDP (unsupervised):
    if(_network_mode == TRAINRESERVOIR || _network_mode == TRAININPUT || _network_mode == TRAINREADOUT){
        assert(train == false);
#ifndef _LUT_HARDWARE_APPROACH
        // using state variable to implement the STDP mechanism
        if(_network_mode == TRAINRESERVOIR){
            // Update the local variable implememnting STDP:
            for(Synapse * synapse : _rsynapses) synapse->LSMUpdate(t);
        }
        else{
            // for training the input/reaodut synapses (unsupervised)
            vector<Synapse*> &tmp_synapses = _network_mode == TRAININPUT ? _isynapses : _rosynapses;
            
            for(Synapse * synapse : tmp_synapses) synapse->LSMUpdate(t);
        }
#endif

        for(Synapse * synapse : _lsmActiveSTDPLearnSyns)    synapse->LSMSTDPLearn(t);
        _lsmActiveSTDPLearnSyns.clear();
    }

    if(train == true) {
        if(_network_mode == READOUT){
            for(Synapse * synapse : _lsmActiveLearnSyns)    synapse->LSMLearn(t, iteration);
            _lsmActiveLearnSyns.clear();
            for(Synapse * synapse : _lsmActiveSTDPLearnSyns){
                if(synapse->IsReadoutSyn()) synapse->LSMSTDPSupvLearn(t, iteration); 
                else    synapse->LSMSTDPLearn(t);
            }
            _lsmActiveSTDPLearnSyns.clear();
        }else if(_network_mode == READOUTSUPV){
            for(Synapse * synapse : _lsmActiveSTDPLearnSyns)    synapse->LSMSTDPSupvLearn(t, iteration); 
            _lsmActiveSTDPLearnSyns.clear();
        }
    }

#ifdef _CORBASED_SPARSE
    if(_network_mode == TRANSIENTSTATE){
            NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
            assert(reservoir);
            reservoir->ComputeCorrelation();
        }
    }
#endif
}


Neuron * Network::SearchForNeuron(const char * name){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        if(strcmp((*iter)->Name(),name) == 0){
            return (*iter);
        }

    return NULL;
}

Neuron * Network::SearchForNeuron(const char * group_name, const char * index){
    int ind = atoi(index);    

    NeuronGroup * group = SearchForNeuronGroup(group_name);
    assert(group); 
    
    Neuron * neuron = group->Order(ind);
    assert(neuron);

    return neuron;
}


NeuronGroup * Network::SearchForNeuronGroup(const char * name){
    string group_name(name);
    if(_groupNeurons.find(group_name) == _groupNeurons.end()){
        cout<<"Warning::Search for neuron group: "<<group_name<<" not found!"<<endl;
        return NULL;
    }
    return _groupNeurons[group_name];
}

// inquiry to know the ending time of the current speech being loaded:
int Network::SpeechEndTime(){
    if(_sp == NULL){
        cout<<"In Network::EndSpeechTime(), find out the attached _sp in network"
            <<" is valid!!"<< endl;
        assert(_sp != NULL);
    }
    return _sp->EndTime();
}

void Network::AddSpeech(Speech * speech){
    _speeches.push_back(speech);
}

void Network::AddTestSpeech(Speech * speech){
    _t_speeches.push_back(speech);
}

//* function wrapper
//  1. Given a iter pointed to speech, load this speech to both input & reservoir layer.
//  2. No need to load the channels of speech to the reservoir during STDP training.
//  3. Alway need to set the neuron mode
void Network::LoadSpeeches(Speech      * sp,
        neuronmode_t neuronmode_input,
        neuronmode_t neuronmode_reservoir,
        neuronmode_t neuronmode_readout,
        bool train
        )
{
    // assign the speech ptr here!
    assert(sp);
    _sp = sp;

    assert(_lsm_input_layer);
    _lsm_input_layer->LSMLoadSpeech(sp,&_lsm_input,neuronmode_input,INPUTCHANNEL);

    assert(_lsm_reservoir_layer);
    // reservoir channell will not be loaded for STDP training
    bool load_reservoir_channel = _network_mode == TRAINRESERVOIR || _network_mode == TRAININPUT ? false : true;
    if(_network_mode == READOUT && neuronmode_reservoir == STDP)
        load_reservoir_channel = false;
    if(load_reservoir_channel){
        _lsm_reservoir_layer->LSMLoadSpeech(sp,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    }else{
        _lsm_reservoir_layer->LSMSetNeurons(neuronmode_reservoir);
    }
    
    // the hidden neuron should be in the same mode as the output neuron
    if(!_lsm_hidden_layers.empty()){
        for(NeuronGroup* hidden : _lsm_hidden_layers){
            assert(hidden);
            hidden->LSMSetNeurons(neuronmode_readout);
        }
    }

    // just set the neuron mode of the output neurons
    assert(_lsm_output_layer);
    _lsm_output_layer->LSMSetNeurons(neuronmode_readout);

    // if train the readout
    if(train == true){
        if(neuronmode_readout == NORMAL){ // original Yong's rule
            _lsm_output_layer->LSMSetTeacherSignalStrength(DEFAULT_TS_STRENGTH_P, DEFAULT_TS_STRENGTH_N, DEFAULT_TS_FREQ);
            _lsm_output_layer->LSMSetTeacherSignal(sp->Class());
        }
        else if(neuronmode_readout == NORMALSTDP){ // supervised STDP and reward STDP
            _lsm_output_layer->LSMSetTeacherSignalStrength(TS_STRENGTH_P_SUPV, TS_STRENGTH_N_SUPV, TS_FREQ_SUPV);
            _lsm_output_layer->LSMSetTeacherSignal(sp->Class());
        }
        else if(neuronmode_readout == NORMALBP){ // error backprop, set the TS = 0
            _lsm_output_layer->LSMSetTeacherSignalStrength(0, 0, 10000);
        }
        else	  assert(0);
    }
}


//* function wrapper for removing speeches in the network 
void Network::LSMNetworkRemoveSpeech(){
    _sp = NULL;
    assert(_lsm_input_layer || _lsm_output_layer);

    if(_lsm_input_layer)
        _lsm_input_layer->LSMRemoveSpeech();
    _lsm_input_layer = NULL;

    if(_lsm_output_layer)
        _lsm_output_layer->LSMRemoveSpeech();
    _lsm_output_layer = NULL;

    if(_lsm_reservoir_layer == NULL)
        _lsm_reservoir_layer->LSMRemoveSpeech();

    _lsm_reservoir_layer = NULL;
}

//* Load the hidden layer ptrs
void Network::InitializeHiddenLayers(){
    _lsm_hidden_layers.clear();
    for(string layer_name : _hidden_layer_names){
        NeuronGroup* hidden = SearchForNeuronGroup(layer_name.c_str());
        assert(hidden);
        _lsm_hidden_layers.push_back(hidden);
    }
}


//* function wrapper for load the input, reservoir and output layers/neurongroups:
void Network::LSMLoadLayers(){
    assert(_lsm_input_layer == NULL);
    _lsm_input_layer = SearchForNeuronGroup("input");
    assert(_lsm_input_layer != NULL);

    assert(_lsm_reservoir_layer == NULL);
    _lsm_reservoir_layer = SearchForNeuronGroup("reservoir");
    assert(_lsm_reservoir_layer != NULL);

    if(!_hidden_layer_names.empty()) InitializeHiddenLayers();
    
    _lsm_output_layer = SearchForNeuronGroup("output");
    assert(_lsm_output_layer != NULL);
}

/************************************************************************
 * The function implement the mapping: networkmode-> neuron modes
 * Implement the different neuron mode here for STDP/Supervised Training
 ***********************************************************************/
void Network::DetermineNetworkNeuronMode(const networkmode_t & networkmode, neuronmode_t & neuronmode_input, neuronmode_t & neuronmode_reservoir, neuronmode_t & neuronmode_readout){
    if(networkmode == TRANSIENTSTATE){
        neuronmode_input = READCHANNEL;
        neuronmode_reservoir = WRITECHANNEL;
        neuronmode_readout = DEACTIVATED;
    }else if(networkmode == TRAINRESERVOIR){
        neuronmode_input = READCHANNEL;
        neuronmode_reservoir = STDP;
        neuronmode_readout = DEACTIVATED;
    }else if(networkmode == TRAININPUT){
        neuronmode_input = READCHANNELSTDP; // read channel and enable STDP
        neuronmode_reservoir = NORMALSTDP;  // enable inputSyns trained by STDP
        neuronmode_readout = NORMAL;
    }else if(networkmode == TRAINREADOUT){ 
        neuronmode_input = DEACTIVATED;
        neuronmode_reservoir = READCHANNELSTDP; 
        neuronmode_readout = NORMALSTDP;
    }else if(networkmode == READOUT){
        neuronmode_input = READCHANNEL;
        neuronmode_reservoir = STDP;
        neuronmode_readout = NORMALSTDP;
    }
    else if(networkmode == READOUTSUPV){
        neuronmode_input = DEACTIVATED;
        neuronmode_reservoir = READCHANNELSTDP;
        neuronmode_readout = NORMALSTDP;
    }
    else if(networkmode == READOUTBP){ // the error backprop mode
        neuronmode_input = READCHANNEL;
        neuronmode_reservoir = DEACTIVATED;
        neuronmode_readout = NORMALBP;
    }else{
        cout<<"Unrecognized network mode!"<<endl;
        exit(EXIT_FAILURE);
    }

}

int Network::LoadFirstSpeech(bool train, networkmode_t networkmode, const string sample_type){
    LSMLoadLayers();

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 
    auto sp_end_ptr = _speeches.end();
    if(sample_type == "test_sample"){
        _sp_iter = _t_speeches.begin();
        sp_end_ptr = _t_speeches.end();
    }
    else{
        _sp_iter = _speeches.begin();
        sp_end_ptr = _speeches.end();
    }

    if(_sp_iter != sp_end_ptr){
        LoadSpeeches(*_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, train);
#ifdef _READOUT_HELPER_CURRENT
        if(!train && networkmode == TRAINREADOUT && _lsm_output_layer){
            _lsm_output_layer->LSMSetTeacherSignal((*_sp_iter)->Class());
            _lsm_output_layer->LSMSetTeacherSignalStrength(TS_STRENGTH_P, TS_STRENGTH_N, TS_FREQ);
        }
#endif
        return (*_sp_iter)->Index();
    }else{
        _sp = NULL;
        return -1;
    }
}

int Network::LoadNextSpeech(bool train, networkmode_t networkmode, const string sample_type){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    if(train == true) _lsm_output_layer->LSMRemoveTeacherSignal((*_sp_iter)->Class());
    _sp_iter++;

    auto sp_end_ptr = _speeches.end();
    if(sample_type == "test_sample"){
        sp_end_ptr = _t_speeches.end();
    }
    else{
        sp_end_ptr = _speeches.end();
    }

    if(_sp_iter != sp_end_ptr){
        LoadSpeeches(*_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, train);
#ifdef _READOUT_HELPER_CURRENT
        if(!train && networkmode==TRAINREADOUT && _lsm_output_layer) 
            _lsm_output_layer->LSMSetTeacherSignal((*_sp_iter)->Class());
#endif
        return (*_sp_iter)->Index();
    }else{
        LSMNetworkRemoveSpeech();
        return -1;
    }
}


int Network::LoadFirstSpeechTrainCV(networkmode_t networkmode){
    assert((_fold_ind>=0)&&(_fold_ind<_fold));

    LSMLoadLayers();
    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);


    if(_fold_ind == 0) _train_fold_ind = 1;
    else _train_fold_ind = 0;
    assert(!_CVspeeches.empty());
    _cv_train_sp_iter = _CVspeeches[_train_fold_ind].begin();
    if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind].end()){
        LoadSpeeches(*_cv_train_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout,true); 
        return (*_cv_train_sp_iter)->Index();
    }else{
        cout<<"Warning, no training speech is specified!!"<<endl;
        _sp = NULL;
        return -1;
    }
}

int Network::LoadNextSpeechTrainCV(networkmode_t networkmode){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 

    _lsm_output_layer->LSMRemoveTeacherSignal((*_cv_train_sp_iter)->Class());
    _cv_train_sp_iter++;
    assert(!_CVspeeches.empty());
    if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind].end()){
        LoadSpeeches(*_cv_train_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout,true);
        return (*_cv_train_sp_iter)->Index();
    }else{
        ++_train_fold_ind;
        if(_train_fold_ind == _fold_ind)  ++_train_fold_ind;

        if(_train_fold_ind < _fold){
            _cv_train_sp_iter = _CVspeeches[_train_fold_ind].begin();
            if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind].end()){
                LoadSpeeches(*_cv_train_sp_iter, neuronmode_input, neuronmode_reservoir,neuronmode_readout, true);
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

    LSMLoadLayers();
    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 


    assert(!_CVspeeches.empty());
    _cv_test_sp_iter = _CVspeeches[_fold_ind].begin();
    if(_cv_test_sp_iter != _CVspeeches[_fold_ind].end()){
        LoadSpeeches(*_cv_test_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, false);
        return (*_cv_test_sp_iter)->Index();
    }else{
        _sp = NULL;
        return -1;
    }
}

int Network::LoadNextSpeechTestCV(networkmode_t networkmode){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    _cv_test_sp_iter++;
    assert(!_CVspeeches.empty());
    if(_cv_test_sp_iter != _CVspeeches[_fold_ind].end()){
        LoadSpeeches(*_cv_test_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, false);
        return (*_cv_test_sp_iter)->Index();
    }else{
        LSMNetworkRemoveSpeech();
        return -1;
    }
}


void Network::PrintAllNeuronName(){
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) cout<<(*iter)->Name()<<endl;
}

//* count the number of speech from each class
vector<int> Network::NumEachSpeech(){
    vector<int> res(CLS, 0);
#ifdef USE_TEST_SAMPLE
    for(auto & sp : _t_speeches){
#else
    for(auto & sp : _speeches){
#endif
        assert(sp && sp->Class() < CLS);
        res[sp->Class()]++;
    }
    return res;
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
    for(auto & b_neuron : _allBiasNeurons)
        b_neuron->LSMClear();
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClear();
    // Remember to clear the list to store the synapses:      
    _lsmActiveLearnSyns.clear();                              
    _lsmActiveSyns.clear();
    _lsmActiveSTDPLearnSyns.clear(); 
    _sp = NULL;
}

void Network::LSMClearWeights(){
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClearLearningSynWeights();
}
bool Network::LSMEndOfSpeech(networkmode_t networkmode, int end_time){
    /*
    if(_lsm_t <= end_time + 10) return false;
    else{
        _lsm_t = 0;
        return true;
    }
    */   
    
    if((networkmode == TRAINRESERVOIR||networkmode ==TRAININPUT)&&(_lsm_input>0)){
        return false;
    }

    if((networkmode == TRANSIENTSTATE)&&(_lsm_input>0)){
        return false;
    }
    if((networkmode == READOUT)&&(_lsm_input>0)){
        return false;
    }
    if((networkmode != VOID)&&(_lsm_reservoir > 0 && _lsm_input > 0)){
        return false;
    }
    return true;
    
}

void Network::SpeechInfo(){
    if(_sp_iter != _speeches.end())
        (*_sp_iter)->Info();
}

void Network::SpeechPrint(int info, const string& channel_name){
    // prepare the paths
    string s;
    for(int i=0;i<CLS;i++){
        s="spikes/Input_Response/"+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
        s="spikes/Reservoir_Response/"+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
        s="spikes/Readout_Response_Trans/"+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
    }
    if(_sp_iter != _speeches.end())
        (*_sp_iter)->PrintSpikes(info, channel_name);
}


/***********************************************************************************************
 * Collect the max/min of EP/EN/IP/IN and max active # of pre spikes for different neuron group
 ***********************************************************************************************/
void Network::CollectEPENStat(const char * type){
    NeuronGroup * ngPtr = NULL;
    int ep_max=INT_MIN,ep_min=INT_MAX,ip_max=INT_MIN,ip_min=INT_MAX;
    int en_max=INT_MIN,en_min=INT_MAX,in_max=INT_MIN,in_min=INT_MAX;
    int pre_active_max = 0;
    if(strcmp(type, "input") == 0 || strcmp(type, "readout") == 0){
        ngPtr = !strcmp(type, "input") ? SearchForNeuronGroup(type) : SearchForNeuronGroup("output");
        assert(ngPtr);
        ngPtr->Collect4State(ep_max, ep_min, ip_max, ip_min, en_max, en_min, in_max, in_min, pre_active_max);
    }
    else if(strcmp(type, "reservoir") == 0){
        NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
        assert(reservoir);
        reservoir->Collect4State(ep_max, ep_min, ip_max, ip_min, en_max, en_min, in_max, in_min, pre_active_max);
    }  

    cout<<"\n ***********************************************************"
        <<"\n * Max/Min for EP/EN/IP/IN and max prefire count for "<<type
        <<"\n ***********************************************************"
        <<"\nMax for EP: "<<ep_max<<" -> "<<(ep_max == INT_MIN ? 0 : ceil(log2(ep_max)))<<" bits"
        <<"\nMin for EP: "<<ep_min<<" -> "<<(ep_min == INT_MAX ? 0 : ceil(log2(abs(ep_min))))<<" bits"
        <<"\nMax for EN: "<<en_max<<" -> "<<(en_max == INT_MIN ? 0 : ceil(log2(en_max)))<<" bits"
        <<"\nMin for EN: "<<en_min<<" -> "<<(en_min == INT_MAX ? 0 : ceil(log2(abs(en_min))))<<" bits"
        <<"\nMax for IP: "<<ip_max<<" -> "<<(ip_max == INT_MIN ? 0 : ceil(log2(ip_max)))<<" bits"
        <<"\nMin for IP: "<<ip_min<<" -> "<<(ip_min == INT_MAX ? 0 : ceil(log2(abs(ip_min))))<<" bits"
        <<"\nMax for IN: "<<in_max<<" -> "<<(in_max == INT_MIN ? 0 : ceil(log2(in_max)))<<" bits"
        <<"\nMin for IN: "<<in_min<<" -> "<<(in_min == INT_MAX ? 0 : ceil(log2(abs(in_min))))<<" bits"
        <<"\nMax prespike count: "<<pre_active_max
        <<endl;
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

    double p = 0.0, avg_i = 0.0;
    int max_i = 0;
    // Collect the presynaptic firing activity from the neurongroup level:
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);
    double p_r = 0.0, avg_i_r = 0.0;
    int max_i_r = 0;
    reservoir->CollectPreSynAct(p_r, avg_i_r, max_i_r);
    p += p_r, avg_i += avg_i_r, max_i = max( max_i, max_i_r);

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
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);
 
    reservoir->ScatterFreq(fs, bias, cnt);
    
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
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);
 
    reservoir->CollectVariance(my_map);

    // apply var-based sparisfication to the neurons with low variance :
    int cnt_limit = _TOP_PERCENT*my_map.size(), cnt = 0;
#ifdef _DEBUG_VARBASE
    cout<<"The lower "<<cnt_limit<<" neurons are applied with sparification."
        <<endl;
#endif
    for(multimap<double, Neuron*>::iterator it = my_map.begin(); it != my_map.end(); ++it){
        if(cnt < cnt_limit){
#ifdef _DEBUG_VARBASE
            cout<<"Low "<<cnt+1<<"th "<<(*it).second->Name()<<" with var: "<<(*it).first<<endl;
#endif
            // Apply the sparsification here:
            (*it).second->DisableOutputSyn(syn_type);
        }
        ++cnt;
    }
}

// Correlation-based sparsification, only applied to the reservoir!
void Network::CorBasedSparsify(){
    // only support for reservoir now
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    assert(reservoir);
    reservoir->MergeCorrelatedNeurons(_speeches.size());

#ifdef _DEBUG_CORBASE
    NeuronGroup * output = SearchForNeuronGroup("output");
    ofstream f_out("readout_input_connections.txt");
    assert(f_out.is_open());
    output->LSMPrintInputSyns(f_out);
    f_out.close();
    string filename = "r_weights_info_after_corsparse.txt";
    WriteSynWeightsToFile("reservoir", "reservoir", filename.c_str());
#endif

}    

void Network::LSMChannelDecrement(channelmode_t channelmode){
    if(channelmode == INPUTCHANNEL) _lsm_input--;
    else if(channelmode == RESERVOIRCHANNEL) _lsm_reservoir--;
    else assert(0); // you code should never go here!
}

//* calculate the error: # spike/max{spike}, and prop it back to modify the weights
void Network::BackPropError(int iteration, int end_time){
    assert(_lsm_output_layer);
    assert(_sp);
    
    _lsm_output_layer->BpOutputError(_sp->Class(), iteration, end_time, _sp->Weight());
    
    for(NeuronGroup * hidden : _lsm_hidden_layers){
        hidden->BpHiddenError(iteration, end_time, _sp->Weight());
    }
}

//* calculate the cost function of the error-bp
double Network::GetBpCost(){
    assert(_lsm_output_layer);
    assert(_sp);
    return _lsm_output_layer->GetCost(_sp->Class(), _sp->Weight());
}

//* update the learning weight in the end of each sample
void Network::UpdateLearningWeights(){
    assert(_lsm_output_layer);
    _lsm_output_layer->UpdateLearningWeights();
}

//* collect the test error for a single sample
void Network::CollectErrorPerSample(vector<double>& each_sample_errors){
    assert(_lsm_output_layer);
    assert(_sp);

    double error = _lsm_output_layer->GetCost((_sp)->Class(), _sp->Weight());
    each_sample_errors.push_back(error);
}

//* print the fire spike count of a specific layer 
void Network::PrintSpikeCount(string layer){
    NeuronGroup * ng = SearchForNeuronGroup(layer.c_str());
    assert(ng);
#ifdef CV
    assert(*_cv_test_sp_iter);
    ng->PrintSpikeCount((*_cv_test_sp_iter)->Class());
#else
    assert(*_sp_iter);
    ng->PrintSpikeCount((*_sp_iter)->Class());
#endif

}

//* judge the readout result:
void Network::ReadoutJudge(vector<pair<int, int> >& correct, vector<pair<int, int> >& wrong, vector<pair<int, int> >& even){
    pair<int, int> res = this->LSMJudge(); // +/- 1, true cls

    if(res.first == 1) correct.push_back(res);
    else if(res.first == -1) wrong.push_back(res);
    else if(res.first == 0) even.push_back(res);
    else{
        cout<<"In Network::ReadoutJudge(vector<pair<int, int> >&, vector<pair<int, int> >&, vector<pair<int, int> >&)\n"
            <<"Undefined return type: "<<res.first<<" returned by Network::LSMJudge()"
            <<endl;
    }
}

/*************************************************************************************
 * this function is to find the readout neuron with maximum firing frequency and see 
 * if it is the desired one with correct label. 
 * @ret: pair <first, second>; first: +1 ->correct,-1 -> wrong, 0: even
 *                             second: the true class index 
 **************************************************************************************/
pair<int, int> Network::LSMJudge(){
    assert(_lsm_output_layer); 
    assert(_sp);
    int cls = _sp->Class();
    return {_lsm_output_layer->Judge(cls), cls};
}


//* Compute the error for the boosting method
void Network::ComputeBoostError(vector<pair<Speech*, bool> >& predictions){
    pair<int, int> res = this->LSMJudge();
    assert(_sp);
    predictions.push_back({_sp, res.first == 1});
}


//* update the boosting weight associated with each sample
void Network::BoostWeightUpdate(const vector<pair<Speech*, bool> >& predictions){
    // 1. compute the sum of the boosting weights for each sample class
    vector<double> sum_weights(CLS, 0);
    for(auto & p : predictions){
        assert(p.first);
        int cls = p.first->Class();
        double weight = p.first->Weight();
        assert(cls < CLS);
        sum_weights[cls] += weight;
    }
    
    // 2. compute the weighted error for each sample class
    vector<double> error_weighted(CLS, 0);
    for(auto & p : predictions){
        bool prediction = p.second;
        int cls = p.first->Class();
        double weight = p.first->Weight();
        error_weighted[cls] += weight*(!prediction)/sum_weights[cls];
    }
    // 3. update the boost weight for each training sample:
    for(auto & p : predictions){
        bool prediction = p.second;
        int cls = p.first->Class(); 
        double weight = p.first->Weight();
        double stage = error_weighted[cls]/20; 
        double new_weight = weight * exp(stage * (!prediction));
        p.first->SetWeight(new_weight);
    }
}


//* add the active reservoir synapse about to be trained by STDP
void Network::LSMAddSTDPActiveSyn(Synapse * synapse){
    assert(synapse->GetActiveSTDPStatus() == false);
    _lsmActiveSTDPLearnSyns.push_back(synapse);
}

void Network::CrossValidation(int fold){
    int i, j, k, cls, sample;
    std::srand(0);

    assert(fold>1);
    _fold = fold;
    _CVspeeches = vector<vector<Speech*> >(fold, vector<Speech*>());
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
                _CVspeeches[k].push_back(_speeches[(*index[i])[j]]);
            }

    for(i = 0; i < cls; i++) delete index[i];
    delete [] index;
}


void Network::IndexSpeech(){
    for(int i = 0; i < _speeches.size(); i++) _speeches[i]->SetIndex(i);
    for(int i = 0; i < _t_speeches.size(); ++i) _t_speeches[i]->SetIndex(i);
}

//* this function is used to print out the recognition rate at the current iteration
void Network::CurrentPerformance(int iter_n)
{
    int total = _readout_correct[iter_n] + _readout_wrong[iter_n] + _readout_even[iter_n];
    if(_tid == 0){
        cout<<"The performance @"<<iter_n<<" = "<<_readout_correct[iter_n]<<"/"<<total<<" = "<<double(_readout_correct[iter_n])*100/ total<<'%'<<endl;
    }
}

//* push the readout results into the corresponding vector:
void Network::LSMPushResults(const vector<pair<int, int> >& correct, const vector<pair<int, int> >& wrong, const vector<pair<int, int> >& even, int iter_n){
    assert(iter_n < _readout_correct.size());
    _readout_correct[iter_n] += correct.size();
    _readout_wrong[iter_n] += wrong.size();
    _readout_even[iter_n] += even.size();

    for(auto & p : correct){
        assert(p.second < _readout_correct_breakdown[iter_n].size());
        _readout_correct_breakdown[iter_n][p.second]++;
    }
}

//* log the test error into the corresponding vector:
//* test error defined: \sum_j 1/2*(o_j - y_j)^2, where o_j = spike_j /max_k(spike_k)
void Network::LogTestError(const vector<double>& each_sample_errors, int iter_n){
    assert(iter_n < _readout_test_error.size());
    double error_sum = 0;
    for(double e : each_sample_errors)  error_sum += e;
    _readout_test_error[iter_n] = error_sum;
}

//* return the most recent results of the readout (correct, wrong, even) in a vector:
vector<int> Network::LSMViewResults(){
    vector<int> ret;
    ret.push_back(_readout_correct.empty()? -1 : _readout_correct.back());
    ret.push_back(_readout_wrong.empty()? -1 : _readout_wrong.back());
    ret.push_back(_readout_even.empty()? -1 : _readout_even.back());
    return ret;
}

//* merge the readout results from the network
void Network::MergeReadoutResults(vector<int>& r_correct, vector<int>& r_wrong, vector<int>& r_even, vector<vector<int> >& r_correct_bd){
    assert(!r_correct.empty() &&
            r_correct.size() == _readout_correct.size() &&
            r_wrong.size() == _readout_wrong.size() &&
            r_even.size() == _readout_even.size() &&
            _readout_correct.size() == _readout_wrong.size() &&
            _readout_correct.size() == _readout_correct_breakdown.size());

    for(size_t i = 0; i < r_correct.size(); ++i){
        r_correct[i] += _readout_correct[i];
        r_wrong[i] += _readout_wrong[i];
        r_even[i] += _readout_even[i];
        r_correct_bd[i] = r_correct_bd[i] + _readout_correct_breakdown[i];  
    }

}

//* merge the test error from the network:
void Network::MergeTestErrors(vector<double>& test_errors){
    assert(!test_errors.empty() && test_errors.size() == _readout_test_error.size());
    for(size_t i = 0; i < test_errors.size(); ++i)
        test_errors[i] += _readout_test_error[i];
}

//* random shuffle the training samples:
void Network::ShuffleTrainingSamples(){
#ifdef CV
    for(int i = 0; i < _CVspeeches.size(); ++i){
        if(i == _fold_ind)  continue;
        random_shuffle(_CVspeeches[i].begin(), _CVspeeches[i].end());
    }    
#else
    random_shuffle(_speeches.begin(), _speeches.end());
#endif
}


//* This is a supporting function. Given a synapse type (r/o synapses)
//* determine the syn_type to be returned (ret_syn_type).
//* The third parameter is the function name used to handle exception.
void Network::DetermineSynType(const char * syn_type, synapsetype_t & ret_syn_type, const char * func_name){
    if(strcmp(syn_type, "reservoir") == 0){
        // Write reservoir syns into the file
        ret_syn_type = RESERVOIR_SYN;
    }else if(strcmp(syn_type, "readout") == 0){
        ret_syn_type = READOUT_SYN;
    }else if(strcmp(syn_type, "input") == 0){
        ret_syn_type = INPUT_SYN;
    }else{
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

//* this function is to remove the synapses with zero weights:
void Network::RemoveZeroWeights(const char * type){
    synapsetype_t  syn_type = INVALID;
    DetermineSynType(type, syn_type, "RemoveZeroWeights()");
    assert(syn_type != INVALID);

    if(syn_type == RESERVOIR_SYN){
        NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
        reservoir->RemoveZeroSyns(syn_type);
    }
    else if(syn_type == READOUT_SYN){
        NeuronGroup * output = SearchForNeuronGroup("output");
        if(!output){
            cout<<"In Network::RemoveZeroWeights(), no 'output' layer is found!!"<<endl;
            assert(output);
        }
        output->RemoveZeroSyns(syn_type);
    }
    else{
        assert(0); // your code shoule never go here.
    }
}

//* Dump the spikes of the hidden and output layer
void Network::DumpHiddenOutputResponse(const string & status, int sp){
    string path_hidden, path_output;
    string s;
    if(status == "unsupervised"){
        path_hidden = "spikes/Hidden_Response_Unsupv/";
        path_output = "spikes/Readout_Response_Unsupv/";
    } 
    else{
        assert(status == "test" || status == "train");
        path_hidden = "spikes/Hidden_Response_Supv/" + status+"/";
        path_output = "spikes/Readout_Response_Supv/" + status+"/"; 
    }
    for(int i=0;i<CLS;i++){
        s=path_hidden+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
        s=path_output+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
    }
    // dump the output spikes: 
    string output_filename = path_output + "/" +to_string(_sp->Class())+ "/readout_spikes_" + to_string(sp) + "_"+ to_string(_sp->Class())+".dat";
    assert(_lsm_output_layer);
    _lsm_output_layer->DumpSpikeTimes(output_filename);

    // dump the hidden spikes:
    for(NeuronGroup * hidden : _lsm_hidden_layers){
        assert(hidden);
        string hidden_filename = path_hidden + "/"+to_string(_sp->Class())+"/" + string(hidden->Name())+ "_spikes_" + to_string(sp) + "_"+ to_string(_sp->Class()) +".dat";
        hidden->DumpSpikeTimes(hidden_filename);
    }

}


//* Dump the v_mem of the network @param2: the speech index
void Network::DumpVMems(string dir, int info, const string& group_name){
    string s;
    // make the directory if needed
    s=dir+"/"+to_string(_sp->Class());
    if(access(s.c_str(),0)!=0)
        MakeDirs(s);
    NeuronGroup * ng = SearchForNeuronGroup(group_name.c_str());
    if(ng == NULL){
        cout<<"Warning::Cannot find the neuron group: "<<group_name<<endl;
        return;
    }
    assert(ng);

    string filename = dir + "/"+to_string(_sp->Class())+"/" + group_name + "_" + to_string(info) + ".dat";
    ofstream f_out(filename.c_str());
    if(!f_out.is_open()){
        cout<<"Cannot file : "<<filename<<endl;
        assert(f_out.is_open());
    }
    // output file format: 
    // 1st line : # of reservoir # of readout neuron
    // the rest : the v_mem value

    ng->DumpVMems(f_out);
    f_out.close();
}


//* Dump the calcium levels of the readout:
void Network::DumpCalciumLevels(string neuron_group, string dir, string filename){
    NeuronGroup * ng = NULL;
	string s;
    if(neuron_group == "readout"){
        ng = SearchForNeuronGroup("output");
        assert(ng);
    }
    else{
        cout<<"Unrecognized neuron group: "<<neuron_group<<endl;
        exit(EXIT_FAILURE);
    }
    // Create the directory if necessary
	s=dir+"/"+to_string(_sp->Class());   
	if(access(s.c_str(),0)!=0)
		MakeDirs(s);

    string f_name = dir+"/"+to_string(_sp->Class()) + "/" + filename + ".txt";
    ofstream f_out(f_name.c_str());
    ng->DumpCalciumLevels(f_out);
    f_out.close();
}

//* load the spikes to the specific channel correspond to the neuron group
//* the spikes file is in the specific path
void Network::LoadResponse(const string & ng_name,int sample_size){

    if(ng_name != "reservoir"){
        cout<<"Unsupported loading of response to neuron group: "<<ng_name<<endl;
        exit(EXIT_FAILURE);
    }
    // pre-allocate the speeches pointers
    assert(_speeches.empty());
    for(int i = 0; i < CLS; i++){
        // set the path for reading the reservoir response
        string path = "spikes/Reservoir_Response/" + to_string(i) + "/";
        vector<string> sp_files = GetFilesEndWith(path, ".dat");

        NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
        assert(reservoir);
        int reservoir_size = reservoir->Size();
        NeuronGroup * input = SearchForNeuronGroup("input");
        int input_size = input->Size();
        int filenum_read=0;

        for(string filename : sp_files){
            int cls = -1, index = -1;
            GetSpeechIndexClass(filename, cls, index);
            assert(cls > -1 && cls < CLS && index != -1);
            if(filenum_read>=TB_PER_CLASS){
                break;
            }
            filename = path + filename;
            ifstream f_in(filename.c_str());
            if(!f_in.is_open()){
                cout<<"Cannot open the file: "<<filename<<endl;
                exit(EXIT_FAILURE);
            }
            Speech * speech = new Speech(cls);
            AddSpeech(speech);

            speech->SetNumChannel(input_size, INPUTCHANNEL);
            speech->SetNumChannel(reservoir_size, RESERVOIRCHANNEL);
            speech->LoadSpikes(f_in, RESERVOIRCHANNEL);

            f_in.close();
            filenum_read++;
        }
    }
    // verify that all the speeches are properly loaded!
    for(int i = 0; i < _speeches.size(); ++i){
        if(_speeches[i] == NULL){
            cout<<"The "<<i<<"th speech is not properly loaded!"<<endl;
            exit(EXIT_FAILURE);
        }
    }
}

//* write the weights of the selected synaptic type
void Network::WriteSelectedSynToFile(const string& syn_type, char * filename){

    ofstream f_out(filename);
    if(!f_out.is_open()){
        cout<<"In Network::WriteSelectedSynToFile(), cannot open the file : "<<filename<<endl;
        exit(EXIT_FAILURE);
    }

    synapsetype_t  wrt_syn_type = INVALID;
    DetermineSynType(syn_type.c_str(), wrt_syn_type, "WriteSelectedSynToFile()");
    assert(wrt_syn_type != INVALID);
    vector<Synapse*> tmp;

    const vector<Synapse*> & synapses = 
        wrt_syn_type == RESERVOIR_SYN ? _rsynapses : 
        wrt_syn_type == READOUT_SYN ? _rosynapses : 
        wrt_syn_type == INPUT_SYN ? _isynapses : tmp;
    for(size_t i = 0; i < synapses.size(); ++i)
        f_out<<i<<"\t"<<synapses[i]->PreNeuron()->Name()
            <<"\t"<<synapses[i]->PostNeuron()->Name()
#ifdef DIGITAL
            <<"\t"<<synapses[i]->DWeight()<<endl;
#else
            <<"\t"<<synapses[i]->Weight()<<endl;
#endif
    f_out.close();
}


//* write the weights of output synapses of the neurons in the group into a file:
void Network::WriteSynWeightsToFile(const string& pre_g, const string& post_g, char * filename){
    NeuronGroup * pre = SearchForNeuronGroup(pre_g.c_str());
    NeuronGroup * post = SearchForNeuronGroup(post_g.c_str());
    if(pre == NULL){
        cout<<"Warning::Cannot find neuron group named: "<<pre_g
            <<" for writing the weights"<<endl;
        return;
    }
    if(post == NULL){
        cout<<"Warning::Cannot find neuron group named: "<<post_g
            <<" for writing the weights"<<endl;
        return;
    }
    ofstream f_out(filename);
    if(!f_out.is_open()){
        cout<<"WriteSynWeightsToFile(), cannot open the file : "<<filename<<endl;
        exit(EXIT_FAILURE);
    }

    int index = 0;
    pre->WriteSynWeightsToFile(f_out, index, post_g);
    f_out.close();
}

//* Given a filename, load all the weigths recorded in the file to the network
void Network::LoadSynWeightsFromFile(const string& filename)
{
    ifstream f_in(filename);
    if(!f_in.is_open()){
        cout<<"In Network::LoadSynWeightsFromFile(), cannot open the file: "<<filename<<endl;
        assert(0);
    }
    int index;
    string pre;
    string post; 
#ifdef DIGITAL
    int weight;
#else
    double weight;
#endif
    while(f_in>>index>>pre>>post>>weight){
        assert(_synapses_map.find(pre) != _synapses_map.end());
        assert(_synapses_map[pre].find(post) != _synapses_map[pre].end());
        _synapses_map[pre][post]->Weight(weight);
    }
   
}

//* This function is to load the synaptic weights from file and assign it to the synapses:
//* "syn_type" can be reservoir or readout or input
void Network::LoadSynWeightsFromFile(const char * syn_type, char * filename){
    ifstream f_in(filename);
    if(!f_in.is_open()){
        cout<<"In Network::LoadSynWeightsFromFile(), cannot open the file: "<<filename<<endl;
        assert(0);
    }

    synapsetype_t read_syn_type = INVALID;
    DetermineSynType(syn_type, read_syn_type, "LoadSynWeightsFromFile()");
    assert(read_syn_type != INVALID);
    vector<Synapse*> tmp;

    const vector<Synapse*> & synapses = 
        read_syn_type == RESERVOIR_SYN ? _rsynapses : 
        read_syn_type == READOUT_SYN ? _rosynapses :
        read_syn_type == INPUT_SYN ? _isynapses : tmp;

    assert(!synapses.empty());  
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
    while(f_in>>index>>pre>>post>>weight){//>>pre_ext>>post_ext){
        if(index < 0 || index >= synapses.size()){
            cout<<"In Network::LoadSynWeightsFromFile(), the index of the synapse you read : "<<index<<" is out of bound of the container size: "<<synapses.size()<<endl;
            cout<<"Synapse type: "<<syn_type<<endl;
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
    NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
    for(Neuron * neuron = reservoir->First(); neuron != NULL; neuron = reservoir->Next()){
        if(!neuron->LSMCheckNeuronMode(DEACTIVATED))
            count_neuron++;
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
    
    for(Neuron * neuron = reservoir->First(); neuron != NULL; neuron = reservoir->Next()){
        if(!neuron->LSMCheckNeuronMode(DEACTIVATED))
            // the label here is randomly assigned by me:
            f_out<<"<node id=\""<<neuron->Index()<<"\" label=\"OldMan\"/>\n";
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

