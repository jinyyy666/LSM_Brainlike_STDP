#ifndef PARSER_H
#define PARSER_H

class Network;

class Parser{
private:
  Network * _network;
public:
  Parser(Network *);
  void Parse(const char *);
  void ParseNeuron(char *, char *);
  void ParseNeuronGroup(char *, int, char *);
 
  void ParseLSMSynapse(char *, char *, int, int, int, int, char *);
  void ParseSpeech(int, char*);
};

#endif

