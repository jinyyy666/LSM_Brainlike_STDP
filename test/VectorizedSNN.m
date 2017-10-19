%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Vectorized simulation of the SNN
% 
% Input: 
%       spikes : input spikes, binary matrix (n x end_time)
%       weights : the weight matrix (m x n), the row represents the output (m), 
%       col represents input (n)
%       self_weights : weight matrix (m x m) models the recurrent connectivity
%       vth : the threshold voltage
%       tm : the time constants of vmem
%       ts : the time constant of the synaptic response
%
% Output: 
%       vmem : matrix of vmem (m x end_time)
%       o_spikes : output spike matrix (m x end_time)
%       num_o_spikes : vector of output spikes (m x 1)
%       A_k : the variable to keep track of accumulative effect (m x 1)
%       h : the weighted spikes sum (m x 1)
%       a_k : the break down effect of each synapse (m x n)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [vmem, o_spikes, num_o_spikes, A_k, h, a_k] = VectorizedSNN(spikes, weights, self_weights, vth, tm, ts)
    [m, n] = size(weights);
    [r, end_time] = size(spikes);
    assert(r == n, 'The row dim %d of the spike does not match with the input: %d!', r, n);

    const_t_ref = 2; % the pre-defined refractory period
    
    % allocate the output matrix:
    vmem = zeros(m, end_time);
    o_spikes = zeros(m, end_time);
    num_o_spikes = zeros(m, 1);
    A_k = zeros(m, 1);
    h = zeros(m, 1);
    a_k = zeros(m, n);
    
    % local variables
    v = zeros(m, 1); % v_mem at each time step
    ep = zeros(m, 1); % track synaptic response
    t_ref = zeros(m, 1); % refractory period
    o_spike = zeros(m, 1); % output spikes at previous time step
    
    % begin the computation for each step:
    for i = 1:end_time
        % 1. Leakage:
        v = v - v/tm;
        ep = ep - ep/ts;
        
        if(i == 1)  
            continue
        end
        
        % 2. Receive the spike inputs
        if(isempty(self_weights))
            response = weights * spikes(:, i - 1);
        else
            response = weights * spikes(:, i - 1) + self_weights * o_spike;
        end
        
        % 3. Add up the response to ep
        ep = ep + response;
        
        % 4. Update the vmem accordingly (first order response)
        v = v + ep/ts;
        
        % Reset the v if the t_ref > 0
        v(t_ref > 0) = 0;
        
        % Decrease the t_ref
        t_ref(t_ref > 0) = t_ref(t_ref > 0) - 1;
        
        vmem(:, i) = v; 
        
        % 5. See fire or not
        o_spike = zeros(m, 1);
        o_spike(v > vth) = 1;
        o_spikes(:, i) = o_spike;
        
        % 6. Set the t_ref for the fired neurons
        t_ref(v > vth) = const_t_ref;
    end
    vmem(:, end) = 0; % in the simulator, the last time point is not actually simulated!
end