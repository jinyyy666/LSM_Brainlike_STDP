#!/usr/bin/octave -q --persist

# use the octave to plot the weight distribution

a = load('weights.txt');

%a(find(a <0)) = [];
sum(a(find(a<0)))
sum(a(find(a>0)))

hist(a, 20);

%h = hist(a,20)

pause(100)
