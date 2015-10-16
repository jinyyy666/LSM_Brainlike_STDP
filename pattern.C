#include "def.h"
#include "pattern.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include <list>
#include <assert.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace std;

void Pattern::AddNeuron(Neuron * neuron, double externalInput){
  assert(externalInput <= MAXINPUT);
  assert(externalInput >= MININPUT);
  _activates.push_back(neuron);
  _externalInputs.push_back(externalInput);
}

void Pattern::SetName(char * name){
  _name = new char[strlen(name)+2];
  strcpy(_name,name);
}

const char * Pattern::Name(){
  return _name;
}


void Pattern::FixPattern(){
  assert(_activates.size() == _externalInputs.size());
  _activates.reserve(_activates.size());
  _externalInputs.reserve(_externalInputs.size());
}

void Pattern::PrintFile(FILE * fp){
  assert(_externalInputs.size() == 784);
  int i, j;
  int k = 0;
  for(i = 0; i < 28; i++){
    for(j = 0; j < 28; j++) fprintf(fp,"%.5f\t",_externalInputs[k++]);
    fprintf(fp,"\n");
  }
}

int Pattern::NonZeroPixel(){
  int total = 0;
  for(int i = 0; i < _externalInputs.size(); i++) if(_externalInputs[i] > 1e-6) total++;
  return total;
}

double Pattern::TotalPixelStrength(){
  double total = 0;
  for(int i = 0; i < _externalInputs.size(); i++) total += _externalInputs[i];
  return total;
}

void Pattern::Normalization(){
  double max = -1e12;
  double min = 1e12;
  for(int i = 0; i < _externalInputs.size(); i++){
    if(_externalInputs[i] > max) max = _externalInputs[i];
    if(_externalInputs[i] < min) min = _externalInputs[i];
  }
  if(max < (min + 1e-12)) return;
  for(int i = 0; i < _externalInputs.size(); i++){
    _externalInputs[i] = (_externalInputs[i]-min)/(max-min);
  }
}
