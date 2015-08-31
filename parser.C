#include"def.h"
#include"parser.h"
#include"pattern.h"
#include"neuron.h"
#include"network.h"
#include"speech.h"
#include"channel.h"
#include<stdlib.h>
#include<iostream>
#include<stdio.h>
#include<assert.h>
#include<string.h>

using namespace std;

Parser::Parser(Network * network){
  _network = network;
}

void Parser::Parse(const char * filename){
  int i;
  char linestring[8192];
  char ** token = new char*[64];
  FILE * fp = fopen(filename,"r");
  assert(fp != NULL);

  assert(fp != 0);
  while(fgets(linestring, 8191, fp) != NULL){
    if(strlen(linestring) <= 1) continue;
    if(linestring[0] == '#') continue;

    token[0] = strtok(linestring," \t\n");
    assert(token[0] != NULL);

    if(strcmp(token[0],"neuron")==0){
      // neuron name
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);

      ParseNeuron(token[1],token[2]);
    }else if(strcmp(token[0],"neurongroup")==0){
      // neurongroup name number
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      token[3] = strtok(NULL," \t\n");
      assert(token[3] != NULL);
      ParseNeuronGroup(token[1],atoi(token[2]),token[3]);
    }else if(strcmp(token[0],"column")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      token[3] = strtok(NULL," \t\n");
      assert(token[3] != NULL);
      token[4] = strtok(NULL," \t\n");
      assert(token[4] != NULL);

      assert(_network->CheckExistence(token[1]) == false);
      int dim1 = atoi(token[2]);
      int dim2 = atoi(token[3]);
      int dim3 = atoi(token[4]);
      assert((dim1>0)&&(dim2>0)&&(dim3>0));
      _network->LSMAddNeuronGroup(token[1],dim1,dim2,dim3);
    }else if(strcmp(token[0],"brain")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      token[3] = strtok(NULL," \t\n");
      assert(token[3] != NULL);

      assert(_network->CheckExistence(token[1]) == false);
      _network->LSMAddNeuronGroup(token[1],token[2],token[3]);
    }else if(strcmp(token[0],"synapse")==0){
      // synapse preNeuron postNeuron excitatory/inhibitory fixed/learn (values)
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      token[3] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      token[4] = strtok(NULL," \t\n");
      assert(token[2] != NULL);

      if(strcmp(token[4],"fixed") == 0){
        token[5] = strtok(NULL," \t\n");
        assert(token[5] != NULL);

        ParseSynapse(token[1], token[2], token[3], token[4], atoi(token[5]));
      }else if((strcmp(token[4],"learn") == 0)||(strcmp(token[4],"fixedrandom") == 0)){
        token[5] = strtok(NULL," \t\n");
        assert(token[5] != NULL);
        token[6] = strtok(NULL," \t\n");
        assert(token[6] != NULL);
        token[7] = strtok(NULL," \t\n");
        assert(token[7] != NULL);
//        token[8] = strtok(NULL," \t\n");
//        assert(token[8] != NULL);

//        ParseSynapse(token[1], token[2], token[3], token[4], atoi(token[5]), atoi(token[6]), atoi(token[7]), atoi(token[8]));
        ParseSynapse(token[1], token[2], token[3], token[4], atoi(token[5]), atoi(token[6]), atoi(token[7]));
      }else assert(0);
    }else if(strcmp(token[0],"lsmsynapse")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      token[3] = strtok(NULL," \t\n");
      assert(token[3] != NULL);
      token[4] = strtok(NULL," \t\n");
      assert(token[4] != NULL);
      token[5] = strtok(NULL," \t\n");
      assert(token[5] != NULL);
      token[6] = strtok(NULL," \t\n");
      assert(token[6] != NULL);
      token[7] = strtok(NULL," \t\n");
      assert(token[7] != NULL);

      assert(_network->CheckExistence(token[1]) == true);
      assert(_network->CheckExistence(token[2]) == true);

      ParseLSMSynapse(token[1],token[2],atoi(token[3]),atoi(token[4]),atoi(token[5]),atoi(token[6]),token[7]);
    }else if(strcmp(token[0],"train") == 0){
      _network->LoadPatternsTrain();
    }else if(strcmp(token[0],"test") == 0){
      _network->LoadPatternsTest();
    }else if(strcmp(token[0],"activate")==0){
      Neuron * neuron;
      Pattern * pattern = new Pattern();
      _network->AddPattern(pattern);
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      pattern->SetName(token[1]);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);
      neuron = _network->SearchForNeuron(token[2]);
      if(neuron == NULL){
        NeuronGroup * neuronGroup = _network->SearchForNeuronGroup(token[2]);
        assert(neuronGroup != NULL);
        for(i = 0; i < neuronGroup->Size(); i++){
          token[3] = strtok(NULL," \t\n");
          assert(token[3] != NULL);
          pattern->AddNeuron(neuronGroup->Order(i),atof(token[3]));
        }
        token[3] = strtok(NULL," \t\n");
        assert(token[3] == NULL);
        pattern->FixPattern();
/*
        token[3] = strtok(NULL," \t\n");
        assert(token[3] != NULL);
        do{
          neuron = _network->SearchForNeuron(token[2],token[3]);
          pattern->AddNeuron(neuron);
          token[3] = strtok(NULL," \t\n");
        }while(token[3] != NULL);
*/
      }else assert(0); //pattern->AddNeuron(neuron);
    }else if(strcmp(token[0],"speech")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);

      ParseSpeech(atoi(token[1]),token[2]);
    }else if(strcmp(token[0],"end")==0){
      break;
    }else{
      cout<<token[0]<<endl;
      assert(0);
    }

//    while(1){
//      token = strtok(NULL," \t\n");
//    }
  }

  fclose(fp);
}

void Parser::ParseNeuron(char * name, char * e_i){
  assert(_network->CheckExistence(name) == false);
  assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
  bool excitatory;
  if(strcmp(e_i,"excitatory") == 0) excitatory = true;
  else excitatory = false;
  _network->AddNeuron(name,excitatory);
}

void Parser::ParseNeuronGroup(char * name, int num, char * e_i){
  assert(_network->CheckExistence(name) == false);
  assert(num > 1);
  assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
  bool excitatory;
  if(strcmp(e_i,"excitatory") == 0) excitatory = true;
  else excitatory = false;
  _network->AddNeuronGroup(name,num,excitatory);
}

void Parser::ParseSynapse(char * from, char * to, char * e_i, char * fixed_learn, int value){
  assert(_network->CheckExistence(from) == true);
  assert(_network->CheckExistence(to) == true);
  assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
  assert(strcmp(fixed_learn, "fixed")==0);
  assert(value > -1e-18);

  bool excitatory, fixed;
  if(strcmp(e_i,"excitatory") == 0) excitatory = true;
  else excitatory = false;
  fixed = true;
  _network->AddSynapse(from,to,excitatory,fixed,value);
}

/*
void Parser::ParseSynapse(char * from, char * to, char * e_i, char * fixed_learn, int min, int max, int ini_min, int ini_max){
  assert(_network->CheckExistence(from) == true);
  assert(_network->CheckExistence(to) == true);
  assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
  assert(strcmp(fixed_learn, "learn")==0);
  assert(min > -1e-18);
  assert(max > min);
  assert(ini_min >= min-1e-18);
  assert(ini_max <= max+1e-18);

  bool exitatory, fixed;
  if(strcmp(e_i,"excitatory") == 0) exitatory = true;
  else exitatory = false;
  fixed = false;
  _network->AddSynapse(from,to,exitatory,fixed,min,max,ini_min,ini_max);
}
*/

void Parser::ParseSynapse(char * from, char * to, char * e_i, char * fixed_learn, int factor, int ini_min, int ini_max){
  assert(_network->CheckExistence(from) == true);
  assert(_network->CheckExistence(to) == true);
  assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
  assert((strcmp(fixed_learn, "learn")==0)||(strcmp(fixed_learn, "fixedrandom")==0));
//  assert(min > -1e-18);
//  assert(max > min);
  assert(ini_min >= 0);
  assert(ini_max < SYNLVL);
  if(factor <= 0) cout<<from<<'\t'<<to<<'\t'<<e_i<<'\t'<<fixed_learn<<'\t'<<factor<<'\t'<<ini_min<<'\t'<<ini_max<<endl;
  assert(factor > 0);

  bool exitatory, fixed;
  if(strcmp(e_i,"excitatory") == 0) exitatory = true;
  else exitatory = false;
  if(strcmp(fixed_learn, "learn")==0) fixed = false;
  else if(strcmp(fixed_learn, "fixedrandom")==0) fixed = true;
  else assert(0);
  _network->AddSynapse(from,to,exitatory,fixed,factor,ini_min,ini_max);
}

void Parser::ParseLSMSynapse(char * from, char * to, int fromN, int toN, int value, int random, char * fixed_learning){
  bool fixed;
  if(strcmp(fixed_learning,"fixed") == 0) fixed = true;
  else if(strcmp(fixed_learning,"learning") == 0) fixed = false;
  else{
    cout<<fixed_learning<<endl;
    assert(0);
  }
  _network->LSMAddSynapse(from, to, fromN, toN, value, random, fixed);
}

void Parser::ParseSpeech(int cls, char* path){
  Speech * speech = new Speech(cls);
  Channel * channel;

  char linestring[8192];
  char * token;
  //cout<<path<<endl;
  FILE * fp = fopen(path,"r");
  assert(fp != NULL);

  _network->AddSpeech(speech);
  while(fgets(linestring,8191,fp)!=NULL){
    if(strlen(linestring) <= 1) continue;
    channel = speech->AddChannel(10,1);
    token = strtok(linestring," \t\n");
    while(token != NULL){
      channel->AddAnalog(atof(token));
      token = strtok(NULL," \t\n");
    }
  }

  fclose(fp);
}

