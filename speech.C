#include "speech.h"
#include "channel.h"
#include <assert.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <algorithm>

using namespace std;

Speech::Speech(int cls):
_index(-1),
_class(cls)
{}

Speech::~Speech(){
  for(vector<Channel*>::iterator iter = _channels.begin(); iter != _channels.end(); iter++) delete *iter;
}

Channel * Speech::AddChannel(int step_analog, int step_spikeT){
  Channel * channel = new Channel(step_analog, step_spikeT);
  _channels.push_back(channel);
  return channel;
}

void Speech::SetNumReservoirChannel(int size){
  assert(size >= 0);
  while(_rChannels.size() < size){
    Channel * channel = new Channel;
    _rChannels.push_back(channel);
  }
}

Channel * Speech::GetChannel(int index, channelmode_t channelmode){
  if(channelmode == INPUTCHANNEL){
    if(index < 0 && index >= _channels.size()){
      cout<<"Invalid channel index: "<<index
	  <<" seen in aquiring input channels!\n"
	  <<"Total number of input channels: "<<_channels.size()
	  <<endl;
      exit(EXIT_FAILURE);
    }
    return _channels[index];
  }else{
    if(index < 0 && index >= _rChannels.size()){
      cout<<"Invalid channel index: "<<index
	  <<" seen in aquiring reservoir channels!\n"
	  <<"Total number of reservoir channels: "<<_rChannels.size()
	  <<endl;
      exit(EXIT_FAILURE);
    }
    return _rChannels[index];
  }
}

void Speech::AnalogToSpike(){
  for(int i = 0; i < _channels.size(); i++){
    assert(_channels[i]->SizeAnalog() == _channels[0]->SizeAnalog());
    _channels[i]->BSA();
//if(i>=4)assert(0);
//    cout<<"Channel "<<i<<"\t"<<_channels[i]->SizeAnalog()<<"\tvs.\t"<<_channels[i]->SizeSpikeT()<<endl;
  }
}

void Speech::RateToSpike(){
  for(int i = 0; i < _channels.size(); i++){
    assert(_channels[i]->SizeAnalog() == _channels[0]->SizeAnalog());
    _channels[i]->PoissonSpike();
  }
}


int Speech::NumChannels(channelmode_t channelmode){
  if(channelmode == INPUTCHANNEL) return _channels.size();
  else return _rChannels.size();
}

void Speech::Info(){
  cout<<"# of input channels = "<<_channels.size()<<endl;
  for(int i = 0; i < _channels.size(); i++) cout<<_channels[i]->SizeSpikeT()<<"\t";
  cout<<endl;
  cout<<"# of reservoir channels = "<<_rChannels.size()<<endl;
  for(int i = 0; i < _rChannels.size(); i++) cout<<_rChannels[i]->SizeSpikeT()<<"\t";
  cout<<endl;
}

void Speech::PrintSpikes(int info){
  FILE * Fp_input;
  FILE * Fp_reservoir;
  char filename[64];
  sprintf(filename,"Input_Response/input_spikes_%d.dat",info);
  // Fp_input = fopen(filename,"w");
  ofstream f_input(filename);
  assert(f_input.is_open());
  // assert(Fp_input != NULL);
  for(int i = 0; i < _channels.size(); i++){ 
      /*
    fprintf(Fp_input,"%d\n",-1);
    _channels[i]->Print(Fp_input);
      */
    _channels[i]->Print(f_input);
  }
  f_input.close();

  // fprintf(Fp_input,"%d\n",-1);
  // fclose(Fp_input);

  sprintf(filename,"Reservoir_Response/reservoir_spikes_%d.dat",info);
  // Fp_reservoir = fopen(filename,"w");
  ofstream f_reservoir(filename);
  assert(f_reservoir.is_open());
  // assert(Fp_reservoir != NULL);
  for(int i = 0; i < _rChannels.size(); i++){
      /*
    fprintf(Fp_reservoir,"%d\n",-1);
    _rChannels[i]->Print(Fp_reservoir);
      */
    _rChannels[i]->Print(f_reservoir);
  }
  f_reservoir.close();

  // fprintf(Fp_reservoir,"%d\n",-1);
  // fclose(Fp_reservoir); 
}

//* this function read each channel and output the firing frequency into a matrix
//* defined by the output stream: f_out.
//* there are two type of channels to be considered.
//* @return: the associated class label.
int Speech::PrintSpikeFreq(const char * type, ofstream & f_out){
    assert(type);
    bool f;
    if(strcmp(type, "input") == 0) f = false;
    else if(strcmp(type, "reservoir") == 0) f = true;
    else assert(0);
    
    if(!f){
	SpikeFreq(f_out, _channels);	
    }
    else{
	SpikeFreq(f_out, _rChannels);
    }
    return _class;
}

//* Print the spike rate to the target file:
void Speech::SpikeFreq(ofstream & f_out, const vector<Channel*> & channels){
    // find out the stop time for each speech:
    int stop_t = INT_MIN;
    for(int i = 0; i < channels.size(); i++){
	assert(channels[i]);
	stop_t = max(stop_t, channels[i]->LastSpikeT());
    }
    for(int i = 0; i < channels.size(); i++){ 
	//f_out<<(double)channels[i]->SizeSpikeT()/stop_t<<"\t";
        f_out<<channels[i]->SizeSpikeT()<<"\t";
    }
    f_out<<endl;
}
