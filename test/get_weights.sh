#!/bin/bash

filename="../o_weights_info.txt"
rm weights_hidden_output.txt
while read -r line
do
	echo $line | awk '{print $NF}' >> weights_hidden_output.txt
done < "$filename"


