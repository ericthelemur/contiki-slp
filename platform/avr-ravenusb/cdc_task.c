/* This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file cdc_task.c **********************************************************
 *
 * \brief
 *      Manages the CDC-ACM Virtual Serial Port Dataclass for the USB Device
 *
 * \addtogroup usbstick
 *
 * \author 
 *        Colin O'Flynn <coflynn@newae.com>
 *
 ******************************************************************************/
/* Copyright (c) 2008  ATMEL Corporation
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
/**
 \ingroup usbstick
 \defgroup cdctask CDC Task
 @{
 */

//_____  I N C L U D E S ___________________________________________________


#include "contiki.h"
#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_specific_request.h"
#include "cdc_task.h"
#include "serial/uart_usb_lib.h"
#include "rndis/rndis_protocol.h"
#include "rndis/rndis_task.h"
#include "sicslow_ethernet.h"
#if RF230BB
#include "rf230bb.h"
#else
#include "radio.h"
#endif
#if USB_CONF_RS232
#include "dev/rs232.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include "dev/watchdog.h"
#include "rng.h"

#include "bootloader.h"

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>

#if JACKDAW_CONF_USE_SETTINGS
#include "settings.h"
#endif

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define PRINTF printf
#define PRINTF_P printf_P

//_____ M A C R O S ________________________________________________________


#define bzero(ptr,size)	memset(ptr,0,size)

//_____ D E F I N I T I O N S ______________________________________________


#define IAD_TIMEOUT_DETACH 300
#define IAD_TIMEOUT_ATTACH 600

//_____ D E C L A R A T I O N S ____________________________________________


void menu_print(void);
void menu_process(char c);

extern char usb_busy;

//! Counter for USB Serial port
extern U8    tx_counter;

//! Timers for LEDs
uint8_t led3_timer;


//! previous configuration
static uint8_t previous_uart_usb_control_line_state = 0;


static uint8_t timer = 0;
static struct etimer et;


PROCESS(cdc_process, "CDC serial process");

/**
 * \brief Communication Data Class (CDC) Process
 *
 *   This is the link between USB and the "good stuff". In this routine data
 *   is received and processed by CDC-ACM Class
 */
PROCESS_THREAD(cdc_process, ev, data_proc)
{
	PROCESS_BEGIN();

	while(1) {
	    // turn off LED's if necessary
		if (led3_timer) led3_timer--;
		else			Led3_off();
		

 		if(Is_device_enumerated()) {
			// If the configuration is different than the last time we checked...
			if((uart_usb_get_control_line_state()&1)!=previous_uart_usb_control_line_state) {
				previous_uart_usb_control_line_state = uart_usb_get_control_line_state()&1;
				static FILE* previous_stdout;
				
				if(previous_uart_usb_control_line_state&1) {
					previous_stdout = stdout;
					uart_usb_init();
					uart_usb_set_stdout();
					menu_print();
				} else {
					stdout = previous_stdout;
				}
			}

			//Flush buffer if timeout
	        if(timer >= 4 && tx_counter!=0 ){
	            timer = 0;
	            uart_usb_flush();
	        } else {
				timer++;
			}

			while (uart_usb_test_hit()){
  		  	   menu_process(uart_usb_getchar());   // See what they want
            }


		}//if (Is_device_enumerated())


		if (USB_CONFIG_HAS_DEBUG_PORT(usb_configuration_nb)) {
			etimer_set(&et, CLOCK_SECOND/80);
		} else {
			etimer_set(&et, CLOCK_SECOND);
		}

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));	
		
	} // while(1)

	PROCESS_END();
}

/**
 \brief Print debug menu
 */
void menu_print(void)
{
		PRINTF_P(PSTR("\n\r********** Jackdaw Menu ******************\n\r"));
		PRINTF_P(PSTR("*                                        *\n\r"));
		PRINTF_P(PSTR("* Main Menu:                             *\n\r"));
		PRINTF_P(PSTR("*  h,?             Print this menu       *\n\r"));
		PRINTF_P(PSTR("*  m               Print current mode    *\n\r"));
		PRINTF_P(PSTR("*  s               Set to sniffer mode   *\n\r"));
		PRINTF_P(PSTR("*  n               Set to network mode   *\n\r"));
		PRINTF_P(PSTR("*  c               Set RF channel        *\n\r"));
		PRINTF_P(PSTR("*  6               Toggle 6lowpan        *\n\r"));
		PRINTF_P(PSTR("*  r               Toggle raw mode       *\n\r"));
		PRINTF_P(PSTR("*  e               Energy Scan           *\n\r"));
#if USB_CONF_STORAGE
		PRINTF_P(PSTR("*  u               Switch to mass-storage*\n\r"));
#endif
		if(bootloader_is_present())
			PRINTF_P(PSTR("*  D               Switch to DFU mode    *\n\r"));
		PRINTF_P(PSTR("*  R               Reset (via WDT)       *\n\r"));
		PRINTF_P(PSTR("*                                        *\n\r"));
		PRINTF_P(PSTR("* Make selection at any time by pressing *\n\r"));
		PRINTF_P(PSTR("* your choice on keyboard.               *\n\r"));
		PRINTF_P(PSTR("******************************************\n\r"));
		PRINTF_P(PSTR("[Built "__DATE__"]\n\r"));
}


/**
 \brief Process incomming char on debug port
 */
void menu_process(char c)
{

	static enum menustate_enum            /* Defines an enumeration type    */
	{
		normal,
		channel
	} menustate = normal;
	
	static char channel_string[3];
	static uint8_t channel_string_i = 0;
	
	int tempchannel;

	if (menustate == channel) {

		switch(c) {
			case '\r':
			case '\n':		
								
				if (channel_string_i)  {
					channel_string[channel_string_i] = 0;
					tempchannel = atoi(channel_string);

#if RF230BB
					if ((tempchannel < 11) || (tempchannel > 26))  {
						PRINTF_P(PSTR("\n\rInvalid input\n\r"));
					} else {
						rf230_set_channel(tempchannel);
#else
					if(radio_set_operating_channel(tempchannel)!=RADIO_SUCCESS) {
						PRINTF_P(PSTR("\n\rInvalid input\n\r"));
					} else {
#endif
#if JACKDAW_CONF_USE_SETTINGS
						if(settings_set_uint8(SETTINGS_KEY_CHANNEL, tempchannel)!=SETTINGS_STATUS_OK) {
							PRINTF_P(PSTR("\n\rChannel changed to %d, but unable to store in EEPROM!\n\r"),tempchannel);
						} else
#else						
						AVR_ENTER_CRITICAL_REGION();
						eeprom_write_byte((uint8_t *) 9, tempchannel);   //Write channel
						eeprom_write_byte((uint8_t *)10, ~tempchannel); //Bit inverse as check
						AVR_LEAVE_CRITICAL_REGION();
#endif
						PRINTF_P(PSTR("\n\rChannel changed to %d and stored in EEPROM.\n\r"),tempchannel);
					}
				} else {
					PRINTF_P(PSTR("\n\rChannel unchanged.\n\r"));
				}

				menustate = normal;
				break;
		
			case '\b':
			
				if (channel_string_i) {
					channel_string_i--;
					PRINTF_P(PSTR("\b \b"));
				}
				break;
					
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (channel_string_i > 1) {
					// This time the user has gone too far.
					// Beep at them.
					putc('\a', stdout);
					//uart_usb_putchar('\a');
					break;
				}
				putc(c, stdout);
				//uart_usb_putchar(c);
				
				channel_string[channel_string_i] = c;
				channel_string_i++;
				break;

			default:
				break;
		}


	} else {

		uint8_t i;
		switch(c) {
			case '\r':
			case '\n':
				break;

			case 'h':
			case '?':
				menu_print();
				break;
			case '-':
				PRINTF_P(PSTR("Bringing interface down\n\r"));
				usb_eth_set_active(0);
				break;
			case '=':
			case '+':
				PRINTF_P(PSTR("Bringing interface up\n\r"));
				usb_eth_set_active(1);
				break;

			case 't':
				// I added this to test my "strong" random number generator.
				PRINTF_P(PSTR("RNG Output: "));
				{
					uint8_t value = rng_get_uint8();
					uint8_t i;
					for(i=0;i<8;i++) {
						uart_usb_putchar(((value>>(7-i))&1)?'1':'0');
					}
					PRINTF_P(PSTR("\n\r"));
					uart_usb_flush();
					watchdog_periodic();
				}
				break;

			case 's':
				PRINTF_P(PSTR("Jackdaw now in sniffer mode\n\r"));
				usbstick_mode.sendToRf = 0;
				usbstick_mode.translate = 0;
				break;

			case 'n':
				PRINTF_P(PSTR("Jackdaw now in network mode\n\r"));
				usbstick_mode.sendToRf = 1;
				usbstick_mode.translate = 1;
				break;

			case '6':
				if (usbstick_mode.sicslowpan) {
					PRINTF_P(PSTR("Jackdaw does not perform 6lowpan translation\n\r"));
					usbstick_mode.sicslowpan = 0;
				} else {
					PRINTF_P(PSTR("Jackdaw now performs 6lowpan translations\n\r"));
					usbstick_mode.sicslowpan = 1;
				}	
				
				break;

			case 'r':
				if (usbstick_mode.raw) {
					PRINTF_P(PSTR("Jackdaw does not capture raw frames\n\r"));
					usbstick_mode.raw = 0;
				} else {
					PRINTF_P(PSTR("Jackdaw now captures raw frames\n\r"));
					usbstick_mode.raw = 1;
				}	
				break;

			case 'c':
#if RF230BB
				PRINTF_P(PSTR("Select 802.15.4 Channel in range 11-26 [%d]: "), rf230_get_channel());
#else
				PRINTF_P(PSTR("Select 802.15.4 Channel in range 11-26 [%d]: "), radio_get_operating_channel());
#endif
				menustate = channel;
				channel_string_i = 0;
				break;
				
				
			
			case 'm':
				PRINTF_P(PSTR("Currently Jackdaw:\n\r  * Will "));
				if (usbstick_mode.sendToRf == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("send data over RF\n\r  * Will "));
				if (usbstick_mode.translate == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("change link-local addresses inside IP messages\n\r  * Will "));
				if (usbstick_mode.sicslowpan == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("decompress 6lowpan headers\n\r  * Will "));
				if (usbstick_mode.raw == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("Output raw 802.15.4 frames\n\r"));
				PRINTF_P(PSTR("  * USB Ethernet MAC: %02x:%02x:%02x:%02x:%02x:%02x\n"),
					((uint8_t *)&usb_ethernet_addr)[0],
					((uint8_t *)&usb_ethernet_addr)[1],
					((uint8_t *)&usb_ethernet_addr)[2],
					((uint8_t *)&usb_ethernet_addr)[3],
					((uint8_t *)&usb_ethernet_addr)[4],
					((uint8_t *)&usb_ethernet_addr)[5]
				);
				extern uint64_t macLongAddr;
				PRINTF_P(PSTR("  * 802.15.4 EUI-64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"),
					((uint8_t *)&macLongAddr)[0],
					((uint8_t *)&macLongAddr)[1],
					((uint8_t *)&macLongAddr)[2],
					((uint8_t *)&macLongAddr)[3],
					((uint8_t *)&macLongAddr)[4],
					((uint8_t *)&macLongAddr)[5],
					((uint8_t *)&macLongAddr)[6],
					((uint8_t *)&macLongAddr)[7]
				);
#if RF230BB
				PRINTF_P(PSTR("  * Operates on channel %d\n\r"), rf230_get_channel());
				PRINTF_P(PSTR("  * TX Power Level: 0x%02X\n\r"), rf230_get_txpower());
				PRINTF_P(PSTR("  * Current RSSI: %ddB\n\r"), -91+3*(rf230_rssi()-1));
				PRINTF_P(PSTR("  * Last RSSI: %ddB\n\r"), -91+3*(rf230_last_rssi-1));
#else // RF230BB
				PRINTF_P(PSTR("  * Operates on channel %d\n\r"), radio_get_operating_channel());
				PRINTF_P(PSTR("  * TX Power Level: 0x%02X\n\r"), radio_get_tx_power_level());
				{
					PRINTF_P(PSTR("  * Current RSSI: "));
					int8_t rssi = 0;
					if(radio_get_rssi_value(&rssi)==RADIO_SUCCESS)
						PRINTF_P(PSTR("%ddB\n\r"), -91+3*(rssi-1));
					else
						PRINTF_P(PSTR("Unknown\n\r"));
				}
				
#endif // !RF230BB
				PRINTF_P(PSTR("  * Configuration: %d\n\r"), usb_configuration_nb);
				PRINTF_P(PSTR("  * usb_eth_is_active: %d\n\r"), usb_eth_is_active);
				break;

			case 'e':
				PRINTF_P(PSTR("Energy Scan:\n"));
				uart_usb_flush();
				{
					uint8_t i;
					uint16_t j;
#if RF230BB
					uint8_t previous_channel = rf230_get_channel();
#else // RF230BB
					uint8_t previous_channel = radio_get_operating_channel();
#endif
					int8_t RSSI, maxRSSI[17];
					uint16_t accRSSI[17];
					
					bzero((void*)accRSSI,sizeof(accRSSI));
					bzero((void*)maxRSSI,sizeof(maxRSSI));
					
					for(j=0;j<(1<<12);j++) {
						for(i=11;i<=26;i++) {
#if RF230BB
							rf230_set_channel(i);
#else // RF230BB
							radio_set_operating_channel(i);
#endif
							_delay_us(3*10);
#if RF230BB
							RSSI = rf230_rssi();
#else // RF230BB
							radio_get_rssi_value(&RSSI);
#endif
							maxRSSI[i-11]=Max(maxRSSI[i-11],RSSI);
							accRSSI[i-11]+=RSSI;
						}
						if(j&(1<<7)) {
							Led3_on();
							if(!(j&((1<<7)-1))) {
								PRINTF_P(PSTR("."));
								uart_usb_flush();
							}
						}
						else
							Led3_off();
						watchdog_periodic();
					}
#if RF230BB
					rf230_set_channel(previous_channel);
#else // RF230BB
					radio_set_operating_channel(previous_channel);
#endif
					PRINTF_P(PSTR("\n"));
					for(i=11;i<=26;i++) {
						uint8_t activity=Min(maxRSSI[i-11],accRSSI[i-11]/(1<<7));
						PRINTF_P(PSTR(" %d: %02ddB "),i, -91+3*(maxRSSI[i-11]-1));
						for(;activity--;maxRSSI[i-11]--) {
							PRINTF_P(PSTR("#"));
						}
						for(;maxRSSI[i-11]>0;maxRSSI[i-11]--) {
							PRINTF_P(PSTR(":"));
						}
						PRINTF_P(PSTR("\n"));
						uart_usb_flush();
					}

				}
				PRINTF_P(PSTR("Done.\n"));
				uart_usb_flush();
				
				break;


			case 'D':
				{
					PRINTF_P(PSTR("Entering DFU Mode...\n\r"));
					uart_usb_flush();
					Leds_on();
					for(i = 0; i < 10; i++)_delay_ms(100);
					Leds_off();
					Jump_To_Bootloader();
				}
				break;
			case 'R':
				{
					PRINTF_P(PSTR("Resetting...\n\r"));
					uart_usb_flush();
					Leds_on();
					for(i = 0; i < 10; i++)_delay_ms(100);
					Usb_detach();
					for(i = 0; i < 20; i++)_delay_ms(100);
					watchdog_reboot();
				}
				break;
				
#if USB_CONF_STORAGE
			case 'u':

				//Mass storage mode
				usb_mode = mass_storage;

				//No more serial port
				stdout = NULL;

				//RNDIS is over
				rndis_state = 	rndis_uninitialized;
				Leds_off();

				//Deatch USB
				Usb_detach();

				//Wait a few seconds
				for(i = 0; i < 50; i++)
					_delay_ms(100);

				//Attach USB
				Usb_attach();


				break;
#endif

			default:
				PRINTF_P(PSTR("%c is not a valid option! h for menu\n\r"), c);
				break;
		}


	}

	return;

}


/**
    @brief This will enable the VCP_TRX_END LED for a period
*/
void vcptx_end_led(void)
{
    Led3_on();
    led3_timer = 5;
}
/** @}  */
