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
#ifndef COPYSERVICESEXTRINSIC_H_
#define COPYSERVICESEXTRINSIC_H_

#ifdef __cplusplus
extern "C" {
#endif
	
extern int CopyServicesCreateReplica(
        CMPIBroker *_BROKER,
	const char *ns,
	const CMPIContext* context,
	const CMPIObjectPath* cop,
	const char *methodName,
	const CMPIArgs *in,
	CMPIArgs *out,
	const CMPIResult* results,
	CMPIStatus *pStatus);

/*extern int CopyServicesModifySynchronization(
        CMPIBroker *_BROKER,
	const char *ns,
	const CMPIContext* context,
	const CMPIObjectPath* cop,
	const char *methodName,
	const CMPIArgs *in,
	CMPIArgs *out,
	const CMPIResult* results,
	CMPIStatus *pStatus);
*/
#ifdef __cplusplus
}
#endif

#endif /*COPYSERVICESEXTRINSIC_H_*/
