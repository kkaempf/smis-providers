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
 |   Provider code dealing with the OMC_LogicalDisk class
 |
 +-------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include <cmpiutil/base.h>
#include <cmpiutil/modifyFile.h>
#include <cmpiutil/string.h>
#include <cmpiutil/cmpiUtils.h>
#include <cmpiutil/cmpiSimpleAssoc.h>

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#ifdef __cplusplus
}
#endif

#include "Utils.h"
#include "ArrayProvider.h"
#include "StorageVolume.h"

extern CMPIBroker * _BROKER;

// Exported globals

// Module globals
static StorageVolume **VolumeArray = NULL;
static CMPICount NumVolumes = 0;
static CMPICount MaxNumVolumes = 0;

/////////////////////////////////////////////////////////////////////////////
//////////// Private helper functions ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//////////// Exported functions /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
StorageVolume* VolumeAlloc(const char *name, const char *deviceID)
{
	StorageVolume* volume = (StorageVolume *)malloc(sizeof (StorageVolume));
	if (volume)
	{
		memset(volume, 0, sizeof(StorageVolume));
		volume->estatus = ESTAT_EXPORTED;
		volume->ostatus = OSTAT_OK;
		volume->name = (char *)malloc(strlen(name) + 1);
		if (!volume->name)
		{
			free(volume);
			volume = NULL;
		}
		else
		{
			strcpy(volume->name, name);

			if (deviceID != NULL)
			{
				volume->deviceID = (char *)malloc(strlen(deviceID) + 1);
				if (!volume->deviceID)
				{
					free(volume->name);
					free(volume);
					volume = NULL;
				}
				else
				{
					strcpy(volume->deviceID, deviceID);
				}
			}
		}
	}
	return(volume);
}

/////////////////////////////////////////////////////////////////////////////
void VolumeFree(StorageVolume *volume)
{
	if (volume)
	{
		if (volume->deviceID)
		{
			free(volume->deviceID);
		}
		if (volume->name)
		{
			free(volume->name);
		}
		free(volume);
	}
}

/////////////////////////////////////////////////////////////////////////////
char *VolumeGenerateName(const char *prefix, char *genName, CMPICount nameMaxSize)
{
	CMPICount i;

	if (nameMaxSize < (strlen(prefix)+2))
	{
		return NULL;
	}

	// We need to generate a unique volume name based on prefix (i.e. <prefix>1, 
	// <prefix>2..., etc)

	i = 1;
	while (true)
	{
		sprintf(genName, "%s%d", prefix, i);
		StorageVolume *vol = VolumesFind(genName);
		if (vol == NULL)
		{
			break;
		}
		i++;
	}
	return genName;
}


/////////////////////////////////////////////////////////////////////////////
void VolumeCreateAllocFromAssociation(
						StoragePool *antecedentPool,
						StorageVolume *dependentVolume,
						CMPIUint64 spaceConsumed)
{
	CMPICount i;
	AllocFrom *depAF; 

	_SMI_TRACE(1,("VolumeCreateAllocFromAssociation() called"));
	_SMI_TRACE(1,("\tAntecedentPool = %s DependentVolume = %s", antecedentPool->name, dependentVolume->name));
	_SMI_TRACE(1,("\tSpacedConsumed = %llu",spaceConsumed));

	// See if we need to allocate our pool AllocFrom 
	if (antecedentPool->dependentVolumes == NULL)
	{
		// First time - we need to alloc our dependent AllocFrom volume array
		antecedentPool->dependentVolumes = (AllocFrom **)malloc(POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT * sizeof(AllocFrom *));
		if (antecedentPool->dependentVolumes == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to allocate pool-volume AllocFrom array"));
			goto exit;
		}
		antecedentPool->maxDepVolumes = POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT;
		antecedentPool->numDepVolumes = 0;
		for (i = 0; i < antecedentPool->maxDepVolumes; i++)
		{
			antecedentPool->dependentVolumes[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new pool->dependentVolumes %x, size = %d", antecedentPool->dependentVolumes, antecedentPool->maxDepVolumes));
	}

	// Allocate basedOn struct for pool dep volume array
	depAF = (AllocFrom *)malloc(sizeof(AllocFrom));

	if (depAF == NULL)
	{
		_SMI_TRACE(0,("ERROR: Unable to allocate AllocFrom structure"));
		goto exit;
	}

	depAF->element = dependentVolume;
	depAF->spaceConsumed = spaceConsumed;
	dependentVolume->antecedentPool.element = antecedentPool;
	dependentVolume->antecedentPool.spaceConsumed = spaceConsumed;

	// Find an available slot in pool-volume AllocFrom array and add new entry
	for (i = 0; i < antecedentPool->maxDepVolumes; i++)
	{
		if (antecedentPool->dependentVolumes[i] == NULL)
		{
			antecedentPool->dependentVolumes[i] = depAF;
			antecedentPool->numDepVolumes++;
			break;
		}
	}

	// See if we need to resize our array
	AllocFrom **newArray;
	if (antecedentPool->numDepVolumes == antecedentPool->maxDepVolumes)
	{
		_SMI_TRACE(1,("Resizing dependent volumes AllocFrom array"));

		newArray = (AllocFrom **) realloc (
						antecedentPool->dependentVolumes, 
						(antecedentPool->maxDepVolumes + POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT) 
						* sizeof(AllocFrom*));

		if (newArray == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to realloc dependent AllocFrom structures"));
		}
		else
		{
			for (i = antecedentPool->maxDepVolumes; 
					i < (antecedentPool->maxDepVolumes + POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT); 
						i++)
			{
				newArray[i] = NULL;
			}
			antecedentPool->dependentVolumes = newArray;
			antecedentPool->maxDepVolumes += POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT;
		}
	}

exit:
	_SMI_TRACE(1,("VolumeCreateAllocFromAssociation() done"));
}

/////////////////////////////////////////////////////////////////////////////
void VolumeCreateBasedOnAssociation(
						StorageVolume *dependent,
						StorageExtent *antecedent,
						CMPIUint64 startingAddress,
						CMPIUint64 endingAddress,
						CMPIUint16 orderIndex)
{
	CMPICount i;
	BasedOn *depBO; 

	_SMI_TRACE(1,("VolumeCreateBasedOnAssociation() called"));
	_SMI_TRACE(1,("\tDep = %s, Ant = %s", dependent->name, antecedent->name));
	_SMI_TRACE(1,("\tStart = %llu, End = %llu, Order = %d", startingAddress, endingAddress, orderIndex));

	if (antecedent->dependents == NULL)
	{
		// First time - we need to alloc our dependent basedOn extent array
		antecedent->dependents = (BasedOn **)malloc(EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT * sizeof(BasedOn *));
		if (antecedent->dependents == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to allocate extent based on array"));
			goto exit;
		}
		antecedent->maxNumDependents = EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT;
		antecedent->numDependents = 0;
		for (i = 0; i < antecedent->maxNumDependents; i++)
		{
			antecedent->dependents[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new extent->dependents %x, size = %d", antecedent->dependents, antecedent->maxNumDependents));
	}

	// Allocate basedOn struct
	depBO = (BasedOn *)malloc(sizeof(BasedOn));

	if (depBO == NULL)
	{
		_SMI_TRACE(0,("ERROR: Unable to allocate volume basedOn structure"));
		goto exit;
	}

	depBO->volume = dependent;
	depBO->extent = NULL;
	depBO->startingAddress = startingAddress;
	depBO->endingAddress = endingAddress;
	depBO->orderIndex = orderIndex;
	dependent->antecedentExtent.extent = antecedent;
	dependent->antecedentExtent.startingAddress = startingAddress;
	dependent->antecedentExtent.endingAddress = endingAddress;
	dependent->antecedentExtent.orderIndex = orderIndex;

	// Find an available slot in extent basedOn array and add new entry
	for (i = 0; i < antecedent->maxNumDependents; i++)
	{
		if (antecedent->dependents[i] == NULL)
		{
			antecedent->dependents[i] = depBO;
			antecedent->numDependents++;
			break;
		}
	}

	// See if we need to resize our array
	BasedOn **newArray;
	if (antecedent->numDependents == antecedent->maxNumDependents)
	{
		_SMI_TRACE(1,("Resizing dependent BasedOn array"));

		newArray = (BasedOn **) realloc (
						antecedent->dependents, 
						(antecedent->maxNumDependents + EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT) 
						* sizeof(BasedOn*));

		if (newArray == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to realloc dependent basedOn structures"));
		}
		else
		{
			for (i = antecedent->maxNumDependents; 
					i < (antecedent->maxNumDependents + EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT); 
						i++)
			{
				newArray[i] = NULL;
			}
			antecedent->dependents = newArray;
			antecedent->maxNumDependents += EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT;
		}
	}

exit:
	_SMI_TRACE(1,("VolumeCreateBasedOnAssociation() done"));
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath* VolumeCreateObjectPath(const char *classname,StorageVolume *volume, const char *ns, CMPIStatus *status)
{
	char buf[1024];

	_SMI_TRACE(1,("VolumeCreateObjectPath() called"));

	_SMI_TRACE(1,("VolumeCreateObjectPath() classname is %s", classname));

	/*CMPIObjectPath *cop = CMNewObjectPath(
								_BROKER, ns,
								"OMC_LogicalDisk",
								status); */
	CMPIObjectPath *cop = CMNewObjectPath(
								_BROKER, ns,
								classname,
								status); 

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(0,("VolumeCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
	}

	CMAddKey(cop, "SystemCreationClassName", cmpiutilGetComputerSystemClassName(), CMPI_chars);
	CMAddKey(cop, "SystemName", cmpiutilGetComputerSystemName(buf, 1024), CMPI_chars);
	CMAddKey(cop, "CreationClassName", classname, CMPI_chars);
	/* Vijay TODO */

	CMAddKey(cop, "DeviceID", volume->deviceID, CMPI_chars);
/*	if (strcasecmp (classname, "OMC_LogicalDisk"))
		CMAddKey(cop, "DeviceID", strcat("LD:",volume->deviceID), CMPI_chars);
	else if (strcasecmp (classname, "OMC_StorageVolume"))
		CMAddKey(cop, "DeviceID", strcat("SV:",volume->deviceID), CMPI_chars); */

exit:
	_SMI_TRACE(1,("VolumeCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance* VolumeCreateInstance(const char *classname,StorageVolume *vol, const char *ns, CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[1024];
	CMPIValue val;
	CMPIArray *arr; 
	StoragePool *antPool; 

	_SMI_TRACE(1,("VolumeCreateInstance() called, classname: %s", classname));
	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, classname, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(0,("VolumeCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "SystemCreationClassName", 
		cmpiutilGetComputerSystemClassName(), CMPI_chars);
	CMSetProperty(ci, "SystemName", 
		cmpiutilGetComputerSystemName(buf, 1024), CMPI_chars);

	CMSetProperty(ci, "CreationClassName", classname, CMPI_chars);
	CMSetProperty(ci, "DeviceID", vol->deviceID, CMPI_chars);

	val.uint16 = 1;
	CMSetProperty(ci, "DataOrganization", &val, CMPI_uint16);
	val.uint16 = 3;
	CMSetProperty(ci, "Access", &val, CMPI_uint16);


	val.uint64 = 1;
	CMSetProperty(ci, "BlockSize", &val, CMPI_uint64);
	val.uint64 = vol->size;
	CMSetProperty(ci, "NumberOfBlocks", &val, CMPI_uint64);
	val.uint64 = vol->size;
	CMSetProperty(ci, "ConsumableBlocks", &val, CMPI_uint64);
//	val.uint64 = vol->size;
//	CMSetProperty(ci, "Size", &val, CMPI_uint64);
	val.boolean = 0;
	CMSetProperty(ci, "SequentialAccess", &val, CMPI_boolean);
	val.uint8 = 100;
	CMSetProperty(ci, "DeltaReservation", &val, CMPI_uint8);

	antPool = (StoragePool *)vol->antecedentPool.element;
	val.boolean = antPool->capability->noSinglePointOfFailure;
	CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
	val.uint16 = antPool->capability->dataRedundancy;
	CMSetProperty(ci, "DataRedundancy", &val, CMPI_uint16);
	val.uint16 = antPool->capability->packageRedundancy;
	CMSetProperty(ci, "PackageRedundancy", &val, CMPI_uint16);

	if (antPool->capability->dataRedundancy > 1)
	{
		val.boolean = 1;
		CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
	}
	else
	{
		val.boolean = 0;
		CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
	}

	val.dateTime = CMNewDateTime(_BROKER, NULL);
	CMSetProperty(ci, "InstallDate", &val, CMPI_dateTime);

	arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
	val.uint16 = vol->ostatus;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "OperationalStatus", &val, CMPI_uint16A);
	val.uint16 = vol->estatus;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "ExtentStatus", &val, CMPI_uint16A);
	val.uint16 = GetHealthState(vol->ostatus);
	CMSetProperty(ci, "HealthState", &val, CMPI_uint16);
//	CMSetProperty(ci, "StatusDescriptions", GetStatusDescription(vol->ostatus), CMPI_chars);

//	sprintf(buf, "%s%s%s", "/dev/", antPool->name, vol->name);
//	CMSetProperty(ci, "Name", buf, CMPI_chars);
	CMSetProperty(ci, "Name", vol->name, CMPI_chars);
	CMSetProperty(ci, "ElementName", vol->name, CMPI_chars);
	CMSetProperty(ci, "Caption", vol->name, CMPI_chars);

exit:
	_SMI_TRACE(1,("VolumeCreateInstance() done"));
	return ci;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *VolumeCreateDeviceAssocInstance(
					const char *classname,
					StorageVolume *vol,
					const char *ns,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "GroupComponent",  "PartComponent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *ldcop;

	_SMI_TRACE(1,("VolumeCreateDeviceAssocInstance() called"));

	// Create and populate ComputerSystem object path
	CMPIObjectPath *cscop = cmpiutilCreateCSObjectPath(_BROKER, ns, pStatus);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create ComputerSystem cop");
		return NULL;
	}

	// Create and populate LogicalDisk object path
	//Here check if the classname is OMC_LogicalDiskDevice or OMC_StorageVolumeDevice
/*	if (strcasecmp(classname, LogicalDiskDeviceClassName) == 0)
	{
		ldcop = VolumeCreateObjectPath(LogicalDiskClassName, vol, ns, &status);
	}
	else if (strcasecmp(classname, StorageVolumeDeviceClassName) == 0)
	{
		ldcop = VolumeCreateObjectPath(StorageVolumeClassName, vol, ns, &status);
	}
*/
	ldcop = VolumeCreateObjectPath(classname, vol, ns, &status);
	if (CMIsNullObject(ldcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageVolume/LogicalDisk cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											classname,
											classKeys,
											properties,
											"GroupComponent",
											"PartComponent",
											cscop,
											ldcop,
											pStatus);

	_SMI_TRACE(1,("Leaving VolumeCreateDeviceAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *VolumeCreateDeviceAssocObjectPath(
					const char *classname,
					StorageVolume *vol,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *ldcop;
	_SMI_TRACE(1,("VolumeCreateDeviceAssocObjectPath() called"));
	_SMI_TRACE(1,("VolumeCreateDeviceAssocObjectPath():classname is %s",classname));

	CMPIObjectPath *cscop = cmpiutilCreateCSObjectPath( _BROKER, ns, pStatus);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create ComputerSystem cop");
		return NULL;
	}

	// Create and populate StorageVolume/LogicalDisk object path
	//Here check if the classname is OMC_LogicalDiskDevice or OMC_StorageVolumeDevice
	if (strcasecmp(classname, LogicalDiskDeviceClassName) == 0)
	{
		ldcop  = VolumeCreateObjectPath(LogicalDiskClassName, vol, ns, &status);
	}
	else if (strcasecmp(classname, StorageVolumeDeviceClassName) == 0)
	{
		ldcop = VolumeCreateObjectPath(StorageVolumeClassName, vol, ns, &status);
	}

//	ldcop = VolumeCreateObjectPath(classname, vol, ns, &status);
	if (CMIsNullObject(ldcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageVolume/LogicalDisk cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									classname,
									"GroupComponent",
									"PartComponent",
									cscop,
									ldcop,
									pStatus); 


	_SMI_TRACE(1,("Leaving VolumeCreateDeviceAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance* VolumeCreateBasedOnAssocInstance(
					const char *classname,
					StorageVolume *dependent,
					StorageExtent *antecedent,
					BasedOn *basedOn,
					const char *ns, 
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "Antecedent", "Dependent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *depcop ;
	_SMI_TRACE(1,("VolumeCreateBasedOnAssocInstance() called"));

	// Create and populate antecedent extent object path
	CMPIObjectPath *antcop = ExtentCreateObjectPath(antecedent, ns, &status);
	if (CMIsNullObject(antcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create antecedent StorageExtent cop");
		return NULL;
	}

	// Create and populate dependent StorageVolume object path
/*	if (strcasecmp(classname, LogicalDiskClassName) == 0)
	{
		depcop = VolumeCreateObjectPath(LogicalDiskClassName, dependent, ns, &status);
	}
	else if (strcasecmp(classname, StorageVolumeClassName) == 0)
	{
		depcop = VolumeCreateObjectPath(StorageVolumeClassName, dependent, ns, &status);
	}
*/
	depcop = VolumeCreateObjectPath(classname, dependent, ns, &status);
	if (CMIsNullObject(depcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StorageVolume cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											BasedOnClassName,
											classKeys,
											properties,
											"Antecedent",
											"Dependent",
											antcop,
											depcop,
											pStatus);

	// We also need to set address/orderidx properties
	CMPIValue val;
	val.uint64 = basedOn->startingAddress;
	CMSetProperty(assocInst, "StartingAddress", &val, CMPI_uint64);
	val.uint64 = basedOn->endingAddress;
	CMSetProperty(assocInst, "EndingAddress", &val, CMPI_uint64);
	val.uint16 = basedOn->orderIndex;
	CMSetProperty(assocInst, "OrderIndex", &val, CMPI_uint16);

	_SMI_TRACE(1,("Leaving VolumeCreateBasedOnAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath*	VolumeCreateBasedOnAssocObjectPath(
						const char *classname,
						StorageVolume *dependent,
						StorageExtent *antecedent,
  						const char *ns, 
						CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *depcop;
	_SMI_TRACE(1,("VolumeCreateBasedOnAssocObjectPath() called"));

	// Create and populate antecedent extent object path
	CMPIObjectPath *antcop = ExtentCreateObjectPath(antecedent, ns, &status);
	if (CMIsNullObject(antcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create antecedent StorageExtent cop");
		return NULL;
	}

	// Create and populate dependent extent object path
/*	if (strcasecmp(classname, LogicalDiskClassName) == 0)
	{
		depcop  = VolumeCreateObjectPath(LogicalDiskClassName, dependent, ns, &status);
	}
	else if (strcasecmp(classname, StorageVolumeClassName) == 0)
	{
		depcop = VolumeCreateObjectPath(StorageVolumeClassName, dependent, ns, &status);
	}
*/
	depcop = VolumeCreateObjectPath(classname,dependent, ns, &status);
	if (CMIsNullObject(depcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StorageVolume cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									BasedOnClassName,
									"Antecedent",
									"Dependent",
									antcop,
									depcop,
									pStatus);

	_SMI_TRACE(1,("Leaving VolumeCreateBasedOnAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance*	VolumeCreateAllocFromAssocInstance(
					const char *classname,
					StorageVolume *depVolume,
					StoragePool *antPool,
					CMPIUint64 spaceConsumed,
					const char *ns, 
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "Dependent",  "Antecedent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *dvcop;
	_SMI_TRACE(1,("VolumeCreateAllocFromAssocInstance() called"));

	_SMI_TRACE(1,(" VolumeCreateAllocFromAssocInstance : classname is %s", classname));
	// Create and populate dependent StorageVolume object path
	//Here check if the classname is OMC_AllocatedFromStoragePool or OMC_StorageVolumeAllocatedFromStoragePool
/*	if (strcasecmp(classname, AllocatedFromStoragePoolClassName) == 0)
	{
		dvcop  = VolumeCreateObjectPath(LogicalDiskClassName, depVolume, ns, &status);
	}
	else if (strcasecmp(classname, StorageVolumeAllocatedFromStoragePoolClassName) == 0)
	{
		dvcop = VolumeCreateObjectPath(StorageVolumeClassName, depVolume, ns, &status);
	}
*/
	dvcop = VolumeCreateObjectPath(classname, depVolume, ns, &status);

	if (CMIsNullObject(dvcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent LogicalDisk cop");
		return NULL;
	}

	// Create and populate antecedent StorageExtent object path
	CMPIObjectPath *apcop = PoolCreateObjectPath(antPool, ns, &status);
	if (CMIsNullObject(apcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create antecedent StoragePool cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											AllocatedFromStoragePoolClassName,
											classKeys,
											properties,
											"Dependent",
											"Antecedent",
											dvcop,
											apcop,
											pStatus);

	// Don't forget to set the spaceConsumed
	CMSetProperty(assocInst, "SpaceConsumed", &spaceConsumed, CMPI_uint64);

	_SMI_TRACE(1,("Leaving VolumeCreateAllocFromAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath*	VolumeCreateAllocFromAssocObjectPath(
					const char *classname,
					StorageVolume *depVolume,
					StoragePool *antPool,
  					const char *ns, 
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("VolumeCreateAllocFromAssocObjectPath() called"));

	_SMI_TRACE(1,("VolumeCreateAllocFromAssocObjectPath() classname is %s", classname));
	// Create and populate dependent StorageVolume object path
	CMPIObjectPath *dvcop;
/*	if (strcasecmp(classname, AllocatedFromStoragePoolClassName) == 0)
		dvcop = VolumeCreateObjectPath(LogicalDiskClassName,depVolume, ns, &status);
	else if (strcasecmp(classname, StorageVolumeAllocatedFromStoragePoolClassName) == 0)
		dvcop = VolumeCreateObjectPath(StorageVolumeClassName,depVolume, ns, &status);
*/
	dvcop = VolumeCreateObjectPath(classname,depVolume, ns, &status);
	if (CMIsNullObject(dvcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StorageVolume/LogicalDisk cop");
		return NULL;
	}

	// Create and populate antecedent StoragePool object path
	CMPIObjectPath *apcop = PoolCreateObjectPath(antPool, ns, &status);
	if (CMIsNullObject(apcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create antecedent StoragePool cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									AllocatedFromStoragePoolClassName,
									"Dependent",
									"Antecedent",
									dvcop,
									apcop,
									pStatus);

	_SMI_TRACE(1,("Leaving VolumeCreateAllocFromAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *VolumeCreateSettingAssocInstance(
					const char *classname,
					StorageVolume *vol,
					const char *ns,
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "ManagedElement",  "SettingData", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("VolumeCreateSettingAssocInstance() called"));

	// Create and populate StorageVolume object path
	CMPIObjectPath *svcop;
/*	if (strcasecmp(classname, StorageElementSettingDataClassName) == 0)
		svcop = VolumeCreateObjectPath(LogicalDiskClassName,vol, ns, &status);
	else if (strcasecmp(classname,  StorageVolumeStorageElementSettingDataClassName) == 0)
		svcop = VolumeCreateObjectPath(StorageVolumeClassName,vol, ns, &status);
*/
	svcop = VolumeCreateObjectPath(classname,vol, ns, &status);

	if (CMIsNullObject(svcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create LogicalDisk cop");
		return NULL;
	}

	// Create and populate StorageSetting object path
	CMPIObjectPath *sscop = SettingCreateObjectPath(vol->setting, ns, &status);
	if (CMIsNullObject(sscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageSetting cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											StorageElementSettingDataClassName,
											classKeys,
											properties,
											"ManagedElement",
											"SettingData",
											svcop,
											sscop,
											pStatus);
	CMPIValue val;
	val.uint16 = 1;
	CMSetProperty(assocInst, "IsDefault", &val, CMPI_uint16);
	CMSetProperty(assocInst, "IsCurrent", &val, CMPI_uint16);

	_SMI_TRACE(1,("Leaving VolumeCreateSettingAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *VolumeCreateSettingAssocObjectPath(
					const char *classname,
					StorageVolume *vol,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("VolumeCreateGFCAssocObjectPath() called"));

	// Create and populate StorageVolume object path
	CMPIObjectPath *svcop;
/*	if (strcasecmp(classname, StorageElementSettingDataClassName) == 0)
		svcop = VolumeCreateObjectPath(LogicalDiskClassName,vol, ns, &status);
	else if (strcasecmp(classname,  StorageVolumeStorageElementSettingDataClassName) == 0)
		svcop = VolumeCreateObjectPath(StorageVolumeClassName,vol, ns, &status);
*/
	svcop = VolumeCreateObjectPath(classname,vol, ns, &status);
	if (CMIsNullObject(svcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageVolume cop");
		return NULL;
	}

	// Create and populate StorageSetting object path
	CMPIObjectPath *sscop = SettingCreateObjectPath(vol->setting, ns, &status);
	if (CMIsNullObject(sscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageSetting cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									StorageElementSettingDataClassName,
									"ManagedElement",
									"SettingData",
									svcop,
									sscop,
									pStatus);

	_SMI_TRACE(1,("Leaving VolumeCreateSettingAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPICount VolumesSize()
{
	return NumVolumes;
}

/////////////////////////////////////////////////////////////////////////////
StorageVolume* VolumesGet(const CMPICount index)
{
	if (MaxNumVolumes <= index)
	{
		return NULL;
	}

	return VolumeArray[index];
}

/////////////////////////////////////////////////////////////////////////////
StorageVolume* VolumesFind(const char *name)
{
	CMPICount i;
	StorageVolume *currVolume;

	if (VolumeArray == NULL)
		return NULL;

	for (i = 0; i < MaxNumVolumes; i++)
	{
		currVolume = VolumeArray[i];
		if (currVolume)
		{
			if (strcmp(currVolume->name, name) == 0)
			{
				// Found it
				return currVolume;
			}
		}
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
void VolumesAdd(StorageVolume *volume)
{
	StorageVolume	**newArray;
	CMPICount		i;
	StorageVolume	*currVolume;

	// See if this is the first time thru
	if (VolumeArray == NULL)
	{
		// First time - we need to alloc our VolumeArray
		VolumeArray = (StorageVolume **)malloc(VOLUME_ARRAY_SIZE_DEFAULT * sizeof(StorageVolume *));
		if (VolumeArray == NULL)
		{
			_SMI_TRACE(0,("VolumesAdd() ERROR: Unable to allocate volume array"));
			return;
		}
		MaxNumVolumes = VOLUME_ARRAY_SIZE_DEFAULT;
		for (i = 0; i < MaxNumVolumes; i++)
		{
			VolumeArray[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new VolumeArray %x, size = %d", VolumeArray, MaxNumVolumes));
	}

	// See if this volume is already in the array
	currVolume = VolumesFind(volume->deviceID);
	if (currVolume)
	{
		_SMI_TRACE(1,("VolumesAdd() volume %s already exists, replace old with new", volume->deviceID));
		VolumesRemove(currVolume);
		VolumeFree(currVolume);
	}

	// Try to find an empty slot
	for (i = 0; i < MaxNumVolumes; i++)
	{
		if (VolumeArray[i] == NULL)
		{
			// We found an available slot
			_SMI_TRACE(1,("VolumesAdd(): Adding %s", volume->deviceID));
			VolumeArray[i] = volume;
			NumVolumes++;
			break;
		}
	}

	// See if we need to resize the array
	if (i == MaxNumVolumes)
	{
		// No available slots, need to resize
		_SMI_TRACE(1,("VolumesAdd(): VolumeArray out of room, resizing"));

		newArray = (StorageVolume **)realloc(VolumeArray, (MaxNumVolumes + VOLUME_ARRAY_SIZE_DEFAULT) * sizeof(StorageVolume *));
		if (newArray == NULL)
		{
			_SMI_TRACE(0,("VolumesAdd() ERROR: Unable to resize volume array"));
			return;
		}
		for (i = MaxNumVolumes; i < (MaxNumVolumes + VOLUME_ARRAY_SIZE_DEFAULT); i++)
		{
			newArray = NULL;
		}

		VolumeArray = newArray;

		// Don't forget to add the new volume
		_SMI_TRACE(1,("VolumesAdd()-resize-: Adding %s", volume->deviceID));
		VolumeArray[MaxNumVolumes] = volume;
		NumVolumes++;
		MaxNumVolumes += VOLUME_ARRAY_SIZE_DEFAULT;
	}
}

/////////////////////////////////////////////////////////////////////////////
void VolumesRemove(StorageVolume *volume)
{
	CMPICount	i;
	StorageVolume *currVolume;

	// Find & remove volume
	for (i = 0; i < MaxNumVolumes; i++)
	{
		currVolume = VolumeArray[i];
		if (strcmp(currVolume->deviceID, volume->deviceID) == 0)
		{
			// Found it, delete it
			_SMI_TRACE(1,("VolumesAdd(): Removing %s", volume->deviceID));
			VolumeArray[i] = NULL;
			NumVolumes--;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void VolumesFree()
{
	CMPICount i;
	StorageVolume *volume;

	_SMI_TRACE(1,("VolumesFree() called, VolumeArray = %x, NumVolumes = %d, MaxNumVolumes = %d", VolumeArray, NumVolumes, MaxNumVolumes));

	if (VolumeArray == NULL)
		return;

	for (i = 0; i < MaxNumVolumes; i++)
	{
		volume = VolumeArray[i];
		if (volume)
		{
			VolumeFree(volume);
		}
	}

	free(VolumeArray);
	VolumeArray = NULL;
	NumVolumes = 0;
	MaxNumVolumes = 0;
}

