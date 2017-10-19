%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load the spike timings the simulation dumped file
% 
% Input : 
%       end_time : the end time of the speeches       
%       input_size : the number of input neurons
%       reservoir_size : the number of reservoir neurons
%       hidden_size : the number of hidden neurons
%       output_size : the number of output neurons 
% Output:
%       input : the input spikes
%       reservoir : the reservoir spikes
%       hidden : the hidden spikes (might be empty if there is no hidden layer)
%       output : the output spikes
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [input, reservoir, hidden, output] = load_spikes_times(end_time, input_size, reservoir_size, hidden_size, output_size)
% read the data
    ind_speech = 0;
    filename_input = sprintf('../spikes/Input_Response/input_spikes_%d.dat',ind_speech);
    filename_reservoir = sprintf('../spikes/Reservoir_Response/reservoir_spikes_%d.dat',ind_speech);
    filename_hidden = sprintf('../spikes/Hidden_Response_Supv/train/hidden_0_spikes_%d.dat', ind_speech);
    filename_readout = sprintf('../spikes/Readout_Response_Supv/train/readout_spikes_%d.dat', ind_speech);

    input = load_spikes(filename_input, input_size, end_time);
    reservoir = load_spikes(filename_reservoir, reservoir_size, end_time);
    hidden = load_spikes(filename_hidden, hidden_size, end_time);
    output = load_spikes(filename_readout, output_size, end_time);
end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load the spike timings from the dump file
% The file format:
%   -1           -1
%   [neuron_id] [spike_time]
%   ...         ...
%   -1          -1    
%
% The line "-1  -1" is used to separate spike times obtained under
% different iteration
%
% Input : 
%       filename : the file name
%       num_neurons : the number of neurons
% Output:
%       spikes : the matrix of the spike times (num_neurons x t)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function spikes = load_spikes(filename, num_neurons, end_time)
    if(~exist(filename, 'file'))
        spikes = [];
        return;
    end
    data = load(filename);
    indices = find(data(:, 1) == -1);
    
    spikes = zeros(num_neurons, end_time);
    
    end_index = indices(end)-1; % read the hidden layer response
    
    if(end_index ~= 0)
        begin_index = indices(length(indices)-1)+1;
        data = data(begin_index : end_index,:);
        [row, ~] = size(data);

        for j = 1:row 
            spikes(data(j,1)+1, data(j,2)) = 1;
        end
    end
end
