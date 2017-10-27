#include "def.h"
#include "parser.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "speech.h"
#include "channel.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <set>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>

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
            token[3] = strtok(NULL," \t\n");

            double v_mem = token[3] == NULL ? LSM_V_THRESH : atof(token[3]);
            ParseNeuron(token[1],token[2], v_mem);
        }else if(strcmp(token[0],"neurongroup")==0){
            // neurongroup name number
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);
            token[3] = strtok(NULL," \t\n");
            assert(token[3] != NULL);
            // read the v_mem from the netlist if applicable
            token[4] = strtok(NULL," \t\n");
            double v_mem = token[4] == NULL ? LSM_V_THRESH : atof(token[4]);
            ParseNeuronGroup(token[1], atoi(token[2]), token[3], v_mem);
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
        }else if(strcmp(token[0],"labels") == 0){
            // add the labels into reservoir for separating reservoir
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);
            int num_labels = atoi(token[2]); // total number of labels is specified by 3rd 
            assert(num_labels != 0);

            set<int> s;
            // parsing the labels:
            int i=0;
            for(; i < num_labels; ++i){
                token[i+3] = strtok(NULL," \t\n");
                assert(token[i+3] != NULL);
                assert(isdigit(token[i+3][0]) && atoi(token[i+3]) >= 0 && atoi(token[i+3]) < CLS);
                s.insert(atoi(token[i+3]));
            }
            assert(_network->CheckExistence(token[1]) == true);
            _network->LSMAddLabelToReservoirs(token[1], s);

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
        }else if(strcmp(token[0],"NMNIST")==0){
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);
#ifdef LOAD_RESPONSE
            continue; // do not read the NMNIST speech if we will direct load from the response
#endif
#ifdef QUICK_RESPONSE
            QuickLoad(atoi(token[1]),token[2]);
#else
            ParseNMNIST(atoi(token[1]),token[2]);
#endif
        }else if(strcmp(token[0],"end")==0){
            break;
        }else{
            cout<<token[0]<<endl;
            assert(0);
        }

    }
    fclose(fp);
#ifdef LOAD_RESPONSE
    cout<<"Reservoir Response Load Start"<<endl;
    _network->LoadResponse("reservoir");
    cout<<"Reservoir Response Load Complete"<<endl;
#endif
}

void Parser::ParseNeuron(char * name, char * e_i, double v_mem){
    assert(_network->CheckExistence(name) == false);
    assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));

    bool excitatory = strcmp(e_i,"excitatory") == 0 ? true : false;
    _network->AddNeuron(name, excitatory, v_mem);
}

void Parser::ParseNeuronGroup(char * name, int num, char * e_i, double v_mem){
    assert(_network->CheckExistence(name) == false);
    assert(num > 1);
    assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
    bool excitatory;
    if(strcmp(e_i,"excitatory") == 0) excitatory = true;
    else excitatory = false;
    _network->AddNeuronGroup(name, num, excitatory, v_mem);
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
    int index = 0;
    _network->AddSpeech(speech);
    while(fgets(linestring,8191,fp)!=NULL){
        if(strlen(linestring) <= 1) continue;
        channel = speech->AddChannel(10, 1, index++);
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
    int index = 0;
    while(fgets(linestring,8191,fp)!=NULL){
        if(strlen(linestring) <= 1) continue;
        channel = speech->AddChannel(10, 1, index++);
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

    int index = 0;
    _network->AddSpeech(speech);
    while(fgets(linestring,8191,fp)!=NULL){
        if(strlen(linestring) <= 1) continue;
        token = strtok(linestring," \t\n,");
        while(token != NULL){
            //cout<<atof(token)<<endl;
            channel = speech->AddChannel(10, 1, index++); // Here we read the spike rate for this channel.
            channel->AddAnalog(atof(token));
            token = strtok(NULL," \t\n,");
        }
    }

    fclose(fp);
}
// read path for N-NMIST

//* this function is used to parse N-MNIST testbench.
//* for N-MNIST, there are 1156 input neurons
//* They are converted from the original spike based testbench at
//* http://www.garrickorchard.com/datasets
void Parser::ParseNMNIST(int cls,char* path){
    int input_num;
    input_num=_network->SearchForNeuronGroup("input")->Size();
    int index;
    if(path==NULL){
        cout<<"directory path is NULL"<<endl;
        assert(0);
    }

    //check if path is a valid dir
    struct stat s;
    lstat(path,&s);
    if(!S_ISDIR(s.st_mode)){
        cout<<"path is not a valid directory"<<endl;
        assert(0);
    }

    struct dirent* filename;
    DIR *dir;
    dir=opendir(path);
    if(dir==NULL){
        cout<<"Cannot open dir"<<endl;
        assert(0);
    }
    int file_read=0;
    while((filename=readdir(dir))!=NULL){
        const size_t len = strlen(path)+strlen(filename->d_name);
        char *file_path=new char[len+1];
        strcpy(file_path,path);
        if(filename->d_name[0]<'0'||filename->d_name[0]>'9'){
            continue;
        }
        index=atoi(filename->d_name);
        strcat(file_path,filename->d_name);
        file_read++;
        Speech * speech = new Speech(cls);
        Channel * channel;
        char linestring[8192];
        char * token;
        FILE * fp = fopen(file_path,"r");
        speech->SetFileIndex(index);
        assert(fp != NULL);
        _network->AddSpeech(speech);
        int index=0;
        int line_count=0;
        while(line_count<input_num){
            line_count++;
            channel = speech->AddChannel(10, 1, index++);
            if(fgets(linestring,8191,fp)!=NULL&&linestring[0]!='\n'){	
                token=strtok(linestring," \t\n,");
                while(token!=NULL){
                    channel->AddSpike(atoi(token));
                    token=strtok(NULL," \t\n,");
                }
            }
        }
        fclose(fp);
        delete token;
        if(file_read>=TB_PER_CLASS){
            break;
        }
    }
    closedir(dir);
}


void Parser::QuickLoad(int cls,const char* path){
    int input_num;
    input_num=_network->SearchForNeuronGroup("input")->Size();
    struct dirent* filename;
    DIR * dir;
    int tid=_network->GetTid();
    bool test;
    int index;
    int num=0;
    int sample_limit;
    const char *keyword="Test";
    if(tid/10!=cls)
        return;

    if(strstr(path,keyword)){
        sample_limit=100;
    }
    else{
        sample_limit=600;
    }

    dir=opendir(path);
    if(dir==NULL){
        cout<<"Cannot open dir "<<endl;
        assert(0);
    }
    while((filename=readdir(dir))!=NULL){
        if(tid%10!=9&&num>(tid-cls*10+1)*sample_limit){
            break;
        }
        if(filename->d_name[0]<'0'||filename->d_name[0]>'9'){
            continue;
        }
        else if(num<(tid-cls*10)*sample_limit){
            num++;
            continue;
        }
        else{
            num++;
            Speech * speech;
            speech = new Speech(cls);
            Channel * channel;
            char linestring[8192];
            char * token;
            const size_t len = strlen(path)+strlen(filename->d_name);
            char *file_path=new char[len+1];
            strcpy(file_path,path);
            index=atoi(filename->d_name);
            speech->SetFileIndex(index);
            strcat(file_path,filename->d_name);
            FILE * fp = fopen(file_path,"r");
            assert(fp != NULL);
            _network->AddSpeech(speech);
            int index=0;
            int line_count=0;
            while(line_count<input_num){
                line_count++;
                channel = speech->AddChannel(10, 1, index++);
                if(fgets(linestring,8191,fp)!=NULL&&linestring[0]!='\n'){
                    token=strtok(linestring," \t\n,");
                    while(token!=NULL){
                        channel->AddSpike(atoi(token));
                        token=strtok(NULL," \t\n,");
                    }
                }
            }
            fclose(fp);
            delete token;
        }
    }
    closedir(dir);
}



