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
  void ParseSynapse(char *, char *, char *, char *, int);
//  void ParseSynapse(char *, char *, char *, char *, int, int, int, int);
  void ParseSynapse(char *, char *, char *, char *, int, int, int);
  void ParseLSMSynapse(char *, char *, int, int, int, int, char *);
  void ParseSpeech(int, char*);
};

#endif

