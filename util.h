/*
  Utility function for supporting
  Author: Yingyezhe Jin
  Date: Sept 8, 2016
 */
#include <sys/time.h>
#include <iostream>
#include <string>

class Timer{
 private:
  struct timeval m_start, m_cur, m_end;
  struct timezone m_zone;
  bool is_started;
 public:
  Timer();
  void Start();
  void Report(std::string str);
  void End(std::string str="");
};
