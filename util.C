/*
  Implementation of supporting functions
  Author: Yingyezhe Jin
  Date: Sept 8, 2016
 */
#include "util.h"
#include <assert.h>
#include <utility>

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


UnionFind::UnionFind(int n):_n(n), _count(0)
{
  _sz = std::vector<int>(_n, -1);
  _id = std::vector<int>(_n, -1);
}

int UnionFind::root(int ind){
  assert(exist(ind));
  while(ind != _id[ind]){
    _id[ind] = _id[_id[ind]]; // path compression
    ind = _id[ind];
  }
  return ind;
}

// check if the group has one element
bool UnionFind::individual(int ind){
  if(ind >= _n){
    std::cout<<"Warning: UnionFind:: index "<<ind<<" out of bound "<<_n<<std::endl;
    return false;
  }
  return _sz[ind] == 1;
}

bool UnionFind::exist(int ind){
  if(ind >= _n){
    std::cout<<"Warning: UnionFind::exist() index "<<ind<<" out of bound "<<_n<<std::endl;
    return -1;
  }
  return (_sz[ind] != -1);
}

void UnionFind::add(int ind){
  if(ind >= _n){
    std::cout<<"Warning: UnionFind::add, index: "<<ind<<" out of bound "<<_n<<std::endl;
    return;
  }
  if(exist(ind))  return;
  _id[ind] = ind;
  _sz[ind] = 1;
  _count++;
}

bool UnionFind::find(int p, int q){
  return root(p) == root(q);
}

void UnionFind::unite(int p, int q){
  int i = root(p);
  int j = root(q);
  if(_sz[i] >= _sz[j]){
    _sz[i] += _sz[j]; _id[j] = i; // always merge the smaller to the larger one
  }
  else{
    _sz[j] += _sz[i]; _id[i] = j;
  } 
  _count--;
}

void UnitTest::testUnionFind(){
  std::vector<std::pair<int, int>> pairs({{3,4}, {4,9}, {8,0}, {2,3}, {5,6}, {5,9}, {7,3}, {4,8}, {6,1}});
  UnionFind uf(10);
  for(int i = 0; i < pairs.size(); ++i){
    int p = pairs[i].first, q = pairs[i].second;
    uf.add(p), uf.add(q);
    uf.unite(p, q);
  }
  if(uf.count() != 1){
    std::cout<<"In UnitTest::testUnionFind(), something wrong with the size counting!"
	     <<"The size you have: "<<uf.count()<<std::endl;
    exit(EXIT_FAILURE);
  }
  std::vector<int> ids = uf.getGroupVec();
  std::vector<int> szs = uf.getSizeVec();
  bool pass = true;
  for(int i = 0; i < ids.size(); ++i){
    if(i == 0){
      pass = pass && (ids[i] == 8);
    }
    else
      pass = pass && (ids[i] == 3);
  }
  if(pass && szs[3] == 10)
    std::cout<<"UnionFind successfully passes the test!"<<std::endl;
  else
    std::cout<<"In UnitTest::testUnionFind(), wrong group result!"<<std::endl;
}
