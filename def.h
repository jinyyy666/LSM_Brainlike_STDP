#ifndef DEF_H
#define DEF_H

#define _IMAGE    0
#define _MNIST    0
#define _TRAFFIC   0
#define _SPEECH    1
#define _LETTER    0
#define _DIGIT    1

// this control variable is defined to direct compute the \delta_t 
// for stdp rule update. this is what the hardware is going to do.
#define _HARDWARE_CODE 

// this control variable is to enable the simple STDP update rule
// NEED TO ENABLE _HARDWARE_CODE first!
//#define _SIMPLE_STDP

// this control variable is defined to test my thought of separate
// the reservoir into several parts and stimulate each part with 
// speeches that are similar to each other under STDP training:
//#define _SEPARATE_RESERVOIR

// this control variable enable the pure random connection in the reservoir
//#define PURE_RANDOM_RES_CONNECTION
// this is the prob. for the random connections
#define CONNECTION_PROB 0.2

#define MAX_GRAYSCALE 512
#define DURATION_TRAIN 500

// calcium parameters
#define TAU_C		64

/* FOR LIQUID STATE MACHINE */
// the control parameter to enable training the reservoir:
#define STDP_TRAINING_RESERVOIR

// the control variable to enable adaptive power gating:
//#define ADAPTIVE_POWER_GATING

// the in/out degree criterior for gating one node:
#define INDEG_LIMIT  1
#define OUTDEG_LIMIT  1


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

#define A_POS_E 0.02 // define for the continuous case
#define A_NEG_E 0.005
#define A_POS_I 0.02
#define A_NEG_I 0.005

// control parameter for pair-based pairing rule:
#define PAIR_BASED 1
//#define NEAREST_NEIGHBOR 1

// control parameter for stochastic stdp, silimar to the abstract learning rule:
#define STOCHASTIC_STDP


// contro parameter to study synaptic activity
//#define _PRINT_SYN_ACT

#define LSM_T_M 32
#define LSM_T_SYNE 8
#define LSM_T_SYNI 2
#define LSM_T_FO 4
#define LSM_T_REFRAC 2
#define LSM_V_REST 0
#define LSM_V_RESET 0
#define LSM_V_THRESH 20
#define LSM_CAL_MID 5
#define LSM_CAL_MARGIN 0
#define LSM_DELTA_POT 0.006
#define LSM_DELTA_DEP 0.006
#define ITER_SEARCH_CONV 50
//#define CLS 26
#define CLS 26
#define NUM_THREADS 1

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
				   

#define LOST_RATE 0.0
#define DIGITAL
//#define DIGITAL_SYN_ORGINAL 1
#define LIQUID_SYN_MODIFICATION 1

#define SYN_ORDER_2 1 
#define SYN_ORDER_1 0 
#define SYN_ORDER_0 0 

#define CV
#define NFOLD 5

enum channelmode_t {INPUTCHANNEL,RESERVOIRCHANNEL,READOUTCHANNEL};
enum neuronmode_t {DEACTIVATED,READCHANNEL,WRITECHANNEL,STDP,NORMAL};
enum networkmode_t {TRAINRESERVOIR,TRANSIENTSTATE,READOUT,VOID};

enum synapsetype_t {RESERVOIR_SYN, INPUT_SYN, READOUT_SYN, INVALID};


#endif


