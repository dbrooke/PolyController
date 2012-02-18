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

#if CONFIG_APPS_SYSLOG
#include "apps/syslog.h"
#endif
#include "drivers/port_ext.h"

#define XAP_BSC_MSG "xap-header\n{\nv=12\nhop=1\nuid=FFBC010%d\nclass=xapbsc.%s\nsource=bootc.polycontroller.default:relay%d\n}\noutput.state\n{\nstate=%s\n}\n"

extern struct process xap_tx_process;
extern process_event_t xap_send;
extern process_event_t xap_recv;

PROCESS(xap_rly_process, "xAP_rly");
INIT_PROCESS(xap_rly_process);

static int relay_bit = 0;

static struct etimer tmr_xap_info;

PROCESS_THREAD(xap_rly_process, ev, data) {

	static char xap_bsc_msg[512];

	PROCESS_BEGIN();

	etimer_set(&tmr_xap_info, CLOCK_SECOND * 15);

	while (1) {
		PROCESS_WAIT_EVENT();

		if (ev == xap_recv) {
			//printf("--------------------\n%s\n--------------------\n",(char *)data);
		}
		else if (ev == PROCESS_EVENT_TIMER) {
			if (data == &tmr_xap_info && etimer_expired(&tmr_xap_info)) {
				etimer_reset(&tmr_xap_info);
				sprintf(xap_bsc_msg, XAP_BSC_MSG, relay_bit + 1, "info", relay_bit + 1, port_ext_bit_get(0, relay_bit)?"on":"off");
				process_post(&xap_tx_process, xap_send, &xap_bsc_msg);
				if (++relay_bit > 3) relay_bit = 0;
			}
		}
		else if (ev == PROCESS_EVENT_EXIT) {

			process_exit(&xap_rly_process);
			LOADER_UNLOAD();
		}
	}

	PROCESS_END();
}

