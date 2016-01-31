#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include "def.h"
#include <cstdio>
#include <fstream>

class Channel{
private:
  std::vector<double> _analog;
  std::vector<int> _spikeT;
  std::vector<int>::iterator _iter_spikeT;
  int _step_analog;
  int _step_spikeT;
  channelmode_t _mode;
public:
  Channel(int,int);
  Channel();
  void AddAnalog(double);
  void AddSpike(int);
  int FirstSpikeT();
  int NextSpikeT();
  void BSA();
  void PoissonSpike();
  int SizeAnalog(){return _analog.size();}
  int SizeSpikeT(){return _spikeT.size();}
  int LastSpikeT(){return _spikeT.empty() ? 0 : _spikeT.back();}
  void SetMode(channelmode_t channelmode){_mode = channelmode;}
  channelmode_t Mode(){return _mode;}
  void Print(FILE *);
  void Print(std::ofstream& f_out);
};

#endif


