#ifndef PARSER_H
#define PARSER_H

class Network;

class Parser{
private:
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
};

#endif

