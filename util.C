/*
  Implementation of supporting functions
  Author: Yingyezhe Jin
  Date: Sept 8, 2016
 */
#include "util.h"

Timer::Timer(){
  gettimeofday(&m_start, &m_zone);
  is_started = false;
}

void Timer::Start(){
  gettimeofday(&m_start, &m_zone);
  m_cur = m_start;
  is_started = true;
}

void Timer::Report(std::string str){
  timeval tmp;
  gettimeofday(&tmp, &m_zone);
  if(is_started){
    std::cout<<"Total time spent in "<<str<<" : "<<(tmp.tv_sec - m_cur.tv_sec) + double((tmp.tv_usec - m_cur.tv_usec)*1e-6)<<" seconds"<<std::endl;
  }
  else{
    std::cerr<<"Warning: the util Timer is not STARTED! Start the timer...."<<std::endl;
    Start();
    return;
  }
  m_cur = tmp;
}

void Timer::End(std::string str){
  timeval tmp;
  gettimeofday(&tmp, &m_zone);
  if(is_started && !str.empty()){
    std::cout<<"Total time spent in "<<str<<" : "<<(tmp.tv_sec - m_start.tv_sec) + double((tmp.tv_usec - m_start.tv_usec)*1e-6)<<" seconds"<<std::endl;
  }
  else if(!is_started){
    std::cerr<<"Warning: the util Timer is not STARTED! Start the timer...."<<std::endl;
    Start();
    return;
  }  
  m_cur = tmp;
  m_end = tmp;
}
