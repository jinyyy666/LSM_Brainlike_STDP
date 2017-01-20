#!/bin/bash

#filename="ro_weights_info.txt"
#filename="o_weights_info_trained_final.txt"
filename="o_weights_info_trained.txt"
rm weights.txt
while read -r line
do
	echo $line | awk '{print $NF}' >> weights.txt
done < "$filename"

#./plot_hist

#octave -q --persist plot_hist.m
octave -q plot_hist.m &
sleep 5
exit

