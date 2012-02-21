/*
 * This file is part of the PolyController firmware source code.
 * Copyright (C) 2011 Chris Boot.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <contiki-net.h>
#include <init.h>
#include <stdio.h>
#include <string.h>

#include "apps/network.h"
#if CONFIG_APPS_SYSLOG
#include "apps/syslog.h"
#endif

process_event_t xap_send;
process_event_t xap_recv;
process_event_t xap_status;

PROCESS(xap_tx_process, "xAP_tx");
INIT_PROCESS(xap_tx_process);

static int tx_running = 0, rx_running = 0, xap_running = 0;

static struct etimer tmr_hbeat;

PROCESS_THREAD(xap_tx_process, ev, data) {

	uip_ipaddr_t addr;
	static struct uip_udp_conn *c;
	static char xap_hbeat[]="xap-hbeat\n{\nv=12\nhop=1\nuid=FFBC0100\nclass=xap-hbeat.alive\nsource=bootc.polyctrl.default\ninterval=60\n}\n";
	static char xap_send_msg[512];

	PROCESS_BEGIN();

	xap_send = process_alloc_event();
	xap_status = process_alloc_event();

	uip_ipaddr(&addr, 255,255,255,255);
	c = udp_new(&addr, UIP_HTONS(3639), NULL);

	while (1) {
		PROCESS_WAIT_EVENT();

		if (ev == tcpip_event) {
			if (tx_running) {
				if (uip_newdata()) {
					((char *)uip_appdata)[uip_len]='\0';
					printf("------TX received---\n%s\n--------------------\n",(char *)uip_appdata);
				}
			}
		}
		else if (ev == net_event) {
			if (net_status.configured && !tx_running) {
				tx_running = 1;

				etimer_set(&tmr_hbeat, CLOCK_SECOND * 60);

				tcpip_poll_udp(c);
				PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
				uip_send(xap_hbeat, sizeof(xap_hbeat));

				xap_running = tx_running && rx_running;
				process_post(PROCESS_BROADCAST, xap_status, &xap_running);
				syslog_P(LOG_DAEMON | LOG_INFO, PSTR("Starting"));

				process_poll(&xap_tx_process);
			}
			else if (!net_status.configured && tx_running) {
				tx_running = 0;

				etimer_stop(&tmr_hbeat);

				xap_running = tx_running && rx_running;
				process_post(PROCESS_BROADCAST, xap_status, &xap_running);
				syslog_P(LOG_DAEMON | LOG_INFO, PSTR("Stopped"));
			}
		}
		else if (ev == xap_send) {
			if (tx_running) {
				// need to copy event data due to upcoming tcpip_event
				strncpy(xap_send_msg, data, sizeof(xap_send_msg));
				tcpip_poll_udp(c);
				PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
				uip_send(xap_send_msg, sizeof(xap_send_msg));
			}
		}
		else if (ev == PROCESS_EVENT_TIMER) {
			if (data == &tmr_hbeat && etimer_expired(&tmr_hbeat)) {
				etimer_reset(&tmr_hbeat);
				if (tx_running) {
					tcpip_poll_udp(c);
					PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
					uip_send(xap_hbeat, sizeof(xap_hbeat));
				}
			}
		}
		else if (ev == PROCESS_EVENT_EXIT) {
			tx_running = 0;

			process_exit(&xap_tx_process);
			LOADER_UNLOAD();
		}
	}

	PROCESS_END();
}

PROCESS(xap_rx_process, "xAP_rx");
INIT_PROCESS(xap_rx_process);

PROCESS_THREAD(xap_rx_process, ev, data) {

	uip_ipaddr_t addr;
	static struct uip_udp_conn *c;

	PROCESS_BEGIN();

	xap_recv = process_alloc_event();

	uip_ipaddr(&addr, 255,255,255,255);
	c = udp_new(&addr, UIP_HTONS(0), NULL);
	if(c!= NULL) {
		udp_bind(c, UIP_HTONS(3639));
	}

	while (1) {
		PROCESS_WAIT_EVENT();

		if (ev == tcpip_event) {
			if (rx_running) {
				if (uip_newdata()) {
					((char *)uip_appdata)[uip_len]='\0';
					process_post(PROCESS_BROADCAST, xap_recv, uip_appdata);
					PROCESS_PAUSE();
				}
			}
		}
		else if (ev == net_event) {
			if (net_status.configured && !rx_running) {
				rx_running = 1;

				xap_running = tx_running && rx_running;
				process_post(PROCESS_BROADCAST, xap_status, &xap_running);
				syslog_P(LOG_DAEMON | LOG_INFO, PSTR("Starting"));

				process_poll(&xap_rx_process);
			}
			else if (!net_status.configured && rx_running) {
				rx_running = 0;

				xap_running = tx_running && rx_running;
				process_post(PROCESS_BROADCAST, xap_status, &xap_running);
				syslog_P(LOG_DAEMON | LOG_INFO, PSTR("Stopped"));
			}
		}
		else if (ev == PROCESS_EVENT_EXIT) {
			rx_running = 0;

			process_exit(&xap_rx_process);
			LOADER_UNLOAD();
		}
	}

	PROCESS_END();
}

