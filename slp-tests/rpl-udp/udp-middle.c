#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uipbuf.h"
#include "net/ipv6/uip-ds6.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define OFF_TIMER_THRESHOLD 3
#define OFF_TIMER_TIMER SEND_INTERVAL*10000

// static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/

static int messages = 0;
static struct etimer off_timer;

static enum netstack_ip_action ip_input(void) {

  uint8_t proto = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto); // Check protocol is UDP

  if (!etimer_expired(&off_timer)) {
    LOG_INFO("CANCELLED incoming packet proto: %d from ", proto);
    LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_INFO_("\n");
    return NETSTACK_IP_DROP; // Cancel packet if timer still going
  }

  if (proto == UIP_PROTO_UDP) {
    messages++;
    LOG_INFO("Incoming packet proto: %d from ", proto);
    LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_INFO_("\n");

    if (messages > OFF_TIMER_THRESHOLD) {
      etimer_set(&off_timer, OFF_TIMER_TIMER);
      messages = 0;
    }
  }
  return NETSTACK_IP_PROCESS;
}

/*---------------------------------------------------------------------------*/
static enum netstack_ip_action ip_output(const linkaddr_t *localdest) {
  uint8_t proto = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto); // Check protocol is UDP
  if (!etimer_expired(&off_timer)) {
    LOG_INFO("CANCELLED outgoing packet proto: %d from ", proto);
    LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_INFO_("\n");
    return NETSTACK_IP_DROP; // Cancel packet if timer still going
  }
  // uint8_t proto;
  // uint8_t is_me = 0;
  // uipbuf_get_last_header(uip_buf, uip_len, &proto);
  // if (proto == UIP_PROTO_UDP) {
  //   is_me =  uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr);
  //   LOG_INFO("Outgoing packet (%s) proto: %d to ", is_me ? "send" : "fwd ", proto);
  //   LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  //   LOG_INFO_("\n");
  // }
  return NETSTACK_IP_PROCESS;
}
struct netstack_ip_packet_processor packet_processor = {
  .process_input = ip_input,
  .process_output = ip_output
};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  // static struct etimer periodic_timer;
  // static unsigned count;
  // static char str[32];
  // uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  // simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
  //                     UDP_SERVER_PORT, udp_rx_callback);
  netstack_ip_packet_processor_add(&packet_processor);

  // etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  // while(1) {
  //   PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    // if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
    //   /* Send to DAG root */
    //   LOG_INFO("Sending request %u to ", count);
    //   LOG_INFO_6ADDR(&dest_ipaddr);
    //   LOG_INFO_("\n");
    //   snprintf(str, sizeof(str), "hello %d", count);
    //   simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
    //   count++;
    // } else {
    //   LOG_INFO("Not reachable yet\n");
    // }

    /* Add some jitter */
  //   etimer_set(&periodic_timer, SEND_INTERVAL
  //     - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  // }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
