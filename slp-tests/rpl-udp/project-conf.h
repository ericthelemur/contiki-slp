
// #define LOG_CONF_LEVEL_RPL LOG_LEVEL_DBG
// #define LOG_CONF_LEVEL_MAC LOG_LEVEL_DBG
#define RPL_CONF_GROUNDED 1

// Generates logging messages that Cooja can use to display graph.
#define LOG_CONF_WITH_ANNOTATE 1

// Disable policies
#define RADOFF_SLP_NONE 0
#define RADOFF_SLP_RAND 1
#define RADOFF_SLP_CUMUL_RAND 2
#define RADOFF_SLP_COUNTER 3
#define RADOFF_SLP_RANDINIT_COUNTER 4


// TO CHANGE set properties here
#define SEND_INTERVAL (unsigned long) (CLOCK_SECOND / 0.1)
#define RADIO_OFF_SLP RADOFF_SLP_RAND
#define RADIO_OFF_PROB 1.0

// Time radio is disabled for
#define OFF_TIMER_TIME SEND_INTERVAL

// For RAND, prob of disabling for each message
#define OFF_TIMER_PROB 0.1

// For CUML_RAND, increase in prob of disabling for each message, initially 0
#define OFF_TIMER_BASE_PROB 0.03
#define OFF_TIMER_MULTIPLIER 1.5

// For COUNTER and RANDINIT_COUNTER, count of messages to disable at
#define OFF_TIMER_THRESHOLD 20


// Don't Change

// Storing mode is unnecessary, as only upward routing
#define RPL_CONF_MOP RPL_MOP_NON_STORING

// Disable 
#define RPL_CONF_WITH_PROBING 0
#define RPL_CONF_WITH_DAO_ACK 0
#define RPL_CONF_DELAY_BEFORE_LEAVING 5 * CLOCK_SECOND

// #define TIME_THRESHOLD (1 * CLOCK_SECOND)

// #define RPL_CONF_DEFAULT_ROUTE_INFINITE_LIFETIME 0
// #define RPL_CONF_DEFAULT_LIFETIME_UNIT
// Enabled for OF0 Test
// #define RPL_CONF_OF_OCP RPL_OCP_OF0

// #define RPL_CONF_MIN_HOPRANKINC 16