#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

using namespace std;

int main(){
  char linestring[8192];
  char * string;
  char ** token = new char*[64];
  FILE * fp = fopen("netlist_digit_backup.txt","r");
  assert(fp != NULL);
  int cls;
  int i;

  assert(fp != 0);  //Read info from training set first
  while(fgets(linestring, 8191, fp) != NULL){
  	//cout<<linestring<<endl;
    if(strlen(linestring) <= 1) continue;
    token[0] = strtok(linestring," \t\n");
    assert(token[0] != NULL);
    
    if(strcmp(token[0],"speech") != 0) continue;
    token[1] = strtok(NULL," \t\n");
    assert(token[1] != NULL);
    token[2] = strtok(NULL," \t\n");
    assert(token[2] != NULL);
    /*token[3] = strtok(NULL," \t\n");
    assert(token[3] != NULL);
    token[4] = strtok(NULL," \t\n");
    assert(token[4] != NULL);
    token[5] = strtok(NULL," \t\n");
    assert(token[5] != NULL);
    token[6] = strtok(NULL," \t\n");
    assert(token[6] != NULL);
    token[7] = strtok(NULL," \t\n");
    assert(token[7] != NULL);
    token[8] = strtok(NULL," \t\n");
    assert(token[8] != NULL);*/

    string = token[1];
    //for(i = 0; i<strlen(token[8]);++i) if(*(string+i) == 'c') break;

    //cls = atoi(string+i+1);
    cls = atoi(string);
    //cout<<string<<endl;
    if(cls > 9) continue; // Ignore other words 

    //cout<<"speech\t"<<cls<<"\t"<<"AuditoryToolbox/data_new/ti_alpha/train/"<<string<<endl;
    cout<<cls<<"\t";

  }
    //
  cout<<endl;
  fclose(fp);
  return 0;
}

