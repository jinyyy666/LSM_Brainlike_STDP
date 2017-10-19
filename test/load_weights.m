%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load the dumped weights from the simulator
% Input:
%       filename : the path + name of the file, can be read directly
%       r : the row (the input dim)
%       c : the col (the output dim)
%
% Output:
%       weights : the connectivity weights
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function weights = load_weights(filename, r, c)
    if(~exist(filename, 'file'))
        weights = [];
        return;
    end
    
    weights = zeros(r, c);
    % read the file:
    fid = fopen(filename);
    tline = fgetl(fid);
    while(ischar(tline))
        tokens = strsplit(tline);
        x = extract_index(tokens{2}) + 1;
        y = extract_index(tokens{3}) + 1;
        assert(x > 0 && x <= r && y > 0 && y <= c);
        weights(x, y) = weights(x, y) + str2double(tokens{4});
        tline = fgetl(fid);
    end
    
end