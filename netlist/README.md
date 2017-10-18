Some syntax for the netlist:


1) neuronGroup: Use this to create a group of neurons
neurongroup [name] [num_neurons] [excitatory/inhibitory] [vmem]

2) column: Use this to create the reservoir
column [name] [x] [y] [z]

x, y, z are three dimensions of the reservoir

3) lsmsynapse: Specify the connections
lsmsynapse [input_neuron_group] [output_neuron_group] [input_connect] [output_connect] [weight_max] [mode] [fixed/learning]

input_connect, output_connect:

Select $input_connect neurons to randomly connect to $output_connect neurons.

Notice that the input_connect can only be 1 or -1

If input_connect == -1 and output_connect == -1, that means the fully connectivity

mode = 0, 1, 2

0: initialize all weights as zero
1: initialize all weights as a fixed number (weight_max)
2: initialize the weights randomly in (-1, 1)

fixed/learning : tell you whether the synapse can be learned or not.


P.S.

The [input_neuron_group] can be the same as the [output_neuron_group]. In doing this, you are creating the laterial inihibition connection with weight as [weight_max]
