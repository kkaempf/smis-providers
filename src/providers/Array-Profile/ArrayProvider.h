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
#ifndef	ARRAYPROVIDER_H_
#define ARRAYPROVIDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CMPIUTIL_INSTANCEID_PREFIX_SIZE 9	//

//Macros for ClassNames

#define StorageConfigurationServiceClassName "OMC_StorageConfigurationService"
#define StorageConfigurationCapabilitiesClassName "OMC_StorageConfigurationCapabilities"
#define SystemStorageCapabilitiesClassName "OMC_SystemStorageCapabilities"
#define StorageCapabilitiesClassName "OMC_StorageCapabilities"
#define StoragePoolClassName "OMC_StoragePool"
#define StorageExtentClassName "OMC_StorageExtent"
#define CompositeExtentClassName "OMC_CompositeExtent"
#define LogicalDiskClassName "OMC_LogicalDisk"
#define StorageVolumeClassName "OMC_StorageVolume"
#define StorageSettingWithHintsClassName "OMC_StorageSettingWithHints"
#define HostedStorageConfigurationServiceClassName "OMC_HostedStorageConfigurationService"
#define HostedStoragePoolClassName "OMC_HostedStoragePool"
#define LogicalDiskDeviceClassName "OMC_LogicalDiskDevice"
#define StorageVolumeDeviceClassName "OMC_StorageVolumeDevice" 
#define StorageConfigurationElementCapabilitiesClassName "OMC_StorageConfigurationElementCapabilities"
#define StorageElementCapabilitiesClassName "OMC_StorageElementCapabilities"
#define AssociatedComponentExtentClassName "OMC_AssociatedComponentExtent"
#define AssociatedRemainingExtentClassName "OMC_AssociatedRemainingExtent"
#define StorageSettingsGeneratedFromCapabilitiesClassName "OMC_StorageSettingsGeneratedFromCapabilities"
#define BasedOnClassName "OMC_BasedOn"
#define CompositeExtentBasedOnClassName "OMC_CompositeExtentBasedOn"
#define StorageVolumeBasedOnClassName "OMC_StorageVolumeBasedOn"
#define AllocatedFromStoragePoolClassName "OMC_AllocatedFromStoragePool"
#define StorageVolumeAllocatedFromStoragePoolClassName "OMC_StorageVolumeAllocatedFromStoragePool"
#define StorageElementSettingDataClassName "OMC_StorageElementSettingData"
#define StorageVolumeStorageElementSettingDataClassName "OMC_StorageVolumeStorageElementSettingData"
#define UnitaryComputerSystemClassName "Linux_ComputerSystem"


enum errCodes_e
{
	STORERR_OK = 0,
	STORERR_FILE_OPEN = 700001,
	STORERR_FILE_READ = 700002,
	STORERR_TOO_SMALL = 700003,
	STORERR_NOT_FOUND = 700004,
	STORERR_NO_HARDWARE_INFO = 700005,

	// EVMS
	STORERR_EVMS_ERROR_GETTING_INFO = 700101,
	STORERR_EVMS_ERROR_GETTING_EXTENDED_INFO = 70202,

	// CIMOM
	STORERR_CREATING_OBJECT = 700301
};

// enumeration for the health state of objects (i.e. StoragePool)
enum healthState_e
{
	HEALTH_OK = 5,
	HEALTH_DEGRADED = 10,
	HEALTH_MINOR_FAILURE = 15,
	HEALTH_MAJOR_FAILURE = 20,
	HEALTH_CRITICAL_FAILURE = 25,
	HEALTH_NON_RECOVERABLE = 30
};

enum operationalStatus_e
{
	OSTAT_UKNOWN = 0,
	OSTAT_OTHER = 1,
	OSTAT_OK = 2,
	OSTAT_DEGRADED = 3,
	OSTAT_STRESSED = 4,
	OSTAT_PREDICTIVE_FAILURE = 5,
	OSTAT_ERROR = 6,
	OSTAT_NON_RECOVERABLE_ERROR = 7,
	OSTAT_STARTING = 8,
	OSTAT_STOPPING = 9,
	OSTAT_STOPPED = 10,
	OSTAT_IN_SERVICE = 11,
	OSTAT_NO_CONTACT = 12,
	OSTAT_LOST_COMMUNICATION = 13,
	OSTAT_ABORTED = 14,
	OSTAT_DORMANT = 15,
	OSTAT_SUPPORTING_ENTITY_IN_ERROR = 16,
	OSTAT_COMPLETED = 17,
	OSTAT_POWER_MODE = 18
};

enum extentStatus_e
{
	ESTAT_OTHER = 0,
	ESTAT_UNKNOWN = 1,
	ESTAT_NONE = 2,
	ESTAT_BROKEN = 3,
	ESTAT_DATA_LOST = 4,
	ESTAT_DYNAMIC_RECONFIG = 5,
	ESTAT_EXPOSED = 6,
	ESTAT_FRACTIONALLY_EXPOSED = 7,
	ESTAT_PARTIALLY_EXPOSED = 8,
	ESTAT_PROTECTION_DISABLED = 9,
	ESTAT_READYING = 10,
	ESTAT_REBUILD = 11,
	ESTAT_RECALCULATE = 12,
	ESTAT_SPARE_IN_USE = 13,
	ESTAT_VERIFY_IN_PROGRESS = 14,
	ESTAT_INBAND_ACCESS_GRANTED = 15,
	ESTAT_IMPORTED = 16,
	ESTAT_EXPORTED = 17
};

enum createSettingRetCodes_e
{
	CS_SUCCESS			= 0,
	CS_NOT_SUPPORTED	= 1,
	CS_UNSPECIFIED_ERR	= 2,
	CS_TIMEOUT			= 3,
	CS_FAILED			= 4,
	CS_INVALID_PARAM	= 5
};

enum StorageCapabilitiesElementTypes
{
	SCET_ANY					= 2,	
	SCET_STORAGE_VOLUME			= 3,	
	SCET_STORAGE_EXTENT			= 4,
	SCET_STORAGE_POOL			= 5,
	SCET_STORAGE_CONFIG_SERVICE	= 6,
	SCET_LOGICAL_DISK			= 7
};
	
enum StorageCapabilitiesSettingTypes
{
	SCST_DEFAULT				= 2,	
	SCST_GOAL					= 3
};

enum getSupportedStripeLengthRetCodes_e
{
	GSSL_COMPLETED_OK	= 0,
	GSSL_NOT_SUPPORTED	= 1,
	GSSL_NOT_AVAILABLE	= 2,
	GSSL_USE_ALTERNATE	= 3
};

enum getSupportedStripeDepthRetCodes_e
{
	GSSD_COMPLETED_OK	= 0,
	GSSD_NOT_SUPPORTED	= 1,
	GSSD_USE_ALTERNATE	= 2
};

#ifdef __cplusplus
}
#endif

#endif /*ARRAYPROVIDER_H_*/
