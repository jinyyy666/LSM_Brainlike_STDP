#ifndef SPEECH
#define SPEECH

#include<vector>
#include"def.h"

class Channel;

class Speech{
private:
  std::vector<Channel*> _channels;
  std::vector<Channel*> _rChannels;
  int _class;
  int _index;
  int _num_input_channels;
  int _input_connections;
public:
  Speech(int);
  ~Speech();
  void SetNumReservoirChannel(int);
  Channel * AddChannel(int, int);
  Channel * GetChannel(int,channelmode_t);
  void AnalogToSpike();
  void RateToSpike();
  int NumChannels(channelmode_t);
  int Class(){return _class;}
  void SetIndex(int index) {_index = index;}
  int Index() {return _index;}

  void AddInputNumInfo(int num_inputs){ _num_input_channels = num_inputs;}
  void AddInputConnectionInfo(int input_connections){ _input_connections = input_connections;}
  int NumInputs(){return _num_input_channels;}
  int NumConnections(){return _input_connections;}

  void Info();
  void PrintSpikes(int);
};

#endif


