#ifndef DEF_H
#define DEF_H

// calcium parameters
#define TAU_C		64

/* FOR LIQUID STATE MACHINE */
// the control parameter to enable training the reservoir:
#define STDP_TRAINING_RESERVOIR

// damping factor for long-time potientation (under triplet case):
// this value should be small (around 0.01) for multiplicative update!
#define DAMPING 1
// shifting bits implementing the damping factor:
#define DAMPING_BIT 2

// learning rate for STDP:
#define LAMBDA 0.0002
// shifting bits implementing learning rate for STDP:
#define LAMBDA_BIT 12 // 12 for multi; 10 for additive

// parameter for LTP under nearest-neighbor pairing rule:
#define NN_BIT_P -2
#define NN_BIT_N -3


// asymmetry parameter for LTD:
#define ALPHA 1 // for multiplicative : 0.6
// shifting bits implementing the asymmetry parameter for LTD:
#define ALPHA_BIT -3 // -3 for multi; 2 for additive

// time constant for trace y1, y2: tau_1 < tau_2
#define TAU_Y1_TRACE 4//16// 8 //162
#define TAU_Y2_TRACE 8//32//32
// time constant for trace x:
#define TAU_X_TRACE  2//8//64  ; 8 for multiplicative

// control parameter for additive STDP rule:
#define ADDITIVE_STDP
#define A_POS 64
#define A_NEG 32 // the negative "-" already contains in the code

// control parameter for pair-based pairing rule:
#define PAIR_BASED 0
#define NEAREST_NEIGHBOR 1

// control parameter for stochastic stdp, silimar to the abstract learning rule:
#define STOCHASTIC_STDP


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
#define LSM_DELTA_POT 0.008
#define LSM_DELTA_DEP 0.008
//#define CLS 26
#define CLS 26
#define NUM_THREADS 1

#define LSM_TBIT_SYNE 1
#define LSM_TBIT_SYNI 3
#define NUM_DEC_DIGIT 10
#define NBT_STD_SYN 4
#define NUM_BIT_SYN 10
#define NUM_BIT_RESERVOIR 10

#define LOST_RATE 0.0
//#define DIGITAL
#define DIGITAL_SYN_ORGINAL 0
#define LIQUID_SYN_MODIFICATION 1

#define SYN_ORDER_2 1 
#define SYN_ORDER_1 0 
#define SYN_ORDER_0 0 

#define CV
#define NFOLD 5

enum channelmode_t {INPUTCHANNEL,RESERVOIRCHANNEL};
enum neuronmode_t {DEACTIVATED,READCHANNEL,WRITECHANNEL,STDP,NORMAL};
enum networkmode_t {TRAINRESERVOIR,TRANSIENTSTATE,READOUT,VOID};

enum synapsetype_t {RESERVOIR_SYN, INPUT_SYN, READOUT_SYN, INVALID};

#endif


