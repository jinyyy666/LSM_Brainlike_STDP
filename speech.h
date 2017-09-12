#ifndef SPEECH
#define SPEECH

#include <vector>
#include <fstream>
#include "def.h"

class Channel;

class Speech{
private:
    std::vector<Channel*> _channels;
    std::vector<Channel*> _rChannels;
    std::vector<Channel*> _oChannels;
    int _class;
    int _index;
    int _num_input_channels;
    int _input_connections;
public:
    Speech(int);
    ~Speech();
    void SetNumReservoirChannel(int);
    void SetNumReadoutChannel(int);
    Channel * AddChannel(int, int);
    Channel * GetChannel(int,channelmode_t);
    void ClearChannel(channelmode_t channelmode);
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
    void PrintSpikes(int info);
    int PrintSpikeFreq(const char * type, std::ofstream & f_out);
    void SpikeFreq(std::ofstream & f_out, const std::vector<Channel*> & channels);
    int  EndTime();
    void CollectFreq(synapsetype_t syn_t, std::vector<double>& fs, int end_t);
};
#endif


