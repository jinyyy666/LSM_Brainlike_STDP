#include <cstdlib>
#include <math.h>
#include <iostream>
#include <assert.h>
#include <random>
#include "channel.h"

using namespace std;
std::default_random_engine generator;

// input channel
Channel::Channel(int step_analog, int step_spikeT):
_step_analog(step_analog),
_step_spikeT(step_spikeT),
_mode(INPUTCHANNEL)
{} 

// reservoir channel
Channel::Channel():
_mode(RESERVOIRCHANNEL)
{}

void Channel::AddAnalog(double signal){
  _analog.push_back(signal);
}

void Channel::AddSpike(int spikeT){
  int size = _spikeT.size();
  if(size > 0){
    if(spikeT <= _spikeT[size-1])
      cout<<spikeT<<"\t"<<_spikeT[size-1]<<endl;
    assert(spikeT > _spikeT[size-1]);
  }else{
    assert(spikeT > 0);
  }
  _spikeT.push_back(spikeT);
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
  double threshold = 0.6;
  double error1, error2, temp;
  double * kernel = new double[length_kernel];
  double * signal = new double[length_signal];
//double sig, noi;

  for(int i = 0; i < length_kernel; i++) kernel[i] = exp(-(i-double(length_kernel)/2)*(i-double(length_kernel)/2)/25);
  temp = 0;
  for(int i = 0; i < length_kernel; i++) temp += kernel[i];
  for(int i = 0; i < length_kernel; i++) kernel[i] /= temp;

//for(threshold = 0.5; threshold < 1.00001; threshold += 0.01){
  int index = 0;
  for(int i = 0; i < _analog.size(); i++)
    for(; index < (i+1)*_step_analog/_step_spikeT; index++)
      signal[index] = _analog[i]*2e3;
  for(; index < length_signal; index++) signal[index] = 0;
//sig = 0;
//for(int i = 0; i < length_signal; i++) sig += signal[i]*signal[i];

  int j;
  for(int i = 0; i < length_signal-24; i++){
    error1 = 0;
    error2 = 0;
    for(j = 0; j < length_kernel; j++){
      temp = signal[i+j] - kernel[j];
      error1 += (temp<0) ? -temp : temp;
      temp = signal[i+j];
      error2 += (temp<0) ? -temp : temp;
    }
    if(error1 < (error2-threshold)){
      _spikeT.push_back(i+1);
      for(j = 0; j < length_kernel; j++) signal[i+j] -= kernel[j];
    }
  }
//noi = 0;
//for(int i = 0; i < length_signal; i++) noi += signal[i]*signal[i];
//cout<<threshold<<"\t"<<noi/sig<<endl;
//}

//for(int i = 0; i < _analog.size(); i++) cout<<_analog[i]<<endl;
//for(int i = 0; i < _spikeT.size(); i++) cout<<_spikeT[i]<<endl;

  delete [] kernel;
  delete [] signal;
}

void Channel::PoissonSpike(){
  assert(_analog.size() == 1);
  std::uniform_real_distribution<double> dist(0.0, MAX_GRAYSCALE);

  double number;
 
  for(int i = 0; i < DURATION_TRAIN; ++i){
   number = dist(generator);
   if(number < _analog[0])
     _spikeT.push_back(i+1);
  }
}

//* clear the channel:
void Channel::Clear(){
    // Note that this is going to invalid the iterator!!
    // So when you are using the iterator, please call FirstSpike() at first!!
    _spikeT.clear();
}

void Channel::Print(FILE * fp){
  if(_spikeT.empty() == true) fprintf(fp,"%d\n",-99);
  for(_iter_spikeT = _spikeT.begin(); _iter_spikeT != _spikeT.end(); _iter_spikeT++) 
    if(fp != NULL){
        int t = *_iter_spikeT;
	fprintf(fp,"%d\n",t);
    }
}

void Channel::Print(ofstream& f_out){
    assert(f_out.is_open());
    if(_spikeT.empty() == true) f_out<<"-99"<<endl;
    for(_iter_spikeT = _spikeT.begin(); _iter_spikeT != _spikeT.end(); _iter_spikeT++) 
	f_out<<*_iter_spikeT<<"\n";
}
