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
#ifndef STORAGECAPABILITY_H_
#define STORAGECAPABILITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "StorageSetting.h"

#define CAPABILITY_ARRAY_SIZE_DEFAULT	16

enum SupportedActions
{
	SA_POOLCREATE		= 2,	
	SA_POOLDELETE		= 3,	
	SA_POOLMODIFY		= 4,	
	SA_ELEMENTCREATE	= 5,		
	SA_ELEMENTRETURN	= 6,		
	SA_ELEMENTMODIFY	= 7,		
};

enum SupportedStorageElementTypes
{
	ET_STORAGEVOLUME	= 2,	
	ET_STORAGEEXTENT	= 3,	
	ET_LOGICALDISK		= 4,	
};

enum SupportedStoragePoolFeatures
{
	PF_INEXTENTS		= 2,	
	PF_SINGLEINPOOL		= 3,	
	PF_MULTIPLEINPOOLS	= 4,
	PF_QOSCHANGE		= 5,
	PF_EXPANSION		= 6,
	PF_REDUCTION		= 7
};

enum SupportedStorageElementFeatures
{
	EF_EXTENTCREATE			= 2,	
	EF_VOLUMECREATE			= 3,	
	EF_EXTENTMODIFY			= 4,	
	EF_VOLUMEMODIFY			= 5,		
	EF_SINGLEINPOOL			= 6,		
	EF_MULTIPLEINPOOLS		= 7,		
	EF_LOGICALDISKCREATE	= 8,
	EF_LOGICALDISKMODIFY	= 9,
	EF_QOSCHANGE			= 11,
	EF_EXPANSION			= 12,
	EF_REDUCTION			= 13
};

typedef struct _StorageCapability
{
	char				*name;
	char				*instanceID;

	CMPIUint16			packageRedundancy;
	CMPIUint16			packageRedundancyMax;
	CMPIUint16			dataRedundancy;
	CMPIUint16			dataRedundancyMax;
	CMPIUint16			extentStripe;
	CMPIUint16			parity;
	CMPIUint64			userDataStripeDepth;
	CMPIBoolean			noSinglePointOfFailure;

	struct _StoragePool *pool;
} StorageCapability;

//
// Exports
//

// StorageConfigurationCapabilities (SCC) related functions
extern CMPIInstance*	SCCCreateInstance(
								const char *ns, 
								CMPIStatus *status);

extern CMPIObjectPath*	SCCCreateObjectPath(
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	SCCCreateAssocInstance(
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	SCCCreateAssocObjectPath(
  								const char *ns, 
								CMPIStatus *status);

// SystemStorageCapabilities (SSC) related functions
extern CMPIInstance*	SSCCreateInstance(
								const char *ns, 
								CMPIStatus *status);

extern CMPIObjectPath*	SSCCreateObjectPath(
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	SSCCreateAssocInstance(
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	SSCCreateAssocObjectPath(
  								const char *ns, 
								CMPIStatus *status);


// Capability management
extern StorageCapability*	CapabilityAlloc(const char *capabilityName);
extern void					CapabilityFree(StorageCapability *capability);
extern CMPIInstance*		CapabilityCreateInstance(StorageCapability *cap, const char *ns, CMPIStatus *status);
extern CMPIObjectPath*		CapabilityCreateObjectPath(StorageCapability *cap, const char *ns, CMPIStatus *status);
extern CMPIBoolean			CapabilityDefaultIsJBOD(StorageCapability *cap);
extern CMPIBoolean			CapabilityDefaultIsRAID0(StorageCapability *cap);
extern CMPIBoolean			CapabilityDefaultIsRAID1(StorageCapability *cap);
extern CMPIBoolean			CapabilityDefaultIsRAID10(StorageCapability *cap);
extern CMPIBoolean			CapabilityDefaultIsRAID5(StorageCapability *cap);

extern void					CapabilityInvokeMethod(
								StorageCapability *capability,
								const char *ns,
								const char *methodName,
								const CMPIArgs *in,
								CMPIArgs *out,
								const CMPIResult* results,
								CMPIStatus *pStatus);

extern CMPIUint32			CapabilityCreateSetting(
								StorageCapability *capability,
								const char *ns,
								const CMPIUint16 type,
								StorageSetting** newSetting);

// Capabilities array manipulation
extern CMPICount			CapabilitiesSize();
extern StorageCapability*	CapabilitiesGet(const CMPICount index);
extern StorageCapability*	CapabilitiesFind(const char *instanceID);
extern void					CapabilitiesAdd(StorageCapability *capability);
extern void					CapabilitiesRemove(StorageCapability *capability);
extern void					CapabilitiesFree();


// Methods called by InvokeMethod

#define CreateSetting CapabilityCreateSetting

static CMPIUint32	GetSupportedStripeLengthRange(
						StorageCapability *capability,													
						CMPIUint16* minStripeLen,
						CMPIUint16* maxStripeLen,
						CMPIUint32* divisor);

static CMPIUint32	GetSupportedStripeDepths(CMPIArray** stripeDepths);



#ifdef __cplusplus
}
#endif

#endif /*STORAGECAPABILITY_H_*/
