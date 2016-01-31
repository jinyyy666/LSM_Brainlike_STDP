#include "speech.h"
#include "channel.h"
#include <assert.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>

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
  Fp_input = fopen(filename,"w");
  assert(Fp_input != NULL);
  for(int i = 0; i < _channels.size(); i++){ 
    fprintf(Fp_input,"%d\n",-1);
    _channels[i]->Print(Fp_input);
  }
  fprintf(Fp_input,"%d\n",-1);
  fclose(Fp_input);

  sprintf(filename,"Reservoir_Response/reservoir_spikes_%d.dat",info);
  Fp_reservoir = fopen(filename,"w");
  assert(Fp_reservoir != NULL);
  for(int i = 0; i < _rChannels.size(); i++){
    fprintf(Fp_reservoir,"%d\n",-1);
    _rChannels[i]->Print(Fp_reservoir);
  }
  fprintf(Fp_reservoir,"%d\n",-1);
  fclose(Fp_reservoir); 
}
