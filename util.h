/*
  Utility function for supporting
  Author: Yingyezhe Jin
  Date: Sept 8, 2016
 */
#ifndef UTIL_H
#define UTIL_H

#include <sys/time.h>
#include <iostream>
#include <string>
#include <vector>

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

class UnionFind{
 private:
  int _n;
  int _count;
  std::vector<int> _sz;
  std::vector<int> _id;

 public:
  UnionFind(int n);
  int root(int ind);
  bool individual(int ind);
  bool exist(int ind);
  void add(int ind);
  bool find(int p, int q);
  void unite(int p, int q);
  int count(){return _count;}
  int size(){return _n;}
  std::vector<int> getGroupVec(){return _id;}
  std::vector<int> getSizeVec(){return _sz;}
};

class UnitTest{
 public:
  void testUnionFind();

};


#endif
