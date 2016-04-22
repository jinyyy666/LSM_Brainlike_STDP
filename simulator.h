#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "network.h"
#include <vector>

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

  void LSMRun(long tid);
  void ReadoutJudge(int& correct, int& wrong, int& even);
  void PrintSynAct(int info);
  void PrintOutFreqs(const std::vector<std::vector<double> >& all_fs);
  void CollectPAStat(std::vector<double>& prob, 
		     std::vector<double>& avg_intvl, 
		     std::vector<int>& max_intvl, 
		     double& prob_f, 
		     double& avg_intvl_f, 
		     int& max_intvl_f
		    );
};

#endif

