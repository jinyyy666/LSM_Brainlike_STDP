%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% A matlab script to simulate the network using the vectorized simulation
%
% Use to verify the simulator.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% For the spoken English letter
cls = 26;
num_samples = 26;

% Define the network size and params
% Network: input - reservoir - hidden - output
input_size = 78;
reservoir_size = 135;
hidden_size = 64;
output_size = 26;

tm = 64;
ts = 8;

% load the weights:
input_weights = load_weights('../i_weights_info.txt', input_size, reservoir_size);
reservoir_weights = load_weights('../r_weights_info.txt', reservoir_size, reservoir_size);
hidden_weights = load_weights('../h_weights_info.txt', reservoir_size, hidden_size);
output_weights = load_weights('../o_weights_info.txt', hidden_size, output_size);

% transpose them for further use
input_weights = input_weights';
reservoir_weights = reservoir_weights';
hidden_weights = hidden_weights';
output_weights = output_weights';

disp('Successfully load the weights');

% load the spike information and vmem from the file

[wave_r, wave_h, wave_o, end_time] = ReadVmem;
[input, reservoir, hidden, output] = load_spikes_times(end_time, input_size, reservoir_size, hidden_size, output_size);

disp('Successfully load the spike times and the vmem');



% conduct the simulation
% 1. input - reservoir
disp('Begin simulating the input -- reservoir layer')
vth = 20;
[vmem, reservoir_spikes, n_reservoir_spikes, A_k, h, a_k] = VectorizedSNN(input, input_weights, reservoir_weights, vth, tm, ts);
% match the vmem and output spike times
assert(sum(sum(abs(vmem - wave_r)))/(input_size*reservoir_size) < 1e-3, 'Reservoir vmem does not match!');
assert(sum(sum(abs(reservoir_spikes - reservoir))) < 1e-4, 'Reservoir spike timing does not match!');
disp('Simulation done! The result is correct!')

% 2. reservoir - hidden
disp('Begin simulating the reservoir -- hidden layer')
[vmem, hidden_spikes, n_hidden_spikes, A_k, h, a_k] = VectorizedSNN(reservoir, hidden_weights, [], vth, tm, ts);
assert(sum(sum(abs(vmem - wave_h)))/(reservoir_size*hidden_size) < 1e-3, 'Hidden vmem does not match!');
assert(sum(sum(abs(hidden_spikes - hidden))) < 1e-4, 'Hidden spike timing does not match!');
disp('Simulation done! The result is correct!')


% 3. hidden - output
vth = 5;
disp('Begin simulating the hidden -- output layer')
[vmem, output_spikes, n_output_spikes, A_k, h, a_k] = VectorizedSNN(hidden, output_weights, [], vth, tm, ts);
assert(sum(sum(abs(vmem - wave_o)))/(hidden_size*output_size) < 1e-3, 'Output vmem does not match!');
assert(sum(sum(abs(output_spikes - output))) < 1e-4, 'Output spike timing does not match!');
disp('Simulation done! The result is correct!')
