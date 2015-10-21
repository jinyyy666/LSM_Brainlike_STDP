#include "def.h"
#include "parser.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "speech.h"
#include "channel.h"
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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
    }else if(strcmp(token[0],"speech")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);

      ParseSpeech(atoi(token[1]),token[2]);
    }else if(strcmp(token[0],"poisson")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);

      ParsePoissonSpeech(atoi(token[1]),token[2]);
    }else if(strcmp(token[0],"MNIST")==0){
      token[1] = strtok(NULL," \t\n");
      assert(token[1] != NULL);
      token[2] = strtok(NULL," \t\n");
      assert(token[2] != NULL);

      ParseMNISTSpeech(atoi(token[1]),token[2]);
    }else if(strcmp(token[0],"end")==0){
      break;
    }else{
      cout<<token[0]<<endl;
      assert(0);
    }

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


//* this function is used when computing the r_s (rank under the 
//* separation case). DONOT use it now, bc you need to have more features to support //* it
void Parser::ParsePoissonSpeech(int cls, char* path){
  Speech * speech = new Speech(cls);
  Channel * channel;

  char linestring[8192];
  char * token;
  char ** tokens = new char*[64];
  //cout<<path<<endl;
  FILE * fp = fopen(path,"r");
  assert(fp != NULL);

  _network->AddSpeech(speech);

  // Read the first line of the poisson spike sample,
  // which contains the information of the input layer.
  if(fgets(linestring,8191,fp) != NULL){
    assert(strlen(linestring) > 1);
    tokens[0] = strtok(linestring," \t\n"); //# of input neurons/channels
    assert(tokens[0] != NULL);
    tokens[1] = strtok(NULL," \t\n"); // # of input connections to reservoir
    assert(tokens[1] != NULL);
#ifdef DEBUG
    cout<<tokens[0]<<'\t'<<tokens[1]<<endl;
#endif
    // Add the input layer information into the speech and 
    // generate the input layer before loading the speech !!! 
    speech->AddInputNumInfo(atoi(tokens[0]));
  
    // Add the connections information about between input layer and reservoir
    // into the speech and generate the connections before load the speech
    speech->AddInputConnectionInfo(atoi(tokens[1]));
  }
  else{
    cout<<"File :"<<path<<" is completely empty!"<<endl;
    exit(-1);
  }

  while(fgets(linestring,8191,fp)!=NULL){
    if(strlen(linestring) <= 1) continue;
    channel = speech->AddChannel(10,1);
    token = strtok(linestring," \t\n");
    while(token != NULL){
      //channel->AddAnalog(atof(token));
      channel->AddSpike(atoi(token));
      token = strtok(NULL," \t\n");
    }
  }

  delete[] tokens;
  fclose(fp);
}

//* this function is used to parse the images like MNIST.
//* for MNIST, there are 100 input neurons and each of them look at
//* a 5*5 region and encode the edge information
void Parser::ParseMNISTSpeech(int cls, char* path){
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
    token = strtok(linestring," \t\n");
    while(token != NULL){
      //cout<<atof(token)<<endl;
      channel = speech->AddChannel(10,1); // Here we read the spike rate for this channel.
      channel->AddAnalog(atof(token));
      token = strtok(NULL," \t\n");
    }
  }

  fclose(fp);
}
