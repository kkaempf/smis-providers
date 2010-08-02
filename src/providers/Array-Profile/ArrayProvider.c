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
 |   Provider initialization and CMPI function table code.
 |
 +-------------------------------------------------------------------------*/

/* Include the required CMPI macros, data types, and API function headers */
#ifdef __cplusplus
extern "C" {
#endif

#include<libintl.h>
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include <stdio.h>

#include <cmpiutil/base.h>
#include <cmpiutil/cmpiUtils.h>
#include <cmpiutil/cmpiSimpleAssoc.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
}
#endif

/* Local includes  */
#include "Utils.h"
#include "ArrayProvider.h"
#include "StorageConfigurationService.h"
#include "StoragePool.h"
#include "StorageCapability.h"
#include "StorageExtent.h"
#include "StorageVolume.h"
#include "StorageSetting.h"
#include <y2storage/StorageInterface.h>

using namespace storage;

static const char* ClassKeysAD[] = { "Antecedent", 	"Dependent", NULL };
static const char* ClassKeysMC[] = { "ManagedElement",  "Capabilities", NULL };
static const char* ClassKeysMS[] = { "ManagedElement",  "SettingData", NULL };
static const char* ClassKeysGP[] = { "GroupComponent",  "PartComponent", NULL };

/* Global handle to the CIM broker. This is initialized by the CIMOM when the provider is loaded */
const CMPIBroker * _BROKER;

const StorageInterface* s;


// ----------------------------------------------------------------------------
// HELPER FUNCTIONS
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// CMPI INSTANCE PROVIDER FUNCTIONS
// ----------------------------------------------------------------------------

/* EnumInstanceNames() - return a list of all the instances names (i.e. return their object paths only) */
static CMPIStatus EnumInstanceNames(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self') */
		const CMPIContext * context,		/* [in] Additional context info, if any */
		const CMPIResult * results,			/* [out] Results of this operation */
		const CMPIObjectPath * reference)	/* [in] Contains the CIM namespace and classname */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char * ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("EnumInstanceNames() called, className = %s", className));

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns, &status);
		if (status.rc != CMPI_RC_OK)
		{
			goto exit;
		}
	}
	//
	// Handle object enumerations
	//
	if (strcasecmp(className, StorageConfigurationServiceClassName) == 0)
	{
		CMPIObjectPath* cop = SCSCreateObjectPath(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop); 
	}
	else if (strcasecmp(className, StorageConfigurationCapabilitiesClassName) == 0)
	{
		CMPIObjectPath* cop = SCCCreateObjectPath(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop); 
	}
	else if (strcasecmp(className, SystemStorageCapabilitiesClassName) == 0)
	{
		CMPIObjectPath* cop = SSCCreateObjectPath(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop); 
	}
	else if (strcasecmp(className, StorageCapabilitiesClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < CapabilitiesSize(); i++)
		{
			StorageCapability *cap = CapabilitiesGet(i);
			CMPIObjectPath* cop = CapabilityCreateObjectPath(cap, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
			{
				goto exit;
			}
		    CMReturnObjectPath(results, cop); 
		}
	}
	else if (strcasecmp(className, StoragePoolClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			CMPIObjectPath* cop = PoolCreateObjectPath(pool, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
			{
				goto exit;
			}
		    CMReturnObjectPath(results, cop); 
		}
	}
	else if (strcasecmp(className, StorageExtentClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			if (!extent->composite)
			{
				CMPIObjectPath* cop = ExtentCreateObjectPath(extent, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, CompositeExtentClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			_SMI_TRACE(1,("EnumInstanceNames() called, Extent Name :%s Extent Composite %d", extent->name,extent->composite));
			if (extent->composite)
			{
				CMPIObjectPath* cop = ExtentCreateObjectPath(extent, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, LogicalDiskClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIObjectPath* cop = VolumeCreateObjectPath(className,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop);
			}
		}
	}
	else if (strcasecmp(className, StorageVolumeClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(!vol->IsLD)
			{
				CMPIObjectPath* cop = VolumeCreateObjectPath(className,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, StorageSettingWithHintsClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < SettingsSize(); i++)
		{
			StorageSetting *setting = SettingsGet(i);
			CMPIObjectPath* cop = SettingCreateObjectPath(setting, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
			{
				goto exit;
			}
		    CMReturnObjectPath(results, cop); 
		}
	}

	//
	// Handle association enumerations
	//

	else if (strcasecmp(className, HostedStorageConfigurationServiceClassName) == 0)
	{
		CMPIObjectPath *assocCop = SCSCreateHostedAssocObjectPath(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 
	}
	else if (strcasecmp(className, HostedStoragePoolClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			CMPIObjectPath* cop = PoolCreateHostedAssocObjectPath(pool, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
			{
				goto exit;
			}
		    CMReturnObjectPath(results, cop); 
		}
	}
	else if (strcasecmp(className, LogicalDiskDeviceClassName) == 0) 
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIObjectPath* cop = VolumeCreateDeviceAssocObjectPath(className,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
			    CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if(strcasecmp(className, StorageVolumeDeviceClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(!vol->IsLD)
			{
				CMPIObjectPath* cop = VolumeCreateDeviceAssocObjectPath(className,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
			    CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, StorageConfigurationElementCapabilitiesClassName) == 0)
	{
		// First return the fixed association path we know exists between the 
		// StorageConfigurationService and the StorageConfigurationCapabilities

		CMPIObjectPath *assocCop = SCCCreateAssocObjectPath(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		// Now return the fixed association path we know exists between the 
		// StorageConfigurationService and the SystemStorageCapabilities
		assocCop = SSCCreateAssocObjectPath(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 
	}
	else if (strcasecmp(className, StorageElementCapabilitiesClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			if (pool->capability)
			{
				CMPIObjectPath* cop = PoolCreateCapabilityAssocObjectPath(pool, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, AssociatedComponentExtentClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			if (pool)
			{
				for (j = 0; j < PoolExtentsSize(pool); j++)
				{
					if (!ExtentIsFreespace(pool->concreteComps[j]))
					{
						CMPIObjectPath* cop = PoolCreateComponentAssocObjectPath(pool, pool->concreteComps[j], ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}
			}
		}
	}
	else if (strcasecmp(className, AssociatedRemainingExtentClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			if (pool)
			{
				for (j = 0; j < PoolExtentsSize(pool); j++)
				{
					if (ExtentIsFreespace(pool->concreteComps[j]))
					{
						CMPIObjectPath* cop = PoolCreateRemainingAssocObjectPath(pool, pool->concreteComps[j], ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}
			}
		}
	}
	else if (strcasecmp(className, StorageSettingsGeneratedFromCapabilitiesClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < SettingsSize(); i++)
		{
			StorageSetting *setting = SettingsGet(i);
			if (setting->capability != NULL && setting->volume == NULL)
			{
				CMPIObjectPath* cop = SettingCreateGFCAssocObjectPath(setting, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, BasedOnClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			StorageCapability *cap = NULL;
			if (extent->pool != NULL)
			{
				cap = extent->pool->capability;
			}
			
			// Filter out CompositeExtentBasedOn case
			if (!extent->composite || cap == NULL || cap->extentStripe <= 1)
			{
				for (j = 0; j < extent->maxNumAntecedents; j++)
				{
					if (extent->antecedents[j])
					{
						CMPIObjectPath* cop = ExtentCreateBasedOnAssocObjectPath(
													extent, 
													extent->antecedents[j]->extent, 
													ns, 
													className,
													&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}
			}
		}

		// Need to do LogicalDisks and StorageVolume also
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIObjectPath* cop = VolumeCreateBasedOnAssocObjectPath(
											LogicalDiskClassName,
											vol, 
											vol->antecedentExtent.extent,
											ns, 
											&status);

				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
		    		CMReturnObjectPath(results, cop);
			}
			else
			{
				CMPIObjectPath* cop = VolumeCreateBasedOnAssocObjectPath(
											StorageVolumeClassName,
											vol, 
											vol->antecedentExtent.extent,
											ns, 
											&status);

				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
				CMReturnObjectPath(results, cop); 
			}
		}
	}
	else if (strcasecmp(className, CompositeExtentBasedOnClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			StorageCapability *cap = NULL;
			if (extent->pool != NULL)
			{
				cap = extent->pool->capability;
			}
			
			// Filter out vanilla BasedOn case
			if (extent->composite && cap != NULL && cap->extentStripe > 1)
			{
				for (j = 0; j < extent->maxNumAntecedents; j++)
				{
					if (extent->antecedents[j])
					{
						CMPIObjectPath* cop = ExtentCreateBasedOnAssocObjectPath(
													extent, 
													extent->antecedents[j]->extent, 
													ns, 
													className,
													&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}
			}
		}
	}
/*	else if (strcasecmp(className, StorageVolumeBasedOnClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			StorageCapability *cap = NULL;
			if (extent->pool != NULL)
			{
				cap = extent->pool->capability;
			}
			
			// Filter out CompositeExtentBasedOn case
			if (!extent->composite || cap == NULL || cap->extentStripe <= 1)
			{
				for (j = 0; j < extent->maxNumAntecedents; j++)
				{
					if (extent->antecedents[j])
					{
						CMPIObjectPath* cop = ExtentCreateBasedOnAssocObjectPath(
													extent, 
													extent->antecedents[j]->extent, 
													ns, 
													className,
													&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}
			}
		}

		// Need to do LogicalDisks also
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);

			CMPIObjectPath* cop = VolumeCreateBasedOnAssocObjectPath(
										className,
										vol, 
										vol->antecedentExtent.extent,
										ns, 
										&status);

			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
			{
				goto exit;
			}
		    CMReturnObjectPath(results, cop); 
		}
	}
*/
	else if (strcasecmp(className, AllocatedFromStoragePoolClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *antPool = PoolsGet(i);
			if (antPool)
			{
				// First do pools
				for (j = 0; j < antPool->maxDepPools; j++)
				{
					if (antPool->dependentPools[j])
					{
						StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
						CMPIObjectPath* cop = PoolCreateAllocFromAssocObjectPath(className,depPool, antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}

				// Now do volumes/logical-disks
				for (j = 0; j < antPool->maxDepVolumes; j++)
				{
					if (antPool->dependentVolumes[j])
					{
						StorageVolume *depVolume = (StorageVolume *)antPool->dependentVolumes[j]->element;
						if(depVolume->IsLD)
						{
							CMPIObjectPath* cop = VolumeCreateAllocFromAssocObjectPath(LogicalDiskClassName,depVolume, antPool, ns, &status);
							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
							{
								goto exit;
							}
							CMReturnObjectPath(results, cop);
						}
						else
						{
							CMPIObjectPath* cop = VolumeCreateAllocFromAssocObjectPath(StorageVolumeClassName,depVolume, antPool, ns, &status);
							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
							{
								goto exit;
							}
							CMReturnObjectPath(results, cop);
						}
					}
				}
			}
		}
	}
/*	else if (strcasecmp(className, StorageVolumeAllocatedFromStoragePoolClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *antPool = PoolsGet(i);
			if (antPool)
			{
				// First do pools
				for (j = 0; j < antPool->maxDepPools; j++)
				{
					if (antPool->dependentPools[j])
					{
						StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
						CMPIObjectPath* cop = PoolCreateAllocFromAssocObjectPath(className,depPool, antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}

				// Now do volumes/logical-disks
				for (j = 0; j < antPool->maxDepVolumes; j++)
				{
					if (antPool->dependentVolumes[j])
					{
						StorageVolume *depVolume = (StorageVolume *)antPool->dependentVolumes[j]->element;
						CMPIObjectPath* cop = VolumeCreateAllocFromAssocObjectPath(className, depVolume, antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
						{
							goto exit;
						}
						CMReturnObjectPath(results, cop); 
					}
				}
			}
		}
	}
*/
	else if (strcasecmp(className, StorageElementSettingDataClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIObjectPath* cop = VolumeCreateSettingAssocObjectPath(LogicalDiskClassName,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
			    CMReturnObjectPath(results, cop);
			}
			else
			{
				CMPIObjectPath* cop = VolumeCreateSettingAssocObjectPath(StorageVolumeClassName,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
				{
					goto exit;
				}
			    CMReturnObjectPath(results, cop);
			}
		}
	}
/*	else if (strcasecmp(className, StorageVolumeStorageElementSettingDataClassName) == 0)
	{
		_SMI_TRACE(1,("EnumInstanceNames() called, className = %s", className));
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			CMPIObjectPath* cop = VolumeCreateSettingAssocObjectPath(className,vol, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
			{
				goto exit;
			}
		    CMReturnObjectPath(results, cop); 
		}
	}
*/

	/* Finished */
	CMReturnDone(results);
exit:
	_SMI_TRACE(1,("EnumInstanceNames() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* EnumInstances() - return a list of all the instances (i.e. return all the instance data) */
static CMPIStatus EnumInstances(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self') */
		const CMPIContext * context,		/* [in] Additional context info, if any */
		const CMPIResult * results,			/* [out] Results of this operation */
		const CMPIObjectPath * reference,	/* [in] Contains the CIM namespace and classname */
		const char ** properties)			/* [in] List of desired properties (NULL=all) */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char *ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("EnumInstances() called, className = %s", className));

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns, &status);
		if (status.rc != CMPI_RC_OK)
		{
			goto exit;
		}
	}
	//
	// Handle object enumerations
	//
	if (strcasecmp(className, StorageConfigurationServiceClassName) == 0)
	{
		CMPIInstance* ci = SCSCreateInstance(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	else if (strcasecmp(className, StorageConfigurationCapabilitiesClassName) == 0)
	{
		CMPIInstance* ci = SCCCreateInstance(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	else if (strcasecmp(className, SystemStorageCapabilitiesClassName) == 0)
	{
		CMPIInstance* ci = SSCCreateInstance(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	else if (strcasecmp(className, StorageCapabilitiesClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < CapabilitiesSize(); i++)
		{
			StorageCapability *cap = CapabilitiesGet(i);
			CMPIInstance *ci = CapabilityCreateInstance(cap, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
	}
	else if (strcasecmp(className, StoragePoolClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			CMPIInstance *ci = PoolCreateInstance(pool, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
	}
	else if (strcasecmp(className, StorageExtentClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			if (!extent->composite)
			{
				CMPIInstance* ci = ExtentCreateInstance(extent, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, CompositeExtentClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			if (extent->composite)
			{
				CMPIInstance* ci = ExtentCreateInstance(extent, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, LogicalDiskClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIInstance* ci = VolumeCreateInstance(className,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci);
			}
		}
	}
	else if (strcasecmp(className, StorageVolumeClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(!vol->IsLD)
			{
				CMPIInstance* ci = VolumeCreateInstance(className,vol, ns, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, StorageSettingWithHintsClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < SettingsSize(); i++)
		{
			StorageSetting *setting = SettingsGet(i);
			CMPIInstance *ci = SettingCreateInstance(setting, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
	}

	//
	// Handle association enumerations
	//

	else if (strcasecmp(className, HostedStorageConfigurationServiceClassName) == 0)
	{
		CMPIInstance *ci = SCSCreateHostedAssocInstance(ns, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 
	}
	else if (strcasecmp(className, HostedStoragePoolClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			CMPIInstance *ci = PoolCreateHostedAssocInstance(pool, ns, properties, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
	}
	else if (strcasecmp(className, LogicalDiskDeviceClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIInstance* ci = VolumeCreateDeviceAssocInstance(className,vol, ns, properties, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
			    CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, StorageVolumeDeviceClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(!vol->IsLD)
			{
				CMPIInstance* ci = VolumeCreateDeviceAssocInstance(className,vol, ns, properties, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
			    CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, StorageConfigurationElementCapabilitiesClassName) == 0)
	{
		// First return the fixed association we know exists between the 
		// StorageConfigurationService and the Storage ConfigurationCapabilities

		CMPIInstance *ci = SCCCreateAssocInstance(ns, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		// Now return the fixed association we know exists between the 
		// StorageConfigurationService and the SystemStorageCapabilities
		ci = SSCCreateAssocInstance(ns, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 
	}
	else if (strcasecmp(className, StorageElementCapabilitiesClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			if (pool->capability)
			{
				CMPIInstance* ci = PoolCreateCapabilityAssocInstance(pool, ns, properties, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, AssociatedComponentExtentClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			for (j = 0; j < PoolExtentsSize(pool); j++)
			{
				if (!ExtentIsFreespace(pool->concreteComps[j]))
				{
					CMPIInstance* ci = PoolCreateComponentAssocInstance(pool, pool->concreteComps[j], ns, properties, &status);
					if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
					{
						goto exit;
					}
					CMReturnInstance(results, ci); 
				}
			}
		}
	}
	else if (strcasecmp(className, AssociatedRemainingExtentClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *pool = PoolsGet(i);
			for (j = 0; j < PoolExtentsSize(pool); j++)
			{
				if (ExtentIsFreespace(pool->concreteComps[j]))
				{
					CMPIInstance* ci = PoolCreateRemainingAssocInstance(pool, pool->concreteComps[j], ns, properties, &status);
					if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
					{
						goto exit;
					}
					CMReturnInstance(results, ci); 
				}
			}
		}
	}
	else if (strcasecmp(className, StorageSettingsGeneratedFromCapabilitiesClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < SettingsSize(); i++)
		{
			StorageSetting *setting = SettingsGet(i);
			if (setting->capability != NULL && setting->volume == NULL)
			{
				CMPIInstance* ci = SettingCreateGFCAssocInstance(setting, ns, properties, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 
			}
		}
	}
	else if (strcasecmp(className, BasedOnClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			StorageCapability *cap = NULL;
			if (extent->pool != NULL)
			{
				cap = extent->pool->capability;
			}
			
			// Filter out CompositeExtentBasedOn case
			if (!extent->composite || cap == NULL || cap->extentStripe <= 1)
			{
				for (j = 0; j < extent->maxNumAntecedents; j++)
				{
					if (extent->antecedents[j])
					{
						CMPIInstance* ci = ExtentCreateBasedOnAssocInstance(
													extent,
													extent->antecedents[j]->extent,
													extent->antecedents[j], 
													ns, 
													properties,
													className,
													&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci); 
					}
				}
			}
		}

		// Need to do StorageVolumes/LogicalDisks also
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{

				CMPIInstance* ci = VolumeCreateBasedOnAssocInstance(LogicalDiskClassName,
											vol,
											vol->antecedentExtent.extent,
											&vol->antecedentExtent, 
											ns, 
											properties,
											&status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 

			}
			else
			{
				CMPIInstance* ci = VolumeCreateBasedOnAssocInstance(StorageVolumeClassName,
											vol,
											vol->antecedentExtent.extent,
											&vol->antecedentExtent, 
											ns, 
											properties,
											&status);

				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
				CMReturnInstance(results, ci); 
			}
		}
	}
/*	else if (strcasecmp(className, StorageVolumeBasedOnClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			StorageCapability *cap = NULL;
			if (extent->pool != NULL)
			{
				cap = extent->pool->capability;
			}
			
			// Filter out CompositeExtentBasedOn case
			if (!extent->composite || cap == NULL || cap->extentStripe <= 1)
			{
				for (j = 0; j < extent->maxNumAntecedents; j++)
				{
					if (extent->antecedents[j])
					{
						CMPIInstance* ci = ExtentCreateBasedOnAssocInstance(
													extent,
													extent->antecedents[j]->extent,
													extent->antecedents[j], 
													ns, 
													properties,
													className,
													&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci); 
					}
				}
			}
		}

		// Need to do StorageVolumes also
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);

			CMPIInstance* ci = VolumeCreateBasedOnAssocInstance(className,
										vol,
										vol->antecedentExtent.extent,
										&vol->antecedentExtent, 
										ns, 
										properties,
										&status);

			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci); 
		}
	}
*/
	else if (strcasecmp(className, CompositeExtentBasedOnClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < ExtentsSize(); i++)
		{
			StorageExtent *extent = ExtentsGet(i);
			StorageCapability *cap = NULL;
			if (extent->pool != NULL)
			{
				cap = extent->pool->capability;
			}
			
			// Filter out CompositeExtentBasedOn case
			if (extent->composite && cap != NULL && cap->extentStripe > 1)
			{
				for (j = 0; j < extent->maxNumAntecedents; j++)
				{
					if (extent->antecedents[j])
					{
						CMPIInstance* ci = ExtentCreateBasedOnAssocInstance(
													extent, 
													extent->antecedents[j]->extent,
													extent->antecedents[j], 
													ns, 
													properties,
													className,
													&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci); 
					}
				}
			}
		}
	}
	else if (strcasecmp(className, AllocatedFromStoragePoolClassName) == 0)
	{
		
		_SMI_TRACE(1,("EnumInstances() :AllocatedFromStoragePool"));
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *antPool = PoolsGet(i);
			if (antPool)
			{
				// First do pools
				for (j = 0; j < antPool->maxDepPools; j++)
				{
					if (antPool->dependentPools[j])
					{
						StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
						CMPIInstance* ci = PoolCreateAllocFromAssocInstance(
className,
												depPool, 
												antPool, 
												antPool->dependentPools[j]->spaceConsumed,
												ns, 
												properties,
												&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci); 
					}
				}

				// Now do volumes
				for (j = 0; j < antPool->maxDepVolumes; j++)
				{
					if (antPool->dependentVolumes[j])
					{
						StorageVolume *depVolume = (StorageVolume *)antPool->dependentVolumes[j]->element;
						if(depVolume->IsLD)
						{
							CMPIInstance* ci = VolumeCreateAllocFromAssocInstance(LogicalDiskClassName,
													depVolume, 
													antPool, 
													antPool->dependentVolumes[j]->spaceConsumed,
													ns, 
													properties,
													&status);

							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
							{
								goto exit;
							}
							CMReturnInstance(results, ci);
						}
						else
						{
							CMPIInstance* ci = VolumeCreateAllocFromAssocInstance(StorageVolumeClassName,
													depVolume, 
													antPool, 
													antPool->dependentVolumes[j]->spaceConsumed,
													ns, 
													properties,
													&status);

							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
							{
								goto exit;
							}
							CMReturnInstance(results, ci);
						}
					}
				}
			}
		}
	}
/*	else if (strcasecmp(className, StorageVolumeAllocatedFromStoragePoolClassName) == 0)
	{
		CMPICount i, j;
		for (i = 0; i < PoolsSize(); i++)
		{
			StoragePool *antPool = PoolsGet(i);
			if (antPool)
			{
				// First do pools
				for (j = 0; j < antPool->maxDepPools; j++)
				{
					if (antPool->dependentPools[j])
					{
						StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
						CMPIInstance* ci = PoolCreateAllocFromAssocInstance(
className,
												depPool, 
												antPool, 
												antPool->dependentPools[j]->spaceConsumed,
												ns, 
												properties,
												&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci); 
					}
				}

				// Now do volumes
				for (j = 0; j < antPool->maxDepVolumes; j++)
				{
					if (antPool->dependentVolumes[j])
					{
						StorageVolume *depVolume = (StorageVolume *)antPool->dependentVolumes[j]->element;
						CMPIInstance* ci = VolumeCreateAllocFromAssocInstance(className,
												depVolume, 
												antPool, 
												antPool->dependentVolumes[j]->spaceConsumed,
												ns, 
												properties,
												&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci); 
					}
				}
			}
		}
	}
*/
	else if (strcasecmp(className, StorageElementSettingDataClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			if(vol->IsLD)
			{
				CMPIInstance* ci = VolumeCreateSettingAssocInstance(LogicalDiskClassName,vol, ns, properties, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
			    CMReturnInstance(results, ci);
			}
			else
			{
				CMPIInstance* ci = VolumeCreateSettingAssocInstance(StorageVolumeClassName,vol, ns, properties, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
				{
					goto exit;
				}
			    CMReturnInstance(results, ci);
			}
		}
	}
/*	else if (strcasecmp(className, StorageVolumeStorageElementSettingDataClassName) == 0)
	{
		CMPICount i;
		for (i = 0; i < VolumesSize(); i++)
		{
			StorageVolume *vol = VolumesGet(i);
			CMPIInstance* ci = VolumeCreateSettingAssocInstance(className, vol, ns, properties, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
		    CMReturnInstance(results, ci); 
		}
	}
*/
	/* Finished */
	CMReturnDone(results);
exit:
	_SMI_TRACE(1,("EnumInstances() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
return status;
}


// ----------------------------------------------------------------------------


/* GetInstance() -  return the instance data for the specified instance only */
static CMPIStatus GetInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self') */
		const CMPIContext * context,		/* [in] Additional context info, if any */
		const CMPIResult * results,			/* [out] Results of this operation */
		const CMPIObjectPath * reference,	/* [in] Contains the CIM namespace, classname and desired object path */
		const char ** properties)			/* [in] List of desired properties (NULL=all) */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char * ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("GetInstance() called"));

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns, &status);
		if (status.rc != CMPI_RC_OK)
		{
			goto exit;
		}
	}
	_SMI_TRACE(1,("GetInstance(): Before classes "));
	_SMI_TRACE(1,("GetInstance(): classname:%s ",className));
	//
	// Get object instance
	//
	if (strcasecmp(className, StorageConfigurationServiceClassName) == 0)
	{
		CMPIInstance* ci = SCSCreateInstance(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	else if (strcasecmp(className, StorageConfigurationCapabilitiesClassName) == 0)
	{
		CMPIInstance* ci = SCCCreateInstance(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	else if (strcasecmp(className, SystemStorageCapabilitiesClassName) == 0)
	{
		CMPIInstance* ci = SSCCreateInstance(ns, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	else if (strcasecmp(className, StorageCapabilitiesClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to get capability with instanceID = %s", instanceID));

		StorageCapability *cap = CapabilitiesFind(instanceID);
		if (cap != NULL)
		{
			CMPIInstance *ci = CapabilityCreateInstance(cap, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified StorageCapabilities object not found in system");
			goto exit;
		}
	}
	else if (strcasecmp(className, StoragePoolClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to get pool with instanceID = %s", instanceID));

		StoragePool *pool = PoolsFind(instanceID);
		if (pool != NULL)
		{
			CMPIInstance *ci = PoolCreateInstance(pool, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified StoragePool object not found in system");
			goto exit;
		}
	}
	else if (strcasecmp(className, StorageExtentClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "DeviceID", NULL);
		const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to get extent with deviceID = %s", deviceID));

		StorageExtent *extent = ExtentsFind(deviceID);
		if (extent != NULL && !extent->composite)
		{
			CMPIInstance *ci = ExtentCreateInstance(extent, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified StorageExtent object not found in system");
			goto exit;
		}
	}
	else if (strcasecmp(className, CompositeExtentClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "DeviceID", NULL);
		const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to get extent with deviceID = %s", deviceID));

		StorageExtent *extent = ExtentsFind(deviceID);
		if (extent != NULL && extent->composite)
		{
			CMPIInstance *ci = ExtentCreateInstance(extent, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified CompositeExtent object not found in system");
			goto exit;
		}
	}
	else if (strcasecmp(className, LogicalDiskClassName) == 0 || strcasecmp(className, StorageVolumeClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "DeviceID", NULL);
		const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to get logical disk/Storage Volume with deviceID = %s", deviceID));

		StorageVolume *vol = VolumesFind(deviceID);
		if (vol != NULL)
		{
			CMPIInstance *ci = VolumeCreateInstance(className,vol, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified LogicalDisk/Storage Volume  object not found in system");
			goto exit;
		}
	}
	else if (strcasecmp(className, StorageSettingWithHintsClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to get setting with instanceID = %s", instanceID));

		StorageSetting *setting = SettingsFind(instanceID);
		if (setting != NULL)
		{
			CMPIInstance *ci = SettingCreateInstance(setting, ns, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified StorageSetting object not found in system");
			goto exit;
		}
	}

	//
	// Get association instance
	//

	else if ( (strcasecmp(className, HostedStorageConfigurationServiceClassName) == 0) ||
			  (strcasecmp(className, StorageSettingsGeneratedFromCapabilitiesClassName) == 0) )
	{
		CMPIData leftkey = CMGetKey(reference, "Antecedent", &status);
		CMPIData rightkey = CMGetKey(reference, "Dependent", &status);
		if (!CMIsNullValue(leftkey) && !CMIsNullValue(rightkey))
		{
			CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
												ns,
												className,
												ClassKeysAD,
												properties,
												"Antecedent",
												"Dependent",
												leftkey.value.ref,
												rightkey.value.ref,
												&status);
			CMReturnInstance(results, assocInst);
		}
	}
	else if ( (strcasecmp(className, HostedStoragePoolClassName) == 0) ||
			  (strcasecmp(className, LogicalDiskDeviceClassName) == 0) ||
			  (strcasecmp(className, StorageVolumeDeviceClassName) == 0) ||
			  (strcasecmp(className, AssociatedComponentExtentClassName) == 0) ||
			  (strcasecmp(className, AssociatedRemainingExtentClassName) == 0) )
	{
		CMPIData leftkey = CMGetKey(reference, "GroupComponent", &status);
		CMPIData rightkey = CMGetKey(reference, "PartComponent", &status);
		if (!CMIsNullValue(leftkey) && !CMIsNullValue(rightkey))
		{
			CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
												ns,
												className,
												ClassKeysGP,
												properties,
												"GroupComponent",
												"PartComponent",
												leftkey.value.ref,
												rightkey.value.ref,
												&status);
			CMReturnInstance(results, assocInst);
		}
	}
	else if ( (strcasecmp(className, StorageConfigurationElementCapabilitiesClassName) == 0) ||
			  (strcasecmp(className, StorageElementCapabilitiesClassName) == 0) )
	{
		CMPIData leftkey = CMGetKey(reference, "ManagedElement", &status);
		CMPIData rightkey = CMGetKey(reference, "Capabilities", &status);
		if (!CMIsNullValue(leftkey) && !CMIsNullValue(rightkey))
		{
			CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
												ns,
												className,
												ClassKeysMC,
												properties,
												"ManagedElement",
												"Capabilities",
												leftkey.value.ref,
												rightkey.value.ref,
												&status);
			CMReturnInstance(results, assocInst);
		}
	}
	else if (strcasecmp(className, BasedOnClassName) == 0 ||
			 strcasecmp(className, CompositeExtentBasedOnClassName) == 0)
//			strcasecmp(className, StorageVolumeBasedOnClassName) == 0 )
	{
		CMPIData keyData = CMGetKey(reference, "Dependent", &status);
		if (CMIsNullValue(keyData))
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_INVALID_PARAMETER, "Dependent key must be specified.");
			goto exit;
		}
		CMPIObjectPath *depExtCop = keyData.value.ref;
		keyData = CMGetKey(depExtCop, "DeviceID", NULL);
		const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Dependent = %s", deviceID));
		StorageExtent *depExt = ExtentsFind(deviceID);
		if (depExt != NULL)
		{
			keyData = CMGetKey(reference, "Antecedent", &status);
			if (CMIsNullValue(keyData))
			{

				CMSetStatusWithChars(
					_BROKER, 
					&status, 
					CMPI_RC_ERR_INVALID_PARAMETER, "Antecedent key must be specified.");
				goto exit;
			}
			CMPIObjectPath *antExtCop = keyData.value.ref;
			keyData = CMGetKey(antExtCop, "DeviceID", NULL);
			deviceID = CMGetCharsPtr(keyData.value.string, NULL);
			_SMI_TRACE(1,("Antecedent = %s", deviceID));
			CMPICount j;
			for (j = 0; j < depExt->maxNumAntecedents; j++)
			{
				if (depExt->antecedents[j])
				{
					if (strcmp(deviceID, depExt->antecedents[j]->extent->deviceID) == 0)
					{
						CMPIInstance *ci = ExtentCreateBasedOnAssocInstance(
												depExt, 
												depExt->antecedents[j]->extent,
												depExt->antecedents[j],
												ns,
												properties,
												className,
												&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci);
						break;
					}
				}
			}
		}
		else if (strcasecmp(className, BasedOnClassName) == 0)
		{
			// See if Dependent is a StorageVolume/LogicalDisk rather than extent
			StorageVolume *depVol = VolumesFind(deviceID);
			if (depVol != NULL)
			{
				keyData = CMGetKey(reference, "Antecedent", &status);
				if (CMIsNullValue(keyData))
				{
	
					CMSetStatusWithChars(
						_BROKER, 
						&status, 
						CMPI_RC_ERR_INVALID_PARAMETER, "Antecedent key must be specified.");
					goto exit;
				}
				CMPIObjectPath *antExtCop = keyData.value.ref;
				keyData = CMGetKey(antExtCop, "DeviceID", NULL);
				deviceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Antecedent = %s", deviceID));
	
				if (strcmp(deviceID, depVol->antecedentExtent.extent->deviceID) == 0)
				{
					if(depVol->IsLD)
					{
						CMPIInstance *ci = VolumeCreateBasedOnAssocInstance(
												LogicalDiskClassName,
												depVol, 
												depVol->antecedentExtent.extent,
												&depVol->antecedentExtent,
												ns,
												properties,
												&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci);
					}
					else
					{
						CMPIInstance *ci = VolumeCreateBasedOnAssocInstance(
												StorageVolumeClassName,
												depVol, 
												depVol->antecedentExtent.extent,
												&depVol->antecedentExtent,
												ns,
												properties,
												&status);

						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
						{
							goto exit;
						}
						CMReturnInstance(results, ci);
					}
				}
			}
		}
/*		else if (strcasecmp(className, StorageVolumeBasedOnClassName) == 0)
		{
			// See if Dependent is a StorageVolume/LogicalDisk rather than extent
			StorageVolume *depVol = VolumesFind(deviceID);
			if (depVol != NULL)
			{
				keyData = CMGetKey(reference, "Antecedent", &status);
				if (CMIsNullValue(keyData))
				{

					CMSetStatusWithChars(
						_BROKER, 
						&status, 
						CMPI_RC_ERR_INVALID_PARAMETER, "Antecedent key must be specified.");
					goto exit;
				}
				CMPIObjectPath *antExtCop = keyData.value.ref;
				keyData = CMGetKey(antExtCop, "DeviceID", NULL);
				deviceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Antecedent = %s", deviceID));

				if (strcmp(deviceID, depVol->antecedentExtent.extent->deviceID) == 0)
				{
					CMPIInstance *ci = VolumeCreateBasedOnAssocInstance(
											className,
											depVol, 
											depVol->antecedentExtent.extent,
											&depVol->antecedentExtent,
											ns,
											properties,
											&status);

					if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
					{
						goto exit;
					}
					CMReturnInstance(results, ci);
				}
			}
		}
*/
	}
	else if (strcasecmp(className, AllocatedFromStoragePoolClassName) == 0)
	{
		_SMI_TRACE(1,("OMC_AllocatedFromStoragePool() Entry"));
		CMPIData keyData = CMGetKey(reference, "Antecedent", &status);
		if (CMIsNullValue(keyData))
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_INVALID_PARAMETER, "Antecedent key must be specified.");
			goto exit;
		}
		CMPIObjectPath *antPoolCop = keyData.value.ref;
		keyData = CMGetKey(antPoolCop, "InstanceID", &status);
		_SMI_TRACE(1,("OMC_AllocatedFromStoragePool(): Got the antecedent's instance ID"));
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Antecedent: InstanceID = %s", instanceID));
		StoragePool *antPool = PoolsFind(instanceID);
		if (antPool != NULL)
		{
			keyData = CMGetKey(reference, "Dependent", &status);
			if (CMIsNullValue(keyData))
			{

				CMSetStatusWithChars(
					_BROKER, 
					&status, 
					CMPI_RC_ERR_INVALID_PARAMETER, "Dependent key must be specified.");
				goto exit;
			}
			CMPIObjectPath *depPoolCop = keyData.value.ref;

			const char *depClassName = CMGetCharPtr(CMGetClassName(depPoolCop, NULL));
			_SMI_TRACE(1,("\tdepClassName = %s", depClassName));
			if (strcasecmp(depClassName, StoragePoolClassName) == 0)
			{
				keyData = CMGetKey(depPoolCop, "InstanceID", NULL);
				instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("DependentPool = %s", instanceID));
				CMPICount j;
				for (j = 0; j < antPool->maxDepPools; j++)
				{
					if (antPool->dependentPools[j])
					{
						StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
						_SMI_TRACE(1,("Looking for InstanceID match: instanceID = %s, depPool->instanceID = %s", instanceID, depPool->instanceID));

						if (strcmp(instanceID, depPool->instanceID) == 0)
						{
							CMPIInstance *ci = PoolCreateAllocFromAssocInstance(
     													className,
													depPool, 
													antPool,
													antPool->dependentPools[j]->spaceConsumed,
													ns,
													properties,
													&status);

							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
							{
								goto exit;
							}
							CMReturnInstance(results, ci);
							break;
						}
					}
				}
			}
			else if (strcasecmp(depClassName, LogicalDiskClassName) == 0 ||
				strcasecmp(depClassName, StorageVolumeClassName) == 0)
			{
				// Handle OMC_LogicalDisk/StorageVolume case 
				CMPIObjectPath *depVolCop = keyData.value.ref;
				keyData = CMGetKey(depVolCop, "DeviceID", NULL);
				const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("DependentVol = %s", deviceID));
				CMPICount j;
				for (j = 0; j < antPool->maxDepVolumes; j++)
				{
					if (antPool->dependentVolumes[j])
					{
						StorageVolume *depVolume = (StorageVolume *)antPool->dependentVolumes[j]->element;
						_SMI_TRACE(1,("Looking for DeviceID match: deviceID = %s, depVolume->deviceID = %s", deviceID, depVolume->deviceID));

						if (strcmp(deviceID, depVolume->deviceID) == 0)
						{
							if(depVolume->IsLD)
							{
								CMPIInstance *ci = VolumeCreateAllocFromAssocInstance(LogicalDiskClassName,
														depVolume, 
														antPool,
														antPool->dependentVolumes[j]->spaceConsumed,
														ns,
														properties,
														&status);

								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
								{
									goto exit;
								}
								CMReturnInstance(results, ci);
								break;
							}
							else
							{
								CMPIInstance *ci = VolumeCreateAllocFromAssocInstance(StorageVolumeClassName,
														depVolume, 
														antPool,
														antPool->dependentVolumes[j]->spaceConsumed,
														ns,
														properties,
														&status);

								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
								{
									goto exit;
								}
								CMReturnInstance(results, ci);
								break;
							}
						}
					}
				}
			}
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified antecedent object not found in system");
			goto exit;
		}
	}
/*	else if (strcasecmp(className, StorageVolumeAllocatedFromStoragePoolClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "Antecedent", &status);
		if (CMIsNullValue(keyData))
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_INVALID_PARAMETER, "Antecedent key must be specified.");
			goto exit;
		}
		CMPIObjectPath *antPoolCop = keyData.value.ref;
		keyData = CMGetKey(antPoolCop, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Antecedent = %s", instanceID));
		StoragePool *antPool = PoolsFind(instanceID);
		if (antPool != NULL)
		{
			keyData = CMGetKey(reference, "Dependent", &status);
			if (CMIsNullValue(keyData))
			{

				CMSetStatusWithChars(
					_BROKER, 
					&status, 
					CMPI_RC_ERR_INVALID_PARAMETER, "Dependent key must be specified.");
				goto exit;
			}
			CMPIObjectPath *depPoolCop = keyData.value.ref;

			const char *depClassName = CMGetCharPtr(CMGetClassName(depPoolCop, NULL));
			_SMI_TRACE(1,("\tdepClassName = %s", depClassName));
			if (strcasecmp(depClassName, StoragePoolClassName) == 0)
			{
				keyData = CMGetKey(depPoolCop, "InstanceID", NULL);
				instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("DependentPool = %s", instanceID));
				CMPICount j;
				for (j = 0; j < antPool->maxDepPools; j++)
				{
					if (antPool->dependentPools[j])
					{
						StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
						_SMI_TRACE(1,("Looking for InstanceID match: instanceID = %s, depPool->instanceID = %s", instanceID, depPool->instanceID));

						if (strcmp(instanceID, depPool->instanceID) == 0)
						{
							CMPIInstance *ci = PoolCreateAllocFromAssocInstance(
													className,
													depPool, 
													antPool,
													antPool->dependentPools[j]->spaceConsumed,
													ns,
													properties,
													&status);

							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
							{
								goto exit;
							}
							CMReturnInstance(results, ci);
							break;
						}
					}
				}
			}
			else if (strcasecmp(depClassName, StorageVolumeClassName) == 0)
			{
				// Handle OMC_StorageVolume case 
				CMPIObjectPath *depVolCop = keyData.value.ref;
				keyData = CMGetKey(depVolCop, "DeviceID", NULL);
				const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("DependentVol = %s", deviceID));
				CMPICount j;
				for (j = 0; j < antPool->maxDepVolumes; j++)
				{
					if (antPool->dependentVolumes[j])
					{
						StorageVolume *depVolume = (StorageVolume *)antPool->dependentVolumes[j]->element;
						_SMI_TRACE(1,("Looking for DeviceID match: deviceID = %s, depVolume->deviceID = %s", deviceID, depVolume->deviceID));

						if (strcmp(deviceID, depVolume->deviceID) == 0)
						{
							CMPIInstance *ci = VolumeCreateAllocFromAssocInstance(className,
													depVolume, 
													antPool,
													antPool->dependentVolumes[j]->spaceConsumed,
													ns,
													properties,
													&status);

							if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
							{
								goto exit;
							}
							CMReturnInstance(results, ci);
							break;
						}
					}
				}
			}
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified antecedent object not found in system");
			goto exit;
		}
	}
*/
	else if (strcasecmp(className, StorageElementSettingDataClassName) == 0 || strcasecmp(className, StorageVolumeStorageElementSettingDataClassName) == 0)
	{
		CMPIData leftkey = CMGetKey(reference, "ManagedElement", &status);
		CMPIData rightkey = CMGetKey(reference, "SettingData", &status);
		if (!CMIsNullValue(leftkey) && !CMIsNullValue(rightkey))
		{
			CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
												ns,
												className,
												ClassKeysMS,
												properties,
												"ManagedElement",
												"SettingData",
												leftkey.value.ref,
												rightkey.value.ref,
												&status);
			CMPIValue val;
			val.uint16 = 1;
			CMSetProperty(assocInst, "IsDefault", &val, CMPI_uint16);
			CMSetProperty(assocInst, "IsCurrent", &val, CMPI_uint16);
			CMReturnInstance(results, assocInst);
		}
	}
	else
	{
		CMSetStatusWithChars(
			_BROKER, 
			&status, 
			CMPI_RC_ERR_NOT_FOUND, "Specified object not found in system");
		goto exit;
	}

	/* Finished */
	CMReturnDone(results);
exit:
	_SMI_TRACE(1,("GetInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* CreateInstance() - create a new instance from the specified instance data. */
static CMPIStatus CreateInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference,	/* [in] Contains the target namespace, classname and objectpath. */
		const CMPIInstance * newinstance)	/* [in] Contains all the new instance data. */
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */
   
	_SMI_TRACE(1,("CreateInstance() called"));
	/* Instance creation not supported. */

	/* Finished. */
exit:
	_SMI_TRACE(1,("CreateInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* SetInstance() - save modified instance data for the specified instance. */
static CMPIStatus SetInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference,	/* [in] Contains the target namespace, classname and objectpath. */
		const CMPIInstance * newinstance,	/* [in] Contains all the new instance data. */
		const char ** properties)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char * ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("SetInstance() called, className = %s", className));

	/* We only support modifying StorageSettings */
	if (strcasecmp(className, StorageSettingWithHintsClassName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to modify setting with instanceID = %s", instanceID));

		StorageSetting *setting = SettingsFind(instanceID);
		if (setting != NULL)
		{
			CMPIData data = CMGetProperty(newinstance, "PackageRedundancyGoal", NULL);
			if (!CMIsNullValue(data))
				setting->packageRedundancyGoal = data.value.uint16;
			data = CMGetProperty(newinstance, "PackageRedundancyMax", NULL);
			if (!CMIsNullValue(data))
				setting->packageRedundancyMax = data.value.uint16;
			data = CMGetProperty(newinstance, "PackageRedundancyMin", NULL);
			if (!CMIsNullValue(data))
				setting->packageRedundancyMin = data.value.uint16;

			data = CMGetProperty(newinstance, "DataRedundancyGoal", NULL);
			if (!CMIsNullValue(data))
				setting->dataRedundancyGoal = data.value.uint16;
			data = CMGetProperty(newinstance, "DataRedundancyMax", NULL);
			if (!CMIsNullValue(data))
				setting->dataRedundancyMax = data.value.uint16;
			data = CMGetProperty(newinstance, "DataRedundancyMin", NULL);
			if (!CMIsNullValue(data))
				setting->dataRedundancyMin = data.value.uint16;

			data = CMGetProperty(newinstance, "UserDataStripeDepth", NULL);
			if (!CMIsNullValue(data))
				setting->userDataStripeDepth = data.value.uint64;
			data = CMGetProperty(newinstance, "UserDataStripeDepthMax", NULL);
			if (!CMIsNullValue(data))
				setting->userDataStripeDepthMax = data.value.uint64;
			data = CMGetProperty(newinstance, "UserDataStripeDepthMin", NULL);
			if (!CMIsNullValue(data))
				setting->userDataStripeDepthMin = data.value.uint64;

			data = CMGetProperty(newinstance, "ExtentStripeLength", NULL);
			if (!CMIsNullValue(data))
				setting->extentStripeLength = data.value.uint16;
			data = CMGetProperty(newinstance, "ExtentStripeLengthMax", NULL);
			if (!CMIsNullValue(data))
				setting->extentStripeLengthMax = data.value.uint16;
			data = CMGetProperty(newinstance, "ExtentStripeLengthMin", NULL);
			if (!CMIsNullValue(data))
				setting->extentStripeLengthMin = data.value.uint16;

			data = CMGetProperty(newinstance, "ParityLayout", NULL);
			if (!CMIsNullValue(data))
				setting->parityLayout = data.value.uint16;

			data = CMGetProperty(newinstance, "NoSinglePointOfFailure", NULL);
			if (!CMIsNullValue(data))
				setting->noSinglePointOfFailure = data.value.boolean;
		}
		else
		{
			CMSetStatusWithChars(
				_BROKER, 
				&status, 
				CMPI_RC_ERR_NOT_FOUND, "Specified StorageSetting object not found in system");
			goto exit;
		}
	}
	else
	{
		CMSetStatusWithChars(
			_BROKER, 
			&status, 
			CMPI_RC_ERR_NOT_SUPPORTED, "Instance modification not supported for class");
		goto exit;
	}

	/* Finished. */
exit:
	_SMI_TRACE(1,("SetInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* DeleteInstance() - delete/remove the specified instance. */
static CMPIStatus DeleteInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference)	/* [in] Contains the target namespace, classname and objectpath. */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations. */
	const char * ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */

	_SMI_TRACE(1,("DeleteInstance() called"));

	
	/* Finished. */
exit:
	_SMI_TRACE(1,("DeleteInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* ExecQuery() - return a list of all the instances that satisfy the desired query filter. */
static CMPIStatus ExecQuery(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference,	/* [in] Contains the target namespace and classname. */
		const char * language,				/* [in] Name of the query language (e.g. "WQL"). */ 
		const char * query)					/* [in] Text of the query, written in the query language. */ 
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */

	_SMI_TRACE(1,("ExecQuery() called"));

	/* Query filtering is not supported */

	/* Finished. */
exit:
	_SMI_TRACE(1,("ExecQuery() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------
// CMPI ASSOCIATION PROVIDER FUNCTIONS
// ----------------------------------------------------------------------------

// ****************************************************************************
// doReferences()
// 		This is the callback called from the SimpleAssociators helper functions
//		It "handles" one or more instances of the association class
// 		to be filtered by the SimpleAssociatior helper functions to return
// 		the correct object (instance or object path)
// ****************************************************************************
static CMPIStatus
doReferences(
		cmpiutilSimpleAssocCtx ctx,
		CMPIAssociationMI* self,
		const CMPIBroker *broker,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *assocClass,
		const char *resultClass,
		const char *role,
		const char *resultRole,
		const char** properties)
{
	_SMI_TRACE(1,("doReferences() called, assocClass: %s", assocClass));
	_SMI_TRACE(1,("doReferences() called, resultClass: %s", resultClass));

	CMPIStatus status = {CMPI_RC_OK, NULL};
	char key[128] = {0};
	const char *objClassName;
	const char * ns = CMGetCharPtr(CMGetNameSpace(cop, NULL));

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns, &status);
		if (status.rc != CMPI_RC_OK)
		{
			goto exit;
		}
	}

	if (assocClass == NULL || strcasecmp(assocClass, HostedStorageConfigurationServiceClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, UnitaryComputerSystemClassName) == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageConfigurationServiceClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Dependent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					cmpiutilGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return the one and only instance we have:
								CMPIInstance * instance = SCSCreateHostedAssocInstance(ns, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create HostedStorageConfigurationService instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, StorageConfigurationServiceClassName) == 0)
			{
				// this is a StorageConfigurationService
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "Name", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(UnitaryComputerSystemClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					if (strcasecmp(inName, "StorageConfigurationService") == 0)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = SCSCreateHostedAssocInstance(ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							_SMI_TRACE(1,("After SCSCreateHostedAssocInstance, instance is NULL "));
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create HostedStorageConfigurationService instance");
							return status;
						}
						_SMI_TRACE(1,("Calling cmpiutilSimpleAssocResults"));
						cmpiutilSimpleAssocResults(ctx, instance, &status);
						_SMI_TRACE(1,("Calling cmpiutilSimpleAssocResults: done"));
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_HostedStorageConfigurationService"

	if (assocClass == NULL || strcasecmp(assocClass, StorageConfigurationElementCapabilitiesClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageConfigurationCapabilitiesClassName) == 0)
			{
				// this is a StorageConfigurationCapabilities
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageConfigurationServiceClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					char instanceID[256];
					cmpiutilMakeInstanceID("StorageConfigurationCapabilities", instanceID, 256);
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inInstance = CMGetCharPtr(data.value.string);
						if (inInstance)
						{
							_SMI_TRACE(1,("Comparing inInstance: %s   with instanceID: %s", inInstance, instanceID));
							if (strcasecmp(inInstance, instanceID) == 0)
							{
								// it matches, return the one and only instance we have:
								CMPIInstance * instance = SCCCreateAssocInstance(ns, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageConfigurationElementCapabilities instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key InstanceID"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key InstanceID"));
					}
				}
			}
			else if(strcasecmp(objClassName, SystemStorageCapabilitiesClassName) == 0)
			{
				// this is a SystemStorageCapabilities
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageConfigurationServiceClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					cmpiutilMakeInstanceID("SystemStorageCapabilities", key, 256);
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inInstance = CMGetCharPtr(data.value.string);
						if (inInstance)
						{
							_SMI_TRACE(1,("Comparing inInstance: %s   with instanceID: %s", inInstance, key));
							if (strcasecmp(inInstance, key) == 0)
							{
								// it matches, return the one and only instance we have:
								CMPIInstance * instance = SSCCreateAssocInstance(ns, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageConfigurationElementCapabilities instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key InstanceID"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key InstanceID"));
					}
				}
			}

			else if(strcmp(objClassName, StorageConfigurationServiceClassName) == 0)
			{
				// this is a StorageConfigurationService
				// need to get appropriate Capabilities for assocInst

				CMPIData data = CMGetKey(cop, "Name", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				int bReturnSCC = 1; // Return StorageConfigurationCapabilities reference
				int bReturnSSC = 1; // Return SystemStorageCapabilities

				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageConfigurationCapabilitiesClassName,
												  resultClass,
												  broker,ns,&status))
					{
						if (!cmpiutilClassIsDerivedFrom(SystemStorageCapabilitiesClassName,
												  resultClass,
												  broker,ns,&status))
						{
							bHaveMatch = 0; // FALSE
						}
						else
						{
							bReturnSSC = 1;
							bReturnSCC = 0;
						}
					}
					else
					{
						bReturnSCC = 1;
						bReturnSSC = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Capabilities") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					if (strcasecmp(inName, "StorageConfigurationService") == 0)
					{
						// it matches, return the instance(s) requested:
						if (bReturnSCC)
						{
							CMPIInstance * instance = SCCCreateAssocInstance(ns, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageConfigurationElementCapabilities instance");
								return status;
							}
							_SMI_TRACE(1,("SCC: came here"));
							cmpiutilSimpleAssocResults(ctx, instance, &status);
							_SMI_TRACE(1,("SCC: done"));
						}
						if (bReturnSSC)
						{
							CMPIInstance * instance = SSCCreateAssocInstance(ns, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageConfigurationElementCapabilities instance");
								return status;
							}
							_SMI_TRACE(1,("SCC: came here"));
							cmpiutilSimpleAssocResults(ctx, instance, &status);
							_SMI_TRACE(1,("SCC: done"));
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageConfigurationElementCapabilities"

	if (assocClass == NULL || strcasecmp(assocClass, HostedStoragePoolClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, UnitaryComputerSystemClassName) == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					cmpiutilGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								CMPICount i;
								for (i = 0; i < PoolsSize(); i++)
								{
									StoragePool *pool = PoolsGet(i);
									CMPIInstance *instance = PoolCreateHostedAssocInstance(pool, ns, properties, &status);
									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create HostedStoragePool instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, StoragePoolClassName) == 0)
			{
				// this is a StoragePool
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(UnitaryComputerSystemClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get hosted pool association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = PoolCreateHostedAssocInstance(pool, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create HostedStoragePool instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_HostedStoragePool"

	if (assocClass == NULL || strcasecmp(assocClass, LogicalDiskDeviceClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, UnitaryComputerSystemClassName) == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(LogicalDiskClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					cmpiutilGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								CMPICount i;
								for (i = 0; i < VolumesSize(); i++)
								{
									StorageVolume *vol = VolumesGet(i);
									CMPIInstance *instance = VolumeCreateDeviceAssocInstance(LogicalDiskDeviceClassName,vol, ns, properties, &status);
									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create LogicalDiskDevice instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, LogicalDiskClassName) == 0)
			{
				// this is a LogicalDisk
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(UnitaryComputerSystemClassName,

												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get LogicalDiskDevice association for volume with deviceID = %s", deviceID));

					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = VolumeCreateDeviceAssocInstance(LogicalDiskDeviceClassName,vol, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create LogicalDiskDevice instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_LogicalDiskDevice"

	if (assocClass == NULL || strcasecmp(assocClass, StorageVolumeDeviceClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, UnitaryComputerSystemClassName) == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageVolumeClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					cmpiutilGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								CMPICount i;
								for (i = 0; i < VolumesSize(); i++)
								{
									StorageVolume *vol = VolumesGet(i);
									CMPIInstance *instance = VolumeCreateDeviceAssocInstance(StorageVolumeDeviceClassName,vol, ns, properties, &status);
									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageVolumeDevice instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, StorageVolumeClassName) == 0)
			{
				// this is a StorageVolume
				// need to get appropriate UnitaryComputerSystel for asqocInst

				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(UnitaryComputerSystemClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupCompnnent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
				_SMI_TRACE(1,("Request to get StorageVolumeDevice association for volume with deviceID = %s", deviceID));

					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = VolumeCreateDeviceAssocInstance(StorageVolumeDeviceClassName, vol, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create StorageVolumeDevice instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_ StorageVolumeDevice "

	if (assocClass == NULL || strcasecmp(assocClass, StorageElementCapabilitiesClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageCapabilitiesClassName) == 0)
			{
				// this is a StorageCapability
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get capabilities association for capability with instanceID = %s", instanceID));
					StorageCapability *cap = CapabilitiesFind(instanceID);
					if (cap != NULL && cap->pool != NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = PoolCreateCapabilityAssocInstance(cap->pool, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create ElementCapabilities instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else if(strcmp(objClassName, StoragePoolClassName) == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageCapabilities for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageCapabilitiesClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}
				

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Capabilities") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get element capabilities association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->capability != NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = PoolCreateCapabilityAssocInstance(pool, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create ElementCapabilities instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageElementCapabilities"

	if (assocClass == NULL || strcasecmp(assocClass, AssociatedComponentExtentClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageExtentClassName) == 0 ||
				strcasecmp(objClassName, CompositeExtentClassName) == 0)
			{
				// this is a StorageExtent
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get component association for extent with deviceID = %s", deviceID));
					StorageExtent *extent = ExtentsFind(deviceID);
					if (extent != NULL && extent->pool != NULL && !ExtentIsFreespace(extent))
					{
						// it matches, return the one and only instance we have:
						StoragePool *pool = extent->pool;
						CMPIInstance * instance = PoolCreateComponentAssocInstance(pool, extent, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create AssociatedComponentExtent instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else if(strcmp(objClassName, StoragePoolClassName) == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageExtent for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageExtentClassName, 
												  resultClass, 
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get component extent association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->concreteComps != NULL)
					{
						// it matches, return all the instance we have:
						CMPICount i;
						for (i = 0; i < PoolExtentsSize(pool); i++)
						{
							if (!ExtentIsFreespace(pool->concreteComps[i]))
							{
								CMPIInstance * instance = PoolCreateComponentAssocInstance(pool, pool->concreteComps[i], ns, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create AssociatedComponentExtent instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AssociatedComponentExtent"

	if (assocClass == NULL || strcasecmp(assocClass, AssociatedRemainingExtentClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageExtentClassName) == 0 ||
				strcasecmp(objClassName, CompositeExtentClassName) == 0)
			{
				// this is a StorageExtent
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get remaining association for extent with deviceID = %s", deviceID));
					StorageExtent *extent = ExtentsFind(deviceID);
					if (extent != NULL && extent->pool != NULL && ExtentIsFreespace(extent))
					{
						// it matches, return the one and only instance we have:
						StoragePool *pool = extent->pool;
						_SMI_TRACE(1,("Got the pool to be returned, instanceID = %s", pool->instanceID));
						CMPIInstance * instance = PoolCreateRemainingAssocInstance(pool, extent, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create AssociatedRemainingExtent instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else if(strcmp(objClassName, StoragePoolClassName) == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageExtent for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageExtentClassName, 
												  resultClass, 
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get remaining extent association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->concreteComps != NULL)
					{
						// it matches, return all the instance we have:
						CMPICount i;
						for (i = 0; i < PoolExtentsSize(pool); i++)
						{
							if (ExtentIsFreespace(pool->concreteComps[i]))
							{
								CMPIInstance * instance = PoolCreateRemainingAssocInstance(pool, pool->concreteComps[i], ns, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create AssociatedRemainingExtent instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AssociatedRemainingExtent"

	if (assocClass == NULL || strcasecmp(assocClass, StorageSettingsGeneratedFromCapabilitiesClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageCapabilitiesClassName) == 0)
			{
				// this is a StorageCapability
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageSettingWithHintsClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Dependent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get generated settings association for capability with instanceID = %s", instanceID));
					StorageCapability *cap = CapabilitiesFind(instanceID);
					if (cap != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						CMPICount i;
						for (i = 0; i < SettingsSize(); i++)
						{
							StorageSetting *setting = SettingsGet(i);
							if (setting->capability == cap && setting->volume == NULL)
							{
								CMPIInstance* instance = SettingCreateGFCAssocInstance(setting, ns, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SettingsGeneratedFromCapabilities instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, StorageSettingWithHintsClassName) == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageCapabilities for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageCapabilitiesClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get generated settings association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->capability != NULL && setting->volume == NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = SettingCreateGFCAssocInstance(setting, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create SettingsGeneratedFromCapabilities instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageSettingsGeneratedFromCapabilities"

	if (assocClass == NULL || strcasecmp(assocClass, BasedOnClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageExtentClassName) == 0 ||
			   strcasecmp(objClassName, CompositeExtentClassName) == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHaveExtentMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (cmpiutilClassIsDerivedFrom(StorageExtentClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveExtentMatch = 1; 

					}
					else if (cmpiutilClassIsDerivedFrom(LogicalDiskClassName,
											  resultClass,
											  broker,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
					else if (cmpiutilClassIsDerivedFrom(StorageVolumeClassName,
											  resultClass,
											  broker,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out CompositeExtentBasedOn case
						if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									CMPIInstance* instance = ExtentCreateBasedOnAssocInstance(
																depExt, 
																depExt->antecedents[j]->extent,
																depExt->antecedents[j], 
																ns, 
																properties,
																BasedOnClassName,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create BasedOn instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
									{
										CMPIInstance* instance = ExtentCreateBasedOnAssocInstance(
																	depExt, 
																	antExt,
																	antExt->dependents[j], 
																	ns, 
																	properties,
																	BasedOnClassName,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create BasedOn instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
								}
							}
						}
						if (bHaveVolumeMatch)
						{
							// Return all basedOn's where in input extent is antecedent for dependent volume/logicalDisk
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->volume)
								{
									if(antExt->dependents[j]->volume->IsLD)
									{
										CMPIInstance* instance = VolumeCreateBasedOnAssocInstance(LogicalDiskClassName,
																	antExt->dependents[j]->volume, 
																	antExt,
																	antExt->dependents[j], 
																	ns, 
																	properties,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create BasedOn instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
									else
									{
										CMPIInstance* instance = VolumeCreateBasedOnAssocInstance(StorageVolumeClassName,
																	antExt->dependents[j]->volume, 
																	antExt,
																	antExt->dependents[j], 
																	ns, 
																	properties,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create BasedOn instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, LogicalDiskClassName) == 0 || strcmp(objClassName, StorageVolumeClassName) == 0)
			{
				// this is a LogicalDisk/StorageVolume
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageExtentClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveAntMatch = 1; // TRUE
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveAntMatch = 0;

					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHaveExtentMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						if(depVol->IsLD)
						{
							CMPIInstance* instance = VolumeCreateBasedOnAssocInstance(LogicalDiskClassName,
														depVol, 
														depVol->antecedentExtent.extent,
														&depVol->antecedentExtent, 
														ns, 
														properties,
														&status);

							if ((instance == NULL) || CMIsNullObject(instance))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create BasedOn instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else
						{
							CMPIInstance* instance = VolumeCreateBasedOnAssocInstance(StorageVolumeClassName,
														depVol, 
														depVol->antecedentExtent.extent,
														&depVol->antecedentExtent, 
														ns, 
														properties,
														&status);

							if ((instance == NULL) || CMIsNullObject(instance))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create BasedOn instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_BasedOn"

/*	if (assocClass == NULL || strcasecmp(assocClass, StorageVolumeBasedOnClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageExtentClassName) == 0 ||
			   strcasecmp(objClassName, CompositeExtentClassName) == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHaveExtentMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (cmpiutilClassIsDerivedFrom(StorageExtentClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveExtentMatch = 1; 

					}
					else if (cmpiutilClassIsDerivedFrom(StorageVolumeClassName,
											  resultClass,
											  broker,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out CompositeExtentBasedOn case
						if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									CMPIInstance* instance = ExtentCreateBasedOnAssocInstance(
																depExt, 
																depExt->antecedents[j]->extent,
																depExt->antecedents[j], 
																ns, 
																properties,
																assocClass,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create BasedOn instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
									{
										CMPIInstance* instance = ExtentCreateBasedOnAssocInstance(
																	depExt, 
																	antExt,
																	antExt->dependents[j], 
																	ns, 
																	properties,
																	assocClass,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create BasedOn instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
								}
							}
						}
						if (bHaveVolumeMatch)
						{
							// Return all basedOn's where in input extent is antecedent for dependent volume/logicalDisk
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->volume)
								{
									CMPIInstance* instance = VolumeCreateBasedOnAssocInstance(BasedOnClassName,
																antExt->dependents[j]->volume, 
																antExt,
																antExt->dependents[j], 
																ns, 
																properties,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create BasedOn instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, StorageVolumeClassName) == 0)
			{
				// this is a StorageVolume
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageExtentClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveAntMatch = 1; // TRUE
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveAntMatch = 0;

					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHaveExtentMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						CMPIInstance* instance = VolumeCreateBasedOnAssocInstance(StorageVolumeBasedOnClassName,
													depVol, 
													depVol->antecedentExtent.extent,
													&depVol->antecedentExtent, 
													ns, 
													properties,
													&status);

						if ((instance == NULL) || CMIsNullObject(instance))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create BasedOn instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeBasedOn"
*/
	if (assocClass == NULL || strcasecmp(assocClass, CompositeExtentBasedOnClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageExtentClassName) == 0 ||
			   strcasecmp(objClassName, CompositeExtentClassName) == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageExtentClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get composite based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out BasedOn case
						if (depExt->composite && cap != NULL && cap->extentStripe > 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									CMPIInstance* instance = ExtentCreateBasedOnAssocInstance(
																depExt, 
																depExt->antecedents[j]->extent,
																depExt->antecedents[j], 
																ns, 
																properties,
																CompositeExtentBasedOnClassName,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create BasedOn instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get composite based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (depExt->composite && cap != NULL && cap->extentStripe > 1)
									{
										CMPIInstance* instance = ExtentCreateBasedOnAssocInstance(
																	depExt, 
																	antExt,
																	antExt->dependents[j], 
																	ns, 
																	properties,
																	CompositeExtentBasedOnClassName,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create CompositeExtentBasedOn instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
								}
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_CompositeExtentBasedOn"

	if (assocClass == NULL || strcasecmp(assocClass, AllocatedFromStoragePoolClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StoragePoolClassName) == 0)
			{
				// this is a StoragePool
				// need to get approprate StoragePool or LogicalDisk for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHavePoolMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHavePoolMatch = 1; 

					}
					else if (cmpiutilClassIsDerivedFrom(LogicalDiskClassName,
											  resultClass,
											  broker,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
					else if (cmpiutilClassIsDerivedFrom(StorageVolumeClassName,
											  resultClass,
											  broker,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent pool with instanceID = %s", instanceID));
					StoragePool *depPool = PoolsFind(instanceID);
					if (depPool != NULL && depPool->antecedentPools != NULL)
					{
						// Return all allocFrom pools where input pool is the dependent
						CMPICount j;
						for (j = 0; j < depPool->maxAntPools; j++)
						{
							if (depPool->antecedentPools[j])
							{
								StoragePool *antPool = (StoragePool *)depPool->antecedentPools[j]->element;
								CMPIInstance* instance = PoolCreateAllocFromAssocInstance(
		AllocatedFromStoragePoolClassName,
															depPool, 
															antPool,
															depPool->antecedentPools[j]->spaceConsumed,
															ns,
															properties,
															&status);

								if ((instance == NULL) || CMIsNullObject(instance))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create AllocFrom instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for antecedent pool with instanceID = %s", instanceID));
					StoragePool *antPool = PoolsFind(instanceID);
					if (antPool != NULL)
					{
						if (bHavePoolMatch && antPool->dependentPools != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent pool
							CMPICount j;
							for (j = 0; j < antPool->maxDepPools; j++)
							{
								if (antPool->dependentPools[j])
								{
									StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
									CMPIInstance* instance = PoolCreateAllocFromAssocInstance(
		AllocatedFromStoragePoolClassName,
																depPool, 
																antPool,
																antPool->dependentPools[j]->spaceConsumed,
																ns,
																properties,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create AllocFrom instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
						if (bHaveVolumeMatch && antPool->dependentVolumes != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent volume
							CMPICount j;
							for (j = 0; j < antPool->maxDepVolumes; j++)
							{
								if (antPool->dependentVolumes[j])
								{
									StorageVolume *depVol = (StorageVolume *)antPool->dependentVolumes[j]->element;
									if(depVol->IsLD)
									{
										CMPIInstance* instance = VolumeCreateAllocFromAssocInstance(LogicalDiskClassName,
																	depVol, 
																	antPool,
																	antPool->dependentVolumes[j]->spaceConsumed,
																	ns,
																	properties,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create AllocFrom instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
									else
									{
										CMPIInstance* instance = VolumeCreateAllocFromAssocInstance(StorageVolumeClassName,
																	depVol, 
																	antPool,
																	antPool->dependentVolumes[j]->spaceConsumed,
																	ns,
																	properties,
																	&status);

										if ((instance == NULL) || CMIsNullObject(instance))
										{
											CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
												"Could not create AllocFrom instance");
											return status;
										}
										cmpiutilSimpleAssocResults(ctx, instance, &status);
									}
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, LogicalDiskClassName) == 0 || strcmp(objClassName, StorageVolumeClassName) == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHavePoolMatch = 0; 
					}
				}

				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHavePoolMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						if(depVol->IsLD)
						{
							// Return our single allocFrom pool where input volume is the dependent
							StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
							CMPIInstance* instance = VolumeCreateAllocFromAssocInstance(LogicalDiskClassName,
														depVol, 
														antPool,
														depVol->antecedentPool.spaceConsumed,
														ns,
														properties,
														&status);

							if ((instance == NULL) || CMIsNullObject(instance))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create AllocFrom instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else
						{
							// Return our single allocFrom pool where input volume is the dependent
							StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
							CMPIInstance* instance = VolumeCreateAllocFromAssocInstance(StorageVolumeClassName,
														depVol, 
														antPool,
														depVol->antecedentPool.spaceConsumed,
														ns,
														properties,
														&status);

							if ((instance == NULL) || CMIsNullObject(instance))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create AllocFrom instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AllocatedFromStoragePool"

/*	if (assocClass == NULL || strcasecmp(assocClass, StorageVolumeAllocatedFromStoragePoolClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StoragePoolClassName) == 0)
			{
				// this is a StoragePool
				// need to get approprate StoragePool or StorageVolume for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHavePoolMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHavePoolMatch = 1; 

					}
					else if (cmpiutilClassIsDerivedFrom(StorageVolumeClassName,

											  resultClass,
											  broker,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent pool with instanceID = %s", instanceID));
					StoragePool *depPool = PoolsFind(instanceID);
					if (depPool != NULL && depPool->antecedentPools != NULL)
					{
						// Return all allocFrom pools where input pool is the dependent
						CMPICount j;
						for (j = 0; j < depPool->maxAntPools; j++)
						{
							if (depPool->antecedentPools[j])
							{
								StoragePool *antPool = (StoragePool *)depPool->antecedentPools[j]->element;
							_SMI_TRACE(1,("This instance has an antecedent pool with InstanceID = %s", antPool->instanceID));
								CMPIInstance* instance = PoolCreateAllocFromAssocInstance(
		StorageVolumeAllocatedFromStoragePoolClassName,
															depPool, 
															antPool,
															depPool->antecedentPools[j]->spaceConsumed,
															ns,
															properties,
															&status);

								if ((instance == NULL) || CMIsNullObject(instance))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create AllocFrom instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for antecedent pool with instanceID = %s", instanceID));
					StoragePool *antPool = PoolsFind(instanceID);
					if (antPool != NULL)
					{
						if (bHavePoolMatch && antPool->dependentPools != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent pool
							CMPICount j;
							for (j = 0; j < antPool->maxDepPools; j++)
							{
								if (antPool->dependentPools[j])
								{
									StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
									_SMI_TRACE(1,("This instance has a dependent pool with InstanceID = %s", depPool->instanceID));
									CMPIInstance* instance = PoolCreateAllocFromAssocInstance(
																StorageVolumeAllocatedFromStoragePoolClassName,
																depPool, 
																antPool,
																antPool->dependentPools[j]->spaceConsumed,
																ns,
																properties,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create AllocFrom instance");
										return status;
									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
									_SMI_TRACE(1,("cmpiutilSimpleAssocResults finished"));
								}
							}
						}
						if (bHaveVolumeMatch && antPool->dependentVolumes != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent volume
							CMPICount j;
							for (j = 0; j < antPool->maxDepVolumes; j++)
							{
								if (antPool->dependentVolumes[j])
								{
									StorageVolume *depVol = (StorageVolume *)antPool->dependentVolumes[j]->element;
									_SMI_TRACE(1,("This instance has a dependent volume with InstanceID = %s", depVol->name));
									CMPIInstance* instance = VolumeCreateAllocFromAssocInstance(StorageVolumeAllocatedFromStoragePoolClassName,
																depVol, 
																antPool,
																antPool->dependentVolumes[j]->spaceConsumed,
																ns,
																properties,
																&status);

									if ((instance == NULL) || CMIsNullObject(instance))
									{
									_SMI_TRACE(1,("VolumeCreateAllocFromAssocInstance: instance is NULL"));
										CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
											"Could not create AllocFrom instance");
										return status;
									}
									else
									{
									CMPIObjectPath *incop = CMGetObjectPath(instance, &status);
									CMPIString *keyname;
									unsigned int i = 0;
									CMPIData key;
									key = CMGetKeyAt(incop, i, &keyname, &status);
									_SMI_TRACE(1,("VolumeCreateAllocFromAssocInstance: instance is not NULL"));
									_SMI_TRACE(1,("Keyname: %s\n", CMGetCharPtr(keyname)));
									_SMI_TRACE(1,("Key type: %d\n", key.type));
//									_SMI_TRACE(1,("COP1: %s \n", 
//										CMGetCharPtr(CMObjectPathToString(key.value.ref, NULL))));
//									_SMI_TRACE(1,("COP2: %s \n", 
//										CMGetCharPtr(CMObjectPathToString(ctx->origCop, NULL))));
//									}
									cmpiutilSimpleAssocResults(ctx, instance, &status);
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, StorageVolumeClassName) == 0)
			{
				// this is a StorageVolume
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StoragePoolClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHavePoolMatch = 0; 
					}
				}

				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHavePoolMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						// Return our single allocFrom pool where input volume is the dependent
						StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
						CMPIInstance* instance = VolumeCreateAllocFromAssocInstance(StorageVolumeAllocatedFromStoragePoolClassName,
													depVol, 
													antPool,
													depVol->antecedentPool.spaceConsumed,
													ns,
													properties,
													&status);

						if ((instance == NULL) || CMIsNullObject(instance))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create AllocFrom instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeAllocatedFromStoragePool"
*/
	if (assocClass == NULL || strcasecmp(assocClass, StorageElementSettingDataClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, LogicalDiskClassName) == 0 || strcasecmp(objClassName, LogicalDiskClassName) == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageSettingWithHintsClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "SettingData") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingData association for logicalDisk with deviceID = %s", deviceID));
					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						if(vol->IsLD)
						{
							CMPIInstance* instance = VolumeCreateSettingAssocInstance(LogicalDiskClassName,vol, ns, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSettingData instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else
						{
							CMPIInstance* instance = VolumeCreateSettingAssocInstance(StorageVolumeClassName,vol, ns, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSettingData instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
			}
			else if(strcmp(objClassName, StorageSettingWithHintsClassName) == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageVolume for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(LogicalDiskClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
					else if (!cmpiutilClassIsDerivedFrom(StorageVolumeClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingsData association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->volume != NULL)
					{
						// it matches, return the one and only instance we have:
						if(setting->volume->IsLD)
						{
							CMPIInstance * instance = VolumeCreateSettingAssocInstance(LogicalDiskClassName,setting->volume, ns, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSettingData instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else
						{
							CMPIInstance * instance = VolumeCreateSettingAssocInstance(StorageVolumeClassName,setting->volume, ns, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSettingData instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageElementSettingData"

/*	if (assocClass == NULL || strcasecmp(assocClass, StorageVolumeStorageElementSettingDataClassName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, StorageVolumeClassName) == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageSettingWithHintsClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "SettingData") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingData association for logicalDisk with deviceID = %s", deviceID));
					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						CMPIInstance* instance = VolumeCreateSettingAssocInstance(StorageVolumeStorageElementSettingDataClassName,vol, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create ElementSettingData instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else if(strcmp(objClassName, StorageSettingWithHintsClassName) == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageVolume for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(StorageVolumeClassName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingsData association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->volume != NULL)
					{
						// it matches, return the one and only instance we have:
						CMPIInstance * instance = VolumeCreateSettingAssocInstance(StorageVolumeStorageElementSettingDataClassName,setting->volume, ns, properties, &status);
						if ((instance == NULL) || (CMIsNullObject(instance)))
						{
							CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
								"Could not create ElementSettingData instance");
							return status;
						}
						cmpiutilSimpleAssocResults(ctx, instance, &status);
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeStorageElementSettingData"
*/	

	//close return handler
	CMReturnDone(results);

exit:
	_SMI_TRACE(1,("Leaving doReferences"));
	return status;
}



// ****************************************************************************
// Associators()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname
// 											and desired objectpath
//             char *assocClass
//             char *resultClass
//             char *role
//             char *resultRole
//             char **properties
// ****************************************************************************
static CMPIStatus
Associators(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *assocClass,
		const char *resultClass,
		const char *role,
		const char *resultRole,
		const char** properties)
{
	_SMI_TRACE(1,("Associators() called.  assocClass: %s", assocClass));
	CMPIStatus status = cmpiutilSimpleAssociators( doReferences, self,
				 _BROKER, context, results, cop, assocClass,
				resultClass, role, resultRole, properties);
/*
	CMPIStatus status = {CMPI_RC_OK, NULL};
	char isReturned=0;
	char key[128] = {0};
	const char * objClassName;
	const char *ns = CMGetCharPtr(CMGetNameSpace(cop, NULL));
	CMPIInstance *srcInstance=NULL;
	CMPIObjectPath *srcOP=NULL;
	CMPIData scsOPData;

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns, &status);
	}

	if (assocClass == NULL || strcasecmp(assocClass, "OMC_HostedStorageConfigurationService") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));

		_SMI_TRACE(1,("Inside assocClass: OMC_HostedStorageConfigurationService: class name %s", objClassName));
		if(status.rc != CMPI_RC_OK)
		{
			_SMI_TRACE(1,("Unable to get class name\n"));
			CMSetStatusWithChars( _BROKER, &status, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return status;			
		}
		if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
		{
				// this is a UnitaryComputerSystem
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

			if ((!resultClass || !strcasecmp(resultClass, "OMC_StorageConfigurationService")) &&
       			(!role || !strcasecmp(role, "Antecedent")) &&
       			(!resultRole || !strcasecmp(resultRole, "Dependent")))
			{
				srcOP = CMNewObjectPath(_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), 
				"OMC_StorageConfigurationService", &status);
				if(CMIsNullObject(srcOP))
				{
					_SMI_TRACE(1,("Unable to create Object Path\n"));
					CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return status;
				}
				status = EnumInstances(NULL, context, results, srcOP, NULL);

				CMRelease(srcOP);
				isReturned = 1;
			}
		 	else if(assocClass)
				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		else if (strcasecmp(objClassName, "OMC_StorageConfigurationService") == 0)
		{
			// this is a StorageConfigurationService
			// need to get appropriate UnitaryComputerSystem for assocInst
			_SMI_TRACE(1,("Inside objClassName: OMC_StorageConfigurationService"));
			if ((!resultClass || !strcasecmp(resultClass, "OMC_UnitaryComputerSystem")) &&
	    		(!role || !strcasecmp(role, "Dependent")) &&
       			(!resultRole || !strcasecmp(resultRole, "Antecedent")))
			{
				_SMI_TRACE(1,("Inside if clause"));
				CMPIObjectPath *oPath=NULL;
				oPath = cmpiutilCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
				if(CMIsNullObject(oPath))
				{
					_SMI_TRACE(1,("Unable to create Object Path\n"));
					CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return status;
				}
					
				//Get the list of all target class object paths from the CIMOM 
				CMPIEnumeration * instances  = CBEnumInstances(_BROKER, context, oPath, NULL, &status);
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(instances))
				{ 
					_SMI_TRACE(1,("--- CBEnumInstance() failed - %s", CMGetCharPtr(status.msg)));
				 	CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  	return status;
				}
				
				scsOPData = CMGetNext(instances , &status);
				srcInstance = (CMPIInstance *) scsOPData.value.ref;
				CMReturnInstance(results, srcInstance);
				CMRelease(instances);

				isReturned = 1;
			}
			else
				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		else
		{
			_SMI_TRACE(1,("Inside objClassName: Invalid"));
			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
	}
/*
	else if (strcasecmp(assocClass, "OMC_StorageConfigurationElementCapabilities") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageConfigurationCapabilities") == 0)
			{
				// this is a StorageConfigurationCapabilities
				// need to get appropriate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageConfigurationService",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					char instanceID[256];
					omcMakeInstanceID("StorageConfigurationCapabilities", instanceID, 256);
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inInstance = CMGetCharPtr(data.value.string);
						if (inInstance)
						{
							_SMI_TRACE(1,("Comparing inInstance: %s   with instanceID: %s", inInstance, instanceID));
							if (strcasecmp(inInstance, instanceID) == 0)
							{
								// it matches, return the one and only OP we have:
								
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
								"OMC_StorageConfigurationService", &status);
								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageConfigurationElementCapabilities object path");
									return status;
								}
								status = EnumInstances(NULL, context, results, srcOP, NULL);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key InstanceID"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key InstanceID"));
					}
				}
			}
			else if(strcasecmp(objClassName, "OMC_SystemStorageCapabilities") == 0)
			{
				// this is a SystemStorageCapabilities
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageConfigurationService",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcMakeInstanceID("SystemStorageCapabilities", key, 256);
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inInstance = CMGetCharPtr(data.value.string);
						if (inInstance)
						{
							_SMI_TRACE(1,("Comparing inInstance: %s   with instanceID: %s", inInstance, key));
							if (strcasecmp(inInstance, key) == 0)
							{
								// it matches, return the one and only OP we have:

								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
								"OMC_StorageConfigurationService", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageConfigurationElementCapabilities object path");
									return status;
								}
								status = EnumInstances(NULL, context, results, srcOP, NULL);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key InstanceID"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key InstanceID"));
					}
				}
			}

			else if(strcmp(objClassName, "OMC_StorageConfigurationService") == 0)
			{
				// this is a StorageConfigurationService
				// need to get appropriate Capabilities for assocInst

				CMPIData data = CMGetKey(cop, "Name", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				int bReturnSCC = 1; // Return StorageConfigurationCapabilities reference
				int bReturnSSC = 1; // Return SystemStorageCapabilities

				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageConfigurationCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
					{
						if (!omccmpiClassIsDerivedFrom("OMC_SystemStorageCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
						{
							bHaveMatch = 0; // FALSE
						}
						else
						{
							bReturnSSC = 1;
							bReturnSCC = 0;
						}
					}
					else
					{
						bReturnSCC = 1;
						bReturnSSC = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Capabilities") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					if (strcasecmp(inName, "StorageConfigurationService") == 0)
					{
						// it matches, return the OP (s) requested:
						if (bReturnSCC)
						{
							srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
							"OMC_StorageConfigurationCapabilities", &status);
							if (CMIsNullObject(srcOP))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageConfigurationCapabilities object path");
								return status;
							}
							status = EnumInstances(NULL, context, results, srcOP, NULL);
							CMRelease(srcOP);
							isReturned = 1;
						}
						if (bReturnSSC)
						{
							srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
							"OMC_SystemStorageCapabilities", &status);
							if (CMIsNullObject(srcOP))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create SystemStorageCapabilities object path");
								return status;
							}
							status = EnumInstances(NULL, context, results, srcOP, NULL);
							CMRelease(srcOP);
							isReturned = 1;
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageConfigurationElementCapabilities"

	else if (strcasecmp(assocClass, "OMC_HostedStoragePool") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), "OMC_StoragePool", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create HostedStoragePool object path");
									return status;
								}
								status = EnumInstances(NULL, context, results, srcOP, NULL);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get hosted pool association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = omccmpiCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						if (CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create ComputerSystem object path");
							return status;
						}
					
						CMPIEnumeration * instances = CBEnumInstances(_BROKER, context, srcOP,NULL, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(instances))
						{ 
							_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  			return status;
						}
				
						scsOPData = CMGetNext(instances , &status);
						srcInstance = (CMPIInstance *) scsOPData.value.ref;
						CMReturnInstance(results, srcInstance);
						CMRelease(instances);
		
						isReturned = 1;
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_HostedStoragePool"

	else if (strcasecmp(assocClass, "OMC_LogicalDiskDevice") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), "OMC_LogicalDisk", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create LogicalDisk object path");
									return status;
								}
								status = EnumInstances(NULL, context, results, srcOP, NULL);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get LogicalDiskDevice association for volume with deviceID = %s", deviceID));

					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = omccmpiCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						CMPIEnumeration *instances = CBEnumInstances(_BROKER, context, srcOP, NULL, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(instances))
						{ 
							_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  			return status;
						}
				
						scsOPData = CMGetNext(instances , &status);
						srcInstance = (CMPIInstance *) scsOPData.value.ref;
						CMReturnInstance(results, srcInstance);
						CMRelease(instances);
		
						isReturned = 1;
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_LogicalDiskDevice"
	else if (strcasecmp(assocClass, "OMC_StorageVolumeDevice") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageVolume",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								srcOP = CMNewObjectPath(_BROKER, ns, "OMC_StorageVolume", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageVolume object path");
									return status;
								}
								status = EnumInstances(NULL, context, results, srcOP, NULL);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a StorageVolume
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupCompnnent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
				_SMI_TRACE(1,("Request to get StorageVolumeDevice association for volume with deviceID = %s", deviceID));

					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = omccmpiCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						CMPIEnumeration *instances = CBEnumInstances(_BROKER, context, srcOP, NULL, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(instances))
						{ 
							_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  			return status;
						}
				
						scsOPData = CMGetNext(instances , &status);
						srcInstance = (CMPIInstance *) scsOPData.value.ref;
						CMReturnInstance(results, srcInstance);
						CMRelease(instances);
		
						isReturned = 1;
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}

	}// END--if assocClass == "OMC_ StorageVolumeDevice "

	else if (strcasecmp(assocClass, "OMC_StorageElementCapabilities") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageCapabilities") == 0)
			{
				// this is a StorageCapability
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get capabilities association for capability with instanceID = %s", instanceID));
					StorageCapability *cap = CapabilitiesFind(instanceID);
					if (cap != NULL && cap->pool != NULL)
					{
						// it matches, return the one and only instance we have:
						srcInstance = PoolCreateInstance(cap->pool, ns, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageCapabilities for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Capabilities") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get element capabilities association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->capability != NULL)
					{
						// it matches, return the one and only instance we have:
						srcInstance = CapabilityCreateInstance(pool->capability, ns, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageCapabilities instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageElementCapabilities"


	else if (strcasecmp(assocClass, "OMC_AssociatedComponentExtent") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
				strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get component association for extent with deviceID = %s", deviceID));
					StorageExtent *extent = ExtentsFind(deviceID);
					if (extent != NULL && extent->pool != NULL && !ExtentIsFreespace(extent))
					{
						// it matches, return the one and only instance we have:
						StoragePool *pool = extent->pool;
						srcInstance = PoolCreateInstance(pool, ns, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageExtent for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent", 
												  resultClass, 
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get component extent association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->concreteComps != NULL)
					{
						// it matches, return all the instance we have:
						CMPICount i;
						for (i = 0; i < PoolExtentsSize(pool); i++)
						{
							if (!ExtentIsFreespace(pool->concreteComps[i]))
							{
								srcInstance = ExtentCreateInstance(pool->concreteComps[i], ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageExtent nstance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AssociatedComponentExtent"

	else if (strcasecmp(assocClass, "OMC_AssociatedRemainingExtent") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
				strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get remaining association for extent with deviceID = %s", deviceID));
					StorageExtent *extent = ExtentsFind(deviceID);
					if (extent != NULL && extent->pool != NULL && ExtentIsFreespace(extent))
					{
						// it matches, return the one and only instance we have:
						StoragePool *pool = extent->pool;
						srcInstance = PoolCreateInstance(pool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageExtent for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent", 
												  resultClass, 
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get remaining extent association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->concreteComps != NULL)
					{
						// it matches, return all the instance we have:
						CMPICount i;
						for (i = 0; i < PoolExtentsSize(pool); i++)
						{
							if (ExtentIsFreespace(pool->concreteComps[i]))
							{
								srcInstance = ExtentCreateInstance(pool->concreteComps[i], ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageExtent instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AssociatedRemainingExtent"

	else if (strcasecmp(assocClass, "OMC_StorageSettingsGeneratedFromCapabilities") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageCapabilities") == 0)
			{
				// this is a StorageCapability
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageSettingWithHints",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Dependent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get generated settings association for capability with instanceID = %s", instanceID));
					StorageCapability *cap = CapabilitiesFind(instanceID);
					if (cap != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						CMPICount i;
						for (i = 0; i < SettingsSize(); i++)
						{
							StorageSetting *setting = SettingsGet(i);
							if (setting->capability == cap && setting->volume == NULL)
							{
								srcInstance = SettingCreateInstance(setting, ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageSetting instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageSettingWithHints") == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageCapabilities for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get generated settings association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->capability != NULL && setting->volume == NULL)
					{
						// it matches, return the one and only instance we have:
						srcInstance = CapabilityCreateInstance(setting->capability, ns, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageCapabilities instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageSettingsGeneratedFromCapabilities"

	else if (strcasecmp(assocClass, "OMC_BasedOn") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
			   strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHaveExtentMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("OMC_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out CompositeExtentBasedOn case
						if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									srcInstance = ExtentCreateInstance(depExt->antecedents[j]->extent, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageExtent instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
									{
										srcInstance = ExtentCreateInstance(depExt, ns, &status);
										if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
										{
											CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageExtent instance");
											return status;
										}
										CMReturnInstance(results, srcInstance); 
									}
								}
							}
						}
						if (bHaveVolumeMatch)
						{
							// Return all basedOn's where in input extent is antecedent for dependent volume/logicalDisk
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->volume)
								{
									srcInstance = VolumeCreateInstance("OMC_LogicalDisk",antExt->dependents[j]->volume, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create LogicalDisk instance");
										return status;
									}
									CMReturnInstance(results, srcInstance);
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveAntMatch = 1; // TRUE
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveAntMatch = 0;

					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHaveExtentMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						srcInstance = ExtentCreateInstance(depVol->antecedentExtent.extent, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageExtent instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_BasedOn"

	else if (strcasecmp(assocClass, "OMC_StorageVolumeBasedOn") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
			   strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHaveExtentMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("CIM_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_StorageVolume",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out CompositeExtentBasedOn case
						if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									srcInstance = ExtentCreateInstance(depExt->antecedents[j]->extent, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageExtent instance");
										return status;
									}
									CMReturnInstance(results, srcInstance);
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
									{
										srcInstance = ExtentCreateInstance(depExt, ns, &status);
										if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
										{
											CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageExtent Instance");
											return status;
										}
										CMReturnInstance(results, srcInstance); 
									}
								}
							}
						}
						if (bHaveVolumeMatch)
						{
							// Return all basedOn's where in input extent is antecedent for dependent volume/logicalDisk
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->volume)
								{
									srcInstance = VolumeCreateInstance("OMC_StorageVolume",antExt->dependents[j]->volume, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create Storage Volume instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a StorageVolume
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveAntMatch = 1; // TRUE
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveAntMatch = 0;

					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHaveExtentMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						srcInstance = ExtentCreateInstance(depVol->antecedentExtent.extent, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageExtent instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeBasedOn"

	else if (strcasecmp(assocClass, "OMC_CompositeExtentBasedOn") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
			   strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("CIM_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get composite based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out BasedOn case
						if (depExt->composite && cap != NULL && cap->extentStripe > 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									srcInstance = ExtentCreateInstance(depExt->antecedents[j]->extent, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageExtent instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get composite based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (depExt->composite && cap != NULL && cap->extentStripe > 1)
									{
										srcInstance = ExtentCreateInstance(depExt, ns, &status);
										if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
										{
											CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageExtent instance");
											return status;
										}
										CMReturnInstance(results, srcInstance); 
									}
								}
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_CompositeExtentBasedOn"

	else if (strcasecmp(assocClass, "OMC_AllocatedFromStoragePool") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get approprate StoragePool or LogicalDisk for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHavePoolMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent pool with instanceID = %s", instanceID));
					StoragePool *depPool = PoolsFind(instanceID);
					if (depPool != NULL && depPool->antecedentPools != NULL)
					{
						// Return all allocFrom pools where input pool is the dependent
						CMPICount j;
						for (j = 0; j < depPool->maxAntPools; j++)
						{
							if (depPool->antecedentPools[j])
							{
								StoragePool *antPool = (StoragePool *)depPool->antecedentPools[j]->element;
								srcInstance = PoolCreateInstance(antPool, ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StoragePool instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for antecedent pool with instanceID = %s", instanceID));
					StoragePool *antPool = PoolsFind(instanceID);
					if (antPool != NULL)
					{
						if (bHavePoolMatch && antPool->dependentPools != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent pool
							CMPICount j;
							for (j = 0; j < antPool->maxDepPools; j++)
							{
								if (antPool->dependentPools[j])
								{
									StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
									srcInstance = PoolCreateInstance(depPool, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StoragePool instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
						if (bHaveVolumeMatch && antPool->dependentVolumes != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent volume
							CMPICount j;
							for (j = 0; j < antPool->maxDepVolumes; j++)
							{
								if (antPool->dependentVolumes[j])
								{
									StorageVolume *depVol = (StorageVolume *)antPool->dependentVolumes[j]->element;
									srcInstance = VolumeCreateInstance("OMC_LogicalDisk", depVol, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create Logical Disk instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 0; 
					}
				}

				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHavePoolMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						// Return our single allocFrom pool where input volume is the dependent
						StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
						srcInstance = PoolCreateInstance(antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AllocatedFromStoragePool"
	else if (strcasecmp(assocClass, "OMC_StorageVolumeAllocatedFromStoragePool") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get approprate StoragePool or StorageVolume for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHavePoolMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_StorageVolume",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent pool with instanceID = %s", instanceID));
					StoragePool *depPool = PoolsFind(instanceID);
					if (depPool != NULL && depPool->antecedentPools != NULL)
					{
						// Return all allocFrom pools where input pool is the dependent
						CMPICount j;
						for (j = 0; j < depPool->maxAntPools; j++)
						{
							if (depPool->antecedentPools[j])
							{
								StoragePool *antPool = (StoragePool *)depPool->antecedentPools[j]->element;
								srcInstance = PoolCreateInstance(antPool, ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StoragePool instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for antecedent pool with instanceID = %s", instanceID));
					StoragePool *antPool = PoolsFind(instanceID);
					if (antPool != NULL)
					{
						if (bHavePoolMatch && antPool->dependentPools != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent pool
							CMPICount j;
							for (j = 0; j < antPool->maxDepPools; j++)
							{
								if (antPool->dependentPools[j])
								{
									StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
									srcInstance = PoolCreateInstance(depPool, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StoragePool instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
						if (bHaveVolumeMatch && antPool->dependentVolumes != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent volume
							CMPICount j;
							for (j = 0; j < antPool->maxDepVolumes; j++)
							{
								if (antPool->dependentVolumes[j])
								{
									StorageVolume *depVol = (StorageVolume *)antPool->dependentVolumes[j]->element;
									srcInstance = VolumeCreateInstance("OMC_StorageVolume", depVol, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create Storage Volume instance");
										return status;
									}
									CMReturnInstance(results, srcInstance); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a StorageVolume
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 0; 
					}
				}

				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHavePoolMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						// Return our single allocFrom pool where input volume is the dependent
						StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
						srcInstance = PoolCreateInstance(antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeAllocatedFromStoragePool"

	else if (strcasecmp(assocClass, "OMC_StorageElementSettingData") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageSettingWithHints",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "SettingData") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingData association for logicalDisk with deviceID = %s", deviceID));
					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						srcInstance = SettingCreateInstance(vol->setting, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageSetting instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageSettingWithHints") == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageVolume for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingsData association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->volume != NULL)
					{
						// it matches, return the one and only instance we have:
						srcInstance = VolumeCreateInstance("OMC_LogicalDisk", setting->volume, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create Logical Disk instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageElementSettingData"
	else if (strcasecmp(assocClass, "OMC_StorageVolumeStorageElementSettingData") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageSettingWithHints",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "SettingData") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingData association for logicalDisk with deviceID = %s", deviceID));
					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						srcInstance = SettingCreateInstance(vol->setting, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageSetting instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageSettingWithHints") == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageVolume for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageVolume",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingsData association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->volume != NULL)
					{
						// it matches, return the one and only instance we have:
						srcInstance = VolumeCreateInstance("OMC_StorageVolume", setting->volume, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcInstance))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create Storage Volume instance");
							return status;
						}
						CMReturnInstance(results, srcInstance); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeStorageElementSettingData"
*/
	_SMI_TRACE(1,("Leaving Associatiors(): %s",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ****************************************************************************
// AssociatorNames()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname
// 											and desired objectpath
//             char *assocClass
//             char *resultClass
//             char *role
//             char *resultRole
// ****************************************************************************
static CMPIStatus
AssociatorNames(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *assocClass,
		const char *resultClass,
		const char *role,
		const char *resultRole)
{
	_SMI_TRACE(1,("AssociatorNames() called. assocClass: %s", assocClass));

	CMPIStatus status = cmpiutilSimpleAssociatorNames( doReferences, self,
				 _BROKER, context, results, cop, assocClass,
				resultClass, role, resultRole);
/*
	char isReturned=0;
	char key[128] = {0};
	CMPIStatus status = {CMPI_RC_OK, NULL};
	const char * objClassName;
	const char *ns = CMGetCharPtr(CMGetNameSpace(cop, NULL));
	CMPIObjectPath *srcOP=NULL;
	CMPIData scsOPData;

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns);
	}

	if (strcasecmp(assocClass, "OMC_HostedStorageConfigurationService") == 0)
	{
		_SMI_TRACE(1,("Inside assocClass: OMC_HostedStorageConfigurationService"));
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));

//		objClassName = CMGetClassName(cop, &status);
//		objClassName = cop->ft->getClassName(cop,&status);
		_SMI_TRACE(1,("Inside assocClass: OMC_HostedStorageConfigurationService: class name %s", objClassName));
		if(status.rc != CMPI_RC_OK)
		{
			_SMI_TRACE(1,("Unable to get class name\n"));
			CMSetStatusWithChars( _BROKER, &status, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return status;			
		}
		if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
		{
			_SMI_TRACE(1,("Inside objClassName: OMC_UnitaryComputerSystem"));
				// this is a UnitaryComputerSystem
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

			if ((!resultClass || !strcasecmp(resultClass, "OMC_StorageConfigurationService")) &&
       			(!role || !strcasecmp(role, "Antecedent")) &&
       			(!resultRole || !strcasecmp(resultRole, "Dependent")))
			{
				srcOP = CMNewObjectPath(_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), 
				"OMC_StorageConfigurationService", &status);
				if(CMIsNullObject(srcOP))
				{
					_SMI_TRACE(1,("Unable to create Object Path\n"));
					CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return status;
				}
				status = EnumInstanceNames(NULL, context, results, srcOP);

				CMRelease(srcOP);
				isReturned = 1;
			}
		 	else if(assocClass)
				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		else if (strcasecmp(objClassName, "OMC_StorageConfigurationService") == 0)
		{
			// this is a StorageConfigurationService
			// need to get appropriate UnitaryComputerSystem for assocInst
			_SMI_TRACE(1,("Inside objClassName: OMC_StorageConfigurationService"));
			if ((!resultClass || !strcasecmp(resultClass, "OMC_UnitaryComputerSystem")) &&
	    		(!role || !strcasecmp(role, "Dependent")) &&
       			(!resultRole || !strcasecmp(resultRole, "Antecedent")))
			{
				CMPIObjectPath *oPath=NULL;
				oPath = omccmpiCreateCSObjectPath (_BROKER, ns, &status);
				_SMI_TRACE(1,("Finished creating system OP"));
				if(CMIsNullObject(oPath))
				{
					_SMI_TRACE(1,("Unable to create Object Path\n"));
					CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return status;
				}
					
				//Get the list of all target class object paths from the CIMOM 
				CMPIEnumeration * objectpaths = CBEnumInstanceNames(_BROKER, context, oPath, &status);
				
				if ((status.rc != CMPI_RC_OK) || CMIsNullObject(objectpaths))
				{ 
					_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 	CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  	return status;
				}
				
				_SMI_TRACE(1,("--- CBEnumInstanceNames() passed - %d", status.rc));
				
				scsOPData = CMGetNext(objectpaths , &status);
				CMRelease(objectpaths);
				CMReturnObjectPath(results, oPath);

				
				isReturned = 1;
			}
			else
				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		else
		{
			_SMI_TRACE(1,("Inside objClassName: Invalid"));
			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
	}

	else if (strcasecmp(assocClass, "OMC_StorageConfigurationElementCapabilities") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageConfigurationCapabilities") == 0)
			{
				// this is a StorageConfigurationCapabilities
				// need to get appropriate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageConfigurationService",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					char instanceID[256];
					omcMakeInstanceID("StorageConfigurationCapabilities", instanceID, 256);
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inInstance = CMGetCharPtr(data.value.string);
						if (inInstance)
						{
							_SMI_TRACE(1,("Comparing inInstance: %s   with instanceID: %s", inInstance, instanceID));
							if (strcasecmp(inInstance, instanceID) == 0)
							{
								// it matches, return the one and only OP we have:
								
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
								"OMC_StorageConfigurationService", &status);
								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageConfigurationElementCapabilities object path");
									return status;
								}
								status = EnumInstanceNames(NULL, context, results, srcOP);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key InstanceID"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key InstanceID"));
					}
				}
			}
			else if(strcasecmp(objClassName, "OMC_SystemStorageCapabilities") == 0)
			{
				// this is a SystemStorageCapabilities
				// need to get approprate StorageConfigurationService for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageConfigurationService",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcMakeInstanceID("SystemStorageCapabilities", key, 256);
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inInstance = CMGetCharPtr(data.value.string);
						if (inInstance)
						{
							_SMI_TRACE(1,("Comparing inInstance: %s   with instanceID: %s", inInstance, key));
							if (strcasecmp(inInstance, key) == 0)
							{
								// it matches, return the one and only OP we have:

								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
								"OMC_StorageConfigurationService", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageConfigurationElementCapabilities object path");
									return status;
								}
								status = EnumInstanceNames(NULL, context, results, srcOP);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key InstanceID"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key InstanceID"));
					}
				}
			}

			else if(strcmp(objClassName, "OMC_StorageConfigurationService") == 0)
			{
				// this is a StorageConfigurationService
				// need to get appropriate Capabilities for assocInst

				CMPIData data = CMGetKey(cop, "Name", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				int bReturnSCC = 1; // Return StorageConfigurationCapabilities reference
				int bReturnSSC = 1; // Return SystemStorageCapabilities

				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageConfigurationCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
					{
						if (!omccmpiClassIsDerivedFrom("OMC_SystemStorageCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
						{
							bHaveMatch = 0; // FALSE
						}
						else
						{
							bReturnSSC = 1;
							bReturnSCC = 0;
						}
					}
					else
					{
						bReturnSCC = 1;
						bReturnSSC = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Capabilities") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					if (strcasecmp(inName, "StorageConfigurationService") == 0)
					{
						// it matches, return the OP (s) requested:
						if (bReturnSCC)
						{
							srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
							"OMC_StorageConfigurationCapabilities", &status);
							if (CMIsNullObject(srcOP))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageConfigurationCapabilities object path");
								return status;
							}
							status = EnumInstanceNames(NULL, context, results, srcOP);
							CMRelease(srcOP);
							isReturned = 1;
						}
						if (bReturnSSC)
						{
							srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), 
							"OMC_SystemStorageCapabilities", &status);
							if (CMIsNullObject(srcOP))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create SystemStorageCapabilities object path");
								return status;
							}
							status = EnumInstanceNames(NULL, context, results, srcOP);
							CMRelease(srcOP);
							isReturned = 1;
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageConfigurationElementCapabilities"

	else if (strcasecmp(assocClass, "OMC_HostedStoragePool") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), "OMC_StoragePool", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create HostedStoragePool object path");
									return status;
								}
								status = EnumInstanceNames(NULL, context, results, srcOP);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get hosted pool association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = omccmpiCreateCSObjectPath (_BROKER, ns, &status);
						if (CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create ComputerSystem object path");
							return status;
						}
					
						CMPIEnumeration * objectpaths = CBEnumInstanceNames(_BROKER, context, srcOP, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(objectpaths))
						{ 
							_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  			return status;
						}
				
						scsOPData = CMGetNext(objectpaths , &status);
						CMReturnObjectPath(results, scsOPData.value.ref);
						CMRelease(objectpaths);
		
						isReturned = 1;
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_HostedStoragePool"

	else if (strcasecmp(assocClass, "OMC_LogicalDiskDevice") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), "OMC_LogicalDisk", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create LogicalDisk object path");
									return status;
								}
								status = EnumInstanceNames(NULL, context, results, srcOP);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get LogicalDiskDevice association for volume with deviceID = %s", deviceID));

					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = omccmpiCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						if (CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create ComputerSystem object path");
							return status;
						}
					
						CMPIEnumeration * objectpaths = CBEnumInstanceNames(_BROKER, context, srcOP, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(objectpaths))
						{ 
							_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  			return status;
						}
				
						scsOPData = CMGetNext(objectpaths , &status);
						CMReturnObjectPath(results, scsOPData.value.ref);
						CMRelease(objectpaths);
		
						isReturned = 1;
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_LogicalDiskDevice"
	else if (strcasecmp(assocClass, "OMC_StorageVolumeDevice") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageVolume",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return all the instances we have:
								srcOP = CMNewObjectPath(_BROKER,CMGetCharPtr(CMGetNameSpace(cop, &status)), "OMC_StorageVolume", &status);

								if (CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageVolume object path");
									return status;
								}
								status = EnumInstanceNames(NULL, context, results, srcOP);
								CMRelease(srcOP);
								isReturned = 1;
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a StorageVolume
				// need to get appropriate UnitaryComputerSystem for assocInst

				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupCompnnent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
				_SMI_TRACE(1,("Request to get StorageVolumeDevice association for volume with deviceID = %s", deviceID));

					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = omccmpiCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						if (CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create ComputerSystem object path");
							return status;
						}
					
						CMPIEnumeration * objectpaths = CBEnumInstanceNames(_BROKER, context, srcOP, &status);
						if((status.rc != CMPI_RC_OK) || CMIsNullObject(objectpaths))
						{ 
							_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
				 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
				  			return status;
						}
				
						scsOPData = CMGetNext(objectpaths , &status);
						CMReturnObjectPath(results, scsOPData.value.ref);
						CMRelease(objectpaths);
		
						isReturned = 1;
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}

	}// END--if assocClass == "OMC_ StorageVolumeDevice "

	else if (strcasecmp(assocClass, "OMC_StorageElementCapabilities") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageCapabilities") == 0)
			{
				// this is a StorageCapability
				// need to get approprate StoragePools for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get capabilities association for capability with instanceID = %s", instanceID));
					StorageCapability *cap = CapabilitiesFind(instanceID);
					if (cap != NULL && cap->pool != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = PoolCreateObjectPath(cap->pool, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageCapabilities for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Capabilities") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get element capabilities association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->capability != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = CapabilityCreateObjectPath(pool->capability, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageCapabilities object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageElementCapabilities"


	else if (strcasecmp(assocClass, "OMC_AssociatedComponentExtent") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
				strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get component association for extent with deviceID = %s", deviceID));
					StorageExtent *extent = ExtentsFind(deviceID);
					if (extent != NULL && extent->pool != NULL && !ExtentIsFreespace(extent))
					{
						// it matches, return the one and only instance we have:
						StoragePool *pool = extent->pool;
						srcOP = PoolCreateObjectPath(pool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageExtent for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent", 
												  resultClass, 
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get component extent association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->concreteComps != NULL)
					{
						// it matches, return all the instance we have:
						CMPICount i;
						for (i = 0; i < PoolExtentsSize(pool); i++)
						{
							if (!ExtentIsFreespace(pool->concreteComps[i]))
							{
								srcOP = ExtentCreateObjectPath(pool->concreteComps[i], ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageExtent object path");
									return status;
								}
								CMReturnObjectPath(results, srcOP); 
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AssociatedComponentExtent"

	else if (strcasecmp(assocClass, "OMC_AssociatedRemainingExtent") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
				strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "GroupComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get remaining association for extent with deviceID = %s", deviceID));
					StorageExtent *extent = ExtentsFind(deviceID);
					if (extent != NULL && extent->pool != NULL && ExtentIsFreespace(extent))
					{
						// it matches, return the one and only instance we have:
						StoragePool *pool = extent->pool;
						srcOP = PoolCreateObjectPath(pool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get appropriate StorageExtent for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("CIM_StorageExtent", 
												  resultClass, 
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "PartComponent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get remaining extent association for pool with instanceID = %s", instanceID));

					StoragePool *pool = PoolsFind(instanceID);
					if (pool != NULL && pool->concreteComps != NULL)
					{
						// it matches, return all the instance we have:
						CMPICount i;
						for (i = 0; i < PoolExtentsSize(pool); i++)
						{
							if (ExtentIsFreespace(pool->concreteComps[i]))
							{
								srcOP = ExtentCreateObjectPath(pool->concreteComps[i], ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageExtent object path");
									return status;
								}
								CMReturnObjectPath(results, srcOP); 
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AssociatedRemainingExtent"

	else if (strcasecmp(assocClass, "OMC_StorageSettingsGeneratedFromCapabilities") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageCapabilities") == 0)
			{
				// this is a StorageCapability
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageSettingWithHints",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Dependent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get generated settings association for capability with instanceID = %s", instanceID));
					StorageCapability *cap = CapabilitiesFind(instanceID);
					if (cap != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						CMPICount i;
						for (i = 0; i < SettingsSize(); i++)
						{
							StorageSetting *setting = SettingsGet(i);
							if (setting->capability == cap && setting->volume == NULL)
							{
								srcOP = SettingCreateObjectPath(setting, ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StorageSetting object path");
									return status;
								}
								CMReturnObjectPath(results, srcOP); 
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageSettingWithHints") == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageCapabilities for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageCapabilities",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get generated settings association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->capability != NULL && setting->volume == NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = CapabilityCreateObjectPath(setting->capability, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageCapabilities object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageSettingsGeneratedFromCapabilities"

	else if (strcasecmp(assocClass, "OMC_BasedOn") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
			   strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHaveExtentMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("CIM_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out CompositeExtentBasedOn case
						if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									srcOP = ExtentCreateObjectPath(depExt->antecedents[j]->extent, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageExtent object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
									{
										srcOP = ExtentCreateObjectPath(depExt, ns, &status);
										if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
										{
											CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageExtent object path");
											return status;
										}
										CMReturnObjectPath(results, srcOP); 
									}
								}
							}
						}
						if (bHaveVolumeMatch)
						{
							// Return all basedOn's where in input extent is antecedent for dependent volume/logicalDisk
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->volume)
								{
									srcOP = VolumeCreateObjectPath("OMC_Logical_Disk",antExt->dependents[j]->volume, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create LogicalDisk object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveAntMatch = 1; // TRUE
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveAntMatch = 0;

					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHaveExtentMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						srcOP = ExtentCreateObjectPath(depVol->antecedentExtent.extent, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageExtent object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_BasedOn"

	else if (strcasecmp(assocClass, "OMC_StorageVolumeBasedOn") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
			   strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHaveExtentMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("CIM_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_StorageVolume",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out CompositeExtentBasedOn case
						if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									srcOP = ExtentCreateObjectPath(depExt->antecedents[j]->extent, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageExtent object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (!depExt->composite || cap == NULL || cap->extentStripe <= 1)
									{
										srcOP = ExtentCreateObjectPath(depExt, ns, &status);
										if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
										{
											CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageExtent object path");
											return status;
										}
										CMReturnObjectPath(results, srcOP); 
									}
								}
							}
						}
						if (bHaveVolumeMatch)
						{
							// Return all basedOn's where in input extent is antecedent for dependent volume/logicalDisk
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->volume)
								{
									srcOP = VolumeCreateObjectPath("OMC_StorageVolume",antExt->dependents[j]->volume, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create Storage Volume object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a StorageVolume
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveAntMatch = 1; // TRUE
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveAntMatch = 0;

					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHaveExtentMatch)
				{
					_SMI_TRACE(1,("Request to get based on associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						srcOP = ExtentCreateObjectPath(depVol->antecedentExtent.extent, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageExtent object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeBasedOn"

	else if (strcasecmp(assocClass, "OMC_CompositeExtentBasedOn") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageExtent") == 0 ||
			   strcasecmp(objClassName, "OMC_CompositeExtent") == 0)
			{
				// this is a StorageExtent
				// need to get approprate StorageExtent for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHaveExtentMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("CIM_StorageExtent",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveExtentMatch = 0; 

					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get composite based on associations for dependent extent with deviceID = %s", deviceID));
					StorageExtent *depExt = ExtentsFind(deviceID);
					if (depExt != NULL && depExt->antecedents != NULL)
					{
						StorageCapability *cap = NULL;
						if (depExt->pool != NULL)
						{
							cap = depExt->pool->capability;
						}
						
						// Filter out BasedOn case
						if (depExt->composite && cap != NULL && cap->extentStripe > 1)
						{
							// Return all basedOn's where in input extent is dependent
							CMPICount j;
							for (j = 0; j < depExt->maxNumAntecedents; j++)
							{
								if (depExt->antecedents[j])
								{
									srcOP = ExtentCreateObjectPath(depExt->antecedents[j]->extent, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StorageExtent object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get composite based on associations for antecedent extent with deviceID = %s", deviceID));
					StorageExtent *antExt = ExtentsFind(deviceID);
					if (antExt != NULL && antExt->dependents != NULL)
					{
						if (bHaveExtentMatch )
						{
							// Return all basedOn's where input extent is antecedent for dependent extent
							CMPICount j;
							for (j = 0; j < antExt->maxNumDependents; j++)
							{
								if (antExt->dependents[j] && antExt->dependents[j]->extent)
								{
									StorageExtent *depExt = antExt->dependents[j]->extent;
									StorageCapability *cap = NULL;
									if (depExt->pool != NULL)
									{
										cap = depExt->pool->capability;
									}
									
									// Filter out CompositeExtentBasedOn case
									if (depExt->composite && cap != NULL && cap->extentStripe > 1)
									{
										srcOP = ExtentCreateObjectPath(depExt, ns, &status);
										if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
										{
											CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
											"Could not create StorageExtent object path");
											return status;
										}
										CMReturnObjectPath(results, srcOP); 
									}
								}
							}
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_CompositeExtentBasedOn"

	else if (strcasecmp(assocClass, "OMC_AllocatedFromStoragePool") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get approprate StoragePool or LogicalDisk for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHavePoolMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent pool with instanceID = %s", instanceID));
					StoragePool *depPool = PoolsFind(instanceID);
					if (depPool != NULL && depPool->antecedentPools != NULL)
					{
						// Return all allocFrom pools where input pool is the dependent
						CMPICount j;
						for (j = 0; j < depPool->maxAntPools; j++)
						{
							if (depPool->antecedentPools[j])
							{
								StoragePool *antPool = (StoragePool *)depPool->antecedentPools[j]->element;
								srcOP = PoolCreateObjectPath(antPool, ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StoragePool object path");
									return status;
								}
								CMReturnObjectPath(results, srcOP); 
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for antecedent pool with instanceID = %s", instanceID));
					StoragePool *antPool = PoolsFind(instanceID);
					if (antPool != NULL)
					{
						if (bHavePoolMatch && antPool->dependentPools != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent pool
							CMPICount j;
							for (j = 0; j < antPool->maxDepPools; j++)
							{
								if (antPool->dependentPools[j])
								{
									StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
									srcOP = PoolCreateObjectPath(depPool, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StoragePool object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
						if (bHaveVolumeMatch && antPool->dependentVolumes != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent volume
							CMPICount j;
							for (j = 0; j < antPool->maxDepVolumes; j++)
							{
								if (antPool->dependentVolumes[j])
								{
									StorageVolume *depVol = (StorageVolume *)antPool->dependentVolumes[j]->element;
									srcOP = VolumeCreateObjectPath("OMC_LogicalDisk", depVol, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create Logical Disk object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 0; 
					}
				}

				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHavePoolMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						// Return our single allocFrom pool where input volume is the dependent
						StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
						srcOP = PoolCreateObjectPath(antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_AllocatedFromStoragePool"
	else if (strcasecmp(assocClass, "OMC_StorageVolumeAllocatedFromStoragePool") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StoragePool") == 0)
			{
				// this is a StoragePool
				// need to get approprate StoragePool or StorageVolume for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				int bHaveVolumeMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					bHavePoolMatch = 0; 
					bHaveVolumeMatch = 0;

					// check
					if (omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 1; 

					}
					else if (omccmpiClassIsDerivedFrom("OMC_StorageVolume",
											  resultClass,
											  _BROKER,ns,&status))
					{
						bHaveVolumeMatch = 1;
					}
				}

				int bHaveDepMatch = 1;
				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					bHaveDepMatch = 0;
					bHaveAntMatch = 0;

					// check
					if (strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 1; 
					}
					else if (strcasecmp(resultRole, "Dependent") == 0)
					{
						bHaveDepMatch = 1;
					}
				}

				if (bHaveAntMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent pool with instanceID = %s", instanceID));
					StoragePool *depPool = PoolsFind(instanceID);
					if (depPool != NULL && depPool->antecedentPools != NULL)
					{
						// Return all allocFrom pools where input pool is the dependent
						CMPICount j;
						for (j = 0; j < depPool->maxAntPools; j++)
						{
							if (depPool->antecedentPools[j])
							{
								StoragePool *antPool = (StoragePool *)depPool->antecedentPools[j]->element;
								srcOP = PoolCreateObjectPath(antPool, ns, &status);
								if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create StoragePool object path");
									return status;
								}
								CMReturnObjectPath(results, srcOP); 
							}
						}
					}
				}
				if (bHaveDepMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for antecedent pool with instanceID = %s", instanceID));
					StoragePool *antPool = PoolsFind(instanceID);
					if (antPool != NULL)
					{
						if (bHavePoolMatch && antPool->dependentPools != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent pool
							CMPICount j;
							for (j = 0; j < antPool->maxDepPools; j++)
							{
								if (antPool->dependentPools[j])
								{
									StoragePool *depPool = (StoragePool *)antPool->dependentPools[j]->element;
									srcOP = PoolCreateObjectPath(depPool, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create StoragePool object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
						if (bHaveVolumeMatch && antPool->dependentVolumes != NULL)
						{
							// Return all allocFrom where in input pool is antecedent for dependent volume
							CMPICount j;
							for (j = 0; j < antPool->maxDepVolumes; j++)
							{
								if (antPool->dependentVolumes[j])
								{
									StorageVolume *depVol = (StorageVolume *)antPool->dependentVolumes[j]->element;
									srcOP = VolumeCreateObjectPath("OMC_StorageVolume", depVol, ns, &status);
									if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
									{
										CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create Storage Volume object path");
										return status;
									}
									CMReturnObjectPath(results, srcOP); 
								}
							}
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a StorageVolume
				// need to get approprate StoragePool for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);

				int bHavePoolMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StoragePool",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHavePoolMatch = 0; 
					}
				}

				int bHaveAntMatch = 1;
				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (!strcasecmp(resultRole, "Antecedent") == 0)
					{
						bHaveAntMatch = 0; 
					}
				}

				if (bHaveAntMatch && bHavePoolMatch)
				{
					_SMI_TRACE(1,("Request to get AllocFrom associations for dependent volume with deviceID = %s", deviceID));
					StorageVolume *depVol = VolumesFind(deviceID);
					if (depVol != NULL)
					{
						// Return our single allocFrom pool where input volume is the dependent
						StoragePool *antPool = (StoragePool *)depVol->antecedentPool.element;
						srcOP = PoolCreateObjectPath(antPool, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StoragePool object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeAllocatedFromStoragePool"

	else if (strcasecmp(assocClass, "OMC_StorageElementSettingData") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_LogicalDisk") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageSettingWithHints",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "SettingData") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingData association for logicalDisk with deviceID = %s", deviceID));
					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						srcOP = SettingCreateObjectPath(vol->setting, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageSetting object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageSettingWithHints") == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageVolume for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_LogicalDisk",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingsData association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->volume != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = VolumeCreateObjectPath("OMC_LogicalDisk", setting->volume, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create Logical Disk object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageElementSettingData"
	else if (strcasecmp(assocClass, "OMC_StorageVolumeStorageElementSettingData") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_StorageVolume") == 0)
			{
				// this is a LogicalDisk
				// need to get approprate StorageSetting for assoc inst
				// but if resultClass is set, it must match
				CMPIData data = CMGetKey(cop, "DeviceID", &status);
				const char *deviceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageSettingWithHints",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "SettingData") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingData association for logicalDisk with deviceID = %s", deviceID));
					StorageVolume *vol = VolumesFind(deviceID);
					if (vol != NULL)
					{
						// it matches, search for and return the one and only instance we have:
						srcOP = SettingCreateObjectPath(vol->setting, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create StorageSetting object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else if(strcmp(objClassName, "OMC_StorageSettingWithHints") == 0)
			{
				// this is a StorageSetting
				// need to get appropriate StorageVolume for assocInst
				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *instanceID = CMGetCharPtr(data.value.string);
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_StorageVolume",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					_SMI_TRACE(1,("Request to get settingsData association for setting with instanceID = %s", instanceID));

					StorageSetting *setting = SettingsFind(instanceID);
					if (setting != NULL && setting->volume != NULL)
					{
						// it matches, return the one and only instance we have:
						srcOP = VolumeCreateObjectPath("OMC_StorageVolume", setting->volume, ns, &status);
						if ((status.rc != CMPI_RC_OK) || CMIsNullObject(srcOP))
						{
							CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
							"Could not create Storage Volume object path");
							return status;
						}
						CMReturnObjectPath(results, srcOP); 
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_StorageVolumeStorageElementSettingData"
*/

	_SMI_TRACE(1,("Leaving AssociatiorNames(): \n",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ****************************************************************************
// References()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname
// 											and desired objectpath
//             char *resultClass
//             char *role
//             char **properties
// ****************************************************************************
static CMPIStatus
References(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *resultClass,
		const char *role ,
		const char** properties)
{
	_SMI_TRACE(1,("References() called"));

	CMPIStatus status = cmpiutilSimpleReferences( doReferences, self, _BROKER,
					context, results, cop, resultClass, role, properties);

	_SMI_TRACE(1,("Leaving References(): %s",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ****************************************************************************
// ReferenceNames()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname,
// 											and desired objectpath
//             char *resultClass
//             char *role
// ****************************************************************************
static CMPIStatus
ReferenceNames(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char* resultClass,
		const char* role)
{
	_SMI_TRACE(1,("ReferenceNames() called"));

	CMPIStatus status = cmpiutilSimpleReferenceNames( doReferences, self,
					_BROKER, context, results, cop, resultClass, role);

	_SMI_TRACE(1,("Leaving ReferenceNames(): %s",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}



// ****************************************************************************
// CMPI METHOD PROVIDER FUNCTIONS
// ****************************************************************************

// ****************************************************************************
// InvokeMethod()
//    params:  CMPIMethodMI* self:	[in] Handle to this provider
//             CMPIContext* context:[in] any additional context info
//             CMPIResult* results:	[out] Results
//             CMPIObjectPath* cop:	[in] target namespace and classname, and desired objectpath
//             char *methodName
//             CMPIArgs *in
//             CMPIArgs *out
// ****************************************************************************
static CMPIStatus 
InvokeMethod(	
		CMPIMethodMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *methodName,
		const CMPIArgs* in,
		CMPIArgs* out)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};
	const char *ns = CMGetCharPtr(CMGetNameSpace(cop, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(cop, NULL));
	_SMI_TRACE(1,("InvokeMethod() called "));

	if (SCSNeedToScan)
	{
		SCSScanStorage(ns, &status);
		if (status.rc != CMPI_RC_OK)
		{
			goto exit;
		}
	}

	_SMI_TRACE(1,("InvokeMethod() called, className = %s, methodName = %s", className, methodName));

	if (strcasecmp(className, StorageConfigurationServiceClassName) == 0)
	{
		SCSInvokeMethod(ns, methodName, in, out, results, &status, context, cop);
	}
	else if (strcasecmp(className, StoragePoolClassName) == 0)
	{
		CMPIData keyData = CMGetKey(cop, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to InvokeMethod for pool with instanceID = %s", instanceID));

		StoragePool *pool = PoolsFind(instanceID);
		if (pool)
		{
			PoolInvokeMethod(pool, ns, methodName, in, out, results, &status);
		}
		else
		{
			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_NOT_FOUND,
				"Unknown StoragePool instance passed to InvokeMethod");
		}
	}
	else if (strcasecmp(className, StorageCapabilitiesClassName) == 0)
	{
		CMPIData keyData = CMGetKey(cop, "InstanceID", NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
		_SMI_TRACE(1,("Request to InvokeMethod for capability with instanceID = %s", instanceID));

		StorageCapability *cap = CapabilitiesFind(instanceID);
		if (cap)
		{
			CapabilityInvokeMethod(cap, ns, methodName, in, out, results, &status);
		}
		else
		{
			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_NOT_FOUND,
				"Unknown StorageCapabilities instance passed to InvokeMethod");
		}
	}
	else
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_METHOD_NOT_AVAILABLE,
			"No method support for object class");
	}

	CMReturnDone(results);

exit:
	_SMI_TRACE(1,("Leaving InvokeMethod(): %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

// ----------------------------------------------------------------------------
// CMPI INDICATION PROVIDER FUNCTIONS
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

/* AuthorizeFilter() - verify whether this filter is allowed. */
static CMPIStatus AuthorizeFilter(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx,
		const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op, 
		const char* user)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */
   
	_SMI_TRACE(1,("AuthorizeFilter() called"));
	/* AuthorizeFilter not supported. */

	/* Finished. */
exit:
	_SMI_TRACE(1,("AuthorizeFilter() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------

/* MustPoll() - report whether polling mode should be used */
static CMPIStatus MustPoll(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx, 
		const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("MustPoll() called"));


	/* Finished. */
exit:
	_SMI_TRACE(1,("MustPool() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;

}

// ----------------------------------------------------------------------------

/* ActivateFilter() - begin monitoring a resource */
static CMPIStatus ActivateFilter(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx,
		const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op, 
		CMPIBoolean first)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("ActivateFilter() called"));
	/* ActivateFilter not supported. */


	/* Finished. */
exit:
	_SMI_TRACE(1,("ActivateFilter() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;

}

/* DeActivateFilter() - end monitoring a resource */
static CMPIStatus DeActivateFilter(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx, 
        	const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op, 
		CMPIBoolean last)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("DeActivateFilter() called"));
	/* DeActivateFilter not supported. */


	/* Finished. */
exit:
	_SMI_TRACE(1,("DeActivateFilter() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;

}

// ----------------------------------------------------------------------------
// INITIALIZE/CLEANUP FUNCTIONS
// ----------------------------------------------------------------------------

/* Cleanup() - perform any necessary cleanup immediately before this provider is unloaded. */
static CMPIStatus Cleanup(
		CMPIInstanceMI * self,			/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,	/* [in] Additional context info, if any. */
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations. */

	_SMI_TRACE(1,("Cleanup() called"));
   
	/* Free everything */
	VolumesFree();
	ExtentsFree();
	PoolsFree();
	CapabilitiesFree();
	SettingsFree();
	StorageInterfaceFree();
 
   /* Finished. */
exit:
	_SMI_TRACE(1,("Cleanup() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

static CMPIStatus AssociationCleanup(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("AssociationCleanup() called"));


	/* Do work here if necessary */

	/* Finished */
exit:
	_SMI_TRACE(1,("AssociationCleanup() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

static CMPIStatus MethodCleanup(
		CMPIMethodMI* self,
		const CMPIContext* context,
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("MethodCleanup() called"));

	/* Do work here if necessary */


	/* Finished */
exit:
	_SMI_TRACE(1,("Leaving MethodCleanup(): %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

static CMPIStatus IndicationCleanup(
		CMPIIndicationMI* self,
		const CMPIContext* context,
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("IndicationCleanup() called"));

	/* Do work here if necessary */

	/* Finished */
exit:
	_SMI_TRACE(1,("IndicationCleanup() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* Initialize() - perform any necessary initialization immediately after this provider is loaded. */
static void Initialize(
		CMPIInstanceMI * self)		/* [in] Handle to this provider (i.e. 'self'). */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations. */	

	_SMI_TRACE(1,("Initialize() called"));

	/* Do any general init stuff here */
//	if(s == NULL)
		s = createDefaultStorageInterface();
//	SCSNeedToScan = 1;

	/* Finished. */
exit:
	_SMI_TRACE(1,("Initialize() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
}

static      void EnableIndications (CMPIIndicationMI* mi,
                                       const CMPIContext *ctx)
{
        CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};  /* Return status of CIM operations */

        _SMI_TRACE(1,("EnableIndications() called"));
        /* DeActivateFilter not supported. */


        /* Finished. */
exit:
        _SMI_TRACE(1,("EnableIndications() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
//        return status;
}

static      void DisableIndications (CMPIIndicationMI* mi,
                                       const CMPIContext *ctx)
{
        CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};  /* Return status of CIM operations */

        _SMI_TRACE(1,("DisableIndications() called"));
        /* DeActivateFilter not supported. */


        /* Finished. */
exit:
        _SMI_TRACE(1,("DisableIndications() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
  //      return status;
}

// ----------------------------------------------------------------------------
// SETUP CMPI PROVIDER FUNCTION TABLES
// ----------------------------------------------------------------------------

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 * ------------------------------------------------------------------ */

/* Factory method that creates the handle to this provider, specifically
   setting up the instance provider function table:
   - 1st param is an optional prefix for the function names in the table.
     It is blank in this sample provider because the instance provider
     function names do not need a unique prefix.
   - 2nd param is the name to call this provider within the CIMOM. It is
     recommended to call providers "<_CLASSNAME>Provider". This name must be
     unique among all providers. Make sure to use the same name when
     registering the provider with the hosting CIMOM.
   - 3rd param is the local static variable acting as a handle to the CIMOM.
     This will be initialized by the CIMOM when the provider is loaded. 
   - 4th param specifies the provider's initialization function to be called
     immediately after loading the provider. Specify "CMNoHook" if no special
     initialization routine is required.
*/

CMInstanceMIStub(, omc_smi_array, _BROKER, Initialize(&mi));

/* ------------------------------------------------------------------ *
 * Association MI Factory
 * ------------------------------------------------------------------ */

CMAssociationMIStub(, omc_smi_array, _BROKER, CMNoHook);

/* ------------------------------------------------------------------ *
 * Method MI Factory
 * ------------------------------------------------------------------ */

CMMethodMIStub(, omc_smi_array, _BROKER, CMNoHook);

/* ------------------------------------------------------------------ *
 * Indication MI Factory
 * ------------------------------------------------------------------ */

CMIndicationMIStub(, omc_smi_array, _BROKER, CMNoHook);






