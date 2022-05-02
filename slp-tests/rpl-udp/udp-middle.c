#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uipbuf.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/node-id.h"
#include "stdlib.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

// static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/

#if RADIO_OFF_SLP

#pragma message "Radio disable SLP is ENABLED as " XSTR(RADIO_OFF_SLP)
#if RADIO_OFF_SLP == RADOFF_SLP_COUNTER
static int count = 0;
#elif RADIO_OFF_SLP == RADOFF_SLP_RANDINIT_COUNTER
static int count = -1;
#elif RADIO_OFF_SLP == RADOFF_SLP_CUMUL_RAND
static double prob = OFF_TIMER_BASE_PROB;
#endif

static struct ctimer off_timer;
static struct ctimer to_off_timer;

// Switch on and off functions, just used for the ctimers now, as using ip processors to "disable"
static void switch_on() {
  LOG_INFO("Radio back on\n");
  // NETSTACK_RADIO.on();
}

static void switch_off() {
  LOG_INFO("Radio off for %d time\n", (int) OFF_TIMER_TIME);
  ctimer_set(&off_timer, OFF_TIMER_TIME, switch_on, NULL);
  // NETSTACK_RADIO.off();
}

// Incoming packet processor, discards if timers going, and checks disable policy
static enum netstack_ip_action ip_input(void) {
  uint8_t proto = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto); // Get protocol (is UDP)

  if (!ctimer_expired(&off_timer) || (!ctimer_expired(&to_off_timer) && proto != UIP_PROTO_UDP)) {
     // Cancel packet if timer still going
    LOG_INFO("CANCELLED incoming packet proto: %d from ", proto);
    LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_INFO_("\n");
    return NETSTACK_IP_DROP;
  }

  if (proto == UIP_PROTO_UDP) {

#if RADIO_OFF_SLP == RADOFF_SLP_RAND
    double r = (double) random_rand() / (double) RAND_MAX;  // Random float in [0, 1]
    LOG_INFO("Random %lf threshold %lf\n", r, OFF_TIMER_PROB);
    if (r < OFF_TIMER_PROB) {
#elif RADIO_OFF_SLP == RADOFF_SLP_CUMUL_RAND
    double r = (double) random_rand() / (double) RAND_MAX;  // Random float in [0, 1]
    prob *= OFF_TIMER_MULTIPLIER;
    LOG_INFO("Random %lf threshold %lf\n", r, prob);
    if (r < prob) {
      prob = OFF_TIMER_BASE_PROB;
#elif RADIO_OFF_SLP == RADOFF_SLP_COUNTER
    count++;
    LOG_INFO("Count %d threshold %d\n", count, OFF_TIMER_THRESHOLD);
    if (count >= OFF_TIMER_THRESHOLD) {
      count = 0;
#elif RADIO_OFF_SLP == RADOFF_SLP_RANDINIT_COUNTER
    if (count == -1) count = random_rand() % OFF_TIMER_THRESHOLD;  // Initialize to random
    count++;
    LOG_INFO("Count %d threshold %d\n", count, OFF_TIMER_THRESHOLD);
    if (count >= OFF_TIMER_THRESHOLD) {
      count = 0;
#else
    #error "ERROR: Unknown RADIO_OFF_SLP"
#endif
      // Disable policy passed, disabling
      LOG_INFO("Reached threshold");
      NETSTACK_ROUTING.leave_network();

      ctimer_set(&to_off_timer, SEND_INTERVAL/2, switch_off, NULL);
    }
  }
  return NETSTACK_IP_PROCESS;
}

// Outgoing packet processor, cancel if still going
static enum netstack_ip_action ip_output(const linkaddr_t *localdest) {
  uint8_t proto = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto); // Check protocol is UDP
  if (!ctimer_expired(&off_timer)) {
    LOG_INFO("CANCELLED outgoing packet proto: %d to ", proto);
    LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
    LOG_INFO_("\n");
    return NETSTACK_IP_DROP; // Cancel packet if timer still going
  }
  return NETSTACK_IP_PROCESS;
}

struct netstack_ip_packet_processor packet_processor = {
  .process_input = ip_input,
  .process_output = ip_output
};

#endif


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  PROCESS_BEGIN();
  
#if RADIO_OFF_SLP
  netstack_ip_packet_processor_add(&packet_processor);  // Register IP listeners
#endif
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
