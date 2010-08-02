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
 |   Provider code dealing with OMC_CompositeExtent classes
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

#include <deque>

#include "Utils.h"
#include "ArrayProvider.h"
#include "StorageExtent.h"
#include "StorageConfigurationService.h"
#include "StoragePool.h"
#include "StorageCapability.h"
#include "y2storage/StorageInterface.h"

using namespace storage;

extern CMPIBroker * _BROKER;
extern StorageInterface* s;

// Exported globals

// Module globals
static StorageExtent **ExtentArray = NULL;
static CMPICount NumExtents = 0;
static CMPICount MaxNumExtents = 0;



/////////////////////////////////////////////////////////////////////////////
//////////// Private helper functions ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
int SortExtentsCompare(
	const void *obj1, 
	const void *obj2)
{
	StorageExtent *extA = (StorageExtent *)(*(StorageExtent **)(obj1));
	StorageExtent *extB = (StorageExtent *)(*(StorageExtent **)(obj2));
	
	if (extA->size > extB->size)
		return 1;
	else if (extA->size == extB->size)
		return 0;
	else
		return -1;
}

/////////////////////////////////////////////////////////////////////////////
//////////// Exported functions /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
StorageExtent* ExtentAlloc(const char *name, const char *deviceID)
{
	StorageExtent* extent = (StorageExtent *)malloc(sizeof (StorageExtent));
	if (extent)
	{
		memset(extent, 0, sizeof(StorageExtent));
		extent->estatus = ESTAT_NONE;
		extent->ostatus = OSTAT_OK;
		extent->name = (char *)malloc(strlen(name) + 1);
		if (!extent->name)
		{
			free(extent);
			extent = NULL;
		}
		else
		{
			strcpy(extent->name, name);

			if (deviceID != NULL)
			{
				extent->deviceID = (char *)malloc(strlen(deviceID) + 1);
				if (!extent->deviceID)
				{
					free(extent->name);
					free(extent);
					extent = NULL;
				}
				else
				{
					strcpy(extent->deviceID, deviceID);
				}
			}
			else
			{
				extent->deviceID = (char *)malloc(strlen(name) + 1);
				if (!extent->deviceID)
				{
					free(extent->name);
					free(extent);
					extent = NULL;
				}
				else
				{
					strcpy(extent->deviceID, name);
				}
			}
		}
	}
	return(extent);
}

/////////////////////////////////////////////////////////////////////////////
void ExtentFree(StorageExtent *extent)
{	
	CMPICount i;
	if (extent)
	{
		if (extent->deviceID)
		{
			free(extent->deviceID);
		}
		if (extent->name)
		{
			free(extent->name);
		}
		if (extent->antecedents)
		{
			for (i = 0; i < extent->maxNumAntecedents; i++)
			{
				if (extent->antecedents[i] != NULL)
				{
					free(extent->antecedents[i]);
				}
			}
			free (extent->antecedents);
		}
		if (extent->dependents)
		{
			for (i = 0; i < extent->maxNumDependents; i++)
			{
				if (extent->dependents[i] != NULL)
				{
					free(extent->dependents[i]);
				}
			}
			free (extent->dependents);
		}
		free(extent);
	}
}

/////////////////////////////////////////////////////////////////////////////
char *ExtentGenerateName(const char *prefix, char *genName, CMPICount nameMaxSize)
{
	CMPICount i;

	if (nameMaxSize < (strlen(prefix)+2))
	{
		return NULL;
	}

	// We need to generate a unique extent name based on prefix (i.e. <prefix>1, 
	// <prefix>2..., etc)

	i = 1;
	while (true)
	{
		sprintf(genName, "%s%d", prefix, i);
		StorageExtent *extent = ExtentsFind(genName);
		if (extent == NULL)
		{
			break;
		}
		i++;
	}
	return genName;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath* ExtentCreateObjectPath(StorageExtent *extent, const char *ns, CMPIStatus *status)
{
	CMPIObjectPath *cop;
	char buf[1024];

	_SMI_TRACE(1,("ExtentCreateObjectPath() called"));

	if (extent->composite)
	{
		cop = CMNewObjectPath(
					_BROKER, ns,
					CompositeExtentClassName,
					status);
	}
	else
	{
		cop = CMNewObjectPath(
					_BROKER, ns,
					StorageExtentClassName,
					status);
	}

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(0,("ExtentCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
	}

	CMAddKey(cop, "SystemCreationClassName", cmpiutilGetComputerSystemClassName(), CMPI_chars);
	CMAddKey(cop, "SystemName", cmpiutilGetComputerSystemName(buf, 1024), CMPI_chars);

	if (extent->composite)
	{
		CMAddKey(cop, "CreationClassName", CompositeExtentClassName, CMPI_chars);
	}
	else
	{
		CMAddKey(cop, "CreationClassName", StorageExtentClassName, CMPI_chars);
	}
	CMAddKey(cop, "DeviceID", extent->deviceID, CMPI_chars);

exit:
	_SMI_TRACE(1,("ExtentCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance* ExtentCreateInstance(StorageExtent *extent, const char *ns, CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[1024];
	CMPIValue val;
	StoragePool *pool = extent->pool;
	CMPIArray *arr; 

	_SMI_TRACE(1,("ExtentCreateInstance() called :extent %s", extent->name));

	if (extent->composite)
	{
		ci = CMNewInstance(
					_BROKER,
					CMNewObjectPath(_BROKER, ns, CompositeExtentClassName, status),
					status);
	}
	else
	{
		ci = CMNewInstance(
					_BROKER,
					CMNewObjectPath(_BROKER, ns, StorageExtentClassName, status),
					status);
	}

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(0,("ExtentCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "SystemCreationClassName", 
		cmpiutilGetComputerSystemClassName(), CMPI_chars);
	CMSetProperty(ci, "SystemName", 
		cmpiutilGetComputerSystemName(buf, 1024), CMPI_chars);

	if (extent->composite)
	{
		_SMI_TRACE(1,("Extent is a composite"));
		CMSetProperty(ci, "CreationClassName", CompositeExtentClassName, CMPI_chars);
		StorageCapability *cap = NULL;

		if (extent->pool == NULL)
		{
			if (extent->numDependents == 1)
			{
				BasedOn *depBO = extent->dependents[0];
				StorageExtent *depExtent = depBO->extent;
				cap = depExtent->pool->capability;
			}
		}
		else
		{
			cap = pool->capability;
		}

		if (cap)
		{
			val.uint16 = cap->extentStripe;
			CMSetProperty(ci, "ExtentStripeLength", &val, CMPI_uint16);
			val.boolean = 0;
			CMSetProperty(ci, "IsConcatenated", &val, CMPI_boolean);

			if (extent->numAntecedents > 1 && cmpiutilStrEndsWith(extent->name, "Composite"))
			{
				val.boolean = 1;
				CMSetProperty(ci, "IsConcatenated", &val, CMPI_boolean);
			}
		}
		else
		{
			_SMI_TRACE(1,("No capability found for composite extent"));
		}
	}
	else
	{
		CMSetProperty(ci, "CreationClassName", StorageExtentClassName, CMPI_chars);
	}
	CMSetProperty(ci, "DeviceID", extent->deviceID, CMPI_chars);

	val.uint16 = 1;
	CMSetProperty(ci, "DataOrganization", &val, CMPI_uint16);
	val.uint16 = 3;
	CMSetProperty(ci, "Access", &val, CMPI_uint16);

	val.uint64 = 1;
	CMSetProperty(ci, "NumberOfBlocks", &val, CMPI_uint64);
	val.uint64 = extent->size;
	CMSetProperty(ci, "BlockSize", &val, CMPI_uint64);
	val.boolean = 0;
	CMSetProperty(ci, "SequentialAccess", &val, CMPI_boolean);

	if (extent->pool)
	{
		_SMI_TRACE(1,("Pool is SET"));

		if (pool->capability)
		{
			val.boolean = pool->capability->noSinglePointOfFailure;
			CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
			val.uint16 = pool->capability->dataRedundancy;
			CMSetProperty(ci, "DataRedundancy", &val, CMPI_uint16);
			val.uint16 = pool->capability->packageRedundancy;
			CMSetProperty(ci, "PackageRedundancy", &val, CMPI_uint16);

			if (pool->capability->dataRedundancy > 1)
			{
				val.boolean = 1;
				CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
			}
			else
			{
				val.boolean = 0;
				CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
			}
		}
		else
		{
			_SMI_TRACE(1,("Extent has pool but no capability!!!"));
		}
	}

	else if (extent->composite)
	{
		if (extent->numDependents == 1)
		{
			_SMI_TRACE(1,("Intermediate Composite case"));
			// Intermediate composite extent case, set capability based
			// on parent/dependent extent

			BasedOn *depBO = extent->dependents[0];
			StorageExtent* depExtent = depBO->extent;

			val.boolean = depExtent->pool->capability->noSinglePointOfFailure;
			CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
			val.uint16 = depExtent->pool->capability->dataRedundancy;
			CMSetProperty(ci, "DataRedundancy", &val, CMPI_uint16);
			val.uint16 = depExtent->pool->capability->packageRedundancy;
			CMSetProperty(ci, "PackageRedundancy", &val, CMPI_uint16);

			if (depExtent->pool->capability->dataRedundancy > 1)
			{
				val.boolean = 1;
				CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
			}
			else
			{
				val.boolean = 0;
				CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("Dummy extent case"));
		// Dummy extent case, set capability based on underlying extent

		BasedOn *antBO = extent->antecedents[0];
		StorageExtent* antExtent = antBO->extent;

		if (antExtent)
		{
			val.boolean = antExtent->pool->capability->noSinglePointOfFailure;
			CMSetProperty(ci, "NoSinglePointOfFailure", &val, CMPI_boolean);
			val.uint16 = antExtent->pool->capability->dataRedundancy;
			CMSetProperty(ci, "DataRedundancy", &val, CMPI_uint16);
			val.uint16 = antExtent->pool->capability->packageRedundancy;
			CMSetProperty(ci, "PackageRedundancy", &val, CMPI_uint16);

			if (antExtent->pool->capability->dataRedundancy > 1)
			{
				val.boolean = 1;
				CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
			}
			else
			{
				val.boolean = 0;
				CMSetProperty(ci, "IsBasedOnUnderlyingRedundancy", &val, CMPI_boolean);
			}
		}
		else
		{
			_SMI_TRACE(1,("Extent appears to be a dummy extent but has no antecedents!!"));
		}
	}

	_SMI_TRACE(1,("EStatus = %d, OStatus = %d", extent->estatus, extent->ostatus));


	CMSetProperty(ci, "Name", extent->name, CMPI_chars);
	CMSetProperty(ci, "ElementName", extent->name, CMPI_chars);
	CMSetProperty(ci, "Caption", extent->name, CMPI_chars);
	val.boolean = extent->primordial;
	CMSetProperty(ci, "Primordial", &val, CMPI_boolean);
	val.dateTime = CMNewDateTime(_BROKER, NULL);
	CMSetProperty(ci, "InstallDate", &val, CMPI_dateTime);
	arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
	val.uint16 = extent->ostatus;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "OperationalStatus", &val, CMPI_uint16A);
	val.uint16 = extent->estatus;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "ExtentStatus", &val, CMPI_uint16A);
	val.uint16 = GetHealthState(extent->ostatus);
	CMSetProperty(ci, "HealthState", &val, CMPI_uint16);
//	CMSetProperty(ci, "StatusDescriptions", GetStatusDescription(extent->ostatus), CMPI_chars);

exit:
	_SMI_TRACE(1,("ExtentCreateInstance() done"));
	return ci;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance* ExtentCreateBasedOnAssocInstance(
					StorageExtent *dependent,
					StorageExtent *antecedent,
					BasedOn *basedOn,
					const char *ns, 
					const char **properties,
					const char *className,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "Antecedent", "Dependent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("ExtentCreateBasedOnAssocInstance() called"));

	// Create and populate antecedent extent object path
	CMPIObjectPath *antcop = ExtentCreateObjectPath(antecedent, ns, &status);
	if (CMIsNullObject(antcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create antecedent StorageExtent cop");
		return NULL;
	}

	// Create and populate dependent StorageExtent object path
	CMPIObjectPath *depcop = ExtentCreateObjectPath(dependent, ns, &status);
	if (CMIsNullObject(depcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StorageExtent cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											className,
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

	// Need to set a few more properties for CompositeBasedOn case
	if (strcasecmp(className, CompositeExtentBasedOnClassName) == 0)
	{
		val.uint64 = basedOn->extent->size;
		CMSetProperty(assocInst, "Size", &val, CMPI_uint64);
		val.uint64 = dependent->pool->capability->userDataStripeDepth;
		CMSetProperty(assocInst, "UserDataStripeDepth", &val, CMPI_uint64);
	}

	_SMI_TRACE(1,("Leaving ExtentCreateBasedOnAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath*	ExtentCreateBasedOnAssocObjectPath(
						StorageExtent *dependent,
						StorageExtent *antecedent,
  						const char *ns, 
						const char *className,
						CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("ExtentCreateBasedOnAssocObjectPath() called"));

	// Create and populate antecedent extent object path
	CMPIObjectPath *antcop = ExtentCreateObjectPath(antecedent, ns, &status);
	if (CMIsNullObject(antcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create antecedent StorageExtent cop");
		return NULL;
	}

	// Create and populate dependent extent object path
	CMPIObjectPath *depcop = ExtentCreateObjectPath(dependent, ns, &status);
	if (CMIsNullObject(depcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create dependent StorageExtent cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									className,
									"Antecedent",
									"Dependent",
									antcop,
									depcop,
									pStatus);

	_SMI_TRACE(1,("Leaving ExtentCreateBasedOnAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}


/////////////////////////////////////////////////////////////////////////////
CMPIBoolean ExtentIsFreespace(StorageExtent *extent)
{
	return (cmpiutilStrEndsWith(extent->name, "Freespace") ? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////
void ExtentGetContainerName(StorageExtent *extent, char *nameBuf, const CMPICount bufSize)
{
	size_t contNameLen;
	nameBuf[0] = '\0';

	char *temp = strstr(extent->name, "Freespace");
	if (temp == NULL)
	{
		temp = strstr(extent->name, "Composite");
	}

	if (temp)
	{
		contNameLen = strlen(extent->name) - strlen(temp) + 1;
		cmpiutilStrNCpy(nameBuf, extent->name, contNameLen);
	}
	_SMI_TRACE(1,("contName = %s", nameBuf));
}

/////////////////////////////////////////////////////////////////////////////
StoragePool *ExtentGetPool(StorageExtent *extent)
{
	_SMI_TRACE(1,("ExtentGetPool() called"));
	StoragePool *pool = extent->pool;
	BasedOn *antBO; 
	if (pool)
		goto exit;

	antBO = extent->antecedents[0];
	while (!pool)
	{
		_SMI_TRACE(1,("Check pool for %s", antBO->extent->name));
		pool = antBO->extent->pool;
		if (pool)
		{
			_SMI_TRACE(1,("Found pool %s", pool->name));
			break;
		}
		antBO = antBO->extent->antecedents[0];
	}

exit:
	_SMI_TRACE(1,("ExtentGetPool() done"));
	return pool;
}


/////////////////////////////////////////////////////////////////////////////
CMPIUint32 ExtentPartition(
				StorageExtent *extent, 
				CMPIUint64 partSize, 
				StorageExtent **partExt,
				StorageExtent **remainingExt)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char *containerName;
	char diskName[10];
	char contNameBuf[256], tempBuf[256];
	const char *partExtName = NULL;
 	StorageExtent *antExtent = NULL;
	deque<PartitionInfo> p_info;
	unsigned long long offset;
	string p_device;


	_SMI_TRACE(1,("ExtentPartition() called for extent %s", extent->name));
	_SMI_TRACE(1,("ExtentPartition(): partSize = %lld", partSize ));

	*partExt = extent;
	*remainingExt = NULL;

	if (!ExtentIsFreespace(extent))
	{
		_SMI_TRACE(1,("Not a remaining extent, need to create one"));
		sprintf(tempBuf, "%s%s", extent->name, "Freespace");
		*remainingExt = ExtentAlloc(tempBuf, tempBuf);
		ExtentsAdd(*remainingExt);

		(*remainingExt)->size = extent->size;
		(*remainingExt)->pool = extent->pool;
		antExtent = extent;
	}

	else
	{
 		_SMI_TRACE(1,("Extent is already a remaining extent"));
		// Current instance is already a remaining extent
		*remainingExt = extent;

		// Portion of the extent name that precedes "Freespace" is our container name
		char *temp = strstr(extent->name, "Freespace");
		size_t contNameLen = (strlen(extent->name) - strlen(temp) + 1);
		cmpiutilStrNCpy(contNameBuf, extent->name, contNameLen);
		containerName = contNameBuf;
		BasedOn *antBO;
		if ((*remainingExt)->antecedents)
		{
			antBO = (*remainingExt)->antecedents[0];
			antExtent = antBO->extent;
		}
	
	}
	_SMI_TRACE(1,("containerName = %s\t antExtentName = %s", containerName, antExtent->name));


	/*******************		LVM Equivalent Code		******************/
	
	_SMI_TRACE(1,("Setting the offset from the start of disk"));
	offset = 0;	
	rc = s->getPartitionInfo(antExtent->name, p_info);
	if(!rc)
	{
		for(deque<PartitionInfo>::iterator p_iter = p_info.begin(); p_iter != p_info.end(); p_iter ++)
		{
			if(p_iter->partitionType != EXTENDED)
				offset += p_iter->v.sizeK;
		}
		_SMI_TRACE(1,("Calling createPartitionKb, offset = %llu", offset));
		if(rc = s->createPartitionKb(antExtent->name, (PartitionType)PRIMARY, offset, partSize/1024, p_device))
		{
			_SMI_TRACE(0,("Error creating primary partition, rc = %d", rc));
			return M_FAILED; 
		}
	}
	else
	{
		_SMI_TRACE(0,("Can't get information about Disk Partitions, checking if the extent is an extended partition!! "));
		if(IsExtendedPartition(antExtent->name, diskName))
		{
		//	_SMI_TRACE(1,("Extent is an extended partition from disk %s", diskName));
			if(rc = s->getPartitionInfo(diskName, p_info))
			{
				_SMI_TRACE(0,("Can't get information about Disk Partitions!!, rc = %d", rc));
				return M_FAILED;
			}
			else
			{
				for(deque<PartitionInfo>::iterator p_iter = p_info.begin(); p_iter != p_info.end(); p_iter ++)
				{
					if(p_iter->partitionType != EXTENDED)
						offset += p_iter->v.sizeK;
				}
				_SMI_TRACE(1,("Calling createPartitionKb, offset = %llu", offset));
				if(rc = s->createPartitionKb(diskName, (PartitionType)LOGICAL, offset, partSize/1024, p_device))
				{
					_SMI_TRACE(0,("Error creating logical partition, rc = %d", rc));
					return M_FAILED; 
				}
			}
		}
		else
		{
			_SMI_TRACE(0,("Error creating partition from the input Storage Extent, rc = %d", rc));
			return M_FAILED; 
		}
	}
	
	if(p_device.c_str() != NULL)
		partExtName = p_device.c_str();

	_SMI_TRACE(1,("Partition created: partExtName = %s", partExtName));
	*partExt = ExtentAlloc(partExtName, partExtName);
	ExtentsAdd(*partExt);
	(*partExt)->size = partSize;
	(*partExt)->primordial = 0;

	// Don't forget to adjust remainingExtents Block properties
	(*remainingExt)->size -= (*partExt)->size;

	StorageExtent *origExt;
	if (*remainingExt != extent)
	{
		origExt = extent;
	}
	else
	{
		// Current object (i.e. this->) is the remaining extent, need to get underlying
		// component extent
		BasedOn *antBO = (*remainingExt)->antecedents[0];
		origExt = antBO->extent;
	}

	_SMI_TRACE(1,("Comp/Phys Cop = %s RECop = %s", origExt->name, (*remainingExt)->name));
	_SMI_TRACE(1,("PartCop = %s", (*partExt)->name));
	_SMI_TRACE(1,("ExtentPartition() done"));
	
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
		
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
CMPIUint32 ExtentUnpartition(StorageExtent *extent)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char nameBuf[256];
	char *deviceID = NULL;

	_SMI_TRACE(1,("ExtentUnpartition() called for %s", extent->name));


	// First see if the passed in extent is a "dummy" extent based on 
	// an underlying concrete component extent. 
	// We know if this is the case by checking to see if this extent 
	// is a ConcreteComponent, if it is we don't have to do anything.

	if (extent->pool == NULL)
	{
		_SMI_TRACE(1,("IS a DUMMY extent"));

		// It is a dummy extent/partition, find the underlying concrete/component
		// extent (i.e. the one we're BasedOn);
		BasedOn *antBO = extent->antecedents[0];
		StorageExtent *srcExtent = antBO->extent;
		StorageExtent *remExtent = NULL;
		BasedOn *remExtentBO = NULL;

		CMPICount i;
		for (i = 0; i < srcExtent->maxNumDependents; i++)
		{
			remExtentBO = srcExtent->dependents[i];
			if (remExtentBO)
			{
				StorageExtent *depExtent = remExtentBO->extent;
				_SMI_TRACE(1,("Trying to find remaining/freespace extent, examining %s", depExtent->name));
				if (ExtentIsFreespace(depExtent))
				{
					_SMI_TRACE(1,("FOUND remaining extent"));
					remExtent = depExtent;
					break;
				}
			}
		}

		if (remExtent != NULL)
		{
			deviceID = ExtentCreateDeviceID(extent->name);
			_SMI_TRACE(1,("Removing extent %s", deviceID));
			if(rc = s->removePartition(deviceID))
			{
				_SMI_TRACE(0,("Unable to remove partition, rc = %d", rc));
				rc = M_FAILED;
				goto exit;
			}
		}
	}

exit:
	_SMI_TRACE(1,("ExtentUnpartition() done, rc = %u", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
	return rc;
}


/////////////////////////////////////////////////////////////////////////////
void ExtentCreateBasedOnAssociation(
			StorageExtent *dependent,
			StorageExtent *antecedent,
			CMPIUint64 startingAddress,
			CMPIUint64 endingAddress,
			CMPIUint16 orderIndex)
{
	CMPICount i;
	BasedOn *antBO; 
	BasedOn *depBO;

	_SMI_TRACE(1,("ExtentCreateBasedOnAssociation() called"));
	_SMI_TRACE(1,("\tDep = %s, Ant = %s", dependent->name, antecedent->name));
	_SMI_TRACE(1,("\tStart = %lld, End = %lld, Order = %d", startingAddress, endingAddress, orderIndex));

	// See if we need to allocate our BasedOn arrays
	if (dependent->antecedents == NULL)
	{
		// First time - we need to alloc our antecedent basedOn extent array
		dependent->antecedents = (BasedOn **)malloc(EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT * sizeof(BasedOn *));
		if (dependent->antecedents == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to allocate extent based on array"));
			goto exit;
		}
		dependent->maxNumAntecedents = EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT;
		dependent->numAntecedents = 0;
		for (i = 0; i < dependent->maxNumAntecedents; i++)
		{
			dependent->antecedents[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new extent->antecedents %x, size = %d", dependent->antecedents, dependent->maxNumAntecedents));
	}

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

	// Allocate basedOn structs
	antBO = (BasedOn *)malloc(sizeof(BasedOn));
	depBO = (BasedOn *)malloc(sizeof(BasedOn));

	if (antBO == NULL || depBO == NULL)
	{
		_SMI_TRACE(0,("ERROR: Unable to allocate basedOn structures"));
		goto exit;
	}

	antBO->extent = antecedent;
	antBO->volume = NULL;
	depBO->extent = dependent;
	depBO->volume = NULL;
	antBO->startingAddress = depBO->startingAddress = startingAddress;
	antBO->endingAddress = depBO->endingAddress = endingAddress;
	antBO->orderIndex = depBO->orderIndex = orderIndex;

	// Find an available slot in basedOn arrays and add new entries
	for (i = 0; i < dependent->maxNumAntecedents; i++)
	{
		if (dependent->antecedents[i] == NULL)
		{
			dependent->antecedents[i] = antBO;
			dependent->numAntecedents++;
			break;
		}
	}

	for (i = 0; i < antecedent->maxNumDependents; i++)
	{
		if (antecedent->dependents[i] == NULL)
		{
			antecedent->dependents[i] = depBO;
			antecedent->numDependents++;
			break;
		}
	}

	// See if we need to resize our arrays
	BasedOn **newArray;
	if (dependent->numAntecedents == dependent->maxNumAntecedents)
	{
		_SMI_TRACE(1,("Resizing antecedent BasedOn array"));

		newArray = (BasedOn **) realloc (
						dependent->antecedents, 
						(dependent->maxNumAntecedents + EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT) 
						* sizeof(BasedOn*));

		if (newArray == NULL)
		{
			_SMI_TRACE(0,("ERROR: Unable to realloc antecedent basedOn structures"));
		}
		else
		{
			for (i = dependent->maxNumAntecedents; 
					i < (dependent->maxNumAntecedents + EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT); 
						i++)
			{
				newArray[i] = NULL;
			}
			dependent->antecedents = newArray;
			dependent->maxNumAntecedents += EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT;
		}
	}

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
	_SMI_TRACE(1,("ExtentCreateBasedOnAssociation() done"));
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPICount ExtentsSize()
{
	return NumExtents;
}

/////////////////////////////////////////////////////////////////////////////
StorageExtent* ExtentsGet(const CMPICount index)
{
	if (MaxNumExtents <= index)
	{
		return NULL;
	}

	return ExtentArray[index];
}

/////////////////////////////////////////////////////////////////////////////
StorageExtent* ExtentsFind(const char *deviceID)
{
	CMPICount i;
	StorageExtent *currExtent;

	if (ExtentArray == NULL)
		return NULL;

	for (i = 0; i < MaxNumExtents; i++)
	{
		currExtent = ExtentArray[i];
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
void ExtentsAdd(StorageExtent *extent)
{
	StorageExtent	**newArray;
	CMPICount		i;
	StorageExtent	*currExtent;

	// See if this is the first time thru
	if (ExtentArray == NULL)
	{
		// First time - we need to alloc our ExtentArray
		ExtentArray = (StorageExtent **)malloc(EXTENT_ARRAY_SIZE_DEFAULT * sizeof(StorageExtent *));
		if (ExtentArray == NULL)
		{
			_SMI_TRACE(0,("ExtentsAdd() ERROR: Unable to allocate extent array"));
			return;
		}
		MaxNumExtents = EXTENT_ARRAY_SIZE_DEFAULT;
		for (i = 0; i < MaxNumExtents; i++)
		{
			ExtentArray[i] = NULL;
		}
		_SMI_TRACE(0,("Allocated new ExtentArray %x, size = %d", ExtentArray, MaxNumExtents));
	}

	// See if this extent is already in the array
	currExtent = ExtentsFind(extent->deviceID);
	if (currExtent)
	{
		_SMI_TRACE(1,("ExtentsAdd() extent %s already exists, replace old with new", extent->deviceID));
		ExtentsRemove(currExtent);
		ExtentFree(currExtent);
	}

	// Try to find an empty slot
	for (i = 0; i < MaxNumExtents; i++)
	{
		if (ExtentArray[i] == NULL)
		{
			// We found an available slot
			_SMI_TRACE(1,("ExtentsAdd(): Adding %s", extent->deviceID));
			ExtentArray[i] = extent;
			NumExtents++;
			break;
		}
	}

	// See if we need to resize the array
	if (i == MaxNumExtents)
	{
		// No available slots, need to resize
		_SMI_TRACE(1,("ExtentsAdd(): ExtentArray out of room, resizing"));

		newArray = (StorageExtent **)realloc(ExtentArray, (MaxNumExtents + EXTENT_ARRAY_SIZE_DEFAULT) * sizeof(StorageExtent *));
		if (newArray == NULL)
		{
			_SMI_TRACE(0,("ExtentsAdd() ERROR: Unable to resize extent array"));
			return;
		}
		for (i = MaxNumExtents; i < (MaxNumExtents + EXTENT_ARRAY_SIZE_DEFAULT); i++)
		{
			newArray[i] = NULL;
		}

		ExtentArray = newArray;

		// Don't forget to add the new extent
		_SMI_TRACE(1,("ExtentsAdd()-resize-: Adding %s", extent->deviceID));
		ExtentArray[MaxNumExtents] = extent;
		NumExtents++;
		MaxNumExtents += EXTENT_ARRAY_SIZE_DEFAULT;
	}
}

/////////////////////////////////////////////////////////////////////////////
void ExtentsRemove(StorageExtent *extent)
{
	CMPICount	i;
	StorageExtent *currExtent;

	// Find & remove extent
	for (i = 0; i < MaxNumExtents; i++)
	{
		currExtent = ExtentArray[i];
		if (strcmp(currExtent->deviceID, extent->deviceID) == 0)
		{
			// Found it, delete it
			_SMI_TRACE(1,("ExtentsAdd(): Removing %s", extent->deviceID));
			ExtentArray[i] = NULL;
			ExtentFree(currExtent);
			NumExtents--;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void ExtentsFree()
{
	CMPICount i;
	StorageExtent *extent;

	_SMI_TRACE(1,("ExtentsFree() called, ExtentArray = %x, NumExtents = %d, MaxNumExtents = %d", ExtentArray, NumExtents, MaxNumExtents));

	if (ExtentArray == NULL)
		return;

	for (i = 0; i < MaxNumExtents; i++)
	{
		extent = ExtentArray[i];
		if (extent)
		{
			ExtentFree(extent);
		}
	}

	free(ExtentArray);
	ExtentArray = NULL;
	NumExtents = 0;
	MaxNumExtents = 0;

	_SMI_TRACE(1,("ExtentsFree() done"));
}

/////////////////////////////////////////////////////////////////////////////
void ExtentsSortBySize(StorageExtent **extArray, CMPICount extArraySize)
{
	CMPICount i;
	_SMI_TRACE(1,("ExtentsSortBySize() called"));
	for (i = 0; i < extArraySize; i++)
	{
		_SMI_TRACE(1,("\t%x:%s", extArray[i], extArray[i]->deviceID));
	}

	qsort(extArray, extArraySize, sizeof(StorageExtent *), SortExtentsCompare);

	_SMI_TRACE(1,("ExtentsSortBySize() done"));
	for (i = 0; i < extArraySize; i++)
	{
		_SMI_TRACE(1,("\t%x:%s", extArray[i], extArray[i]->deviceID));
	}
}

/////////////////////////////////////////////////////////////////////////////
char* ExtentCreateDeviceID(const char *name)
{
	_SMI_TRACE(1,("ExtentCreateDeviceID called for extent %s", name));
	
	char *deviceID;
	deviceID = (char *)malloc(strlen(name) * sizeof(char) + 5);
	sprintf(deviceID, "%s%s", "/dev/", name);

	_SMI_TRACE(1,("ExtentCreateDeviceID done, deviceID = %s", deviceID));
	return deviceID;
}
