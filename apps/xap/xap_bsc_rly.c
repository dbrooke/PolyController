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

#include "xap.h"

#if CONFIG_APPS_SYSLOG
#include "apps/syslog.h"
#endif
#include "drivers/port_ext.h"

#define XAP_BSC_MSG "xap-header\n{\nv=12\nhop=1\nuid=FFBC010%d\nclass=xapbsc.%s\nsource=bootc.polyctrl.default:relay.%d\n}\noutput.state\n{\nstate=%s\n}\n"

extern struct process xap_tx_process;
extern process_event_t xap_send;
extern process_event_t xap_recv;
extern process_event_t xap_status;

process_event_t rly_update;

PROCESS(xap_rly_process, "xAP_rly");
INIT_PROCESS(xap_rly_process);

static int i, relay_bit = 3;
static int state, relay_state[4]={-1,-1,-1,-1};
static const char *name[4]={"1","2","3","4"};

static struct etimer tmr_xap_info;

PROCESS_THREAD(xap_rly_process, ev, data) {

	static char xap_bsc_msg[512];
	static char xap_rcv_msg[512];
	static xaphead xap_header;
	static char *xap_body;
	static xAPendp xap_endpoint;

	PROCESS_BEGIN();

	rly_update = process_alloc_event();

	while (1) {
		PROCESS_WAIT_EVENT();

		if (ev == xap_recv) {
			strncpy(xap_rcv_msg, data, sizeof(xap_rcv_msg));
			xap_body = xapReadHead(xap_rcv_msg, &xap_header);
			if ((xap_body != NULL) && xapEvalTarget(xap_header.target, "bootc", "polyctrl", "default", &xap_endpoint)) {
				if (!strcasecmp(xap_header.class, "xAPBSC.query") && (!strcmp(xap_endpoint.location,"relay") || !strcmp(xap_endpoint.location,"*"))) {
					for (i = 0; i < 4; i++) {
						if (!strcmp(xap_endpoint.name,"*") || !strcmp(xap_endpoint.name,name[i])) {
							relay_state[i] = state = port_ext_bit_get(0, 3 - i);
							sprintf(xap_bsc_msg, XAP_BSC_MSG, i + 1, "info", i + 1, state?"on":"off");
							process_post(&xap_tx_process, xap_send, &xap_bsc_msg);
							// pause to let xap_tx_process send the message
							PROCESS_PAUSE();
						}
					}
				}
				else if (!strcasecmp(xap_header.class, "xAPBSC.cmd")) {
					while ((xap_body = xapReadBscBody(xap_body, xap_header, &xap_endpoint)) != NULL) {
						if (xap_endpoint.UIDsub > 0 && xap_endpoint.UIDsub < 5) {
							if (!strcasecmp(xap_endpoint.state,"on")) {
								port_ext_bit_set(0, 4 - xap_endpoint.UIDsub);
							}
							else if (!strcasecmp(xap_endpoint.state,"off")) {
								port_ext_bit_clear(0, 4 - xap_endpoint.UIDsub);
							}
							// if relay state changed then send xapbsc.event else send xapbsc.info
							if ((state = port_ext_bit_get(0, 4 - xap_endpoint.UIDsub)) != relay_state[xap_endpoint.UIDsub - 1]) {
								relay_state[xap_endpoint.UIDsub - 1] = state;
								sprintf(xap_bsc_msg, XAP_BSC_MSG, xap_endpoint.UIDsub , "event", xap_endpoint.UIDsub, state?"on":"off");
							}
							else {
								sprintf(xap_bsc_msg, XAP_BSC_MSG, xap_endpoint.UIDsub , "info", xap_endpoint.UIDsub, state?"on":"off");
							}
							process_post(&xap_tx_process, xap_send, &xap_bsc_msg);
							// pause to let xap_tx_process send the message
							PROCESS_PAUSE();
						}
					}
				}
				port_ext_update();
			}
		}
		else if (ev == xap_status) {
			if (*(int*)(data) == 1) {
				// xap comms running
				etimer_set(&tmr_xap_info, CLOCK_SECOND * 15);
				// send initial xapbsc.info messages and initialise relay state array
				for (relay_bit = 0; relay_bit < 4; relay_bit++) {
					relay_state[relay_bit] = state = port_ext_bit_get(0, 3 - relay_bit);
					sprintf(xap_bsc_msg, XAP_BSC_MSG, relay_bit + 1, "info", relay_bit + 1, state?"on":"off");
					process_post(&xap_tx_process, xap_send, &xap_bsc_msg);
					// pause to let xap_tx_process send the message
					PROCESS_PAUSE();
				}
			}
			else {
				// xap comms stopped
				etimer_stop(&tmr_xap_info);
			}
		}
		else if (ev == rly_update) {
			for (i = 0; i < 4; i++) {
				// if changed send xapbsc.event
				if ((state = port_ext_bit_get(0, 3 - i)) != relay_state[i]) {
					relay_state[i] = state;
					sprintf(xap_bsc_msg, XAP_BSC_MSG, i + 1, "event", i + 1, state?"on":"off");
					process_post(&xap_tx_process, xap_send, &xap_bsc_msg);
					// pause to let xap_tx_process send the message
					PROCESS_PAUSE();
				}
			}
		}
		else if (ev == PROCESS_EVENT_TIMER) {
			if (data == &tmr_xap_info && etimer_expired(&tmr_xap_info)) {
				etimer_reset(&tmr_xap_info);
				if (++relay_bit > 3) relay_bit = 0;
				relay_state[relay_bit] = state = port_ext_bit_get(0, 3 - relay_bit);
				sprintf(xap_bsc_msg, XAP_BSC_MSG, relay_bit + 1, "info", relay_bit + 1, state?"on":"off");
				process_post(&xap_tx_process, xap_send, &xap_bsc_msg);
			}
		}
		else if (ev == PROCESS_EVENT_EXIT) {

			process_exit(&xap_rly_process);
			LOADER_UNLOAD();
		}
	}

	PROCESS_END();
}

