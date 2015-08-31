#include<stdlib.h>
#include<math.h>
#include"preprocess.h"

using namespace std;

Channel::Channel(int step_analog, int step_spikeT):
_step_analog(step_analog),
_step_spikeT(step_spikeT)
{} 

void Channel::AddAnalog(double signal){
  _analog.push_back(signal);
}

int Channel::FirstSpikeT(){
  if(_spikeT.empty() == true) return -1;
  _iter_spikeT = _spikeT.begin();
  return *_iter_spikeT;
}

int Channel::NextSpikeT(){
  _iter_spikeT++;
  if(_iter_spikeT == _spikeT.end()) return -1;
  else return *_iter_spikeT;
}

void Channel::BSA(){
  int length_kernel = 24;
  int length_signal = (_analog.size()*_step_analog)/_step_spikeT + 24;
  double threshold = 0.95;
  double error1, error2, error;
  double * kernel = new double[length_kernel];
  double * signal = new double[length_signal];

  for(int i = 0; i < length_kernel; i++) kernel[i] = exp(-(i-double(length_kernel)/2)*(i-double(length_kernel)/2)/25);
  int index = 0;
  for(int i = 0; i < _analog.size(); i++)
    for(; index < (i+1)*_step_analog/_step_spikeT; index++)
      signal[index] = (_analog[i]*_step_spikeT)/_step_analog;
  for(; index < length_signal; index++) signal[index] = 0;

  int j;
  for(int i = 0; i < length_signal-24; i++){
    error1 = 0;
    error2 = 0;
    for(j = 0; j < length_kernel; j++){
      error = signal[i+j] - kernel[j];
      error1 += (error<0) ? -error : error;
      error = signal[i+j];
      error2 += (error<0) ? -error : error;
    }
    if(error1 < (error2-threshold)) _spikeT.push_back(i);
  }

  delete [] kernel;
  delete [] signal;
}

