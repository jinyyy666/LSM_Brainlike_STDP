#ifndef DEF_H
#define DEF_H

#define _IMAGE    1
#define _MNIST    0
#define _TRAFFIC   0
#define _CITYSCAPE 0
#define _NMNIST 1

#define _SPEECH    0
#define _LETTER    0
#define _DIGIT    0

// this control variable is defined to direct compute the \delta_t 
// for stdp rule update. hardware is doing that 
#define _LUT_HARDWARE_APPROACH

// this control variable is to enable the simple STDP update rule
// NEED TO ENABLE _LUT_HARDWARE_APPROACH and DIGITAL first!
//#define _SIMPLE_STDP

// this control variable enable the pure random connection in the reservoir
//#define PURE_RANDOM_RES_CONNECTION

// this is the prob. for the random connections
#define CONNECTION_PROB 0.2

// this control variable enable you to remove the zero reservoir weights
// after reservoir training
#define _RM_ZERO_RES_WEIGHT

#define MAX_GRAYSCALE 512
#define DURATION_TRAIN 500

// calcium parameters
#define TAU_C		64

/* FOR LIQUID STATE MACHINE */
// the control parameter to enable STDP training, you only to enable this one
// before enable the following two variables!!
//#define STDP_TRAINING

// the control parameter to enable training the reservoir:
//#define STDP_TRAINING_RESERVOIR

// the control parameter to enable training the input to reservoir syns
//#define STDP_TRAINING_INPUT

// the control parameter to enable training the readout synapses in unsupervised way
//#define STDP_TRAINING_READOUT

// inject additional current to readout during the readout unsupvervised stage
#define _READOUT_HELPER_CURRENT
// tune the teacher signal strength during the readout training to "activate v_mem"
#define TS_STRENGTH_P 0
#define TS_STRENGTH_N 0 
// intended teacher signal freq, one out of x time point to fire
#define TS_FREQ 1

/***********************************
// teacher signal strength used in STDP supervised training:
***********************************/
#define TS_STRENGTH_P_SUPV 1
#define TS_STRENGTH_N_SUPV 0
// give the negative class some firings so that the weight can be somehow depressed!
// intended teacher signal freq, one out of x time point to fire
// but note that the calcium constraint is inposed during firing!
// For the depressive STDP, this term should be zero.
// For the reward STDP, should be -0.1 for continuous case and 0 for digital case
#define TS_FREQ_SUPV 1

// default teacher signal strength used in readout training:
#define DEFAULT_TS_STRENGTH_P 1
#define DEFAULT_TS_STRENGTH_N 0.75
#define DEFAULT_TS_FREQ 1
// the control variable to enable adaptive power gating:
//#define ADAPTIVE_POWER_GATING

// the in/out degree criterior for gating one node:
#define INDEG_LIMIT  1
#define OUTDEG_LIMIT 1

/*******************************************
// the new rule(SRM based/reward modulated)
*******************************************/
#define _REWARD_MODULATE
#define VMEM_DAMPING_FACTOR 1

#define TAU_REWARD_POS_E 4
#define TAU_REWARD_NEG_E 8
#define TAU_REWARD_POS_I 8
#define TAU_REWARD_NEG_I 4

#define A_REWARD_POS 0.2
#define A_REWARD_NEG 0.2

#define D_A_REWARD_POS 144
#define D_A_REWARD_NEG 144


/*****************************************
 * Settings for error-backprop rule
 ****************************************/
#define BP_BETA_REG 0.0 // params for the regularization
#define BP_LAMBDA_REG 0.08
#define BP_DELTA_POT 0.00001 // params for the learning of pos/neg class
#define BP_DELTA_DEP 0.00001
#define BP_ITER_SEARCH_CONV 50 // search and converge learning rate 

//#define BP_OBJ_RATIO 
#define BP_OBJ_COUNT // current best
//#define BP_OBJ_SOFTMAX  // remember to make DELTA_POT == DELTA_DEP for this case

#define _BOOSTING_METHOD

/*****************************************
 * some available modifications 
 * to the original Yong's rule
 *****************************************/
//#define _UPDATE_AT_LAST // update the learning weight at the end of each speech


/*****************************************
 * Settings for the unsupervised STDP
 ****************************************/
// damping factor for long-time potientation (under triplet case):
// this value should be small (around 0.01) for multiplicative update!
#define DAMPING 1
// shifting bits implementing the damping factor:
#define DAMPING_BIT 2

// learning rate for STDP (1/512):
#define LAMBDA 0.00195  

// shifting bits implementing learning rate for STDP:
#define LAMBDA_BIT 12 // 12 for multi; 10 for additive

// parameter for LTP under nearest-neighbor pairing rule:
#define NN_BIT_P 0
#define NN_BIT_N 0

// asymmetry parameter for LTD:
#define ALPHA 1 // for multiplicative : 0.6
// shifting bits implementing the asymmetry parameter for LTD:
#define ALPHA_BIT -3 // -3 for multi; 2 for additive

// time constant for trace y1, y2: tau_1 < tau_2
// assume that the time constants for E-E and E-I are different
// note that now only EXCIT SYN is being trained by STDP
#define TAU_Y1_TRACE_E 8
#define TAU_Y2_TRACE_E 16
// time constant for trace x:
#define TAU_X_TRACE_E  4 //8 for multiplicative

#define TAU_Y1_TRACE_I 4
#define TAU_Y2_TRACE_I 16
// time constant for trace x:
#define TAU_X_TRACE_I  2 //8 for multiplicative

// control parameter for additive STDP rule:
#define ADDITIVE_STDP
// assume that A is different for E-E and E-I
#define D_A_POS_E 96 // those two A's are defined for digital
#define D_A_NEG_E 32 // the negative "-" already contains in the code
#define D_A_POS_I 64
#define D_A_NEG_I 32

#define A_POS_E 0.008 // define for the continuous case
#define A_NEG_E 0.001
#define A_POS_I 0.008
#define A_NEG_I 0.001

/************************************
// for supervised STDP:
************************************/
#define TAU_Y1_TRACE_E_S 8
#define TAU_Y2_TRACE_E_S 16
// time constant for trace x:
#define TAU_X_TRACE_E_S  4 //8 for multiplicative

#define TAU_Y1_TRACE_I_S 4
#define TAU_Y2_TRACE_I_S 16
// time constant for trace x:
#define TAU_X_TRACE_I_S  2 //8 for multiplicative

#define D_A_POS_E_S 96 // those two A's are defined for digital
#define D_A_NEG_E_S 32 // the negative "-" already contains in the code
#define D_A_POS_I_S 64
#define D_A_NEG_I_S 32

#define A_POS_E_S 0.008 // define for the continuous case
#define A_NEG_E_S 0.001
#define A_POS_I_S 0.008
#define A_NEG_I_S 0.001

//#define _REGULARIZATION_STDP_SUPV
#define GAMMA_REG 0.001 // regularization parameter
#define WEIGHT_OMEGA 250 // the targetted weight sums

// control parameter for pair-based pairing rule:
#define PAIR_BASED 1
//#define NEAREST_NEIGHBOR 1

// control parameter for stochastic stdp, silimar to the abstract learning rule:
#define STOCHASTIC_STDP
#define STOCHASTIC_STDP_SUPV

// contro parameter to study synaptic activity
//#define _PRINT_SYN_ACT

#define LSM_T_M 32
#define LSM_T_M_C 64 // time constant for continuous case v_mem
#define LSM_T_SYNE 8
#define LSM_T_SYNI 2
#define LSM_T_FO 8
#define LSM_T_REFRAC 2
#define LSM_V_REST 0
#define LSM_V_RESET 0
#define LSM_V_THRESH 15
#define LSM_CAL_MID 3
#define LSM_CAL_MARGIN 0
#define LSM_DELTA_POT 0.006
#define LSM_DELTA_DEP 0.006
#define ITER_SEARCH_CONV 25.0
#define CLS 10
#define NUM_THREADS 1
#define NUM_ITERS 40

#define LSM_TBIT_SYNE 1
#define LSM_TBIT_SYNI 3

#define NUM_DEC_DIGIT_CALCIUM 6 

#define NUM_DEC_DIGIT 10
#define NBT_STD_SYN 4
#define NUM_BIT_SYN 10

// change all to zero to achieve 6-bit vmem
// need to change NUM_DEC_DIGIT_R and NUM_BIT_SYN_R together 
// NUM_DEC_DIGIT_R and NUM_BIT_SYN_R defines the # of bits of r syn
#define NUM_DEC_DIGIT_R 4
#define NBT_STD_SYN_R 4
#define NUM_BIT_SYN_R 4

#define NUM_BIT_READOUT_MEM 6
#define NUM_BIT_RESERVOIR_MEM 6

#define NUM_DEC_DIGIT_RESERVOIR_MEM 0  // Number of bits represent 'one' for V_mem in the Liquid
#define NUM_DEC_DIGIT_READOUT_MEM 0   // Number of bits represent 'one' for V_mem in the readout

/*****************************
// Var-based Sparsification
*****************************/	

//#define _VARBASED_SPARSE		
#define _TOP_PERCENT 0.1 // % of neuron being sparsified

/*****************************
// Correlation-base Sparsify
// Done in the network level
*****************************/

//#define _CORBASED_SPARSE
#define _LEVEL_PERCENT 0.1 // level % of average variance will be considered correlated

#define LOST_RATE 0.0
//#define DIGITAL
//#define DIGITAL_SYN_ORGINAL 1
#define LIQUID_SYN_MODIFICATION 1

//#define SYN_ORDER_2 0 
#define SYN_ORDER_1 1 
#define SYN_ORDER_0 0 

//#define CV
//#define NFOLD 5

//* Control variable to enable the old way of readout by writing outputs/*.dat
//#define _WRITE_STAT  

//* Control variable to enable print pre-synaptic events, remember to enable _DEBUG_NEURON in neuron.C
//#define _RES_FIRING_CHR

//* Control variable to enable print max/min for 4 state variables of synaptic response, remember to enable
//  _DEBUG_NEURON in neuron.C 
//#define _RES_EPEN_CHR

//* visualize the spikes response and the v_mem
//#define _DUMP_RESPONSE

//#define _DUMP_CALCIUM

//#define _TEST_

enum channelmode_t {INPUTCHANNEL,RESERVOIRCHANNEL, OUTPUTCHANNEL}; // for allocate speech channels
enum neuronmode_t {DEACTIVATED,READCHANNEL,WRITECHANNEL,STDP,READCHANNELSTDP,READCHANNELBP,NORMALSTDP,NORMALBP,NORMAL}; // for implement network stat.
enum networkmode_t {TRAINRESERVOIR,TRAININPUT,TRAINREADOUT,TRANSIENTSTATE,READOUT,READOUTSUPV,READOUTBP,VOID}; // different network mode

enum synapsetype_t {RESERVOIR_SYN, INPUT_SYN, READOUT_SYN, INVALID}; // different synaptic type


//NMNIST functions

#define TB_PER_CLASS 100
#define INPUT_NUM 1156

#define LOAD_RESPONSE
//#define SAVE_RESPONSE

//#define IP
//#define DYNAMIC_IP
#define IP_LEARNING_RATE 2
#define IP_K 0.5
#define IP_N 100
//#define PART_RESERVOIR_IP
#define DT_Portion 70

//#define TRAIN_SAMPLE

//#define QUICK_RESPONSE

//the readout functions are added below
//
#define TEACHER_SIGNAL

#endif


