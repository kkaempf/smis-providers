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
 |   Provider code dealing with various OMC_StorageSetting classes
 |
 +-------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include <libintl.h>
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
#include "StorageSetting.h"
#include "StorageCapability.h"

extern CMPIBroker * _BROKER;

// Exported globals

// Module globals
static StorageSetting **SettingArray = NULL;
static CMPICount NumSettings = 0;
static CMPICount MaxNumSettings = 0;


/////////////////////////////////////////////////////////////////////////////
//////////// Private helper functions ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//////////// Exported functions /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
StorageSetting* SettingAlloc(const char *settingName)
{
	StorageSetting* setting = (StorageSetting *)malloc(sizeof (StorageSetting));
	if (setting)
	{
		memset(setting, 0, sizeof(StorageSetting));

		setting->dataRedundancyGoal = 
			setting->dataRedundancyMin = setting->dataRedundancyMax = 1;

		setting->userDataStripeDepth = 
			setting->userDataStripeDepthMin = setting->userDataStripeDepthMax = 32768;

		setting->instanceID = (char *)malloc(CMPIUTIL_INSTANCEID_PREFIX_SIZE + strlen(settingName) + 1);
		if (!setting->instanceID)
		{
			free(setting);
			setting = NULL;
		}
		else
		{
			cmpiutilMakeInstanceID(
				settingName, 
				setting->instanceID, 
				CMPIUTIL_INSTANCEID_PREFIX_SIZE + strlen(settingName) + 1);

			setting->name = (char *)malloc(strlen(settingName) + 1);
			if (!setting->name)
			{
				free(setting->instanceID);
				free(setting);
				setting = NULL;
			}
			else
			{
				strcpy(setting->name, settingName);
			}

		}
	}
	return(setting);
}

/////////////////////////////////////////////////////////////////////////////
void SettingFree(StorageSetting *setting)
{
	if (setting)
	{
		if (setting->instanceID)
		{
			free(setting->instanceID);
		}
		if (setting->name)
		{
			free(setting->name);
		}
		free(setting);
	}
}

/////////////////////////////////////////////////////////////////////////////
void SettingInitFromCapability(
			StorageSetting *setting, 
			const CMPIUint16 settingType)
{
	StorageCapability *capability = setting->capability;

	setting->packageRedundancyGoal = capability->packageRedundancy;
	setting->dataRedundancyGoal = capability->dataRedundancy;
	setting->extentStripeLength = capability->extentStripe;
	setting->parityLayout = capability->parity;
	setting->noSinglePointOfFailure = capability->noSinglePointOfFailure;
	setting->userDataStripeDepth = 32768;

	if (settingType == SCST_DEFAULT)
	{
		setting->dataRedundancyMin = capability->dataRedundancy;
		setting->dataRedundancyMax = capability->dataRedundancy;
		setting->packageRedundancyMin = capability->packageRedundancy;
		setting->packageRedundancyMax = capability->packageRedundancy;
		setting->extentStripeLengthMin = capability->extentStripe;
		setting->extentStripeLengthMax = capability->extentStripe;
		setting->userDataStripeDepthMin = 32768;
		setting->userDataStripeDepthMax = 32768;
	}
	else
	{
		// settingType == SCST_GOAL
		setting->dataRedundancyMin = 1;
		setting->dataRedundancyMax = capability->dataRedundancyMax;
		setting->packageRedundancyMin = 0;
		setting->packageRedundancyMax = capability->packageRedundancyMax;
		setting->extentStripeLengthMin = 0;
		setting->extentStripeLengthMax = capability->extentStripe;
		setting->userDataStripeDepthMin = 4*1024;
		setting->userDataStripeDepthMax = 4096*1024;
	}
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath* SettingCreateObjectPath(
						StorageSetting *setting, 
						const char *ns, 
						CMPIStatus *status)
{
	CMPIObjectPath *cop;

	cop = CMNewObjectPath(
				_BROKER, ns,
				StorageSettingWithHintsClassName,
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(0,("SettingCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
	}
	if (setting->instanceID)
	{
		CMAddKey(cop, "InstanceID", setting->instanceID, CMPI_chars);
	}
exit:
	_SMI_TRACE(1,("SettingCreateObjectPath() done"));
	return cop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SettingCreateInstance(
					StorageSetting *setting, 
					const char *ns,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	CMPIValue val;

	_SMI_TRACE(1,("SettingCreateInstance() called"));

	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, StorageSettingWithHintsClassName, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(1,("SettingCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "InstanceID", setting->instanceID, CMPI_chars);
	CMSetProperty(ci, "ElementName", setting->name, CMPI_chars);
	CMSetProperty(ci, "Caption", setting->name, CMPI_chars);

	val.boolean = setting->noSinglePointOfFailure;
	CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);

	val.uint16 = 1;
	CMSetProperty(ci, "DataRedundancyMin", &val, CMPI_uint16);
	val.uint16 = setting->dataRedundancyGoal;
	CMSetProperty(ci, "DataRedundancyGoal", &val, CMPI_uint16);
	val.uint16 = setting->dataRedundancyMax;
	CMSetProperty(ci, "DataRedundancyMax", &val, CMPI_uint16);

	val.uint16 = 0;
	CMSetProperty(ci, "PackageRedundancyMin", &val, CMPI_uint16);
	val.uint16 = setting->packageRedundancyGoal;
	CMSetProperty(ci, "PackageRedundancyGoal", &val, CMPI_uint16);
	val.uint16 = setting->packageRedundancyMax;
	CMSetProperty(ci, "PackageRedundancyMax", &val, CMPI_uint16);

	val.uint16 = 1;
	CMSetProperty(ci, "ExtentStripeLengthMin", &val, CMPI_uint16);
	val.uint16 = setting->extentStripeLength;
	CMSetProperty(ci, "ExtentStripeLength", &val, CMPI_uint16);
	val.uint16 = setting->extentStripeLengthMax;
	CMSetProperty(ci, "ExtentStripeLengthMax", &val, CMPI_uint16);

	val.uint64 = 4*1024;
	CMSetProperty(ci, "UserDataStripeDepthMin", &val, CMPI_uint64);
	val.uint64 = setting->userDataStripeDepth;
	CMSetProperty(ci, "UserDataStripeDepth", &val, CMPI_uint64);
	val.uint64 = 4096*1024;
	CMSetProperty(ci, "UserDataStripeDepthMax", &val, CMPI_uint64);

	if (setting->parityLayout == 2)
	{
		val.uint16 = setting->parityLayout;
		CMSetProperty(ci, "ParityLayout", &val, CMPI_uint16);
		val.boolean = 1;
		CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
	}

	if (setting->volume != NULL)
	{
		val.uint16 = 0;
		CMSetProperty(ci, "ChangeableType", &val, CMPI_uint16);
	}
	else
	{
		val.uint16 = 1;
		CMSetProperty(ci, "ChangeableType", &val, CMPI_uint16);
	}

exit:
	_SMI_TRACE(1,("SettingCreateInstance() done"));
	return ci;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SettingCreateGFCAssocInstance(
					StorageSetting *setting,
					const char *ns,
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "Antecedent",  "Dependent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("SettingCreateGFCAssocInstance() called"));

	// Create and populate StorageCapabilities object path
	CMPIObjectPath *sccop = CapabilityCreateObjectPath(setting->capability, ns, &status);
	if (CMIsNullObject(sccop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageCapabilities cop");
		return NULL;
	}

	// Create and populate StorageSetting object path
	CMPIObjectPath *sscop = SettingCreateObjectPath(setting, ns, &status);
	if (CMIsNullObject(sscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageSetting cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											StorageSettingsGeneratedFromCapabilitiesClassName,
											classKeys,
											properties,
											"Antecedent",
											"Dependent",
											sccop,
											sscop,
											pStatus);

	_SMI_TRACE(1,("Leaving SettingCreateGFCAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SettingCreateGFCAssocObjectPath(
					StorageSetting *setting,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("SettingCreateGFCAssocObjectPath() called"));

	// Create and populate StorageCapabilities object path
	CMPIObjectPath *sccop = CapabilityCreateObjectPath(setting->capability, ns, &status);
	if (CMIsNullObject(sccop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageCapabilities cop");
		return NULL;
	}


	// Create and populate StorageSetting object path
	CMPIObjectPath *sscop = SettingCreateObjectPath(setting, ns, &status);
	if (CMIsNullObject(sscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageSetting cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									StorageSettingsGeneratedFromCapabilitiesClassName,
									"Antecedent",
									"Dependent",
									sccop,
									sscop,
									pStatus);

	_SMI_TRACE(1,("Leaving SettingCreateGFCAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	SettingIsJBOD(StorageSetting *setting)
{
	return (setting->packageRedundancyGoal == 0 &&
			setting->dataRedundancyGoal == 1 &&
			setting->extentStripeLength == 1);
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	SettingIsRAID0(StorageSetting *setting)
{
	return (setting->packageRedundancyGoal == 0 &&
			setting->dataRedundancyGoal == 1 &&
			setting->extentStripeLength > 1);
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	SettingIsRAID1(StorageSetting *setting)
{
	return (setting->packageRedundancyGoal == 1 &&
			setting->dataRedundancyGoal > 1 &&
			setting->extentStripeLength == 1);
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	SettingIsRAID10(StorageSetting *setting)
{
	return (setting->packageRedundancyGoal == 1 &&
			setting->dataRedundancyGoal > 1 &&
			setting->extentStripeLength > 1);
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	SettingIsRAID5(StorageSetting *setting)
{
	return (setting->packageRedundancyGoal == 1 &&
			setting->dataRedundancyGoal == 1 &&
			setting->extentStripeLength > 2 &&
			setting->parityLayout == 2);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPICount SettingsSize()
{
	return NumSettings;
}

/////////////////////////////////////////////////////////////////////////////
StorageSetting* SettingsGet(const CMPICount index)
{
	if (MaxNumSettings <= index)
	{
		return NULL;
	}

	return SettingArray[index];
}

/////////////////////////////////////////////////////////////////////////////
StorageSetting* SettingsFind(const char *instanceID)
{
	CMPICount i;
	StorageSetting *currSetting;

	_SMI_TRACE(1,("SettingsFind() called with instanceID = %s, numSettings = %d", instanceID, NumSettings));
	if (SettingArray == NULL)
		return NULL;

	for (i = 0; i < MaxNumSettings; i++)
	{
		currSetting = SettingArray[i];
		if (currSetting)
		{
			_SMI_TRACE(1,("Comparing %s to %s", currSetting->instanceID, instanceID));
			if (strcmp(currSetting->instanceID, instanceID) == 0)
			{
				// Found it
				return currSetting;
			}
		}
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
void SettingsAdd(StorageSetting *setting)
{
	StorageSetting	**newArray;
	CMPICount		i;
	StorageSetting	*currSetting;

	// See if this is the first time thru
	if (SettingArray == NULL)
	{
		// First time - we need to alloc our SettingArray
		SettingArray = (StorageSetting **)malloc(SETTING_ARRAY_SIZE_DEFAULT * sizeof(StorageSetting *));
		if (SettingArray == NULL)
		{
			_SMI_TRACE(0,("SettingsAdd() ERROR: Unable to allocate setting array"));
			return;
		}
		MaxNumSettings = SETTING_ARRAY_SIZE_DEFAULT;
		for (i = 0; i < MaxNumSettings; i++)
		{
			SettingArray[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new SettingArray %x, size = %d", SettingArray, MaxNumSettings));
	}

	// See if this setting is already in the array
	currSetting = SettingsFind(setting->instanceID);
	if (currSetting)
	{
		_SMI_TRACE(1,("SettingsAdd() setting %s already exists, replace old with new", setting->instanceID));
		SettingsRemove(currSetting);
		SettingFree(currSetting);
	}

	// Try to find an empty slot
	for (i = 0; i < MaxNumSettings; i++)
	{
		if (SettingArray[i] == NULL)
		{
			// We found an available slot
			_SMI_TRACE(1,("SettingsAdd(): Adding %s", setting->instanceID));
			SettingArray[i] = setting;
			NumSettings++;
			break;
		}
	}

	// See if we need to resize the array
	if (i == MaxNumSettings)
	{
		// No available slots, need to resize
		_SMI_TRACE(1,("SettingsAdd(): SettingArray out of room, resizing"));

		newArray = (StorageSetting **)realloc(SettingArray, (MaxNumSettings + SETTING_ARRAY_SIZE_DEFAULT) * sizeof(StorageSetting *));
		if (newArray == NULL)
		{
			_SMI_TRACE(0,("SettingsAdd() ERROR: Unable to resize setting array"));
			return;
		}
		for (i = MaxNumSettings; i < (MaxNumSettings + SETTING_ARRAY_SIZE_DEFAULT); i++)
		{
			newArray[i] = NULL;
		}

		SettingArray = newArray;

		// Don't forget to add the new setting
		_SMI_TRACE(1,("SettingsAdd()-resize-: Adding %s", setting->instanceID));
		SettingArray[MaxNumSettings] = setting;
		NumSettings++;
		MaxNumSettings += SETTING_ARRAY_SIZE_DEFAULT;
	}
}

/////////////////////////////////////////////////////////////////////////////
void SettingsRemove(StorageSetting *setting)
{
	CMPICount	i;
	StorageSetting *currSetting;

	// Find & remove setting
	for (i = 0; i < MaxNumSettings; i++)
	{
		currSetting = SettingArray[i];
		if (strcmp(currSetting->instanceID, setting->instanceID) == 0)
		{
			// Found it, delete it
			_SMI_TRACE(1,("SettingsAdd(): Removing %s", setting->instanceID));
			SettingArray[i] = NULL;
			NumSettings--;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void SettingsFree()
{
	CMPICount i;
	StorageSetting *setting;

	_SMI_TRACE(1,("SettingsFree() called, SettingArray = %x, NumSettings = %d, MaxNumSettings = %d", SettingArray, NumSettings, MaxNumSettings));

	if (SettingArray == NULL)
		return;

	for (i = 0; i < MaxNumSettings; i++)
	{
		setting = SettingArray[i];
		if (setting)
		{
			SettingFree(setting);
		}
	}

	free(SettingArray);
	SettingArray = NULL;
	NumSettings = 0;
	MaxNumSettings = 0;
}

