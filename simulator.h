#ifndef SIMULATOR_H
#define SIMULATOR_H

#include"network.h"

class Simulator{
private:
  double _t;
  double _t_end;
  double _t_step;

  Network * _network;
public:
  Simulator(Network*);
  void SetEndTime(double);
  void SetStepSize(double);
  void Run(bool);
  void LSMRun(long tid);
};

#endif

