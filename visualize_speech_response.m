function visualize_speech_response
% Visualize 10 speech samples and their response in the reservoir

cls = 26;

num_samples = 18;

%refer = [0 1 2 3 4 5 6 7 8 9]; % how samples organize in netlist_new.txt
refer = [0, 1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 2, 20, 21, 22, 23, 24, 25, 3, 4, 5, 6, 7, 8, 9]; % for letter case

for times = 1:10 % generate 10 figures with 26 utterances in each figure

    figure
    for class = 1:cls % 26 subfigure in each figure (a - z)
        index = find(refer == (class -1));
        ind_speech = (times - 1)*cls + index - 1; % ind_speech : index of the *.dat
        
        % read the data
        filename_input = sprintf('Input_Response/input_spikes_%d.dat',ind_speech);
        filename_reservoir = sprintf('Reservoir_Response/reservoir_spikes_%d.dat',ind_speech);
        data_i = load(filename_input);
        data_r = load(filename_reservoir);
        indices_i = find(data_i == -1);
        indices_r = find(data_r == -1);
        num_i = length(indices_i) - 1;
        num_r = length(indices_r) - 1;
    
        maximum = 250;
    
        input = zeros(num_i, maximum);
        reservoir = zeros(num_r,maximum);
    
        for j = 1:num_i % read the speech samples
            data = data_i(indices_i(j)+1:indices_i(j+1)-1);
            if(data(1) == -99)
                continue;
            end
            for k = 1:length(data)
                input(j,data(k)) = 1;
            end
        end
    
        for j = 1: num_r % read the response in the reservoir
            data = data_r(indices_r(j)+1:indices_r(j+1)-1);
            if(data(1) == -99)
                continue;
            end
            for k = 1:length(data)
                reservoir(j,data(k)) = 1;
            end
        end
        
        subplot(5,6,class);
        imagesc(input); %  Change here to switch reservoir to input
        
    end
    
    
end
