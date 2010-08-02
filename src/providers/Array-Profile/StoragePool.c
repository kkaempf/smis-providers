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
 |	 OMC SMI-S Volume Management provider
 |
 |---------------------------------------------------------------------------
 |
 | $Id: 
 |
 |---------------------------------------------------------------------------
 | This module contains:
 |   Provider code dealing with OMC_StoragePool class
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
#include "StorageConfigurationService.h"
#include "StoragePool.h"
#include "StorageSetting.h"

extern CMPIBroker * _BROKER;

// Exported globals

// Module globals
static StoragePool **PoolArray = NULL;
/*static CMPICount NumPools = 0;
static CMPICount MaxNumPools = 0; vijay */
static CMPICount NumPools = 0;
static CMPICount MaxNumPools = 0; 


/////////////////////////////////////////////////////////////////////////////
//////////// Private helper functions ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//////////// Exported functions /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
StoragePool* PoolAlloc(const char *poolName)
{
	_SMI_TRACE(1,("PoolAlloc() called: Name = %s", poolName));
	StoragePool* pool = (StoragePool *)malloc(sizeof (StoragePool));
	if (pool)
	{
		memset(pool, 0, sizeof(StoragePool));
		pool->ostatus = OSTAT_OK;
		pool->instanceID = (char *)malloc(CMPIUTIL_INSTANCEID_PREFIX_SIZE + strlen(poolName) + 1);
		if (!pool->instanceID)
		{
			free(pool);
			pool = NULL;
		}
		else
		{
			cmpiutilMakeInstanceID(
				poolName, 
				pool->instanceID, 
				CMPIUTIL_INSTANCEID_PREFIX_SIZE + strlen(poolName) + 1);

			pool->name = (char *)malloc(strlen(poolName) + 1);
			if (!pool->name)
			{
				free(pool->instanceID);
				free(pool);
				pool = NULL;
			}
			else
			{
				strcpy(pool->name, poolName);
				_SMI_TRACE(1,("PoolAlloc() called: pool->name = %s, InstanceID: %s", pool->name,pool->instanceID));
			}
		}
	}
	return(pool);
}

/////////////////////////////////////////////////////////////////////////////
void PoolFree(StoragePool *pool)
{
	CMPICount i;
	if (pool)
	{
		if (pool->instanceID)
		{
			free(pool->instanceID);
		}
		if (pool->name)
		{
			free(pool->name);
		}
		PoolExtentsFree(pool);

 		// Clean up all antecedent AllocFrom information 
		if (pool->antecedentPools)
		{
			for (i = 0; i < pool->maxAntPools; i++)
			{
				AllocFrom *antAF = pool->antecedentPools[i];
				if (antAF)
				{
					free (antAF);
				}
			}
			free(pool->antecedentPools);
		}

		// Also clean up dependent AllocFrom information
		if (pool->dependentPools)
		{
			for (i = 0; i < pool->maxDepPools; i++)
			{
				AllocFrom *depAF = pool->dependentPools[i];
				if (depAF)
				{
					free (depAF);
				}
			}
			free(pool->dependentPools);
		}

		if (pool->dependentVolumes)
		{
			for (i = 0; i < pool->maxDepVolumes; i++)
			{
				AllocFrom *depAF = pool->dependentVolumes[i];
				if (depAF)
				{
					free (depAF);
				}
			}
			free(pool->dependentVolumes);
		}

		// Finally, free the pool itself
		free(pool);
	}
}

/////////////////////////////////////////////////////////////////////////////
char *PoolGenerateName(const char *prefix, char *genName, CMPICount nameMaxSize)
{
	CMPICount i;

	if (nameMaxSize < (strlen(prefix)+2))
	{
		return NULL;
	}

	// We need to generate a unique pool name based on prefix (i.e. <prefix>1, 
	// <prefix>2..., etc)

	i = 1;
	while(true)
	{
		sprintf(genName, "%s%d", prefix, i);
		StoragePool *pool = PoolsFindByName(genName);
		if (pool == NULL)
		{
			break;
		}
		i++;
	}
	return genName;
}

/////////////////////////////////////////////////////////////////////////////
/*CMPIUint32 PoolGetShrinkExtents(
						StoragePool *pool,
						const object_handle_t evmsContHandle,
						CMPIUint64 *shrinkSize,
						CMPIUint64 *excessShrinkage,
						handle_array_t **shrinkHandles)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	*shrinkHandles = NULL;
	CMPIUint64 calcShrinkSize = 0;
	handle_array_t *outHandles = NULL; 
	handle_array_t *acceptableObjects = NULL;

	_SMI_TRACE(1,("PoolGetShrinkExtents() called"));

	task_handle_t taskHandle = 0;
	rc = evms_create_task(evmsContHandle, EVMS_Task_Shrink, &taskHandle);
	if (rc != 0)
	{
		_SMI_TRACE(0,("Call to evms_create_task returned error %d", rc));
		rc = M_FAILED;
		goto exit;
	}

	_SMI_TRACE(1,("\tCalling evms_get_acceptable_objects"));
	rc = evms_get_acceptable_objects(taskHandle, &acceptableObjects);
	if (rc != 0)
	{
		_SMI_TRACE(0,("Call to evms_get_acceptable_objects returned error %d", rc));
		rc = M_FAILED;
		goto exit;
	}

	_SMI_TRACE(1,("\tFound %d acceptable shrink objects", acceptableObjects->count));
	*shrinkHandles = (handle_array_t *)malloc(sizeof(handle_array_t) + acceptableObjects->count*sizeof(object_handle_t));
	outHandles = *shrinkHandles;
	outHandles->count = 0;

	uint i;
	for (i = 0; i < acceptableObjects->count; i++)
	{
		handle_object_info_t *infoHandle;
		rc = evms_get_info(acceptableObjects->handle[i], &infoHandle);
		if (rc != 0)
		{
			_SMI_TRACE(0,("Error getting info for removeable extent, rc = %d", rc));
			rc = M_FAILED;
			goto exit;
		}

		char *extentName = infoHandle->info.object.name;
		CMPIUint64 extentSize = infoHandle->info.object.size * 512;

		_SMI_TRACE(1,("Found acceptable shrink object, name = %s, size = %llu", extentName, extentSize));

		evms_free(infoHandle);

		calcShrinkSize += extentSize;
		outHandles->count++;
		outHandles->handle[i] = acceptableObjects->handle[i];

		if (calcShrinkSize >= *shrinkSize)
		{
			_SMI_TRACE(1,("Found enough shrink objects to satisfy shrinkage goal"));
			break;
		}
	}

	if (calcShrinkSize < *shrinkSize)
	{
		_SMI_TRACE(0, ("Unable to shrink pool by %llu bytes, only %llu bytes available for shrinkage", *shrinkSize, calcShrinkSize));
		*shrinkSize = calcShrinkSize;
		rc = M_SIZE_NOT_SUPPORTED;
		goto exit;
	}

	*excessShrinkage = calcShrinkSize - *shrinkSize;

exit:
	if (acceptableObjects)
	{
		evms_free(acceptableObjects);
	}
	if (taskHandle)
	{
		evms_destroy_task(taskHandle);
	}

	_SMI_TRACE(1,("PoolGetShrinkExtents() done, rc = %d, calcShrinkSize = %llu, excess = %lld", rc, calcShrinkSize, *excessShrinkage));

	return rc;
}

*/
/////////////////////////////////////////////////////////////////////////////
void PoolCreateAllocFromAssociation(
						StoragePool *antecedent,
						StoragePool *dependent,
						CMPIUint64 spaceConsumed)
{
	CMPICount i;

	// Allocate basedOn structs
	AllocFrom *antAF = (AllocFrom *)malloc(sizeof(AllocFrom));
	AllocFrom *depAF = (AllocFrom *)malloc(sizeof(AllocFrom));

	_SMI_TRACE(1,("PoolCreateAllocFromAssociation() called"));
	_SMI_TRACE(1,("\tAntecedentPool = %s DependentPool = %s", antecedent->name, dependent->name));
	_SMI_TRACE(1,("\tSpacedConsumed = %lld",spaceConsumed));

	// See if we need to allocate our AllocFrom arrays
	if (dependent->antecedentPools == NULL)
	{
		// First time - we need to alloc our antecedentPool AllocFrom array
		dependent->antecedentPools = (AllocFrom **)malloc(POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT * sizeof(AllocFrom *));
		if (dependent->antecedentPools == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to allocate pool AllocFrom array"));
			goto exit;
		}
		dependent->maxAntPools = POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT;
		dependent->numAntPools = 0;
		for (i = 0; i < dependent->maxAntPools; i++)
		{
			dependent->antecedentPools[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new pool->antecedentPools %x, size = %d", dependent->antecedentPools, dependent->maxAntPools));
	}

	if (antecedent->dependentPools == NULL)
	{
		// First time - we need to alloc our dependent AllocFrom pool array
		antecedent->dependentPools = (AllocFrom **)malloc(POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT * sizeof(AllocFrom *));
		if (antecedent->dependentPools == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to allocate pool AllocFrom array"));
			goto exit;
		}
		antecedent->maxDepPools = POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT;
		antecedent->numDepPools = 0;
		for (i = 0; i < antecedent->maxDepPools; i++)
		{
			antecedent->dependentPools[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new pool->dependentPools %x, size = %d", antecedent->dependentPools, antecedent->maxDepPools));
	}


	if (antAF == NULL || depAF == NULL)
	{
		_SMI_TRACE(0,("ERROR: Unable to allocate AllocFrom structures"));
		goto exit;
	}

	antAF->element = antecedent;
	depAF->element = dependent;
	antAF->spaceConsumed = depAF->spaceConsumed = spaceConsumed;

	// Find an available slot in AllocFrom arrays and add new entries
	for (i = 0; i < dependent->maxAntPools; i++)
	{
		if (dependent->antecedentPools[i] == NULL)
		{
			dependent->antecedentPools[i] = antAF;
			dependent->numAntPools++;
			break;
		}
	}

	for (i = 0; i < antecedent->maxDepPools; i++)
	{
		if (antecedent->dependentPools[i] == NULL)
		{
			antecedent->dependentPools[i] = depAF;
			antecedent->numDepPools++;
			break;
		}
	}

	// See if we need to resize our arrays
	AllocFrom **newArray;
	if (dependent->numAntPools == dependent->maxAntPools)
	{
		_SMI_TRACE(1,("Resizing antecedent AllocFrom array"));

		newArray = (AllocFrom **) realloc (
							dependent->antecedentPools, 
							(dependent->maxAntPools + POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT) 
							* sizeof(AllocFrom*));

		if (newArray == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to realloc antecedent AllocFrom structures"));
		}
		else
		{
			for (i = dependent->maxAntPools; 
					i < (dependent->maxAntPools + POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT); 
						i++)
			{
				newArray[i] = NULL;
			}
			dependent->antecedentPools = newArray;
			dependent->maxAntPools += POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT;
		}
	}

	if (antecedent->numDepPools == antecedent->maxDepPools)
	{
		_SMI_TRACE(1,("Resizing dependent AllocFrom array"));

		newArray = (AllocFrom **) realloc ( 
							antecedent->dependentPools, 
							(antecedent->maxDepPools + POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT) 
							* sizeof(AllocFrom*));

		if (newArray == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to realloc dependent AllocFrom structures"));
		}
		else
		{
			for (i = antecedent->maxDepPools; 
					i < (antecedent->maxDepPools + POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT); 
						i++)
			{
				newArray[i] = NULL;
			}
			antecedent->dependentPools = newArray;
			antecedent->maxDepPools += POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT;
		}
	}

exit:
	_SMI_TRACE(1,("PoolCreateAllocFromAssociation() done"));
}

/////////////////////////////////////////////////////////////////////////////
void PoolModifyAllocFromAssociation(
						StoragePool *antecedent,
						StoragePool *dependent,
						CMPIUint64 spaceConsumedIncrement,
						CMPIUint64 spaceConsumedDecrement)
{
	CMPICount i;
	AllocFrom *antAF = NULL;
	AllocFrom *depAF = NULL;

	_SMI_TRACE(1,("PoolModifyAllocFromAssociation() called"));
	_SMI_TRACE(1,("\tAnt = %s, Dep = %s", antecedent->name, dependent->name));
	_SMI_TRACE(1,("\tInc = %lld, Dec = %lld", spaceConsumedIncrement, spaceConsumedDecrement));

	for (i = 0; i < antecedent->maxDepPools; i++)
	{
		AllocFrom *af = antecedent->dependentPools[i];
		if (af && af->element == dependent)
		{
			depAF = af;
			break;
		}
	}

	for (i = 0; i < dependent->maxAntPools; i++)
	{
		AllocFrom *af = dependent->antecedentPools[i];
		if (af && af->element == antecedent)
		{
			antAF = af;
			break;
		}
	}

	if (depAF != NULL && antAF != NULL)
	{
		antAF->spaceConsumed += spaceConsumedIncrement;
		depAF->spaceConsumed += spaceConsumedIncrement;
		antAF->spaceConsumed -= spaceConsumedDecrement;
		depAF->spaceConsumed -= spaceConsumedDecrement;
	}

	_SMI_TRACE(1,("PoolModifyAllocFromAssociation() done"));
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath* PoolCreateObjectPath(
						StoragePool *pool, 
						const char *ns, 
						CMPIStatus *status)
{
	CMPIObjectPath *cop;

	cop = CMNewObjectPath(
				_BROKER, ns,
				StoragePoolClassName,
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(0,("PoolCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
	}
	if (pool->instanceID)
	{
		CMAddKey(cop, "InstanceID", pool->instanceID, CMPI_chars);
	}
exit:
	_SMI_TRACE(1,("PoolCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance* PoolCreateInstance(
						StoragePool *pool, 
						const char *ns, 
						CMPIStatus *status)
{
	CMPIInstance *ci;
	CMPIValue val;
	CMPIArray *arr;
	char *temp;

	_SMI_TRACE(1,("PoolCreateInstance() called for pool %s", pool->name));

	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, StoragePoolClassName, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(0,("PoolCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "InstanceID",	pool->instanceID, CMPI_chars);
	CMSetProperty(ci, "Name", pool->name, CMPI_chars);
	CMSetProperty(ci, "ElementName", pool->name, CMPI_chars);
	CMSetProperty(ci, "Caption", pool->name, CMPI_chars);
	CMSetProperty(ci, "PoolID", pool->name, CMPI_chars);

	val.uint64 = pool->totalSize;
	CMSetProperty(ci, "TotalManagedSpace", &val, CMPI_uint64);
	val.uint64 = pool->remainingSize;
	CMSetProperty(ci, "RemainingManagedSpace", &val, CMPI_uint64);

	val.dateTime = CMNewDateTime(_BROKER, NULL);
	CMSetProperty(ci, "InstallDate", &val, CMPI_dateTime);

	val.uint16 = 0;
	CMSetProperty(ci, "LowSpaceWarningThreshold", &val, CMPI_uint16);

	val.boolean = pool->primordial;
	CMSetProperty(ci, "Primordial", &val, CMPI_boolean);

	arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
	val.uint16 = pool->ostatus;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "OperationalStatus", &val, CMPI_uint16A);

	val.uint16 = GetHealthState(pool->ostatus);
	CMSetProperty(ci, "HealthState", &val, CMPI_uint16);

/*
	arr = CMNewArray(_BROKER, 1, CMPI_chars, NULL);
	val.chars = GetStatusDescription(pool->ostatus);
	CMSetArrayElementAt(arr, 0, &val, CMPI_chars);
	val.array = arr;
	CMSetProperty(ci, "StatusDescriptions", &val , CMPI_charsA);
*/

exit:
	_SMI_TRACE(1,("PoolCreateInstance() done"));
	return ci;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *PoolCreateHostedAssocInstance(
					StoragePool *pool,
					const char *ns,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "GroupComponent",  "PartComponent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateHostedAssocInstance() called"));

	// Create and populate ComputerSystem object path
	CMPIObjectPath *cscop = cmpiutilCreateCSObjectPath(_BROKER, ns, pStatus);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create ComputerSystem cop");
		return NULL;
	}

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											HostedStoragePoolClassName,
											classKeys,
											properties,
											"GroupComponent",
											"PartComponent",
											cscop,
											spcop,
											pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateHostedAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *PoolCreateHostedAssocObjectPath(
					StoragePool *pool,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateHostedAssocObjectPath() called"));

	CMPIObjectPath *cscop = cmpiutilCreateCSObjectPath( _BROKER, ns, pStatus);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create ComputerSystem cop");
		return NULL;
	}

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									HostedStoragePoolClassName,
									"GroupComponent",
									"PartComponent",
									cscop,
									spcop,
									pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateHostedAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance *PoolCreateCapabilityAssocInstance(
					StoragePool *pool,
					const char *ns,
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "ManagedElement",  "Capabilities", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateCapabilityAssocInstance() called"));

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	// Create and populate StorageCapabilities object path
	CMPIObjectPath *sccop = CapabilityCreateObjectPath(pool->capability, ns, &status);
	if (CMIsNullObject(sccop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageCapabilities cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											StorageElementCapabilitiesClassName,
											classKeys,
											properties,
											"ManagedElement",
											"Capabilities",
											spcop,
											sccop,
											pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateCapabilityAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *PoolCreateCapabilityAssocObjectPath(
					StoragePool *pool,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateCapabilityAssocObjectPath() called"));

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	// Create and populate StorageCapabilities object path
	CMPIObjectPath *sccop = CapabilityCreateObjectPath(pool->capability, ns, &status);
	if (CMIsNullObject(sccop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageCapabilities cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									StorageElementCapabilitiesClassName,
									"ManagedElement",
									"Capabilities",
									spcop,
									sccop,
									pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateCapabilityAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *PoolCreateComponentAssocInstance(
					StoragePool *pool,
					StorageExtent *extent,
					const char *ns,
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "GroupComponent",  "PartComponent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateComponentAssocInstance() called"));

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	// Create and populate StorageExtent object path
	CMPIObjectPath *secop = ExtentCreateObjectPath(extent, ns, &status);
	if (CMIsNullObject(secop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageExtent cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											AssociatedComponentExtentClassName,
											classKeys,
											properties,
											"GroupComponent",
											"PartComponent",
											spcop,
											secop,
											pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateComponentAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *PoolCreateComponentAssocObjectPath(
					StoragePool *pool,
					StorageExtent *extent,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateComponentAssocObjectPath() called"));

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	// Create and populate StorageExtent object path
	CMPIObjectPath *secop = ExtentCreateObjectPath(extent, ns, &status);
	if (CMIsNullObject(secop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageExtent cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									AssociatedComponentExtentClassName,
									"GroupComponent",
									"PartComponent",
									spcop,
									secop,
									pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateComponentAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *PoolCreateRemainingAssocInstance(
					StoragePool *pool,
					StorageExtent *extent,
					const char *ns,
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "GroupComponent",  "PartComponent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateRemainingAssocInstance() called"));

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	// Create and populate StorageExtent object path
	CMPIObjectPath *secop = ExtentCreateObjectPath(extent, ns, &status);
	if (CMIsNullObject(secop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageExtent cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											AssociatedRemainingExtentClassName,
											classKeys,
											properties,
											"GroupComponent",
											"PartComponent",
											spcop,
											secop,
											pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateRemainingAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *PoolCreateRemainingAssocObjectPath(
					StoragePool *pool,
					StorageExtent *extent,
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateRemainingAssocObjectPath() called"));

	// Create and populate StoragePool object path
	CMPIObjectPath *spcop = PoolCreateObjectPath(pool, ns, &status);
	if (CMIsNullObject(spcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StoragePool cop");
		return NULL;
	}

	// Create and populate StorageExtent object path
	CMPIObjectPath *secop = ExtentCreateObjectPath(extent, ns, &status);
	if (CMIsNullObject(secop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageExtent cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									AssociatedRemainingExtentClassName,
									"GroupComponent",
									"PartComponent",
									spcop,
									secop,
									pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateRemainingAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIInstance*	PoolCreateAllocFromAssocInstance(
					const char *classname,
					StoragePool *depPool,
					StoragePool *antPool,
					CMPIUint64 spaceConsumed,
					const char *ns, 
					const char **properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "Dependent",  "Antecedent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateAllocFromAssocInstance() called"));
	_SMI_TRACE(1,("PoolCreateAllocFromAssocInstance() called: classname is %s", classname));

	// Create and populate dependent StoragePool object path
	CMPIObjectPath *dpcop = PoolCreateObjectPath(depPool, ns, &status);
	if (CMIsNullObject(dpcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StoragePool cop");
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
	_SMI_TRACE(1,("PoolCreateAllocFromAssocInstance(): cmpiutilCreateAssocInst called"));

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											classname,
											classKeys,
											properties,
											"Dependent",
											"Antecedent",
											dpcop,
											apcop,
											pStatus);

	// Don't forget to set the spaceConsumed
	CMSetProperty(assocInst, "SpaceConsumed", &spaceConsumed, CMPI_uint64);
	if(CMIsNullObject(assocInst))
		_SMI_TRACE(1,("assocInst is NULL"));
	else
	{
		_SMI_TRACE(1,("assocInst is not NULL"));
	//	_SMI_TRACE(1,("The Ancedent set is %s", assocInst.getProperty("Antecedent").getCString()));
	}

	_SMI_TRACE(1,("Leaving PoolCreateAllocFromAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath*	PoolCreateAllocFromAssocObjectPath(
					const char *classname,
					StoragePool *depPool,
					StoragePool *antPool,
  					const char *ns, 
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("PoolCreateAllocFromAssocObjectPath() called"));

	// Create and populate dependent StoragePool object path
	CMPIObjectPath *dpcop = PoolCreateObjectPath(depPool, ns, &status);
	if (CMIsNullObject(dpcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StoragePool cop");
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
									classname,
									"Dependent",
									"Antecedent",
									dpcop,
									apcop,
									pStatus);

	_SMI_TRACE(1,("Leaving PoolCreateAllocFromAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	PoolDefaultCapabilitiesMatch(	
				StoragePool *poolA,
				StoragePool *poolB)
{
	return(poolA->capability->packageRedundancy == poolB->capability->packageRedundancy &&
			poolA->capability->dataRedundancy == poolB->capability->dataRedundancy &&
			poolA->capability->extentStripe == poolB->capability->extentStripe &&
			poolA->capability->parity == poolB->capability->parity);
}

/////////////////////////////////////////////////////////////////////////////
CMPIBoolean	PoolGoalMatchesDefault(	
				StoragePool *pool,
				StorageSetting *goal)
{
	return(goal->packageRedundancyGoal == pool->capability->packageRedundancy &&
			goal->dataRedundancyGoal == pool->capability->dataRedundancy &&
			goal->extentStripeLength == pool->capability->extentStripe &&
			goal->parityLayout == pool->capability->parity);
}


/////////////////////////////////////////////////////////////////////////////
void PoolInvokeMethod(
		StoragePool *pool,
		const char *ns,
		const char *methodName,
		const CMPIArgs *in,
		CMPIArgs *out,
		const CMPIResult* results,
		CMPIStatus *pStatus)
{
	CMPIUint32 rc = 0; 
	CMPICount i;

	_SMI_TRACE(1,("PoolInvokeMethod() called"));

	if (strcasecmp(methodName, "GetSupportedSizes") == 0)
	{
		rc = GSS_USE_GET_SUPPORTED_SIZE_RANGE;
	}
	else if (strcasecmp(methodName, "GetSupportedSizeRange") == 0)
	{
		// Make sure the in/out params are legit
		CMPICount inSize = CMGetArgCount(in, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (inSize < 2) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to GetSupportedSizeRange method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to GetSupportedSizeRange method");
			goto exit;
		}

		CMPICount outSize = CMGetArgCount(out, pStatus);
		_SMI_TRACE(1,("PoolInvokeMethod() called: Out count is %d", outSize));
//		if ( (pStatus->rc != CMPI_RC_OK) || (outSize < 3) )
		if ( (pStatus->rc != CMPI_RC_OK))
		{
			_SMI_TRACE(0,("Required output parameter missing in call to GetSupportedSizeRange method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to GetSupportedSizeRange method");
			goto exit;
		}

		// Get input goal storage setting information
		CMPIObjectPath *goalCop = NULL;
		StorageSetting *goal = NULL;
		CMPIData inData = CMGetArg(in, "Goal", NULL);
		if (inData.state == CMPI_goodValue)
		{
			goalCop = inData.value.ref;
		}
		if (goalCop != NULL)
		{
			CMPIData keyData = CMGetKey(goalCop, "InstanceID", NULL);
			const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
			_SMI_TRACE(1,("Input goal setting has instanceID = %s", instanceID));
			goal = SettingsFind(instanceID);
		}

		CMPIUint64 minVolSize, maxVolSize, divisor;
		rc = GetSupportedSizeRange(pool, goal, &minVolSize, &maxVolSize, &divisor);

		_SMI_TRACE(1,("minVolSize = %lld, maxVolSize = %lld, divisor = %lld", minVolSize, maxVolSize, divisor));
		CMAddArg(out, "MinimumVolumeSize", &minVolSize, CMPI_uint64);
		CMAddArg(out, "MaximumVolumeSize", &maxVolSize, CMPI_uint64);
		CMAddArg(out, "VolumeSizeDivisor", &divisor, CMPI_uint64);
	}
	else if (strcasecmp(methodName, "GetAvailableExtents") == 0)
	{
		// Make sure the in/out params are legit
		CMPICount inSize = CMGetArgCount(in, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (inSize < 1) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to GetAvailableExtents method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to GetAvailableExtents method");
			goto exit;
		}

		CMPICount outSize = CMGetArgCount(out, pStatus);
//		if ( (pStatus->rc != CMPI_RC_OK) || (outSize < 1) )
		if ( (pStatus->rc != CMPI_RC_OK))
		{
			_SMI_TRACE(0,("Required output parameter missing in call to GetAvailableExtents method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to GetAvailableExtents method");
			goto exit;
		}

		// Get input goal storage setting information
		CMPIObjectPath *goalCop = NULL;
		StorageSetting *goal = NULL;
		CMPIData inData = CMGetArg(in, "Goal", NULL);
		if (inData.state == CMPI_goodValue)
		{
			goalCop = inData.value.ref;
		}
		if (goalCop != NULL)
		{
			CMPIData keyData = CMGetKey(goalCop, "InstanceID", NULL);
			const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
			_SMI_TRACE(1,("Input goal setting has instanceID = %s", instanceID));
			goal = SettingsFind(instanceID);
		}

		CMPICount maxAvailExtents = PoolExtentsSize(pool);
		CMPICount numAvailExtents = maxAvailExtents;
		StorageExtent **availExtents = (StorageExtent **)malloc(maxAvailExtents * sizeof(StorageExtent *));
		if (availExtents == NULL)
		{
			_SMI_TRACE(0,("PoolInvokeMethod(): Out of memory!!!"));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
				"Out of memory preparing to call GetAvailableExtents method");
			goto exit;
		}

		rc = GetAvailableExtents(pool, goal, availExtents, &numAvailExtents);

		_SMI_TRACE(1,("Pool has %d available extents satisfying goal", numAvailExtents));

		if (numAvailExtents > 0)
		{
			CMPIArray *copArray = CMNewArray(_BROKER, numAvailExtents, CMPI_ref, NULL);
			CMPIValue val;
			for (i = 0; i < numAvailExtents; i++)
			{
				val.ref = ExtentCreateObjectPath(availExtents[i], ns, pStatus);
				CMSetArrayElementAt(copArray, i, &val, CMPI_ref);
			}
			_SMI_TRACE(1,("There are %d available extents printed", numAvailExtents));
			CMPIValue val1;
			val1.array = copArray;
			CMAddArg(out, "AvailableExtents", &val1, CMPI_refA);
		}
		free(availExtents);
	}
	else
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_METHOD_NOT_AVAILABLE,
			"Method not supported by OMC_StoragePool");
	}

	CMReturnData(results, &rc, CMPI_uint32);
exit:
	_SMI_TRACE(1,("PoolInvokeMethod() finished, rc = %d", rc));
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPICount PoolsSize()
{
	return NumPools;
}

/////////////////////////////////////////////////////////////////////////////
StoragePool* PoolsGet(const CMPICount index)
{
	_SMI_TRACE(1,("PoolsGet() called, PoolArray = %x, NumPools = %d, MaxNumPools = %d", PoolArray, NumPools, MaxNumPools));

	if (MaxNumPools <= index)
	{
		return NULL;
	}

	_SMI_TRACE(1,("PoolsGet() done, returning %x", PoolArray[index]));
	return PoolArray[index];
}

/////////////////////////////////////////////////////////////////////////////
StoragePool* PoolsFind(const char *instanceID)
{
	CMPICount i;
	StoragePool *currPool;

	if (PoolArray == NULL)
		return NULL;

	for (i = 0; i < MaxNumPools; i++)
	{
		currPool = PoolArray[i];
		if (currPool)
		{
			if (strcmp(currPool->instanceID, instanceID) == 0)
			{
				// Found it
				return currPool;
			}
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
StoragePool* PoolsFindByGoal(StorageSetting *goal, CMPIUint64 *goalSize)
{
	CMPICount i;
	StoragePool *currPool;
	CMPIUint64 bestSize = 0;

	_SMI_TRACE(1,("PoolsFindByGoal() called, goalSize = %lld", *goalSize));
	if (PoolArray == NULL)
		return NULL;

	// We make 2 passes: 1st we try to find a pool that already fully supports
	// the goal by default.

	for (i = 0; i < MaxNumPools; i++)
	{
		currPool = PoolArray[i];
		if (currPool)
		{
			if (goal == NULL || PoolGoalMatchesDefault(currPool, goal))
			{
				_SMI_TRACE(1,("Examining pool %s", currPool->instanceID));

				CMPIUint64 minSize, maxSize, divisor;
				if (GetSupportedSizeRange(currPool, goal, &minSize, &maxSize, &divisor) 
					 == GAE_COMPLETED_OK)
				{
					_SMI_TRACE(1,("Max supported size of pool is %lld", maxSize));
					if (maxSize >= *goalSize && maxSize >= MIN_VOLUME_SIZE)
					{
						// We found a pool that will work, return it
						_SMI_TRACE(1,("PoolsFindByGoal() done, returning pool %s", currPool->instanceID));

						// If goalSize was 0 then set to maxSize
						if (*goalSize == 0)
						{
							*goalSize = maxSize;
						}
						return currPool;
					}

					if (maxSize > bestSize)
					{
						bestSize = maxSize;
					}
				}
			}
		}
	}

	// If we weren't able to meet the goal with a pool that support it by default,
	// try finding JBOD pool that we can use to construct a composite RAID that
	// can meet the goal requirements.
	_SMI_TRACE(1,("Unable to find primordial pool that matches non-JBOD goal by default...."));
	_SMI_TRACE(1,(" Check for JBOD pool that can meet goal by constructing RAID composite"));
	
	for (i = 0; i < MaxNumPools; i++)
	{
		currPool = PoolArray[i];
		if (currPool)
		{
			if (CapabilityDefaultIsJBOD(currPool->capability))
			{
				_SMI_TRACE(1,("Examining JBOD pool %s", currPool->instanceID));

				CMPIUint64 minSize, maxSize, divisor;
				if (GetSupportedSizeRange(currPool, goal, &minSize, &maxSize, &divisor) 
					 == GAE_COMPLETED_OK)
				{
					_SMI_TRACE(1,("Max supported size of pool is %lld", maxSize));
					if (maxSize >= *goalSize && maxSize >= MIN_VOLUME_SIZE)
					{
						// We found a pool that will work, return it
						_SMI_TRACE(1,("PoolsFindByGoal() done, returning JBOD pool %s", currPool->instanceID));

						// If goalSize was 0 then set to maxSize
						if (*goalSize == 0)
						{
							*goalSize = maxSize;
						}
						return currPool;
					}

					if (maxSize > bestSize)
					{
						bestSize = maxSize;
					}
				}
			}
		}
	}

	_SMI_TRACE(1,("PoolsFindByGoal() done, returning NULL, bestSize = %lld", bestSize));
	*goalSize = bestSize;
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
StoragePool* PoolsFindByName(const char *name)
{
	CMPICount i;
	StoragePool *currPool;

	if (PoolArray == NULL)
		return NULL;

	for (i = 0; i < MaxNumPools; i++)
	{
		currPool = PoolArray[i];
		if (currPool)
		{
			if (strcmp(currPool->name, name) == 0)
			{
				// Found it
				return currPool;
			}
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
void PoolsAdd(StoragePool *pool)
{
	StoragePool	**newArray;
	CMPICount	i;
	StoragePool *currPool;

	// See if this is the first time thru
	if (PoolArray == NULL)
	{
		// First time - we need to alloc our PoolArray
		PoolArray = (StoragePool **)malloc(POOL_ARRAY_SIZE_DEFAULT * sizeof(StoragePool *));
		if (PoolArray == NULL)
		{
			_SMI_TRACE(0,("PoolsAdd() ERROR: Unable to allocate pool array"));
			return;
		}
		MaxNumPools = POOL_ARRAY_SIZE_DEFAULT;
		for (i = 0; i < MaxNumPools; i++)
		{
			PoolArray[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new PoolArray %x, size = %d", PoolArray, MaxNumPools));
	}

	// See if this pool is already in the array
	currPool = PoolsFind(pool->instanceID);
	if (currPool)
	{
		_SMI_TRACE(1,("PoolsAdd() pool %s already exists, replace old with new", pool->instanceID));
		PoolsRemove(currPool);
		PoolFree(currPool);
	}

	// Try to find an empty slot
	for (i = 0; i < MaxNumPools; i++)
	{
		if (PoolArray[i] == NULL)
		{
			// We found an available slot
			_SMI_TRACE(1,("PoolsAdd(): Adding %s", pool->instanceID));
			PoolArray[i] = pool;
			NumPools++;
			break;
		}
	}

	// See if we need to resize the array
	if (i == MaxNumPools)
	{
		// No available slots, need to resize
		_SMI_TRACE(1,("PoolsAdd(): PoolArray out of room, resizing"));

		newArray = (StoragePool **)realloc(PoolArray, (MaxNumPools + POOL_ARRAY_SIZE_DEFAULT) * sizeof(StoragePool *));
		if (newArray == NULL)
		{
			_SMI_TRACE(0,("PoolsAdd() ERROR: Unable to resize pool array"));
			return;
		}
		for (i = MaxNumPools; i < (MaxNumPools + POOL_ARRAY_SIZE_DEFAULT); i++)
		{
			newArray[i] = NULL;
		}

		PoolArray = newArray;

		// Don't forget to add the new pool
		_SMI_TRACE(1,("PoolsAdd()-resize-: Adding %s", pool->instanceID));
		PoolArray[MaxNumPools] = pool;
		NumPools++;
		MaxNumPools += POOL_ARRAY_SIZE_DEFAULT;
	}
}

/////////////////////////////////////////////////////////////////////////////
void PoolsRemove(StoragePool *pool)
{
	CMPICount	i;
	StoragePool *currPool;

	// Find & remove pool
	for (i = 0; i < MaxNumPools; i++)
	{
		currPool = PoolArray[i];
		if (strcmp(currPool->instanceID, pool->instanceID) == 0)
		{
			// Found it, delete it
			_SMI_TRACE(1,("PoolsAdd(): Removing %s", pool->instanceID));
			PoolArray[i] = NULL;
			PoolFree(currPool);
			NumPools--;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void PoolsFree()
{
	CMPICount i, j;
	StoragePool *pool;

	_SMI_TRACE(1,("PoolsFree() called, PoolArray = %x, NumPools = %d, MaxNumPools = %d", PoolArray, NumPools, MaxNumPools));

	if (PoolArray == NULL)
		return;

	for (i = 0; i < MaxNumPools; i++)
	{
		pool = PoolArray[i];
		if (pool)
		{
			PoolFree(pool);
		}
	}

	free(PoolArray);
	PoolArray = NULL;
	NumPools = 0;
	MaxNumPools = 0;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPICount PoolExtentsSize(StoragePool *pool)
{
	return pool->numComps;
}

/////////////////////////////////////////////////////////////////////////////
StorageExtent* PoolExtentsGet(StoragePool *pool, const CMPICount index)
{
	if (pool->maxNumComps <= index)
	{
		return NULL;
	}

	return pool->concreteComps[index];
}

/////////////////////////////////////////////////////////////////////////////
StorageExtent* PoolExtentsFind(StoragePool *pool, const char *deviceID)
{
	CMPICount i;
	StorageExtent *currExtent;

	if (pool->concreteComps == NULL)
		return NULL;

	for (i = 0; i < pool->maxNumComps; i++)
	{
		currExtent = pool->concreteComps[i];
		if (currExtent)
		{
			if (strcmp(currExtent->deviceID, deviceID) == 0)
			{
				// Found it
				return currExtent;
			}
		}
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
void PoolExtentsAdd(StoragePool *pool, StorageExtent *extent)
{
	StorageExtent	**newArray;
	CMPICount		i;
	StorageExtent	*currExtent;

	// See if this is the first time thru
	if (pool->concreteComps == NULL)
	{
		// First time - we need to alloc our concrete component extent array
		pool->concreteComps = (StorageExtent **)malloc(POOL_EXTENT_ARRAY_SIZE_DEFAULT * sizeof(StorageExtent *));
		if (pool->concreteComps == NULL)
		{
			_SMI_TRACE(0,("PoolExtentsAdd() ERROR: Unable to allocate pool concrete component extent array"));
			return;
		}
		pool->maxNumComps = POOL_EXTENT_ARRAY_SIZE_DEFAULT;
		for (i = 0; i < pool->maxNumComps; i++)
		{
			pool->concreteComps[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new pool->concreteComps %x, size = %d", pool->concreteComps, pool->maxNumComps));
	}

	// See if this extent is already in the array
	currExtent = PoolExtentsFind(pool, extent->deviceID);
	if (currExtent)
	{
		_SMI_TRACE(1,("PoolExtentsAdd() extent %s already exists, replace old with new", extent->deviceID));
		PoolExtentsRemove(pool, currExtent);
	}

	// Try to find an empty slot
	for (i = 0; i < pool->maxNumComps; i++)
	{
		if (pool->concreteComps[i] == NULL)
		{
			// We found an available slot
			_SMI_TRACE(1,("PoolExtentsAdd(): Adding %s", extent->deviceID));
			pool->concreteComps[i] = extent;
			pool->numComps++;
			break;
		}
	}

	// See if we need to resize the array
	if (i == pool->maxNumComps)
	{
		// No available slots, need to resize
		_SMI_TRACE(1,("PoolExtentsAdd(): pool->concreteComps out of room, resizing"));

		newArray = (StorageExtent **)realloc(pool->concreteComps, (pool->maxNumComps + POOL_EXTENT_ARRAY_SIZE_DEFAULT) * sizeof(StorageExtent *));
		if (newArray == NULL)
		{
			_SMI_TRACE(0,("PoolExtentsAdd() ERROR: Unable to resize extent array"));
			return;
		}
		for (i = pool->maxNumComps; i < (pool->maxNumComps + POOL_EXTENT_ARRAY_SIZE_DEFAULT); i++)
		{
			newArray[i] = NULL;
		}

		pool->concreteComps = newArray;

		// Don't forget to add the new extent
		_SMI_TRACE(1,("PoolExtentsAdd()-resize-: Adding %s", extent->deviceID));
		pool->concreteComps[pool->maxNumComps] = extent;
		pool->numComps++;
		pool->maxNumComps += POOL_EXTENT_ARRAY_SIZE_DEFAULT;
	}
}

/////////////////////////////////////////////////////////////////////////////
void PoolExtentsRemove(StoragePool *pool, StorageExtent *extent)
{
	CMPICount	i;
	StorageExtent *currExtent;

	// Find & remove extent
	for (i = 0; i < pool->maxNumComps; i++)
	{
		currExtent = pool->concreteComps[i];
		if (strcmp(currExtent->deviceID, extent->deviceID) == 0)
		{
			// Found it, delete it
			_SMI_TRACE(1,("PoolExtentsAdd(): Removing %s", extent->deviceID));
			pool->concreteComps[i] = NULL;
			pool->numComps--;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void PoolExtentsFree(StoragePool *pool)
{
	CMPICount i;
	StorageExtent *extent;

	_SMI_TRACE(1,("PoolExtentsFree() called, pool->concreteComps = %x, pool->numComps = %d, pool->maxNumComps = %d", pool->concreteComps, pool->numComps, pool->maxNumComps));

	if (pool->concreteComps == NULL)
		return;

	free(pool->concreteComps);
	pool->concreteComps = NULL;
	pool->numComps = 0;
	pool->maxNumComps = 0;
}


/////////////////////////////////////////////////////////////////////////////
//////////// CIM methods called by PoolInvokeMethod /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CMPIUint32 GetSupportedSizeRange(
				StoragePool *pool,
				StorageSetting *goal,
				CMPIUint64 *minSize,
				CMPIUint64 *maxSize,
				CMPIUint64 *sizeDivisor)
{
	CMPIUint32 rc = GAE_COMPLETED_OK;
	CMPICount maxAvailExtents;
	CMPICount numAvailExtents;
	StorageExtent **availExtents;

	_SMI_TRACE(1,("GetSupportedSizeRange() called"));
	_SMI_TRACE(1,("GetSupportedSizeRange() called:poolname is %s", pool->name));

	// Handle the simple cases first:

	// 1) If no goal specified we assume defaults--maxVolumeSize = pool remainingSize
	if (goal == NULL)
	{
		_SMI_TRACE(1,("No goal specified, using default"));
		*maxSize = pool->remainingSize;
		goto exit;
	}

	// 2) If pool is NOT primordial we keep it simple with current implementation
	if (!pool->primordial)
	{
		// Currently for concretes we only deal with goals that match our default.
		_SMI_TRACE(1,("Pool is concrete"));
		if (PoolGoalMatchesDefault(pool, goal))
		{
			*maxSize = pool->remainingSize;
		}
		else
		{
			*maxSize = 0;
		}
		goto exit;
	}

	// 3) If JBOD goal or primordial default capability matches the goal then
	//		maxVolumeSize = pool remainingSize	
	if (SettingIsJBOD(goal) || PoolGoalMatchesDefault(pool, goal))
	{
		_SMI_TRACE(1,("Handling goal=JBOD or goal=pool-default"));
		_SMI_TRACE(1,("Pool remaining size is %llu", pool->remainingSize));
		*maxSize = pool->remainingSize;
		goto exit;
	}

	// For Non-JBOD/Non-default case we need to calculate if/how we can best meet the goal
	// First get all available extents suitable for the requested goal
	maxAvailExtents = PoolExtentsSize(pool);
	numAvailExtents = maxAvailExtents;
	availExtents = (StorageExtent **)malloc(maxAvailExtents * sizeof(StorageExtent *));
	if (availExtents == NULL)
	{
		_SMI_TRACE(0,("GetSupportedSizeRange(): Out of memory!!!"));
		*maxSize = 0;
		rc = GAE_FAILED;
		goto exit;
	}

	if ( (GetAvailableExtents(pool, goal, availExtents, &numAvailExtents) == 0) &&
		 (numAvailExtents > 0) )
	{
		_SMI_TRACE(1,("Found %d available extents", numAvailExtents));

		// Sort available extents based on size low-to-high
		ExtentsSortBySize(availExtents, numAvailExtents);
		CMPICount sizeScaleIdx = numAvailExtents - (goal->dataRedundancyGoal * goal->extentStripeLength);
		if (goal->parityLayout == 2)
		{
			// Some flavor or RAID 5
			*maxSize = (goal->extentStripeLength-1) * 
						availExtents[sizeScaleIdx]->size;
		}
		else
		{
			// Some flavor of RAID0/1
			*maxSize = goal->extentStripeLength *
						availExtents[sizeScaleIdx]->size;
		}
	}
	else
	{
		_SMI_TRACE(1,("Available extents found in pool are inadequate to meet the requested goal setting"));
	}

	free(availExtents);

exit:
	if (*maxSize >= MIN_VOLUME_SIZE)
	{
		*minSize = MIN_VOLUME_SIZE;
		*sizeDivisor = MIN_VOLUME_SIZE;
	}
	else
	{
		_SMI_TRACE(1,("MaxVolumeSize is < MIN_VOLUME_SIZE"));
		*minSize = *maxSize = *sizeDivisor = 0;
	}
	_SMI_TRACE(1,("GetSupportedSizeRange() finished, rc = %d", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
CMPIUint32 GetAvailableExtents(
				StoragePool *pool,
				StorageSetting *goal,
				StorageExtent **availExtents,
				CMPICount *numAvailExtents)
{
	CMPIUint32 rc = GAE_COMPLETED_OK;
	CMPICount i, j;

	*numAvailExtents = 0;
	_SMI_TRACE(1,("GetAvailableExtents() called"));

	if (pool->numComps > 0)
	{
		_SMI_TRACE(1,("Found %d component extents for pool", pool->numComps));

		for (i = 0; i < pool->numComps; i++)
		{
			StorageExtent *extent = pool->concreteComps[i];

			// Add all "available" component extents to available extent array
			// An extent is "available" if it has no dependent extents that 
			// are based on it (i.e. it is the antecedent).

			if (extent->numDependents == 0)
			{
				_SMI_TRACE(1,("Adding extent %s to availExtent list @ idx %d", extent->deviceID, *numAvailExtents));
				availExtents[(*numAvailExtents)++] = extent;
			}
			else
			{
				_SMI_TRACE(1,("Extent %s is consumed", extent->deviceID));
			}
		}

		// We are finished building the available extents array.
		// Make sure for non-JBOD goal using a JBOD-default pool that we have enough
		// available extents to meet the goal.
		_SMI_TRACE(1,("Verify we have enough extents to meet goal"));
		if (goal != NULL && !SettingIsJBOD(goal) && CapabilityDefaultIsJBOD(pool->capability))
		{
			if (*numAvailExtents < (goal->dataRedundancyGoal * goal->extentStripeLength))
			{
				_SMI_TRACE(1,("GAE_FAIL: Pool only has %d available extents, %d required to meet goal", 
								*numAvailExtents, (goal->dataRedundancyGoal * goal->extentStripeLength)));
				rc = GAE_FAILED;
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("No extents of any kind in pool"));
	}

	_SMI_TRACE(1,("GetAvailableExtents() finished, rc = %d", rc));
	return rc;
}

