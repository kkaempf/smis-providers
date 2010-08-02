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
 |	 OMC SMI-S Volume Management provider include file
 |
 |---------------------------------------------------------------------------
 |
 | $Id:
 |
 +-------------------------------------------------------------------------*/
#ifndef UTILS_H_
#define UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif	

/* A simple stderr logging/tracing facility. */
#ifndef _SMI_TRACE
#include <stdarg.h>

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"

extern void _logstderr(const char *fmt,...);
#define _SMI_TRACE(tracelevel,args) _logstderr args 
#endif

/* Health/Status monitoring helper functions */
CMPIUint16	GetHealthState(CMPIUint16 opStatus);
char *		GetStatusDescription(CMPIUint16 opStatus);


#ifdef __cplusplus
}
#endif

#endif /*UTILS_H_*/
