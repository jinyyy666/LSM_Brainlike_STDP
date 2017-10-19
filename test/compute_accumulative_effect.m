%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% A function to compute the accumulative effect for all the previous
% spikes within the considered window. 
% Use the model in Page 109 of Spiking Neuron Models
%
% The accumulative effect model is only valid for the interval between the 
% last spike i to the current time
%
% Input: t: the current time
%        last_i: the last firing time of the post-synaptic neuron
%        spikes: the timing of the pre-synaptic spikes (binary matrix)
%        weights: the weights of the input synaptic (vector)
%        tm: time constant for the vmem
%        ts: time constant for the first order synaptic response
%        
%   
% Output:
%       auc_effect: the accumulative effect for all input spikes at 
%       the time t given the current time window considered
%       indv_effects: the break-down effects of each input synapse
%
% Note: ignore the effect of those spikes arrives before the 
%       previous spike of the t_i
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [auc_effect, indv_effects] = compute_accumulative_effect(t, last_i, spikes, weights, tm, ts)
    % select the time window:
    indv_effects = zeros(length(weights), 1);
    auc_effect = 0;
        
    % for all input neurons
    for j = 1:length(weights)
        pre_times = find(spikes(j, :) == 1);
        lb = max(1, t - 4*tm);
        ub = t;
        pre_times = pre_times(pre_times >= lb & pre_times < ub);
        
        % accumulate all the windowed spikes
        for k = 1:length(pre_times)
            ss = t - last_i;
            tt = t - pre_times(k);
            factor = exp(-max(tt - ss, 0)/ts)/(1 - ts/tm);
            eff = weights(j)*factor*(exp(-min(ss, tt)/tm) - exp(-min(ss, tt)/ts));
            indv_effects(j) = indv_effects(j) + eff;
            auc_effect = auc_effect + eff;
        end
    end
end
