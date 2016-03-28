#!/bin/bash

filename="r_weights_info.txt"
rm weights.txt
while read -r line
do
	echo $line | awk '{print $NF}' >> weights.txt
done < "$filename"

./plot_hist

#octave -q --persist plot_hist.m
#octave -q plot_hist.m &
#sleep 5
#exit

