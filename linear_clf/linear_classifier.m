%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% function
% [c,rates, errors] = linear_classifier(num_input_sample,I,group,Nfold,Cls,entire_data_flag,label)
%    
%   main purpose:
%       
%       This function is to use a simple linear classifier to do the
%       classification. PERFORM A LINEAR CLASSIFICATION
%       FOR THE FIRING COUNTS. 
%
%	arguments:
%
%		num_input_sample: number of total input speeches
%
%		I: input samples, with each row being an input vector
%
%		+---------------+
%		|      x1       |
%		+---------------+
%		|      x2       |
%		+---------------+
%		|      x3       |
%		+---------------+
%		|      x4       |
%		+---------------+
%
%       NFOLD: N-FOLD cross validation
%
%		Cls: digit recognition (10 samples 0-9) letter recognition (26 samples A-Z)
%
%       entire_data_flag: indication of the usage of the entire dataset
%
%       label : the label of the speaker (this is used to avoid speaker-dependency)
%
%	returned values:
%
%		c:  class indicates the group to which each row of input sample has been assigned.
%
%       rates: recognition rate for each iteration.
%
%		errors: error rate for each iteration.
%		
%		group: reference label (0-9 or A-Z) of the speech smaples.
%
% Author: Yingyezhe(Jimmy) Jin
% Email: jyyz@tamu.edu
% Sun Feb  23 10:14:37 CST 2015
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [c,rates, errors] = linear_classifier(num_input_sample,I,group,Nfold,Cls,entire_data_flag,label)

%----------------------------------------
% 1. Setup the parameters
%----------------------------------------
ETA = 0.005; % Learning rate;
TAU_L = 250; % Search-and-converge scheme; the TAU_L is the time constant for converge. 
num_iteration = 500;
randomness = 1; % randomness = 1: random shuffle the speeches before training (speaker-dependency exists)
                % if you assign 1 to randomness. YOU MUST ENSURE
                % num_input_sample/Nfold IS AN INTEGER!!!
                % randomness = 0: speaker-dependency is avoided.

%threshold = 0.2; % To distinguish the correctly classified samples. For right classified sample: output y: 1 - threshold < y < 1

[r_I,c_I] = size(I); % r_I should be # of input samples c_I should be total number of reservoir neurons

interval_fold = num_input_sample/Nfold; % # number of samples in each subset.

if(randomness == 1 && (interval_fold - floor(interval_fold)) > 0)
    disp('if you assign 1 to randomness. YOU MUST ENSURE num_input_sample/Nfold IS AN INTEGER!!!');
    exit(-1);
end

%----------------------------------------
% 2. N-Fold Cross Validation
%----------------------------------------

% 1. Normalize the data : I(i,:) = (I(i,:)-min(I(i,:)))/(max(I(i,:)-min(I(i,:))))

for n = 1: num_input_sample
    I(n,:) = (I(n,:)-min(I(n,:)))/(max(I(n,:)-min(I(n,:))));
end


% 2. Shuffle the data

rates = zeros(1,num_iteration);
errors = zeros(1,num_iteration);
c = zeros(num_input_sample,1);

indices_input = 1:num_input_sample;
i_speech = 1:num_input_sample;
i_speech = i_speech';
if(randomness == 1)
    random_indices = indices_input(randperm(length(indices_input))); % Generate the random permutation for shuffle the input samples
else
    random_indices = indices_input;
end

I_temp = I;
group_temp = group;
i_speech_temp = i_speech;
for i = 1:num_input_sample % randomize the input sample
    I(i,:) = I_temp(random_indices(i),:);
    group(i) = group_temp(random_indices(i));
    i_speech(i) = i_speech_temp(random_indices(i));
end

for m = 1:num_input_sample % Double check whether or not the samples get correctly shuffled.
    if(entire_data_flag == 1)
        continue;
    end
    if(group(m) == Cls)
        assert(mod(i_speech(m),Cls) == 0)
    else
        assert(mod(i_speech(m),Cls) == group(m))
    end
end

% 1. Perform cross-validation

for i = 1:Nfold
    I_temp = I;
    group_temp = group;
    i_speech_temp = i_speech; % index of each input speech
    
    if(entire_data_flag == 1)
        Testing = I_temp(label == i,:);
        group_testing = group_temp(label == i,:);
        i_speech_testing = i_speech_temp(label == i,:);
    
        I_temp(label == i,:) = [];
        group_temp(label == i,:) = [];
        i_speech_temp(label == i,:) = [];
    
        Training = I_temp;
        group_training = group_temp;
        i_speech_training = i_speech_temp;
    else
        Testing = I_temp(interval_fold*(i-1)+1:interval_fold*i,:);
        group_testing = group_temp(interval_fold*(i-1)+1:interval_fold*i,:);
        i_speech_testing = i_speech_temp(interval_fold*(i-1)+1:interval_fold*i,:);
        
        I_temp(interval_fold*(i-1)+1:interval_fold*i,:) = [];
        group_temp(interval_fold*(i-1)+1:interval_fold*i,:) = [];
        i_speech_temp(interval_fold*(i-1)+1:interval_fold*i,:) = [];
        
        Training = I_temp;
        group_training = group_temp;
        i_speech_training = i_speech_temp;        
    end
        
        
    Weight = rand(Cls,c_I+1);

    [num_training_sample,~] = size(Training);
    [num_testing_sample,~] = size(Testing);
    
% 2. Training
    for j = 1: num_iteration
        eta = ETA/(1+ j/TAU_L);
        for k = 1:num_training_sample
            ind_cls = group_training(k); % The index to which the speech belongs
            x_in = [Training(k,:),1]; % Might need some normalization here
            y = Weight*x_in';
            d = -ones(Cls,1);
            d(ind_cls) = 1;
            delta_w = eta*(d-y)*x_in;
            Weight = Weight + delta_w;
        end
    
    
% 3. Testing
        for k = 1:num_testing_sample
            ind_cls = group_testing(k);
            i_place = i_speech_testing(k); % Identify the index with respect to the input speech of the testing speech 
            x_in = [Testing(k,:),1];
            y = Weight*x_in';
            if(y(ind_cls) == max(y))
                rates(j) = rates(j) + 1;
            else
                errors(j) = errors(j) + 1;
            end
            [~,decision] = max(y);
            c(i_place) = decision; % class indicates the group to which each row of input sample has been assigned.
            %d = -ones(Cls,1);
            %d(ind) = 1;
        end
    end
end
        
end
