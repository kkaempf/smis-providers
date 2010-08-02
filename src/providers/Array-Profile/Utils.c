/*******************************************************************************
 |
 | 	SMIS-Providers
 | 	Copyright (c) [2008] Novell, Inc.
 | 	All rights reserved. 
 |
 | This program and the accompanying materials
 | are made available under the terms of the Eclipse Public License v1.0
 | which accompanies this distribution, and is available at
 | http://www.eclipse.org/legal/epl-v10.html 
 |
 |********************************************************************************
 |
 |	 OMC SMI-S Volume Management provider
 |
 |---------------------------------------------------------------------------
 |
 | $Id: 
 |
 |---------------------------------------------------------------------------
 | This module contains:
 |   Provider helper/utility code 
 |
 +-------------------------------------------------------------------------*/

/* Include the required CMPI macros, data types, and API function headers */
#ifdef __cplusplus
extern "C" {
#endif	

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "stdio.h"

#ifdef __cplusplus
}
#endif

#ifndef _SMI_TRACE
#include "Utils.h"
#include "ArrayProvider.h"

/* A simple stderr logging/tracing facility. */
void _logstderr(const char *fmt,...)
{
   va_list ap;
   va_start(ap,fmt);
   vfprintf(stderr,fmt,ap);
   va_end(ap);
   fprintf(stderr,"\n");
}
#endif


/////////////////////////////////////////////////////////////////////////////////
CMPIUint16 GetHealthState(CMPIUint16 opStatus)
{
	_SMI_TRACE(1,("getHealthState() called"));

	switch (opStatus)
	{
		case OSTAT_DEGRADED:
			return HEALTH_DEGRADED;

		case OSTAT_ERROR:
			return HEALTH_MAJOR_FAILURE;

		case OSTAT_NON_RECOVERABLE_ERROR:
			return HEALTH_NON_RECOVERABLE;

		default:
			return HEALTH_OK;		
	}
}

/////////////////////////////////////////////////////////////////////////////////
char* GetStatusDescription(CMPIUint16 opStatus)
{
	static char *desc[] = 
	{
		"Unknown",
		"Other",
		"OK",
		"Degraded",
		"Stressed",
		"Predictive Failure",
		"Error",
		"Non-Recoverable Error",
		"Starting",
		"Stopping",
		"In Service",
		"No Contact",
		"Lost Communication",
		"Aborted",
		"Dormant",
		"Supporting Entity In Error",
		"Completed",
		"Power Mode"
	};

	_SMI_TRACE(1,("GetStatusDescription() called"));
	
	_SMI_TRACE(1,("Status: %s", desc[opStatus]));
	if (opStatus > OSTAT_POWER_MODE)
		return desc[0];
	else
		return desc[opStatus];
}


