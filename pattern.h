#ifndef PATTERN_H
#define PATTERN_H

#include <vector>
#include <iostream>

class Neuron;
class Network;

class Pattern{
private:
  std::vector<Neuron*> _activates;
  std::vector<double> _externalInputs;
  char * _name;
public:
  void AddNeuron(Neuron*, double);
  void SetName(char*);
  const char * Name();
  void FixPattern();
  void PrintFile(FILE*);
  int NonZeroPixel();
  double TotalPixelStrength();
  void Normalization();
};

#endif
