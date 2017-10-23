#ifndef NETWORK_H
#define NETWORK_H

#include <list>
#include <vector>
#include <string>
#include <set>
#include <stdlib.h>
#include "def.h"
#include <cstdio>
#include <fstream>
#include <utility>
#include <unordered_map>

class Neuron;
class Synapse;
class NeuronGroup;
class Speech;

class Network{
private:
    std::list<Neuron*> _individualNeurons;
    std::unordered_map<std::string, NeuronGroup*> _groupNeurons;
    std::list<Neuron*> _allNeurons;
    std::list<Neuron*> _allExcitatoryNeurons;
    std::list<Neuron*> _allInhibitoryNeurons;
    std::list<Neuron*> _inputNeurons;
    std::list<Neuron*> _outputNeurons;
    std::list<Synapse*> _synapses;
    std::vector<Synapse*> _rsynapses;       
    std::vector<Synapse*> _rosynapses;
    std::vector<Synapse*> _isynapses;
    std::list<Synapse*> _lsmActiveLearnSyns;
    std::list<Synapse*> _lsmActiveSyns;
    std::list<Synapse*> _lsmActiveSTDPLearnSyns; // for both unsupervised and supervised training 

    std::vector<int> _readout_correct; // sign : +1
    std::vector<int> _readout_wrong;   // sign: -1
    std::vector<int> _readout_even;    // sign: 0
    std::vector<std::vector<int> > _readout_correct_breakdown; // the break down for each class
    std::vector<double> _readout_test_error; // test error: (o - y)^2

    // for LSM
    std::vector<Speech*> _speeches;
    std::vector<Speech*>::iterator _sp_iter;
    int _fold;
    int _fold_ind;
    int _train_fold_ind;
    std::vector<std::vector<Speech*> > _CVspeeches;
    std::vector<Speech*>::iterator _cv_train_sp_iter, _cv_test_sp_iter;
    Speech * _sp;
    void RemoveCVSpeeches();
    NeuronGroup * _lsm_input_layer;
    NeuronGroup * _lsm_reservoir_layer;
    std::vector<std::string> _hidden_layer_names;
    std::vector<NeuronGroup*> _lsm_hidden_layers;
    NeuronGroup * _lsm_output_layer;
    int _lsm_input;
    int _lsm_reservoir;
    int _lsm_output;
    int _lsm_t;
    networkmode_t _network_mode;
	int Tid;

public:
    Network();
    ~Network();
    void LSMSetNetworkMode(networkmode_t networkmode){_network_mode = networkmode;}
    networkmode_t LSMGetNetworkMode(){return _network_mode;}

    bool CheckExistence(char *);
    void AddNeuron(char* name, bool excitatory, double v_mem);
    void AddNeuronGroup(char* name, int num, bool excitatory, double v_mem);
    void LSMDeleteNeuronGroup(char * name);
    void InitializeInputLayer(int num_inputs, int num_connections);
    void LSMAddLabelToReservoirs(const char * name, std::set<int> labels);
    void LSMAddNeuronGroup(char*,int,int,int);
    void LSMAddNeuronGroup(char*,char*,char*); //reservoir, path_info_neuron, path_info_synapse

    void LSMAddSynapse(char*,char*,int,int,int,int,bool);
    void LSMAddSynapse(Neuron*,NeuronGroup*,int,int,int,bool);
    void LSMAddSynapse(NeuronGroup*,NeuronGroup*,int,int,int,int,bool);
    void LSMAddSynapse(NeuronGroup * pre, int npre, int npost, int value, int random, bool fixed);

	void SetTid(int tid){Tid=tid;}
	int GetTid(){return Tid;}

    void PrintSize();
    void PrintAllNeuronName();

    int LSMSumAllOutputSyns();
    int LSMSizeAllNeurons();
    
    void LSMTruncNeurons(int);

    // Supporting functions for enhanced analysis
    void LSMAdaptivePowerGating();
    void LSMSumGatedNeurons();
    void LSMHubDetection();

    void LSMSupervisedTraining(networkmode_t networkmode, int tid, int iteration);
    void LSMUnsupervisedTraining(networkmode_t networkmode, int tid);
    void LSMReservoirTraining(networkmode_t networkmode);
    void LSMNextTimeStep(int t, bool train, int iteration, int end_time, FILE* Foutp);

    Neuron * SearchForNeuron(const char*);
    Neuron * SearchForNeuron(const char*, const char*);
    NeuronGroup * SearchForNeuronGroup(const char*);

    void IndexSpeech();

    // for LSM
    int  SpeechEndTime();
    void AddSpeech(Speech*);
    void LoadSpeeches(Speech      * sp_iter, 
            neuronmode_t neuronmode_input,
            neuronmode_t neuronmode_reservoir,
            neuronmode_t neuronmode_readout,
            bool train
            );
    void LSMNetworkRemoveSpeech();
    void InitializeHiddenLayers();
    void LSMLoadLayers();
    void DetermineNetworkNeuronMode(const networkmode_t &, neuronmode_t &, neuronmode_t &, neuronmode_t&);

    int LoadFirstSpeech(bool train, networkmode_t networkmode);
    int LoadFirstSpeech(bool train, networkmode_t networkmode, bool inputExist);
    int LoadNextSpeech(bool train, networkmode_t networkmode);
    int LoadNextSpeech(bool train, networkmode_t networkmode, bool inputExist);
    int LoadFirstSpeechTrainCV(networkmode_t);
    int LoadNextSpeechTrainCV(networkmode_t);
    int LoadFirstSpeechTestCV(networkmode_t);
    int LoadNextSpeechTestCV(networkmode_t);

    int NumSpeech(){return _speeches.size();}
    int NumIteration(){return _readout_correct.size();}
    std::vector<int> NumEachSpeech();
    void AnalogToSpike();
    void RateToSpike();
    void LSMClearSignals();
    void LSMClearWeights();
    bool LSMEndOfSpeech(networkmode_t networkmode, int end_time);
    void LSMChannelDecrement(channelmode_t);
    void BackPropError(int iteration, int end_time);
    void UpdateLearningWeights();
    void CollectErrorPerSample(std::vector<double>& each_sample_error);
    void PrintSpikeCount(std::string layer);
    void ReadoutJudge(std::vector<std::pair<int, int> >& correct, std::vector<std::pair<int, int> >& wrong, std::vector<std::pair<int, int> >& even, std::vector<int> &wrong_cls);
    std::pair<int, int>  LSMJudge();
    void ComputeBoostError(std::vector<std::pair<Speech*, bool> >& predictions);
    void BoostWeightUpdate(const std::vector<std::pair<Speech*, bool> >& predictions);
    
    //* this function is to add the active learning readout synapses:
    void LSMAddActiveLearnSyn(Synapse*synapse){_lsmActiveLearnSyns.push_back(synapse);}
    void LSMAddActiveSyn(Synapse*synapse){_lsmActiveSyns.push_back(synapse);}

    //* Add the target synapses that should be trained by STDP:
    void LSMAddSTDPActiveSyn(Synapse * synapse);

    void CrossValidation(int);
    void Fold(int fold_ind){_fold_ind = fold_ind;}

    void SpeechInfo();
    // print the spikes into the file
    void SpeechPrint(int info);
    void CollectEPENStat(const char * type);
    void PreActivityStat(const char * type, std::vector<double>& prob, std::vector<double>& avg_intvl, std::vector<int>& max_intvl);
    void CollectSpikeFreq(const char * type, std::vector<double>& fs, int end_t);
    void ScatterSpikeFreq(const char * type, std::vector<double>& fs);
    // print the spiking frequency into the file:
    void SpeechSpikeFreq(const char * type, std::ofstream & f_out, std::ofstream & f_label);
    void VarBasedSparsify(const char * type);
    void CorBasedSparsify();

    // supporting functions:
    // the push/view readout results:
    void LSMPushResults(const std::vector<std::pair<int, int> >& correct, const std::vector<std::pair<int, int> >& wrong, const std::vector<std::pair<int, int> >& even, int n_iter);
    std::vector<int> LSMViewResults();
    void MergeReadoutResults(std::vector<int>& r_correct, std::vector<int>& r_wrong, std::vector<int>& r_even, std::vector<std::vector<int> >& r_correct_bd);
    void MergeTestErrors(std::vector<double>& test_errors);

    // log the test errors:
    void LogTestError(const std::vector<double>& each_sample_errors, int iter_n);
    
    void ShuffleTrainingSamples();
    
    // this first parameter is used to indicate which neurongroup does the syn belongs to.
    void DetermineSynType(const char * syn_type, synapsetype_t & ret_syn_type, const char * func_name);
    void NormalizeContinuousWeights(const char * syn_type);
    void RemoveZeroWeights(const char * type);

    void DumpHiddenOutputResponse(const std::string& status, int sp);
    void DumpVMems(std::string dir, int info, const std::string& group_name);
    void DumpCalciumLevels(std::string neuron_group, std::string dir, std::string filename);
    
    void WriteSelectedSynToFile(const std::string& syn_type, char * filename);
    void WriteSynWeightsToFile(const std::string& pre_g, const std::string& post_g, char * filename);
    void LoadSynWeightsFromFile(const char * syn_type, char * filename);
    void WriteSynActivityToFile(char * pre_name, char * post_name, char * filename);
    void TruncateIntermSyns(const char * syn_type);

    void VisualizeReservoirSyns(int indicator);


    template<class T>
    void LSMAddSynapse(Neuron * pre, Neuron * post, T weight, bool fixed, T weight_limit,bool liquid, NeuronGroup * group = NULL){
        Synapse * synapse = new Synapse(pre, post, weight, fixed, weight_limit, pre->IsExcitatory(),liquid);
        _synapses.push_back(synapse);
        // push back the reservoir, readout and input synapses into the vector:
        if(!synapse->IsReadoutSyn() && !synapse->IsInputSyn())
            _rsynapses.push_back(synapse);
        else if(synapse->IsReadoutSyn())
            _rosynapses.push_back(synapse);
        else
            _isynapses.push_back(synapse);

        if(group)  group->AddSynapse(synapse);
        pre->AddPostSyn(synapse);
        post->AddPreSyn(synapse);	
    } 
	void LoadResponse();
	void PrintTestError();
	void SetLateralInhibition(const char * name);

};

#endif
