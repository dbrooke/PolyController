/**************************************************************************

These xAP utility functions are based on code originally part of the OPNODE
project but have been significantly modified to for use with the uIP TCP/IP
stack and the Contiki OS used for the PolyController.

The original OPNODE code is at http://sourceforge.net/projects/opnode/

***************************************************************************

Original OPNODE files:

	globalxap.h

	Header file for the xAP library

***************************************************************************

Original copyright:

   Copyright (c) 2007 Daniel Berenguer <dberenguer@usapiens.com>

	This file is part of the OPNODE project.

	OPNODE is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	OPNODE is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with OPNODE; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

***************************************************************************/

#define FALSE 0
#define TRUE 1

#define DEVLENGTH		20	// Maximum length for device names and locations
#define EPVALULEN		92	// Maximum length for value strings

#define SIZEXAPUID		40	// Maximum length of xAP UID's
#define SIZEXAPADDR		130	// Maximum length of xAP addresses
#define SIZEXAPKEY		34	// Maximum length of xAP keywords

unsigned short callBodyTimes;		// Number of studied bodies after inspecting the message header

// xAP header structure
typedef struct {
	int v;				// xAP version
	int hop;			// hop-count
	char uid[SIZEXAPUID];		// Unique identifier (UID)
	char class[40];			// Message class
	char source[SIZEXAPADDR];	// Message source
	char target[SIZEXAPADDR];	// Message target
	int interval;			// Heartbeat interval
} xaphead;

// xAP endpoint structure
typedef struct {
	char name[DEVLENGTH];		// name
	char location[DEVLENGTH];	// Location
	char value[EPVALULEN];		// Value
	char state[7];			// on, off, toggle
	int UIDsub;			// UID subaddress number (2-digit hexadecimal code)
	uint8_t type;			// Type of endpoint: 0=binary input; 1=binary output; 2=level/text input; 3=level/text output
	uint8_t nu1;
	uint8_t nu2;
	uint8_t nu3;
} xAPendp;

char * xapReadHead(char *msg, xaphead *header);
short int xapEvalTarget(char *strTarget, char *strVendor, char *strDevice, char *strInstance, xAPendp *endp);
char *xapReadBscBody(int fdSocket, char *pBody, xaphead header, xAPendp *endp);
