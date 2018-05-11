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
#ifdef USE_TEST_SAMPLE
    cout<<"Number of speeches for testing: "<<_network->NumTestSpeech()<<endl;
#endif

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

	networkmode_t phase=_network->PhaseFirst();
	while(phase){
		myTimer.Start();
		switch(phase){
			case TRAININPUT:
				cout<<"stdp train input start"<<endl;
				_network->LSMTrainInput(phase,tid);
				myTimer.End("STDP training of input");
				cout<<"stdp train input end"<<endl;
				break;
			case TRAINRESERVOIR:
				cout<<"stdp train reservoir start"<<endl;
				_network->LSMTrainReservoir(phase,tid);
				myTimer.End("STDP training of input and reservoir");
				break;
				cout<<"stdp train reservoir end"<<endl;
			case TRAINREADOUT:
				cout<<"stdp train readout start"<<endl;
				_network->LSMTrainReadout(phase,tid);
				myTimer.End("training the readout");
				cout<<"stdp train readout end"<<endl;
				break;
			case TRANSIENTSTATE:
				cout<<"transient start"<<endl;
				// produce transient state
				_network->LSMTransientSim(phase, tid, "train_sample");
#ifdef USE_TEST_SAMPLE
				_network->LSMTransientSim(phase, tid, "test_sample");
#endif
				myTimer.End("running transient");			
				cout<<"transient end"<<endl;
				break;
			case READOUT:
				cout<<"train readout start"<<endl;
				_network->LSMReadout(phase, tid);
				myTimer.End("supervised training the readout"); 
				cout<<"train readout end"<<endl;
				break;
			case READOUTSUPV:
				cout<<"supervised stdp train readout start"<<endl;
				_network->LSMReadout(phase, tid);
				myTimer.End("supervised STDP training the readout"); 
				cout<<"supervised stdp train readout end"<<endl;
				break;
			case READOUTBP:
				cout<<"backpropogate train readout start"<<endl;
				_network->LSMReadout(phase, tid);
				myTimer.End("backpropagation training the readout"); 
				cout<<"backpropogate train readout end"<<endl;
				break;
			case FEEDBACKSTDP:
				cout<<"feedback stdp start"<<endl;
				_network->LSMFeedbackSTDP(phase, tid);
				myTimer.End("training the feedback with STDP"); 
				cout<<"feedback stdp end"<<endl;
				break;
			case FEEDBACKREADOUT:
				cout<<"feedback train readout start"<<endl;
				_network->LSMFeedbackReadout(phase, tid);
				myTimer.End("training the feedback network with learning rule"); 
				cout<<"feedback train readout end"<<endl;
				break;
			case VOID:
				return;
			default:
				assert(0);
		}

		phase=_network->PhaseNext();
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
