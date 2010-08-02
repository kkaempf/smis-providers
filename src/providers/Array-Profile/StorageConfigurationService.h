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
#ifndef STORAGECONFIGURATIONSERVICE_H_
#define STORAGECONFIGURATIONSERVICE_H_

#include <y2storage/StorageInterface.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"


// Minimum Volume/Pool size supported
#define MIN_VOLUME_SIZE (1024*1024)	// Try 1MB for now ??

using namespace storage;


enum methodRetCodes_e
{
	M_COMPLETED_OK			= 0,
	M_NOT_SUPPORTED			= 1,
	M_UNKNOWN				= 2,
	M_TIMEOUT				= 3,
	M_FAILED				= 4,
	M_INVALID_PARAM			= 5,
	M_IN_USE				= 6,
	M_JOB_STARTED			= 4096,
	M_SIZE_NOT_SUPPORTED	= 4097
};
	
// Exports

extern CMPIBoolean		SCSNeedToScan;


extern CMPIInstance*	SCSCreateInstance(
							const char *ns, 
							CMPIStatus *status);

extern CMPIObjectPath*	SCSCreateObjectPath(
							const char *ns, 
							CMPIStatus *status);

extern CMPIInstance*	SCSCreateHostedAssocInstance(
							const char *ns, 
							const char ** properties,
							CMPIStatus *status);

extern CMPIObjectPath*	SCSCreateHostedAssocObjectPath(
  							const char *ns, 
							CMPIStatus *status);

extern void				SCSInvokeMethod(
							const char *ns,
							const char *methodName,
							const CMPIArgs *in,
							CMPIArgs *out,
							const CMPIResult* results,
							CMPIStatus *pStatus,
							const CMPIContext *context,
							const CMPIObjectPath *cop);

extern int				SCSScanStorage(const char *ns, CMPIStatus *pStatus);
							
extern void				StorageInterfaceFree();
extern bool IsExtendedPartition(const char *name, char *disk);

#ifdef __cplusplus
}
#endif

#endif /*STORAGECONFIGURATIONSERVICE_H_*/
