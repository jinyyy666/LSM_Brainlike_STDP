#ifndef DEF_H
#define DEF_H

#define SYNLVL 2
#define VREST 0
#define VH    0
#define VTHRE 1000
#define VTHRE_LIQUID 50
#define VSTEP 10
#define VSTEP_LIQUID 1
#define INTVL (VTHRE/VSTEP)
#define MAXINPUT 255
#define MININPUT 0
//#define AUTOTIMER
#define SUPERVISED

//#ifdef SUPERVISED
//#undef AUTOTIMER
//#endif

///////////////* to repeat Fusi's work */
// my def
#define J_C		2
#define J_EX		1
#define X_MAX		1
// neural parameters
//#define LAMBDA		(VTHRE*0.01)
#define THETA_V		(0.7*VTHRE)
// teacher population
#define N_EX		20
#define FREQ_EX	50
// calcium parameters
#define TAU_C		64
//#define TAU_C		100
#define THETA_L_DN	(1*J_C)
#define THETA_L_UP	(1*J_C)
#define THETA_H_DN	(2*J_C)
#define THETA_H_UP	(4*J_C)
// inhibitory population
#define N_INH		1000
#define FRE_INH	50
#define J_INH		(0.035*VTHRE)
// synaptic parameters
#define A			(0.07*X_MAX)
#define B			(0.05*X_MAX)
#define THETA_X	(0.5*X_MAX)
//#define ALPHA		(0.001*X_MAX)
#define BETA		(0.001*X_MAX)
// input layer
#define N_INPUT	2000
#define FRE_STIMULATED	50
#define FRE_UNSTIMULATED	2
#define J_PLUS		J_EX
#define J_MINUS	0

/* FOR LIQUID STATE MACHINE */
// damping factor for long-time potientation:
#define DAMPING 0.01
// learning rate for STDP:
#define LAMBDA 0.0002 
// asymmetry parameter for LTD:
#define ALPHA 1 // for multiplicative : 0.6
// time constant for trace y1, y2: tau_1 < tau_2
#define TAU_Y1_TRACE 8//16
#define TAU_Y2_TRACE 16//32
// time constant for trace x:
#define TAU_X_TRACE  16//64 

// control parameter for additive STDP rule:
#define ADDITIVE_STDP
#define A_POS 64
#define A_NEG 128 // the negative "-" already contains in the code

// control parameter for pair-based pairing rule:
//#define PAIR_BASED


#define LSM_T_M 32
//#define LSM_T_SYNE 2.0
//#define LSM_T_SYNI 4.0
#define LSM_T_SYNE 8
#define LSM_T_SYNI 2
#define LSM_T_FO 4
#define LSM_T_REFRAC 2
#define LSM_V_REST 0
#define LSM_V_RESET 0
#define LSM_V_THRESH 20
#define LSM_CAL_MID 5
//#define LSM_DELTA_POT 0.02
//#define LSM_DELTA_DEP 0.02
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

#define LOST_RATE 0.0
#define DIGITAL
//#define DIGITAL_SYN_ORGINAL
#define LIQUID_SYN_MODIFICATION

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


