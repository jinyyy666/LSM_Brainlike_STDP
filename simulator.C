#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "simulator.h"
#include "network.h"
#include "util.h"
#include <sys/time.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <algorithm>

//#define _DUMP_READOUT_WEIGHTS

using namespace std;
double TSstrength;
extern int file[500];

Simulator::Simulator(Network * network){
    _network = network;
    _t = -1;
}

void Simulator::SetEndTime(double endTime){
    assert(endTime > 0);
    _t_end = endTime;
}

void Simulator::SetStepSize(double stepSize){
    assert(stepSize > 0);
    _t_step = stepSize;
}



void Simulator::LSMRun(long tid){
    //_network->PrintAllNeuronName();

    char filename[64];
    int info, count;
    int x,y,loss_neuron;
    int Tid;
    double temp;
    FILE * Fp = NULL;
    FILE * Foutp = NULL;
    networkmode_t networkmode;

    Timer myTimer;

    srand(0);
    _network->IndexSpeech();
    cout<<"Number of speeches: "<<_network->NumSpeech()<<endl;
#ifndef CV // This is for the case without cross-validation
#ifdef _WRITE_STAT
    for(int count = 0; count < _network->NumSpeech(); count++){
        // empty files
        sprintf(filename,"outputs/spikepattern%d.dat",count);
        InitializeFile(filename);
    }
#endif
#endif

    // visualize the reservoir synapses before stdp training:
    // _network->VisualizeReservoirSyns(0);

    // detect total number of hubs in the reservoir BEFORE stdp training:
    //cout<<"Before STDP training:"<<endl;
    //_network->LSMHubDetection();

#if NUM_THREADS == 1
    sprintf(filename, "reservoir_weights_%ld_org.txt", tid);
    _network->WriteSynWeightsToFile("reservoir", "reservoir", filename);
#endif

    if(tid == 0){
        sprintf(filename, "i_weights_info.txt");
        _network->WriteSynWeightsToFile("input", "reservoir", filename);
        sprintf(filename, "r_weights_info.txt");
        _network->WriteSynWeightsToFile("reservoir", "reservoir", filename);
        sprintf(filename, "h_weights_info.txt");
        _network->WriteSynWeightsToFile("reservoir", "hidden_0", filename);
        sprintf(filename, "o_weights_info.txt");
        _network->WriteSynWeightsToFile("hidden_0", "output", filename);

        sprintf(filename, "o_weights_info_all.txt");
        _network->WriteSelectedSynToFile("readout", filename); 
    }

#ifdef STDP_TRAINING
    myTimer.Start();
#ifdef STDP_TRAINING_RESERVOIR
    // train the reservoir using STDP rule:
    networkmode = TRAINRESERVOIR;
    _network->LSMSetNetworkMode(networkmode);

    // repeatedly training the reservoir for a certain amount of iterations:
    for(int i = 0; i < 25; ++i){
        _network->LSMReservoirTraining(networkmode);

#if NUM_THREADS == 1  
        // Write the weight back to file after training the reservoir with STDP:
        sprintf(filename, "reservoir_weights_%d.txt", i);
        _network->WriteSelectedSynToFile("reservoir", filename);
#endif       
    }
    myTimer.Report("training the reservoir");
#endif

    // visualize the reservoir synapses after stdp training:
    //_network->VisualizeReservoirSyns(1);

#ifdef STDP_TRAINING_INPUT
    // train the input using STDP rule:
    networkmode = TRAININPUT;
    _network->LSMSetNetworkMode(networkmode);

    // repeatedly training the input for a certain amount of iterations:
    for(int i = 0; i < 20; ++i){
        _network->LSMUnsupervisedTraining(networkmode, tid);

#if NUM_THREADS == 1
        // Write the weight back to file after training the input with STDP:
        sprintf(filename, "input_weights_%d.txt", i);
        _network->WriteSelectedSynToFile("input", filename);
#endif
    }
    myTimer.Report("training the input");
#endif
    myTimer.End("STDP training of input and reservoir");


    ////////////////////////////////////////////////////////////////////////
    // REMEMBER TO REMOVE THESE CODES!!
    //sprintf(filename, "o_weights_info_trained_final.txt");
    //_network->LoadSynWeightsFromFile("readout", filename);
    ////////////////////////////////////////////////////////////////////////


    // detect total number of hubs in the reservoir AFTER stdp training:
    //cout<<"After STDP training:"<<endl;
    //_network->LSMHubDetection();

#ifdef ADAPTIVE_POWER_GATING
    // apply the power gating scheme to turn off some neurons with low connectivity
    _network->LSMAdaptivePowerGating(); 
    // retrain the network for few echos:
    for(int i = 0; i < 5; ++i)
        _network->LSMReservoirTraining(networkmode);
    _network->LSMSumGatedNeurons();
#endif

    // Load the weight from file:
    // sprintf(filename, "r_weights_info.txt");
    // _network->LoadSynWeightsFromFile("reservoir", filename);

    // Write the weight back to file after training the reservoir with STDP:
    if(tid == 0){
        sprintf(filename, "r_weights_info_trained.txt");
        _network->WriteSelectedSynToFile("reservoir", filename);
        sprintf(filename, "i_weights_info_trained.txt");
        _network->WriteSelectedSynToFile("input", filename);
    }

#ifdef _RM_ZERO_RES_WEIGHT
    _network->RemoveZeroWeights("reservoir");
#endif  

#endif
    ////////////////////////////////////////////////////////////////////////
    // REMEMBER TO REMOVE THESE CODES!!
    // sprintf(filename, "r_weights_info_best.txt");
    // _network->LoadSynWeightsFromFile("reservoir", filename);
    // _network->TruncateIntermSyns("reservoir");
    // sprintf(filename, "r_weights_info_best_tmp.txt");
    // _network->WriteSelectedSynToFile("reservoir", filename);
    //ofstream f1("spike_freq.txt"), f2("spike_speech_label.txt");
    //assert(f1.is_open() && f2.is_open());
    ////////////////////////////////////////////////////////////////////////

#ifndef LOAD_RESPONSE
    // produce transient state
    networkmode = TRANSIENTSTATE;
    _network->LSMSetNetworkMode(networkmode);
    myTimer.Start();
#ifdef _RES_FIRING_CHR
    vector<double> prob, avg_intvl;
    vector<int> max_intvl;
#endif
    count = 0;
    _network->LSMClearSignals();
    info = _network->LoadFirstSpeech(false, networkmode);
    while(info != -1){
        Fp = NULL;
        Foutp = NULL;
        count++;
        int time = 0, end_time = _network->SpeechEndTime();
        while(!_network->LSMEndOfSpeech(networkmode, end_time)){
            _network->LSMNextTimeStep(++time,false,1, end_time, NULL);
        }
#ifdef QUICK_RESPONSE   //Only for NMNIST
        _network->SpeechPrint(info+(_network->GetTid()%10)*(_network->NumSpeech()>400?600:100), "reservoir"); // dump the reservoir response for quick simulation
#else

#ifdef _DUMP_RESPONSE
        if(tid == 0){
			_network->SpeechPrint(info, "reservoir"); // dump the reservoir response for quick simulation
            _network->DumpVMems("Waveform/transient", info, "reservoir");
        }
#endif
#endif
        // print the firing frequency into the file:
        //_network->SpeechSpikeFreq("input", f1, f2);
#ifdef _RES_FIRING_CHR
        _network->PreActivityStat("reservoir", prob, avg_intvl, max_intvl);
#endif

#ifdef _VARBASED_SPARSE
        // Collect the firing activity of each reservoir neurons from sp
        // and scatter the frequency back to the network:
        cout<<"Begin readout sparsification...."<<endl;
        vector<double> fs;
        _network->CollectSpikeFreq("reservoir", fs, time);
        _network->ScatterSpikeFreq("reservoir", fs);
        cout<<"Done with the readout sparsification!"<<endl;
#endif

        _network->LSMClearSignals();
        info = _network->LoadNextSpeech(false, networkmode);
    }
    myTimer.End("running transient");
    //////////////////////////////////////////////////////////////////////////////
    // REMEMBER TO REMOVE THESE CODES!
    //_network->LSMSumGatedNeurons();
    // f2<<endl;
    // f1.close(); f2.close();
    /////////////////////////////////////////////////////////////////////////////

#ifdef _VARBASED_SPARSE
    // sparsify the reservoir to readout connection using var-based technique:
    _network->VarBasedSparsify("readout");
#endif

#ifdef _CORBASED_SPARSE
    // merge the firing of two neurons based on the correlations
    cout<<"Begin correlation based readout sparsification"<<endl;
    _network->CorBasedSparsify();
#endif

#ifdef _RES_FIRING_CHR
    CollectPAStat(prob, avg_intvl, max_intvl);
    return;
#endif

#endif // end of load response
#ifdef QUICK_RESPONSE
	return;
#endif

#if defined(STDP_TRAINING_READOUT) && defined(STDP_TRAINING)
    // traing the readout using STDP rule first:
    /****************************************************************
      The current version suppose you have trained the reservoir
     *****************************************************************/
    networkmode = TRAINREADOUT;
    _network->LSMSetNetworkMode(networkmode);

    myTimer.Start();
    for(int i = 0; i < 20; ++i){
        cout<<"\n************************"
            <<"\n* i = "<<i
            <<"\n************************"<<endl;
        _network->LSMUnsupervisedTraining(networkmode, tid);
#if NUM_THREADS == 1
        // Write the weight back to file
        sprintf(filename, "readout_weights_%d.txt", i);
        _network->WriteSelectedSynToFile("readout", filename);
#endif
    }
    if(tid == 0){
        sprintf(filename, "o_weights_info_trained.txt");
        _network->WriteSelectedSynToFile("readout", filename);
    }
    _network->RemoveZeroWeights("readout");
    myTimer.End("training the readout");
#endif

    // train the readout layer
    myTimer.Start();
#ifndef TEACHER_SIGNAL
    networkmode = READOUTBP; // choose the readout supervised algorithm here!
#else
    networkmode = READOUT; // choose the readout supervised algorithm here!
#endif
    _network->LSMSetNetworkMode(networkmode);
#ifdef CV
#if NUM_THREADS == 1
    for(int fff = 0; fff < NFOLD; ++fff){
        _network->Fold(fff);
        _network->LSMClearWeights();
        Tid = (int)tid;
        cout<<"Only one thread is running:Tid_"<<Tid<<endl;
#else 
        Tid = (int)tid;
        cout<<"Tid:"<<Tid<<endl;
        _network->Fold(Tid);
#endif

#ifdef _WRITE_STAT
        info = _network->LoadFirstSpeechTestCV(networkmode);
        while(info != -1){
            // write the file array for the purpose of parallel writing protectation:
            file[info] = Tid;
            sprintf(filename,"outputs/spikepattern%d.dat",info);
            InitializeFile(filename);

            info = _network->LoadNextSpeechTestCV(networkmode);
        }
#endif
#endif
        for(int iii = 0; iii < NUM_ITERS; iii++){
            if(tid == 0)    cout<<"Run the iteration: "<<iii<<endl;
            // random shuffle the training samples for better generalization
            _network->ShuffleTrainingSamples();
            _network->LSMSupervisedTraining(networkmode, tid, iii);
#ifdef _DUMP_READOUT_WEIGHTS
            if(tid == 0){
                sprintf(filename, "o_weights_info_trained_intern_%d.txt",iii);
                _network->WriteSelectedSynToFile("readout", filename);
            }
#endif

        }
#if defined(CV) && NUM_THREADS == 1
    }
#endif
    myTimer.End("supervised training the readout"); 
#ifdef _RES_EPEN_CHR
    CollectEPENStat("reservoir");
    CollectEPENStat("readout");
#endif

    if(tid == 0){
        sprintf(filename, "o_weights_info_trained_final.txt");
        _network->WriteSelectedSynToFile("readout", filename); 
    }

}


//* Print the synaptic activity to a file:
void Simulator::PrintSynAct(int info){
    char name1[64], name2[64];
    char filename[128];
    sprintf(name1, "reservoir_1");
    sprintf(name2, "reservoir_15");
    sprintf(filename, "activity/speech_%d.txt", info);

    _network->WriteSynActivityToFile(name1, name2, filename);
}


//* Print out the frequencies to file:
void Simulator::PrintOutFreqs(const vector<vector<double> >& all_fs){
    ofstream f_out("all_readout_freq.txt");
    assert(f_out.is_open());
    for(size_t i = 0; i < all_fs.size(); ++i){
        for(size_t j = 0; j < all_fs[i].size(); ++j){
            f_out<<all_fs[i][j]<<"\t";
        }
        f_out<<endl;
        f_out<<"*******************************************************************"<<endl;
        f_out<<endl;
    }
    f_out.close();
}

/***************************************************************************** 
 * Collect the avg pre-synaptic event statistics of each neuron and compute avg
 * @param1-3 input vectors, each elem is the stat of each speech
 ****************************************************************************/
void Simulator::CollectPAStat(vector<double>& prob, 
        vector<double>& avg_intvl, 
        vector<int>& max_intvl
        )
{
    // initialize the aggregated stat variables:
    double prob_f = 0.0, avg_intvl_f = 0.0;
    int max_intvl_f = 0;
    cout<<prob.size()<<"\t"<<avg_intvl.size()<<"\t"<<max_intvl.size()<<endl;

    assert(prob.size()== avg_intvl.size() 
            && prob.size()==max_intvl.size()
            && prob.size()==(size_t)_network->NumSpeech());

    for(size_t i = 0; i < prob.size(); ++i){
        prob_f += prob[i];
        avg_intvl_f += avg_intvl[i];
        max_intvl_f = max(max_intvl_f, max_intvl[i]);
    }

    prob_f = _network->NumSpeech() ? prob_f/(_network->NumSpeech()) : 0;
    avg_intvl_f = _network->NumSpeech() ? avg_intvl_f/(_network->NumSpeech()) : 0;
    // print out information:
    cout<<"The average probability of a pre-synaptic event: "<<prob_f<<endl;
    cout<<"The average interval between two pre-synaptic events: "<<avg_intvl_f<<endl;
    cout<<"The max interval between two pre-synaptic events: "<<max_intvl_f<<endl;
}

/*********************************************************************************
 * Collect the max/min of four state variable for synaptic response, which is
 * in the neuron class.
 ********************************************************************************/

void Simulator::CollectEPENStat(const char * type){
    assert(type);
    _network->CollectEPENStat(type);
}


void Simulator::InitializeFile(const char * filename){
    FILE * Foutp = fopen(filename,"w");
    assert(Foutp != NULL);
    fprintf(Foutp,"%d\t%d\n",-1,-1);
    fclose(Foutp);
}
