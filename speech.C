#include"speech.h"
#include"channel.h"
#include<assert.h>
#include<iostream>

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
    assert((index >= 0)&&(index < _channels.size()));
    return _channels[index];
  }else{
    assert((index >= 0)&&(index < _rChannels.size()));
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

