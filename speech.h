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
public:
  Speech(int);
  ~Speech();
  void SetNumReservoirChannel(int);
  Channel * AddChannel(int, int);
  Channel * GetChannel(int,channelmode_t);
  void AnalogToSpike();
  int NumChannels(channelmode_t);
  int Class(){return _class;}
  void SetIndex(int index) {_index = index;}
  int Index() {return _index;}

  void Info();
  void PrintSpikes(int);
};

#endif


