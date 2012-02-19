/**************************************************************************

These xAP utility functions are based on code originally part of the OPNODE
project but have been significantly modified to for use with the uIP TCP/IP
stack and the Contiki OS used for the PolyController.

The original OPNODE code is at http://sourceforge.net/projects/opnode/

***************************************************************************

Original OPNODE files:

	xapcommon.c

	Bottom level functions of the xAP interface. Common functions to any
	xAP schema

	xapbsc.c

	Functions needed to communicate under the xAP BSC schema

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

#include <contiki-net.h>
#include <init.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "xap.h"

//*************************************************************************
// xapReadHead
//
//	Evaluates the header section from the supplied message
//
//	'msg'		Pointer to the received xAP message
//	'header'	Pointer to the header struct to return the results
//
// Returns:
//		Pointer to the message body if the function executes succesfully
//		NULL otherwise
//
//*************************************************************************

char * xapReadHead(char *msg, xaphead *header)
{
	char *pStr1, *pStr2;			// Pointers to strings
	static char strKey[SIZEXAPKEY];		// Keyword string
	static char strValue[SIZEXAPADDR];	// Value string
	int intLen;
	char chr;
	uint8_t flgDataCheck = 0;		// Checks the consistency of the message header
	uint8_t flgHbeat = FALSE;		// TRUE if the received message is a xAP heartbeat

	// Initialize the target structure (header)
	header->v = 0;
	header->hop = 0;
	memset(header->uid, 0, sizeof(header->uid));
	memset(header->class, 0, sizeof(header->class));
	memset(header->source, 0, sizeof(header->source));
	memset(header->target, 0, sizeof(header->target));

	callBodyTimes = 0;	// Reset the number of studied bodies (this is a new message)

	// Inspect the message header
	pStr1 = msg;		// Place the pointer at the beginning of the xAP message

	// Skip possible leading <LF>
	if (pStr1[0] == '\n')
		pStr1++;

	// Verify the correct beginning of the message header
	if (!strncasecmp(pStr1, "xap-header\n{\n", 13))
		pStr1 += 13;				// Place the pointer inside the header section
	// Is it a xAP heartbeat
	else if (!strncasecmp(pStr1, "xap-hbeat\n{\n", 12))
	{
		flgHbeat = TRUE;			// it's a xAP heartbeat
		pStr1 += 12;				// Place the pointer inside the header section
	}
	else
		return NULL;

	// Until the end of the header section
	while (pStr1[0] != '}')
	{
		// Capture the key string
		if ((pStr2 = strchr(pStr1, '=')) == NULL)
			return NULL;

		if ((intLen = pStr2 - pStr1) >= sizeof(strKey))		// Only within the limits of strKey
			return NULL;

		strncpy(strKey, pStr1, intLen);				// Copy the key string into strKey
		strKey[intLen] = 0;					// Terminate the string
		pStr1 = pStr2 + 1;					// Move the pointer to the position of the value

		// Capture the value string
		if ((pStr2 = strchr(pStr1, '\n')) == NULL)
			return NULL;

		if ((intLen = pStr2 - pStr1) >= sizeof(strValue))	// Only within the limits of strValue
			return NULL;

		strncpy(strValue, pStr1, intLen);			// Copy the value string into strValue
		strValue[intLen] = 0;					// Terminate the string
		pStr1 = pStr2 + 1;					// Move the pointer to the next line

		// Evaluate the key/value pair
		chr = strKey[0];					// Take strKey first character
		if (isupper((int)chr))					// Convert strKey first character to lower case
			chr = tolower((int)chr);			// if necessary

		switch (chr)						// Consider strKey first character
		{
			case 'v':
				// xAP specification version?
				if (!strcasecmp(strKey, "v"))
				{
					header->v = atoi(strValue);			// Store the version in the struct
					flgDataCheck |= 0x01;				// First bit on
				}
				break;
			case 'h':
				// hop count?
				if (!strcasecmp(strKey, "hop"))
				{
					header->hop = atoi(strValue);			// Store the hop count in the struct
					flgDataCheck |= 0x02;				// Second bit on
				}
				break;
			case 'u':
				// UID?
				if (!strcasecmp(strKey, "uid"))
				{
					if (strlen(strValue) >= sizeof(header->uid))
						return NULL;
					strcpy(header->uid, strValue);			// Store the UID in the struct
					flgDataCheck |= 0x04;				// Third bit on
				}
				break;
			case 'c':
				// Message class?
				if (!strcasecmp(strKey, "class"))
				{
					if (strlen(strValue) > sizeof(header->class))
						return NULL;
					strcpy(header->class, strValue);		// Store the class in the struct
					flgDataCheck |= 0x08;				// Fourth bit on
				}
				break;
			case 's':
				// xAP source?
				if (!strcasecmp(strKey, "source"))
				{
					if (strlen(strValue) > sizeof(header->source))
						return NULL;
					strcpy(header->source, strValue);		// Store the source in the struct
					flgDataCheck |= 0x10;				// Fifth bit on
				}
				break;
			case 't':
				// xAP target?
				if (!strcasecmp(strKey, "target"))
				{
					if (strlen(strValue) > sizeof(header->target))
						return NULL;
					strcpy(header->target, strValue);		// Store the target in the struct
					flgDataCheck |= 0x20;				// Sixth bit on
				}
				break;
			case 'i':
				// Heartbeat interval?
				if (!strcasecmp(strKey, "interval"))
				{
					header->interval = atoi(strValue);		// Store the heartbeat interval in the struct
					flgDataCheck |= 0x40;				// Seventh bit on
				}
				break;
			default:
				break;
		}
	}

	// Verify the consistency of the message header
	// If the message is a xAP heartbeat
	if (flgHbeat)
	{
		// Is the message really not a xAP heartbeat? Consistency error?
		if (strcasecmp(header->class, "xap-hbeat.alive") || flgDataCheck != 0x5F)	// 0x5F = 01011111b
			return NULL;
	}
	// If conventional message
	else
	{
		// Suported schemas

		// If BSC command or query, there should be a target field. Same for xap-x10.request
		if (!strcasecmp(header->class, "xAPBSC.cmd") || !strcasecmp(header->class, "xAPBSC.query") ||
			 !strcasecmp(header->class, "xap-x10.request"))
		{
			if (flgDataCheck != 0x3F)	// 0x3F = 00111111b
				return NULL;
		}
	}

	return pStr1+2;		// Return pointer to the body
}

//*************************************************************************
//	xapEvalTarget
//
//	Evaluates the target field and decides whether the message is addressed
//	to the device or not. It also extracts the target endpoint. This function
//	considers wildcarding.
//	Address format: Vendor.Device.Instance:location.endpointname
//
//	'strTarget'	string containing the target of the xAP message
//	'strVendor'	Vendor name
//	'strDevice'	Device name
//	'strInstance'	Instance name
//	'endp'		Pointer to the xAP endpoint structure where to pass the
//			results
//
// Returns:
//		1 if the message is addressed to our device
//		0 otherwise
//
//*************************************************************************

short int xapEvalTarget(char *strTarget, char *strVendor, char *strDevice, char *strInstance, xAPendp *endp)
{
	char *pStr1, *pStr2, *pSubAddr;	// Pointers to strings
	uint8_t flgGoOn = FALSE;		// If true, parse subaddress string
	uint8_t flgWildCard = FALSE;	// If true, ">" wildcard found in the target base address
	static char strBuffer[SIZEXAPADDR];

	strcpy(strBuffer, strTarget);

	pSubAddr = NULL;
	pStr1 = strBuffer;		// Pointer at the beginning of the target string

	// Search subaddress
	if ((pStr2 = strchr(pStr1, ':')) != NULL)
	{
		pStr2[0] = 0;			// Terminate base address string
		pSubAddr = pStr2 + 1;		// Pointer to the subaddress string
	}

	// Capture vendor name
	if ((pStr2 = strchr(pStr1, '.')) != NULL)
	{
		pStr2[0] = 0;
		if (!strcasecmp(pStr1, strVendor) || !strcmp(pStr1, "*"))
		{
			pStr1 = pStr2 + 1;
			// Capture device name
			if ((pStr2 = strchr(pStr1, '.')) != NULL)
			{
				pStr2[0] = 0;
				if (!strcasecmp(pStr1, strDevice) || !strcmp(pStr1, "*"))
				{
					pStr1 = pStr2 + 1;
					// Instance name
					if (!strcasecmp(pStr1, strInstance) || !strcmp(pStr1, "*"))
						flgGoOn = TRUE;
					// ">" wildcard?
					else if (!strcmp(pStr1, ">"))
						flgWildCard = TRUE;
				}
			}
			// ">" wildcard?
			else if (!strcmp(pStr1, ">"))
				flgWildCard = TRUE;
		}
	}
	// ">" wildcard?
	else if (!strcmp(pStr1, ">"))
		flgWildCard = TRUE;

	// Parse Subaddress

	if (flgWildCard || flgGoOn)
	{
		// The target matches our address
		// Search the target endpoint within the target subaddress

		// Wilcard subaddress too?
		if (pSubAddr == NULL && flgWildCard)
		{
			strcpy(endp->location, "*");	// For every location
			strcpy(endp->name, "*");	// For every endpoint name
			return 1;
		}

		pStr1 = pSubAddr;

		// Capture the endpoint location
		if ((pStr2 = strchr(pStr1, '.')) != NULL)
		{
			pStr2[0] = 0;
			// Avoid exceeding sizes
			if (strlen(pStr1) < sizeof(endp->location))
			{
				strcpy(endp->location, pStr1);		// Copy the endpoint location
				pStr1 = pStr2 + 1;			// Pointer at the beginning of the endpoint name
				// Capture the endpoint name
				// If the message is adressed to any endpoint within the captured location
				if (!strcmp(pStr1, ">"))
					strcpy(endp->name, "*");		// For every endpoint name
				// else, avoid exceeding sizes
				else if (strlen(pStr1) < sizeof(endp->name))
					strcpy(endp->name, pStr1);		// Copy the endpoint name
			}
		}
		// ">" wildcard?
		else if (!strcmp(pStr1, ">"))
		{
			strcpy(endp->location, "*");	// For every location
			strcpy(endp->name, "*");	// For every endpoint name
		}
		return 1;	// Address match. endp contains the targeted endpoint
	}

	return 0;	// Address does not match
}

//*************************************************************************
// xapReadBscBody
//
//	Inspects the xAP BSC message body pointed by pBody
//
//	'pBody'			Pointer to the beginning of the body section within the
//				xAP message. This pointer should be recovered from a
//				previous call to 'xapReadBscBody' or 'xapReadHead'
//	'header'		Header struct obtained from a previous call to xapReadHead
//	'endp'			Pointer to the xAP endpoint struct to write the results into
//
// Returns:
//		Pointer to the next message body (if exists)
//		NULL if BSC.query message, no more bodies or any error
//
//*************************************************************************

char *xapReadBscBody(char *pBody, xaphead header, xAPendp *endp)
{
	char *pStr1, *pStr2;			// Pointers to strings
	static char strKey[SIZEXAPKEY];		// Keyword string
	static char strValue[SIZEXAPADDR];	// Value string
	int intLen;
	char chr;
	uint8_t flgDataCheck = 0;		// Checks the consistency of the message body

	callBodyTimes++;		// Increase the number of studied bodies

	// Inspect the message body
	pStr1 = pBody;		// Place the pointer at the beginning of the xAP body

	// Verify the correct beginning of the body depending of the type of xAP BSC message
	// If xAP BSC command
	if (!strcasecmp(header.class, "xAPBSC.cmd"))
	{
		// Body tittle to expect
		sprintf(strValue, "output.state.%d\n{\n", callBodyTimes);
		// Verify the body tittle
		if (!strncasecmp(pStr1, strValue, strlen(strValue)))
			pStr1 += strlen(strValue);			// Place the pointer inside the body section
		else
			return NULL;
	}
	// If xAP BSC event or info
	else if (!strcasecmp(header.class, "xAPBSC.event") || !strcasecmp(header.class, "xAPBSC.info"))
	{
		//Verify the body tittle
		if (!strncasecmp(pStr1, "input.state\n{\n", 14))
		{
			endp->type = 0;		// input
			pStr1 += 14;		// Place the pointer inside the body section
		}
		else if (!strncasecmp(pStr1, "output.state\n{\n", 15))
		{
			endp->type = 1;		// output
			pStr1 += 15;		// Place the pointer inside the body section
		}
		else
			return NULL;
	}
	else
		return NULL;

	// Up to the end of the body section
	while (pStr1[0] != '}')
	{
		// Capture the key string
		if ((pStr2 = strchr(pStr1, '=')) == NULL)
			return NULL;
		if ((intLen = pStr2 - pStr1) >= sizeof(strKey))		// Only within the limits od strKey
			return NULL;
		strncpy(strKey, pStr1, intLen);				// Copy the key string into strKey
		strKey[intLen] = 0;					// Terminate the string
		pStr1 = pStr2 + 1;					// Move the pointer to the position of the value

		// Capture the value string
		if ((pStr2 = strchr(pStr1, '\n')) == NULL)
			return NULL;
		if ((intLen = pStr2 - pStr1) >= sizeof(strValue))	// Only within the limits od strValue
			return NULL;
		strncpy(strValue, pStr1, intLen);			// Copy the key string into strValue
		strValue[intLen] = 0;					// Terminate the string
		pStr1 = pStr2 + 1;					// Move the pointer to the next line

		// Evaluate the key/value pair
		chr = strKey[0];					// Take strKey first character
		if (isupper((int)chr))					// Convert strKey first character to lower case
			chr = tolower((int)chr);			// if necessary

		switch (chr)						// Consider strKey first character
		{
			case 'i':
				// id of the targeted endpoint?
				if (!strcasecmp(strKey, "id"))
				{
					if (!strcmp(strValue, "*"))	// Wildcard?
						endp->UIDsub = -1;	// Any endpoint
					else
						// Convert the byte to its numeric value (hex)
						endp->UIDsub = strtol(strValue, NULL, 16);
					flgDataCheck |= 0x01;			// First bit on
				}
				break;
			case 't':
				// text?
				if (!strcasecmp(strKey, "text"))
				{
					if (strlen(strValue) >= sizeof(endp->value))
						return NULL;
					strcpy(endp->value, strValue);		// Store the value in the structure
					flgDataCheck |= 0x02;			// second bit on
				}
				break;
			case 'l':
				// level?
				if (!strcasecmp(strKey, "level"))
				{
					if (strlen(strValue) >= sizeof(endp->value))
						return NULL;
					// "level" is only taken into account if there is no "text" in the body
					if (!(flgDataCheck & 0x02))		// If "text" has not been previously detected
					{
						strcpy(endp->value, strValue);	// Store the value in the struct
						flgDataCheck |= 0x04;		// Third bit on
					}
				}
				break;
			case 's':
				// state?
				if (!strcasecmp(strKey, "state"))
				{
					if (strlen(strValue) >= sizeof(endp->state))
						return NULL;
					// Store "state" as value only if there is no "text" nor "level" in the body
					if (!(flgDataCheck & 0x06))		// If "text" nor "level" have not been previously detected
						strcpy(endp->value, strValue);	// Store the state as value in the struct
					// Store "state" as state
					strcpy(endp->state, strValue);

					flgDataCheck |= 0x08;			// Fourth bit on
				}
				break;
			default:
				break;
		}
	}

	// If the body contains a "text"/"level" field
	if (flgDataCheck & 0x06)
		endp->type += 2;	// 0=binary input; 1=binary output; 2=level/text input; 3=level/text output

	// If xAP BSC command and no "id" found in the body
	if (!strcasecmp(header.class, "xAPBSC.cmd") && !(flgDataCheck & 0x01))
		return NULL;

	// Only for BSC commands, return the pointer to the next body if exists
	if (pStr1[1] == '\n' && pStr1[2] != 0 && !strcasecmp(header.class, "xAPBSC.cmd"))
		return pStr1+2;	// Return pointer to the next body
	else
		return NULL;
}

