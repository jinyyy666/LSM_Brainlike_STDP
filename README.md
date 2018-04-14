# LSM_Brainlike_STDP

This is a repo for my research code. 

This is basically a simulator for spiking neuron networks with STDP rule implemented. 

The code can be run in parallel because I implement p-thread here.

I have optimized several parts of the code, which makes it much faster than its initial version.

Hope you enjoy it!

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


Notes for dumping the CPU outputs for GPU verification.

Currently, I need the CPU code to verify 2 N-MNIST test cases, 1 MNIST test case, and 1 spoken English letter test case.

>1) For MNIST, do:
>    a) Modify the def.h for reading the MNIST.
>    b) Enable the OPT_ADAM.
>    c) Make sure the output weights are input->hidden_0 and hidden_0 to output.
>    d) Enable the line:623-624 and disable line:625 in network.C

>2) For N-MNIST, do:
>    a) Modify the def.h for reading N-MNIST.
>    b) Modify the main.C for reading the corresponding netlist (feedforward case or reservoir case)
>    c) Make sure the output weights are recorded correctly (input->h->o or input->r->h->o).
>    d) Rememeber to change the status of neurons (network.C:1047-1050), e.g. if reservoir is used, need to activated reservoir while disable the input. 
>    e) If the effect ratio is  NOT used, remember to delete the _lsm_effect_ratio in Synapse::LSMErrorBack.
>    f) Remember to change the optimizer type to be consistent with GPU.

>3) For Spoken English letter, do:
>    a) Modify the def.h for reading the spoken English letter.
>    b) Remember to change the NUM_CLS to 26.
>    c) Remember to change the optimizer type to be consistent with GPU.

The order of performing the test:

MNIST -> NMNIST(feedforward) -> NMNIST(reservoir) -> Spoken English Letter

