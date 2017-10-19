%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% function
% [c, rates,errors] = linear_classifier_LSM(num_input_sample,CLS,NFOLD) 
%    
%   main purpose:
%       
%       This function is to 1) generate the input data (matrix) from the
%       Reservoir_Response/ and 2)use a simple linear classifier to do the
%       classification. In terms of the Reservoir_Response, I will first
%       translate each response into a matrix where row is the time, column 
%       is the index of reservoir neuron, and each element (can either 
%       be 0 or 1). And then sum up the matrix is the row order to get the 
%       firing count of each reservoir neuron. PERFORM A LINEAR CLASSIFICATION
%       FOR THE FIRING COUNTS. 
%
%	arguments:
%
%		num_input_sample: number of total input speeches
%
%		CLS: digit recognition (10 samples 0-9) letter recognition (26 samples A-Z)
%
%       NFOLD: N-FOLD cross validation
%
%	returned values:
%		
%		c:  class indicates the group to which each row of input sample has been assigned.
%
%		rates: recognition rate for each iteration.
%
%		errors: error rate for each iteration.
%
% Author: Yingyezhe(Jimmy) Jin
% Email: jyyz@tamu.edu
% Sun Feb  22 22:14:37 CST 2015
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [c, rates,errors] = linear_classifier_LSM(num_input_sample,CLS,NFOLD) 

%----------------------------------------
% 1. Setup the parameters
%----------------------------------------

if (nargin<2)
    Cls = 10;
    Nfold = 5;
else
    Cls = CLS;
    Nfold = NFOLD;
end

%----------------------------------------
% 2. Generate the training/testing data
%----------------------------------------
I = [];
group = [];
entire_data_flag = 0; % This is the switch to tell the program whether or not the entire digit words (0-9) is used.
                      % entire_data_flag = 1 (the entire dataset is used)
                      % entire_data_flag = 0 (original 500 data/260 data is used)

if(entire_data_flag == 1)  % entire_data_set is used here. 16 speakers in total and 8-fold cross validation
    Nfold = 8;
    refer = load('netlist/refer.txt');
    assert(length(refer) == num_input_sample);
    label = load('netlist/label_speaker.txt');
    assert(length(label) == num_input_sample);
else                      % entire_data_set is not used here. And the speaker_label can be easily known from flooring (its speech index/interval)
    if(Cls == 10)
      interval = num_input_sample/Nfold;
    elseif(Cls == 26)  % The case for letter recognition should be user-defined!
      interval = 260;  % User-define here!
    else
      assert(0);
    end

    label = [];
    for i = 0:num_input_sample-1
        label = [label;floor(i/interval)+1];
    end
end


for ind_speech = 1: num_input_sample   
        if(entire_data_flag == 0)
            ind = mod(ind_speech-1, Cls) + 1; % ind_speech : index of the *.dat
        elseif(entire_data_flag == 1)
            ind = refer(ind_speech) + 1;
        else
            assert(0);
        end
        
        group = [group;ind];
        
        % read the data
        filename_reservoir = sprintf('Reservoir_Response/reservoir_spikes_%d.dat',ind_speech -1);
        data_r = load(filename_reservoir);
        indices_r = find(data_r == -1);
        num_r = length(indices_r) - 1;
    
        maximum = 1500;
    
        reservoir = zeros(num_r,maximum); % the matrix recording all the firing info
    
        for j = 1: num_r % read the response in the reservoir
            data = data_r(indices_r(j)+1:indices_r(j+1)-1);
            if(data(1) == -99)
                continue;
            end
            for k = 1:length(data)
                reservoir(j,data(k)) = 1;
            end
        end
        I = [I;sum(reservoir')];     
end

I_new = []; % for visualize the data
for i = 1:length(group)
    I_new = [I_new;I(group == i,:)];
end


%----------------------------------------
% 3. Linear classification of the dataset
%----------------------------------------
%[c,err,post,logl,str] = classify(I,I,group,'linear');
[c,rates, errors] = linear_classifier(num_input_sample,I,group,Nfold,Cls,entire_data_flag,label);

end
