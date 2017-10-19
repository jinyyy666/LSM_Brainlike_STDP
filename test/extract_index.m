%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Extract the neuron index from the given string
% 
% Input: 
%       name : the name of the neuron. e.g. 'reservoir_10'. We want to 
%              extract the '10' here
%
% Output:
%       index : the extracted index
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function index = extract_index(name)
    indices = strfind(name, '_');
    begin_idx =  indices(end) + 1;
    index = str2double(name(begin_idx : end));
end