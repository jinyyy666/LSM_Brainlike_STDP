#include "def.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <string>
#include "readout.h"

using namespace std;

/************************************************************
  This code is a substitution for the matlab code called :
  "multireadout_new.m".

  It reads the results in outputs/ .

  The results are in the form:
    index of the firing readout neuron   time
  |				 -1	                  -1  |
  |				  9                       23  |
  |				  0			  24  |					
  |				  4			  56  |
  |			          5                       89  |
  |                              -1                       -1  |

  with -1 as the separating indicator of one iteration.
	
  The first column will be loaded into the vector: multidata1
  The second column will be loaded into the vector: multidata2

  Then, results of each iteration will be extracted and put into
  data1 and data2.

  The goal for this code is to figure out the most active readout 
  neuron when a certain speech is fed to the LSM.

  Remember to change the total # of speeches if you change the training set!!!!


  Author:Yingyezhe(Jimmy) Jin
  Date: Feb. 11, 2015

*************************************************************/


Readout::Readout(char * nums_speech){
	num_of_speeches = atoi(nums_speech);
	if(num_of_speeches == 0){
		cout<<"Wrong amount of total speeches!"<<endl;
		exit(-1);
	}
	cout<<"Total number of speeches:"<<num_of_speeches<<endl;
}

Readout::Readout(int nums_speech){
	num_of_speeches = nums_speech;
	cout<<"Total number of speeches:"<<num_of_speeches<<endl;
}

//* flag: 0 --> 26 letters  flag: 1 --> 10 digits
//* flag: 2 --> 10 MNIST    flag: 3 --> 15 traffic signs
void Readout::SetRefer(int flag){ // To set the reference array according the application
	if(flag == 0){
		for (int i = 0; i < 26; ++i)
		{
			if(i < 2) 
				refer.push_back(i);
			else if(i == 2)
				refer.push_back(10);
			else if(i < 12)
				refer.push_back(refer[i-1] + 1);
			else if(i == 12)
				refer.push_back(2);
			else if(i == 13)
				refer.push_back(20);
			else if(i < 19)
				refer.push_back(refer[i-1] + 1);
			else if(i == 19)
				refer.push_back(3);
			else
				refer.push_back(refer[i-1] + 1);
		}
		// for (int i = 0; i < 26; ++i)
		// {
		// 	cout<<refer[i]<<endl;
		// }
	}
		// refer = {0, 1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 2, 20, 21, 22, 23, 24, 25, 3, 4, 5, 6, 7, 8, 9};
	else if(flag == 1 || flag == 2){
		for (int i = 0; i < 10; ++i)
		{
			refer.push_back(i);
		}
		//	refer = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	}
	else if(flag == 3){
	        for (int i = 0; i < 15; ++i)
		{
			refer.push_back(i);
		}
	}
	else{
	        cout<<"Unallowed parameter passed into Readout::SetRefer(int flag)!"
	        <<" this parameter is "<<flag
	        <<" but it should be 0, 1, 2, 3!!"
		<<endl;
	      
	}
 
}
	
void Readout::LoadData(char * filename){ //Load the data from the file of speech results into multidata 
	ifstream f_in(filename);
	if(f_in.is_open() == false){
		cout<<"Cannot open file: "<<filename<<endl;
		exit(-1);
	}
	// string neuron_idx, t;
	
	// while(f_in>>neuron_index>>t){
	// 	int neuron_index = atoi(neuron_idx.c_str());
	// 	int time_f = atoi(t.c_str());
	multidata1.clear();
	multidata2.clear();
	int neuron_index, time_f;
	while(f_in>>neuron_index>>time_f){

		assert(neuron_index >= -1 && neuron_index <= 25);
		multidata1.push_back(neuron_index);
		multidata2.push_back(time_f);
	}
	f_in.close();
} 

vector<int> Readout::FindVal(const vector<int> & v, int val){  //Find a specific value in the vector and return the indices of that value
	vector<int> v_out;
	for (size_t i = 0; i < v.size(); ++i)
	{
		if(v[i] == val){
			//cout<<v[i]<<"\t"<<val<<endl;
			v_out.push_back(i);
		}
	}
	return v_out;	
}
void Readout::Multireadout(){ // The main part of this code.
#if _SPEECH == 1
#if _LETTER == 1
	SetRefer(0);  // "0" here is corresponding to letter recognition "1" here is cooresponding to digit recognition
#elif _DIGIT == 1
	SetRefer(1);
#else
	assert(0);
#endif
#elif _IMAGE == 1
#if _MNIST == 1
	SetRefer(2);
#elif _TRAFFIC == 1
	SetRefer(3);
#else
	assert(0);
#endif
#else
	assert(0);
#endif
	
	int sp = 21;
	char filename[128];
	sprintf(filename,"outputs/spikepattern%d.dat",sp);
	LoadData(filename); // Load the results into multidata1 & multidata2

	vector<int> v_temp = FindVal(multidata1,-1);
	num_iteration = v_temp.size()-1; // Determine the number of iterations

	vector<int> rates(num_iteration,0);  //Vector storing recognition rates
	vector<int> errors(num_iteration,0); 
	vector<int> ties(num_iteration,0);

	// Outer loop: individually load speech file
	for (int i = 0; i < num_of_speeches; ++i)
	{
		char file[128];
		sprintf(file,"outputs/spikepattern%d.dat",i);
		LoadData(file);
		indices = FindVal(multidata1,-1);
		int realclass = refer[i % refer.size()];
		
		static vector<int> data1; //Vector storing the raw results (column 1)
		static vector<int> data2; //Vector storing the raw results (column 2)

		static vector<int> counts1; // Implement two decision points here
		static vector<int> counts2; // vector to count the firing activity

		static vector<int> counts;

		
		// Inner loop: Examine the results for each iteration
		for (int iter = 0; iter < num_iteration; ++iter)
		{
			int down_limit = indices[iter]+1;
			int up_limit = indices[iter+1];
			assert(multidata1[down_limit - 1] == -1 && multidata1[up_limit] == -1);



			// Extract the result for each iteration
			for (int j = down_limit; j < up_limit; ++j)
			{
				data1.push_back(multidata1[j]);
				data2.push_back(multidata2[j]);
			}

			if(data1.empty()){
				ties[iter]++;
				continue;
			}

			for (int j = 0; j < refer.size(); ++j){
				counts1.push_back(0);
				counts2.push_back(0);
				counts.push_back(0);
			}

			// Read the results 
			
			int data_max = *max_element(data2.begin(),data2.end());
			// cout<<data_max<<endl;
			for (int k = 0; k < data1.size(); ++k){
				if(data2[k] <= data_max/2){
					assert(data1[k] <= *max_element(refer.begin(),refer.end()) && data1[k] >= 0);
					++counts1[data1[k]];
				}
				else if(data2[k] > data_max/2){
					assert(data1[k] <= *max_element(refer.begin(),refer.end()) && data1[k] >= 0);
					++counts2[data1[k]];
				}
				else 
					assert(0);
			}


			for (int k = 0; k < refer.size(); ++k)
			{
				counts[k] = counts1[k] + counts2[k];  // Ratio 1:1
			}

			int classified = distance(counts.begin(),max_element(counts.begin(),counts.end()));
			int flag_tie = 0;
			int max_firing_count = *max_element(counts.begin(),counts.end());
			for(int k = 0; k < counts.size(); ++k)
				if(counts[k] == max_firing_count)
					flag_tie++;

			if(realclass == classified && flag_tie == 1)
				rates[iter]++;
			else if(realclass != classified)
				errors[iter]++;
			else if(flag_tie == 2)
				ties[iter]++;
			else{
				// cout<<"More than two neurons have the same firing counts!"<<endl;
				ties[iter]++;
			}

			counts.clear();  // Clear the vectors
			counts1.clear();
			counts2.clear();
			data1.clear();
			data2.clear();
		}
	}
	assert(rates.size() == num_iteration);
	assert(errors.size() == num_iteration);
	assert(ties.size() == num_iteration);

	ofstream f_rate("outputs/rates.txt");
	ofstream f_error("outputs/errors.txt");

	if(f_rate.is_open() == false){
		cout<<"Fail to open file: outputs/rates.txt to write!"<<endl;
		exit(-1);
	}

	if(f_error.is_open() == false){
		cout<<"Fail to open file: outputs/errors.txt to write!"<<endl;
		exit(-1);
	}

	for (int i = 0; i < num_iteration; ++i)
	{
		if(rates[i] + errors[i] + ties[i] != num_of_speeches){
			cout<<rates[i] + errors[i] + ties[i]<<endl;
			exit(-1);
		}
		else{
			f_rate<<rates[i]<<endl;
			f_error<<errors[i]<<endl;
		}
	}
	int max_rate = *max_element(rates.begin(),rates.end());
	cout<<"Best performance = "<<max_rate<<"/"<<num_of_speeches<<" = "<<double(max_rate)*100/double(num_of_speeches)<<'%'<<endl;
	if(num_iteration >= 50)
		cout<<"Performance at 50th iteration = "<<double(rates[49])*100/double(num_of_speeches)<<'%'<<endl;
	if(num_iteration >= 100)
		cout<<"Performance at 100th iteration = "<<double(rates[99])*100/double(num_of_speeches)<<'%'<<endl;
	if(num_iteration >= 200)
		cout<<"Performance at 200th iteration = "<<double(rates[199])*100/double(num_of_speeches)<<'%'<<endl;
	if(num_of_speeches >= 300)
		cout<<"Performance at 300th iteration = "<<double(rates[299])*100/double(num_of_speeches)<<'%'<<endl;		

	f_rate.close();
	f_error.close();

}

