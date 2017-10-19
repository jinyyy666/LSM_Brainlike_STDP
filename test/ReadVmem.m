%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% A matlab script to read the dumped vmem.
% 
% 
% Output:
%       wave_r : the vmem for reservoir neuron
%       wave_o : the vmem for output neuron
%       t_end : the end time of the speech
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [wave_r, wave_h, wave_o, t_end] = ReadVmem
    filename_output = '../Waveform/train/output_0.dat';
    filename_reservoir = '../Waveform/transient/reservoir_0.dat';
    filename_hidden = '../Waveform/train/hidden_0_0.dat';
    
    % read the reservoir vmems:
    msg = sprintf('%s does not exist!', filename_reservoir);
    if(~exist(filename_reservoir, 'file'))
        disp(msg)
        assert(0);
    end
    wave_r = dlmread(filename_reservoir, '\t');

    msg = sprintf('%s does not exist!', filename_hidden);
    if(~exist(filename_hidden))
        disp(msg)
        assert(0);
    end    
    wave_h = dlmread(filename_hidden, '\t');
    
    msg = sprintf('%s does not exist!', filename_output);
    if(~exist(filename_output))
        disp(msg)
        assert(0);
    end    
    wave_o = dlmread(filename_output, '\t');
    
    
    [~, t_end_r] = size(wave_r);
    [~, t_end_h] = size(wave_h);
    [~, t_end_o] = size(wave_o);
    
    assert(t_end_r == t_end_o && t_end_h == t_end_o);
    t_end = t_end_r; 
end
