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
    void PrintSynAct(int info);
    void PrintOutFreqs(const std::vector<std::vector<double> >& all_fs);
    void CollectPAStat(std::vector<double>& prob, 
            std::vector<double>& avg_intvl, 
            std::vector<int>& max_intvl
            );
    void CollectEPENStat(const char * type);
    void InitializeFile(const char * filename);
};

#endif

