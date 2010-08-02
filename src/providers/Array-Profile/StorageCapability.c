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
 |   Provider code dealing with various OMC Capabilities classes
 |
 +-------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include<libintl.h>
#include <cmpiutil/base.h>
#include <cmpiutil/modifyFile.h>
#include <cmpiutil/string.h>
#include <cmpiutil/cmpiUtils.h>

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#ifdef __cplusplus
}
#endif


#include "Utils.h"
#include "ArrayProvider.h"
#include "StorageConfigurationService.h"
#include "StorageCapability.h"
#include "StoragePool.h"
#include "StorageSetting.h"

extern CMPIBroker * _BROKER;

// Exported globals

// Module globals
static StorageCapability **CapabilityArray = NULL;
static CMPICount NumCapabilities = 0;
static CMPICount MaxNumCapabilities = 0;


/////////////////////////////////////////////////////////////////////////////
//////////// Private helper functions ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//////////// Exported functions /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SCCCreateInstance(
					const char *ns,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[1024];
	CMPIValue val;
	CMPIArray *arr;

	_SMI_TRACE(1,("SCCCreateInstance() called"));

	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, StorageConfigurationCapabilitiesClassName, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(1,("SCCCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "InstanceID", 
		cmpiutilMakeInstanceID("StorageConfigurationCapabilities", buf, 1024), CMPI_chars);
	CMSetProperty(ci, "ElementName", "StorageConfigurationCapabilities", CMPI_chars);
	CMSetProperty(ci, "Caption", "StorageConfigurationCapabilities", CMPI_chars);

	arr = CMNewArray(_BROKER, 6, CMPI_uint16, NULL);
	val.uint16 = SA_POOLCREATE;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.uint16 = SA_POOLDELETE;
	CMSetArrayElementAt(arr, 1, &val, CMPI_uint16);
	val.uint16 = SA_POOLMODIFY;
	CMSetArrayElementAt(arr, 2, &val, CMPI_uint16);
	val.uint16 = SA_ELEMENTCREATE;
	CMSetArrayElementAt(arr, 3, &val, CMPI_uint16);
	val.uint16 = SA_ELEMENTRETURN;
	CMSetArrayElementAt(arr, 4, &val, CMPI_uint16);
	val.uint16 = SA_ELEMENTMODIFY;
	CMSetArrayElementAt(arr, 5, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "SupportedSynchronousActions", &val, CMPI_uint16A);

	arr = CMNewArray(_BROKER, 2, CMPI_uint16, NULL);
	val.uint16 = ET_LOGICALDISK;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.uint16 = ET_STORAGEVOLUME;
	CMSetArrayElementAt(arr, 1, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "SupportedStorageElementTypes", &val, CMPI_uint16A);

	arr = CMNewArray(_BROKER, 4, CMPI_uint16, NULL);
	val.uint16 = PF_SINGLEINPOOL;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.uint16 = PF_QOSCHANGE;
	CMSetArrayElementAt(arr, 1, &val, CMPI_uint16);
	val.uint16 = PF_EXPANSION;
	CMSetArrayElementAt(arr, 2, &val, CMPI_uint16);
	val.uint16 = PF_REDUCTION;
	CMSetArrayElementAt(arr, 3, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "SupportedStoragePoolFeatures", &val, CMPI_uint16A);

	arr = CMNewArray(_BROKER, 5, CMPI_uint16, NULL);
	val.uint16 = EF_LOGICALDISKCREATE;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.uint16 = EF_LOGICALDISKMODIFY;
	CMSetArrayElementAt(arr, 1, &val, CMPI_uint16);
	val.uint16 = EF_SINGLEINPOOL;
	CMSetArrayElementAt(arr, 2, &val, CMPI_uint16);
	val.uint16 = EF_EXPANSION;
	CMSetArrayElementAt(arr, 3, &val, CMPI_uint16);
	val.uint16 = EF_REDUCTION;
	CMSetArrayElementAt(arr, 4, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "SupportedStorageElementFeatures", &val, CMPI_uint16A);

exit:
	_SMI_TRACE(1,("SCCCreateInstance() done, rc = %d", status->rc));
	return ci;
}


/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SCCCreateObjectPath(
					const char *ns,
					CMPIStatus *status)
{
	CMPIObjectPath *cop;
	char buf[256];

	_SMI_TRACE(1,("SCCCreateObjectPath() called"));

	cop = CMNewObjectPath(
				_BROKER, ns,
				StorageConfigurationCapabilitiesClassName,
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(1,("SCCCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
		goto exit;
	}

	CMAddKey(cop, "InstanceID", cmpiutilMakeInstanceID("StorageConfigurationCapabilities", buf, 256), CMPI_chars);

exit:
	_SMI_TRACE(1,("SCCCreateObjectPath() done"));
	return cop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SCCCreateAssocInstance(
					const char *ns,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "ManagedElement", "Capabilities", NULL };

	_SMI_TRACE(1,("SCCCreateHostedAssocInstance() called"));

	// Create and populate StorageConfigurationService object path 
	CMPIObjectPath *scscop = SCSCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(scscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationService cop");
		return NULL;
	}

	CMPIObjectPath *scccop = SCCCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(scccop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationCapabilities cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											StorageConfigurationElementCapabilitiesClassName,
											classKeys,
											properties,
											"ManagedElement",
											"Capabilities",
											scscop,
											scccop,
											pStatus);

	_SMI_TRACE(1,("Leaving SCCCreateHostedAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SCCCreateAssocObjectPath(
					const char *ns,
					CMPIStatus *pStatus)
{
	_SMI_TRACE(1,("SCCCreateAssocObjectPath() called"));

	// Create and populate StorageConfigurationService object path 
	CMPIObjectPath *scscop = SCSCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(scscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationService cop");
		return NULL;
	}

	CMPIObjectPath *scccop = SCCCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(scccop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationCapabilities cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									StorageConfigurationElementCapabilitiesClassName,
									"ManagedElement",
									"Capabilities",
									scscop,
									scccop,
									pStatus);

	_SMI_TRACE(1,("Leaving SCCCreateAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SSCCreateInstance(
					const char *ns,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[1024];
	CMPIValue val;
	CMPIUint16 drMax = 0, prMax = 0;
	StoragePool *pool;
	CMPICount i, numPools = PoolsSize();

	_SMI_TRACE(1,("SSCCreateInstance() called"));

	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, SystemStorageCapabilitiesClassName, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(1,("SSCCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "InstanceID", 
		cmpiutilMakeInstanceID("SystemStorageCapabilities", buf, 1024), CMPI_chars);
	CMSetProperty(ci, "ElementName", "SystemStorageCapabilities", CMPI_chars);
	CMSetProperty(ci, "Caption", "SystemStorageCapabilities", CMPI_chars);

	val.uint16 = SCET_STORAGE_CONFIG_SERVICE;
	CMSetProperty(ci, "ElementType", &val, CMPI_uint16);

	// Initially set capabilities to simple JBOD settings 

	val.uint16 = 0;
	CMSetProperty(ci, "DataRedundancyMin", &val, CMPI_uint16);
	CMSetProperty(ci, "DataRedundancyDefault", &val, CMPI_uint16);
	CMSetProperty(ci, "PackageRedundancyMin", &val, CMPI_uint16);
	CMSetProperty(ci, "PackageRedundancyDefault", &val, CMPI_uint16);
	val.uint16 = 1;
	CMSetProperty(ci, "ExtentStripeLengthDefault", &val, CMPI_uint16);
	val.uint64 = 32768;
	CMSetProperty(ci, "UserDataStripeDepthDefault", &val, CMPI_uint64);

	// Now look at all hosted storage pools and adjust SystemCapabilities
	// to reflect the superset of all pool capabilities
	_SMI_TRACE(1,("Found %d hosted pools on this computer", numPools));
	for (i = 0; i < numPools; i++)
	{
		pool = PoolsGet(i);
		_SMI_TRACE(1,("Found %s", pool->instanceID));
		if (pool->primordial && pool->remainingSize > 0)
		{
			_SMI_TRACE(1,("Found primordial pool capability %s", pool->capability->instanceID));
			_SMI_TRACE(1,("DRMax = %d PRMax = %d", pool->capability->dataRedundancyMax, pool->capability->packageRedundancyMax));
			drMax += pool->capability->dataRedundancyMax;
			prMax += pool->capability->packageRedundancyMax;
		}
	}

	val.uint16 = prMax;
	CMSetProperty(ci, "PackageRedundancyMax", &val, CMPI_uint16);
	val.uint16 = drMax;
	CMSetProperty(ci, "DataRedundancyMax", &val, CMPI_uint16);

	if (drMax > 0)
	{
		val.uint16 = 1;
		CMSetProperty(ci, "DataRedundancyMin", &val, CMPI_uint16);
		CMSetProperty(ci, "DataRedundancyDefault", &val, CMPI_uint16);
	}

	val.boolean = (drMax > 1);
	CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
	val.boolean = 0;
	CMSetProperty(ci, "NoSinglePointOfFailureDefault", &val, CMPI_boolean);

exit:
	_SMI_TRACE(1,("SSCCreateInstance() done"));
	return ci;
}


/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SSCCreateObjectPath(
					const char *ns,
					CMPIStatus *status)
{
	CMPIObjectPath *cop;
	char buf[256];

	_SMI_TRACE(1,("SSCCreateObjectPath() called"));

	cop = CMNewObjectPath(
				_BROKER, ns,
				SystemStorageCapabilitiesClassName,
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(1,("SSCCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
		goto exit;
	}

	CMAddKey(cop, "InstanceID", cmpiutilMakeInstanceID("SystemStorageCapabilities", buf, 256), CMPI_chars);

exit:
	_SMI_TRACE(1,("SSCCreateObjectPath() done"));
	return cop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SSCCreateAssocInstance(
					const char *ns,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "ManagedElement", "Capabilities", NULL };

	_SMI_TRACE(1,("SSCCreateHostedAssocInstance() called"));

	// Create and populate StorageConfigurationService object path 
	CMPIObjectPath *scscop = SCSCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(scscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationService cop");
		return NULL;
	}

	CMPIObjectPath *ssccop = SSCCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(ssccop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create SystemStorageCapabilities cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											StorageConfigurationElementCapabilitiesClassName,
											classKeys,
											properties,
											"ManagedElement",
											"Capabilities",
											scscop,
											ssccop,
											pStatus);

	_SMI_TRACE(1,("Leaving SSCCreateHostedAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SSCCreateAssocObjectPath(
					const char *ns,
					CMPIStatus *pStatus)
{
	_SMI_TRACE(1,("SSCCreateAssocObjectPath() called"));

	// Create and populate StorageConfigurationService object path 
	CMPIObjectPath *scscop = SCSCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(scscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationService cop");
		return NULL;
	}

	CMPIObjectPath *ssccop = SSCCreateObjectPath(ns, pStatus);
	if (CMIsNullObject(ssccop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create SystemStorageCapabilities cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									StorageConfigurationElementCapabilitiesClassName,
									"ManagedElement",
									"Capabilities",
									scscop,
									ssccop,
									pStatus);

	_SMI_TRACE(1,("Leaving SSCCreateAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
StorageCapability* CapabilityAlloc(const char *capabilityName)
{
	StorageCapability* capability = (StorageCapability *)malloc(sizeof (StorageCapability));
	if (capability)
	{
		memset(capability, 0, sizeof(StorageCapability));
		capability->dataRedundancy = capability->dataRedundancyMax = 1;
		capability->userDataStripeDepth = 32768;
		capability->instanceID = (char *)malloc(CMPIUTIL_INSTANCEID_PREFIX_SIZE + strlen(capabilityName) + 1);
		if (!capability->instanceID)
		{
			free(capability);
			capability = NULL;
		}
		else
		{
			cmpiutilMakeInstanceID(
				capabilityName, 
				capability->instanceID, 
				CMPIUTIL_INSTANCEID_PREFIX_SIZE + strlen(capabilityName) + 1);

			capability->name = (char *)malloc(strlen(capabilityName) + 1);
			if (!capability->name)
			{
				free(capability->instanceID);
				free(capability);
				capability = NULL;
			}
			else
			{
				strcpy(capability->name, capabilityName);
			}

		}
	}
	return(capability);
}

/////////////////////////////////////////////////////////////////////////////
void CapabilityFree(StorageCapability *capability)
{
	if (capability)
	{
		if (capability->instanceID)
		{
			free(capability->instanceID);
		}
		if (capability->name)
		{
			free(capability->name);
		}
		free(capability);
	}
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath* CapabilityCreateObjectPath(
						StorageCapability *capability, 
						const char *ns, 
						CMPIStatus *status)
{
	CMPIObjectPath *cop;

	cop = CMNewObjectPath(
				_BROKER, ns,
				StorageCapabilitiesClassName,
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(0,("CapabilityCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
	}
	if (capability->instanceID)
	{
		CMAddKey(cop, "InstanceID", capability->instanceID, CMPI_chars);
	}
exit:
	_SMI_TRACE(1,("CapabilityCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *CapabilityCreateInstance(
					StorageCapability *capability, 
					const char *ns,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	CMPIValue val;

	_SMI_TRACE(1,("CapabilityCreateInstance() called"));

	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, StorageCapabilitiesClassName, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(1,("CapabilityCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "InstanceID", capability->instanceID, CMPI_chars);
	CMSetProperty(ci, "ElementName", capability->name, CMPI_chars);
	CMSetProperty(ci, "Caption", capability->name, CMPI_chars);

	val.uint16 = SCET_STORAGE_POOL;
	CMSetProperty(ci, "ElementType", &val, CMPI_uint16);

	// Initially set capabilities to simple JBOD settings 

	val.uint16 = 1;
	CMSetProperty(ci, "DataRedundancyMin", &val, CMPI_uint16);
	val.uint16 = capability->dataRedundancy;
	CMSetProperty(ci, "DataRedundancyDefault", &val, CMPI_uint16);
	val.uint16 = capability->dataRedundancyMax;
	CMSetProperty(ci, "DataRedundancyMax", &val, CMPI_uint16);

	val.uint16 = 0;
	CMSetProperty(ci, "PackageRedundancyMin", &val, CMPI_uint16);
	val.uint16 = capability->packageRedundancy;
	CMSetProperty(ci, "PackageRedundancyDefault", &val, CMPI_uint16);
	val.uint16 = capability->packageRedundancyMax;
	CMSetProperty(ci, "PackageRedundancyMax", &val, CMPI_uint16);

	val.uint16 = capability->extentStripe;
	CMSetProperty(ci, "ExtentStripeLengthDefault", &val, CMPI_uint16);
	val.uint64 = capability->userDataStripeDepth;
	CMSetProperty(ci, "UserDataStripeDepthDefault", &val, CMPI_uint64);

	if (capability->parity != 0)
	{
		val.uint16 = capability->parity;
		CMSetProperty(ci, "ParityLayoutDefault", &val, CMPI_uint16);
	}

	val.boolean = capability->noSinglePointOfFailure;
	CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);

	if (capability->parity == 2)
	{
		val.boolean = 1;
		CMSetProperty(ci, "NoSinglePointOfFailureDefault", &val, CMPI_boolean);
		CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
		capability->noSinglePointOfFailure = 1;
	}
	else
	{
		val.boolean = (capability->dataRedundancy > 1);
		CMSetProperty(ci, "NoSinglePointOfFailureDefault", &val, CMPI_boolean);
	}

exit:
	_SMI_TRACE(1,("CapabilityCreateInstance() done"));
	return ci;
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	CapabilityDefaultIsJBOD(StorageCapability *cap)
{
	return (cap->packageRedundancy == 0 && 
			cap->dataRedundancy == 1 && 
			cap->extentStripe == 1);
}

CMPIBoolean	CapabilityDefaultIsRAID0(StorageCapability *cap)
{
	return (cap->packageRedundancy == 0 && 
			cap->dataRedundancy == 1 && 
			cap->extentStripe > 1);
}

CMPIBoolean	CapabilityDefaultIsRAID1(StorageCapability *cap)
{
	return (cap->packageRedundancy == 1 && 
			cap->dataRedundancy > 1 && 
			cap->extentStripe == 1);
}

CMPIBoolean	CapabilityDefaultIsRAID10(StorageCapability *cap)
{
	return (cap->packageRedundancy == 1 && 
			cap->dataRedundancy >1 && 
			cap->extentStripe > 1);
}

CMPIBoolean	CapabilityDefaultIsRAID5(StorageCapability *cap)
{
	return (cap->packageRedundancy == 1 && 
			cap->dataRedundancy == 1 && 
			cap->extentStripe > 2 &&
			cap->parity == 2);
}


/////////////////////////////////////////////////////////////////////////////
void CapabilityInvokeMethod(
		StorageCapability *cap,
		const char *ns,
		const char *methodName,
		const CMPIArgs *in,
		CMPIArgs *out,
		const CMPIResult* results,
		CMPIStatus *pStatus)
{
	CMPIUint32 rc = 0; 
	CMPICount i;

	_SMI_TRACE(1,("CapabilityInvokeMethod() called"));

	if (strcasecmp(methodName, "CreateSetting") == 0)
	{
		CMPICount inSize = CMGetArgCount(in, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (inSize < 1) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to CreateSetting method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to CreateSetting method");
			goto exit;
		}

/*
		CMPICount outSize = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (outSize < 1) )
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateSetting method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateSetting method");
			goto exit;
		}
*/
		StorageSetting *newSetting = NULL;
		CMPIObjectPath *newSettingCop = NULL;
		CMPIUint16 settingType = SCST_DEFAULT;
		CMPIData inData = CMGetArg(in, "SettingType", NULL);
		if (inData.state == CMPI_goodValue)
		{
			settingType = inData.value.uint16;
		}
		_SMI_TRACE(1, ("settingType = %d", settingType));
		
		rc = CreateSetting(cap, ns, settingType, &newSetting);

		if (newSetting)
		{
			_SMI_TRACE(1, ("newSetting = %s", newSetting->name));
			newSettingCop = SettingCreateObjectPath(newSetting, ns, pStatus);
		}

		CMPIValue val;
		val.ref = newSettingCop;
		CMAddArg(out, "NewSetting", &val, CMPI_ref);
	}
	else if (strcasecmp(methodName, "GetSupportedStripeLengths") == 0)
	{
		rc = GSSL_USE_ALTERNATE;
	}
	else if (strcasecmp(methodName, "GetSupportedStripeLengthRange") == 0)
	{
		CMPICount outSize = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (outSize < 3) )
		{
			_SMI_TRACE(0,("Required output parameters missing in call to GetSupportedStripeLengthRange method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameters missing in call to GetSupportedStripeLengthRange method");
			goto exit;
		}

		CMPIUint16 minStripeLen;
		CMPIUint16 maxStripeLen;
		CMPIUint32 divisor;

		rc = GetSupportedStripeLengthRange(cap, &minStripeLen, &maxStripeLen, &divisor);

		CMAddArg(out, "MinimumStripeLength", &minStripeLen, CMPI_uint16);
		CMAddArg(out, "MaximumStripeLength", &maxStripeLen, CMPI_uint16);
		CMAddArg(out, "StripeLengthDivisor", &divisor, CMPI_uint32);
	}
	else if (strcasecmp(methodName, "GetSupportedParityLayouts") == 0)
	{
		rc = GSSL_NOT_AVAILABLE;
	}
	else if (strcasecmp(methodName, "GetSupportedStripeDepths") == 0)
	{
		CMPICount outSize = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (outSize < 1) )
		{
			_SMI_TRACE(0,("Required output parameters missing in call to GetSupportedStripeDepths method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameters missing in call to GetSupportedStripeDepths method");
			goto exit;
		}

		CMPIArray *stripeDepths;

		rc = GetSupportedStripeDepths(&stripeDepths);

		CMPIValue val;
		val.array = stripeDepths;
		CMAddArg(out, "StripeDepths", &val, CMPI_uint64A);
	}
	else if (strcasecmp(methodName, "GetSupportedStripeDepthRange") == 0)
	{
		rc = GSSD_USE_ALTERNATE;
	}
	else
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_METHOD_NOT_AVAILABLE,
			"Method not supported by OMC_StorageCapabilities");
		goto exit;
	}

	CMReturnData(results, &rc, CMPI_uint32);
exit:
	_SMI_TRACE(1,("CapabilityInvokeMethod() finished"));
}

/////////////////////////////////////////////////////////////////////////////
CMPIUint32 CapabilityCreateSetting(
				StorageCapability *cap,
				const char *ns,
				const CMPIUint16 settingType,
				StorageSetting **newSetting)
{
	CMPIUint32 rc = CS_SUCCESS;
	CMPICount i;
	CMPIStatus status;

	// Make sure our new setting name is unique
	StorageSetting* ss;
	i = 0;
	char name[256];
	char instanceID[256 + CMPIUTIL_INSTANCEID_PREFIX_SIZE];

	while (true)
	{
		sprintf(name, "%sSetting%u", cap->name, i);
		cmpiutilMakeInstanceID(name, instanceID, 256 + CMPIUTIL_INSTANCEID_PREFIX_SIZE);

		ss = SettingsFind(instanceID);
		if (ss == NULL)
		{
			_SMI_TRACE(1,("Setting %s does not exist, creating...", name));

			// We don't have any setting by this name, create one
			ss = SettingAlloc(name);
			SettingsAdd(ss);
			*newSetting = ss;
			SettingCreateObjectPath(ss, ns, &status);
			break;
		}
		i++;
	}

	ss->capability = cap;
	ss->volume = NULL;
	SettingInitFromCapability(ss, settingType);
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPICount CapabilitiesSize()
{
	return NumCapabilities;
}

/////////////////////////////////////////////////////////////////////////////
StorageCapability* CapabilitiesGet(const CMPICount index)
{
	if (MaxNumCapabilities <= index)
	{
		return NULL;
	}

	return CapabilityArray[index];
}

/////////////////////////////////////////////////////////////////////////////
StorageCapability* CapabilitiesFind(const char *instanceID)
{
	CMPICount i;
	StorageCapability *currCap;

	if (CapabilityArray == NULL)
		return NULL;

	for (i = 0; i < MaxNumCapabilities; i++)
	{
		currCap = CapabilityArray[i];
		if (currCap)
		{
			if (strcmp(currCap->instanceID, instanceID) == 0)
			{
				// Found it
				return currCap;
			}
		}
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
void CapabilitiesAdd(StorageCapability *capability)
{
	StorageCapability	**newArray;
	CMPICount			i;
	StorageCapability	*currCapability;

	// See if this is the first time thru
	if (CapabilityArray == NULL)
	{
		// First time - we need to alloc our CapabilityArray
		CapabilityArray = (StorageCapability **)malloc(CAPABILITY_ARRAY_SIZE_DEFAULT * sizeof(StorageCapability *));
		if (CapabilityArray == NULL)
		{
			_SMI_TRACE(0,("CapabilitiesAdd() ERROR: Unable to allocate capability array"));
			return;
		}
		MaxNumCapabilities = CAPABILITY_ARRAY_SIZE_DEFAULT;
		for (i = 0; i < MaxNumCapabilities; i++)
		{
			CapabilityArray[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new CapabilityArray %x, size = %d", CapabilityArray, MaxNumCapabilities));
	}

	// See if this capability is already in the array
	currCapability = CapabilitiesFind(capability->instanceID);
	if (currCapability)
	{
		_SMI_TRACE(1,("CapabilitiesAdd() capability %s already exists, replace old with new", capability->instanceID));
		CapabilitiesRemove(currCapability);
		CapabilityFree(currCapability);
	}

	// Try to find an empty slot
	for (i = 0; i < MaxNumCapabilities; i++)
	{
		if (CapabilityArray[i] == NULL)
		{
			// We found an available slot
			_SMI_TRACE(1,("CapabilitiesAdd(): Adding %s", capability->instanceID));
			CapabilityArray[i] = capability;
			NumCapabilities++;
			break;
		}
	}

	// See if we need to resize the array
	if (i == MaxNumCapabilities)
	{
		// No available slots, need to resize
		_SMI_TRACE(1,("CapabilitiesAdd(): CapabilityArray out of room, resizing"));

		newArray = (StorageCapability **)realloc(CapabilityArray, (MaxNumCapabilities + CAPABILITY_ARRAY_SIZE_DEFAULT) * sizeof(StorageCapability *));
		if (newArray == NULL)
		{
			_SMI_TRACE(0,("CapabilitiesAdd() ERROR: Unable to resize capability array"));
			return;
		}
		for (i = MaxNumCapabilities; i < (MaxNumCapabilities + CAPABILITY_ARRAY_SIZE_DEFAULT); i++)
		{
			newArray[i] = NULL;
		}

		CapabilityArray = newArray;

		// Don't forget to add the new capability
		_SMI_TRACE(1,("CapabilitiesAdd()-resize-: Adding %s", capability->instanceID));
		CapabilityArray[MaxNumCapabilities] = capability;
		NumCapabilities++;
		MaxNumCapabilities += CAPABILITY_ARRAY_SIZE_DEFAULT;
	}
}

/////////////////////////////////////////////////////////////////////////////
void CapabilitiesRemove(StorageCapability *capability)
{
	CMPICount	i;
	StorageCapability *currCapability;

	// Find & remove capability
	for (i = 0; i < MaxNumCapabilities; i++)
	{
		currCapability = CapabilityArray[i];
		if (strcmp(currCapability->instanceID, capability->instanceID) == 0)
		{
			// Found it, delete it
			_SMI_TRACE(1,("CapabilitiesAdd(): Removing %s", capability->instanceID));
			CapabilityArray[i] = NULL;
			CapabilityFree(currCapability);
			NumCapabilities--;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void CapabilitiesFree()
{
	CMPICount i;
	StorageCapability *capability;

	_SMI_TRACE(1,("CapabilitiesFree() called, CapabilityArray = %x, NumCapabilities = %d, MaxNumCapabilities = %d", CapabilityArray, NumCapabilities, MaxNumCapabilities));

	if (CapabilityArray == NULL)
		return;

	for (i = 0; i < MaxNumCapabilities; i++)
	{
		capability = CapabilityArray[i];
		if (capability)
		{
			CapabilityFree(capability);
		}
	}

	free(CapabilityArray);
	CapabilityArray = NULL;
	NumCapabilities = 0;
	MaxNumCapabilities = 0;
}



/////////////////////////////////////////////////////////////////////////////
//////////// CIM methods called by invokeMethod /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 GetSupportedStripeLengthRange(
						StorageCapability *capability,													
						CMPIUint16* minStripeLen,
						CMPIUint16* maxStripeLen,
						CMPIUint32* divisor)
{
	*minStripeLen = 1;
	*divisor = 1;
	*maxStripeLen = capability->dataRedundancyMax;
	return CS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 GetSupportedStripeDepths(CMPIArray** stripeDepths)
{
	CMPIValue val;
	*stripeDepths = CMNewArray(_BROKER, 11, CMPI_uint64, NULL);

	val.uint64 = 4*1024;
	CMSetArrayElementAt(*stripeDepths, 0, &val, CMPI_uint64);
	val.uint64 = 8*1024;
	CMSetArrayElementAt(*stripeDepths, 1, &val, CMPI_uint64);
	val.uint64 = 16*1024;
	CMSetArrayElementAt(*stripeDepths, 2, &val, CMPI_uint64);
	val.uint64 = 32*1024;
	CMSetArrayElementAt(*stripeDepths, 3, &val, CMPI_uint64);
	val.uint64 = 64*1024;
	CMSetArrayElementAt(*stripeDepths, 4, &val, CMPI_uint64);
	val.uint64 = 128*1024;
	CMSetArrayElementAt(*stripeDepths, 5, &val, CMPI_uint64);
	val.uint64 = 256*1024;
	CMSetArrayElementAt(*stripeDepths, 6, &val, CMPI_uint64);
	val.uint64 = 512*1024;
	CMSetArrayElementAt(*stripeDepths, 7, &val, CMPI_uint64);
	val.uint64 = 1024*1024;
	CMSetArrayElementAt(*stripeDepths, 8, &val, CMPI_uint64);
	val.uint64 = 2048*1024;
	CMSetArrayElementAt(*stripeDepths, 9, &val, CMPI_uint64);
	val.uint64 = 4096*1024;
	CMSetArrayElementAt(*stripeDepths, 10, &val, CMPI_uint64);

	return CS_SUCCESS;
}
