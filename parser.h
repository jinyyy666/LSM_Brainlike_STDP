#ifndef PARSER_H
#define PARSER_H
#include "def.h"
#include <vector>
#include <string>
#include <cstring>

class Network;

class Parser{
private:
#if _NMNIST==1
	std::vector<std::vector<std::string>> path_name;
	std::vector<std::vector<int>> path_index;
	std::vector<std::vector<int>> path_class;
#endif
    Network * _network;
public:
    Parser(Network *);
    void Parse(const char *);
    void ParseNeuron(char * name, char * e_i, double v_mem);
    void ParseNeuronGroup(char * name, int num, char * e_i, double v_mem);

    void ParseLSMSynapse(char *, char *, int, int, int, int, char *);
    void ParseSpeech(int, char*);
    void ParsePoissonSpeech(int cls, char * path);
    void ParseMNISTSpeech(int cls, char * path);
	void SavePath(int cls,char* path);
	void ParseNMNIST();
	void LoadWholeTestBench(bool test);	
	void LoadSingleTestBench();	
	void QuickLoad();
};

#endif

