#ifndef PREPROCESS
#define PREPROCESS

#include<vector>

class Channel{
private:
  std::vector<double> _analog;
  std::vector<int> _spikeT;
  std::vector<int>::iterator _iter_spikeT;
  int _step_analog;
  int _step_spikeT;
public:
  Channel(int,int);
  void AddAnalog(double);
  int FirstSpikeT();
  int NextSpikeT();
  void BSA();
};

#endif


