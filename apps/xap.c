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

#include "apps/network.h"
#if CONFIG_APPS_SYSLOG
#include "apps/syslog.h"
#endif

PROCESS(xap_process, "xAP");
INIT_PROCESS(xap_process);

int running = 0;

static struct etimer tmr_hbeat;

PROCESS_THREAD(xap_process, ev, data) {

	static struct uip_udp_conn *c;
	static char xap_hbeat[]="xap-hbeat\n{\nv=12\nhop=1\nuid=FFBC0100\nclass=xap-hbeat.alive\nsource=bootc.polycontroller.default\ninterval=60\n}\n";

	PROCESS_BEGIN();

	c = udp_broadcast_new(UIP_HTONS(3639), NULL);

	while (1) {
		PROCESS_WAIT_EVENT();

		if (ev == tcpip_event) {
			if (running) {
				if (uip_newdata()) {
					((char *)uip_appdata)[uip_len]='\0';
					printf("--------------------\n%s\n--------------------\n",(char *)uip_appdata);
				}
			}
		}
		else if (ev == net_event) {
			if (net_status.configured && !running) {
				running = 1;

				etimer_set(&tmr_hbeat, CLOCK_SECOND * 60);

				//process_post(PROCESS_BROADCAST, xap_event, &xap_status);
				syslog_P(LOG_DAEMON | LOG_INFO, PSTR("Starting"));

				process_poll(&xap_process);
			}
			else if (!net_status.configured && running) {
				running = 0;

				etimer_stop(&tmr_hbeat);

				//process_post(PROCESS_BROADCAST, xap_event, &xap_status);
				syslog_P(LOG_DAEMON | LOG_INFO, PSTR("Stopped"));
			}
		}
		else if (ev == PROCESS_EVENT_TIMER) {
			if (data == &tmr_hbeat && etimer_expired(&tmr_hbeat)) {
				etimer_reset(&tmr_hbeat);
				if (running) {
					tcpip_poll_udp(c);
					PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
					uip_send(xap_hbeat, sizeof(xap_hbeat));
				}
			}
		}
		else if (ev == PROCESS_EVENT_EXIT) {
			running = 0;

			process_exit(&xap_process);
			LOADER_UNLOAD();
		}
	}

	PROCESS_END();
}

