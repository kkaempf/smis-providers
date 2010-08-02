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
 |   Provider code dealing with OMC_StorageConfigurationService class
 |
 +-------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include <libintl.h>
//#include <appAPI.h>
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
#include "StorageCapability.h"
#include "StorageExtent.h"
#include "StorageVolume.h"
#include "StorageSetting.h"
#include "LvmCopyServices.h"
#include "CopyServicesExtrinsic.h"
#include <y2storage/StorageInterface.h>

using namespace storage;

extern CMPIBroker * _BROKER;
extern StorageInterface* s;

// Exported globals
CMPIBoolean SCSNeedToScan = 1;

// Module globals
//static CMPIBoolean EVMSopen = 0;
static CMPIUint64 oneKB = 1024;



static int ProcessContainer( 
				StorageExtent *childExt, 
				StorageCapability *childCap, 
				const ContainerInfo contInfo,
				const LvmVgInfo vgInfo);


static int ProcessPartition( 
				StorageExtent *childExt, 
				const PartitionInfo partInfo);


static int ProcessRegion( 
				StorageExtent *childExt, 
				const MdInfo mdInfo);


static int ProcessMDRaid0Region( 
				StorageExtent *childExt, 
				const MdInfo mdInfo);
				

static int ProcessMDRaid1Region(
				StorageExtent *childExt, 
				const MdInfo mdInfo);
				

static int ProcessMDRaid5Region(
				StorageExtent *childExt, 
				const MdInfo mdInfo);
				

static int ProcessVolume(
				StoragePool *pool,
				StorageExtent *dummyExtent,
				const LvmLvInfo lvInfo);

static CMPIUint32 DeleteStoragePool(
					const char *ns,
					StoragePool *pool,
					CMPIStatus *pStatus);


/////////////////////////////////////////////////////////////////////////////
//////////// Private helper functions ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
int WriteLineCallback(
		const char* line,
		FILE * const ostrm,
		char *errMsgBfr,
		size_t errMsgBfrLen)
{
	int rc = 0;
	const char *tline = cmpiutilStrTrim((char *)line);
	const char *parmName;
	char **tokens;
	size_t i, numTokens;
	const char *poolName;
	StorageExtent *extent = NULL;
	static StoragePool *currPool = NULL;
	static StorageCapability *currCap = NULL;

	_SMI_TRACE(1,("WriteLineCallback() called, trimmed line = %s", tline));
	_SMI_TRACE(1,("  Current pool = %s", ((currPool == NULL) ? "(NULL)" : currPool->instanceID)));
	_SMI_TRACE(1,("  Current capability = %s", ((currCap == NULL) ? "(NULL)" : currCap->instanceID)));
	
	if (tline[0] != '\0' && tline[0] != '#')
	{
		_SMI_TRACE(1,("Process line"));

		if (tline[0] == '[')
		{
			// We found beginning of pool section
			tokens = cmpiutilStrTokenize(tline, "[]", NULL);
			if (!tokens)
			{
				_SMI_TRACE(0,("Error parsing poolname -- cmpiutilStrTokenize()"));
			}
			else
			{
				if (tokens[0])
				{
					poolName = tokens[0];
					_SMI_TRACE(1,("Begin processing pool %s", poolName));
					
					// Allocate Pool and Capability objects
					currPool = PoolAlloc(poolName);
					PoolsAdd(currPool);
					currCap = CapabilityAlloc(poolName);
					CapabilitiesAdd(currCap);
					currPool->capability = currCap;
					currCap->pool = currPool;
					currPool->primordial = 1;
				}
				free(tokens);
			}
		}
		else
		{
			// Parse next pool parameter
			tokens = cmpiutilStrTokenize(tline, "= ", &numTokens);
			if (!tokens || numTokens < 2)
			{
				_SMI_TRACE(0,("Error parsing poolname parameter -- cmpiutilStrTokenize()"));
			}
			else if (currPool == NULL || currCap == NULL)
			{
				_SMI_TRACE(0,("Error parsing smsetup file, found name=value parameter before finding [<pool>] section"));
			}
			else
			{
				parmName = cmpiutilStrTrim(tokens[0]);

				if (strcmp(parmName, "package_redundancy") == 0)
				{
					currCap->packageRedundancy = currCap->packageRedundancyMax = atoi(tokens[1]);
					_SMI_TRACE(1,("packet_redundancy = %d", currCap->packageRedundancy));
				}
				else if (strcmp(parmName, "data_redundancy") == 0)
				{
					currCap->dataRedundancy = currCap->dataRedundancyMax = atoi(tokens[1]);
					_SMI_TRACE(1,("data_redundancy = %d", currCap->dataRedundancy));
				}
				else if (strcmp(parmName, "num_stripes") == 0)
				{
					currCap->extentStripe = atoi(tokens[1]);
					_SMI_TRACE(1,("num_stripes = %d", currCap->extentStripe));
				}
				else if (strcmp(parmName, "parity_layout") == 0)
				{
					currCap->parity = atoi(tokens[1]);
					_SMI_TRACE(1,("parity_layout = %d", currCap->parity));
				}
				else if (strcmp(parmName, "devices") == 0)
				{
					for (i = 1; i < numTokens; i++)
					{
						// Allocate extent object for device
						extent = ExtentAlloc(tokens[i], tokens[i]);
						if (extent)
						{
							ExtentsAdd(extent);
							extent->pool = currPool;
							extent->primordial = 1;
							PoolExtentsAdd(currPool, extent);
							_SMI_TRACE(1,("deviceName = %s", extent->name));
							_SMI_TRACE(1,("deviceID = %s", extent->deviceID));
						}
						else
						{
							_SMI_TRACE(0,("ExtentAlloc() - out of memory"));
						}
					}
				}
				else
				{
					_SMI_TRACE(1,("Found unrecognized parameter name %s", parmName));
				}
				free(tokens);
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("Skipping comment or blank line"));
	}

	fputs(line, ostrm);

	_SMI_TRACE(1,("WriteLineCallback() done, rc = %d", rc));
	return rc;
}


/////////////////////////////////////////////////////////////////////////////
static int LoadPersistentStorageObjects(
		const char *ns)
{
	int rc = 0;
	char * setupFileName = "/etc/smsetup.conf";

	cmpiutilFileModifier_CBS setupFileModifier = 
						{
							NULL,				// fileOpened()
							NULL,				// fileClosing()
							WriteLineCallback,	// writeLine()
							NULL				// operationComplete()
						};

	_SMI_TRACE(1,("LoadPersistentStorageObjects() called"));

	rc = cmpiutilModifyFile(setupFileName, &setupFileModifier, NULL, 0);

	_SMI_TRACE(1,("LoadPersistentStorageObjects() done, rc = %d", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
bool IsExtendedPartition(const char *name, char *disk)
{
	_SMI_TRACE(1,("IsExtendedPartition called"));
	int rc;
	deque<PartitionInfo> partInfoList;
	ContVolInfo cVolInfo;
	if(rc = s->getContVolInfo(name, cVolInfo))
	{
		_SMI_TRACE(0,("Error calling getContVolInfo for primordial extent, rc = %d", rc));
		return 0;
	}
	if(rc = cVolInfo.type != DISK)
	{
		_SMI_TRACE(0,("Error: Primordial extent is not a extended partition"));
		return 0;
	}
	if(rc = s->getPartitionInfo(cVolInfo.cname, partInfoList))
	{
		_SMI_TRACE(0,("Error getting info on extended partition's parent disk, rc = %d", rc));
		return 0;
	}
		
	strcpy(disk, cVolInfo.cname.c_str());
	_SMI_TRACE(1,("Extent is an extended partition on disk: %s", disk));
	_SMI_TRACE(1,("IsExtendedPartition done, rc = %d", rc));
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
static void SelectJBODExtents(
				const CMPIUint64 goalSize, 
				StorageExtent **availExtents, 
				CMPICount *numAvailExtents)
{
	int i;
	StorageExtent *extent, *tempExtent;

	_SMI_TRACE(1,("\nSelectJBODExtents() called, goalSize = %lld, numExtents = %d", goalSize, *numAvailExtents));
	// First do a prescan and see if we can find a single extent that is 
	// just the right size or a bit larger that we can use all by itself.
	// NOTE: We are assuming the availExtents array is sorted by size 
	// lowest to highest.

	extent = availExtents[(*numAvailExtents)-1];
	_SMI_TRACE(1,("Examining extent %s", extent->deviceID));
	if ((extent->size) >= goalSize)
	{
		_SMI_TRACE(1,("Single extent satisfies goalSize"));
		// We have at least 1 extent that can meet the goalSize solo.
		StorageExtent *bestExtent = extent;

		// See if we can find one even closer to the goal size
		if (*numAvailExtents > 1)
		{
			for (i = ((*numAvailExtents)-2); i >= 0; i--)
			{
				extent = availExtents[i];
				if ((extent->size) >= goalSize)
				{
					bestExtent = extent;
				}
				else
				{
					break;
				}
			}
		}

		// Put the best extent in array slot 0 and adjust arraySize to 1
		availExtents[0] = bestExtent;
		*numAvailExtents = 1;
	}

	// Choose extents to be used to meet the goal/size. 
	// Look at the largest extents first, if the last extent (or only extent) that 
	// we will use is larger than what was requested by MIN_VOLUME_SIZE bytes or more 
	// we will need to split the oversized extent into used and free parts and 
	// perform Extent Conservation.

	CMPIUint64 calcSize = 0;
	for (i = ((*numAvailExtents)-1); i >= 0 ; i--)
	{
		extent = availExtents[i];
		calcSize += extent->size;

		_SMI_TRACE(1,("Using extent[%d] %s, size = %lld, calcSize = %lld", i, extent->deviceID, extent->size, calcSize));

		if (calcSize >= goalSize || ExtentIsFreespace(extent))
		{
			_SMI_TRACE(1,("Check to see if extent requires partitioning"));
			// We have all the extents that we need AND/OR we have a Freespace extent
			if ((calcSize - goalSize) > MIN_VOLUME_SIZE || ExtentIsFreespace(extent))
			{
				// We need to partition this oversized or freespace extent and possibly
				// create a remaining extent (i.e. Extent Conservation) if the extent
				// is not already a remaining/freespace extent.

				_SMI_TRACE(1,("Splitting oversize extent %s", extent->deviceID));

				CMPIUint64 partSize;
				if (calcSize >= goalSize)
				{
					partSize = extent->size - (calcSize - goalSize);
				}
				else
				{
					partSize = extent->size;
				}

				StorageExtent *remExtent;
				ExtentPartition(extent, partSize, &tempExtent, &remExtent);

				_SMI_TRACE(1,("New resized extent = %s", tempExtent->deviceID));
				if (remExtent != NULL)
				{
					_SMI_TRACE(1,("Remaining extent = %s", remExtent->deviceID));
				}

				// Replace the oversized extent with the new right-sized extent
				availExtents[i] = tempExtent;
			}
			// Remove unneeded extents from list
			if (calcSize >= goalSize && i > 0)
			{
				_SMI_TRACE(1,("Removing unneeded extents 0 - %d", i-1));
				CMPICount j;
				for (j = 0; j < ((*numAvailExtents) - i); j++)
				{
					availExtents[j] = availExtents[i+j];
				}
				*numAvailExtents -= i;
				_SMI_TRACE(1,("NEW numAvailExtents = %d", *numAvailExtents));
				break;
			}
		}
	}
	_SMI_TRACE(1,("SelectJBODExtents() finished, numExtents = %d", *numAvailExtents));
	for (i = 0; i < *numAvailExtents; i++)
	{
		_SMI_TRACE(1,("\tUsing extent %s", availExtents[i]->deviceID));
	}
	_SMI_TRACE(1,("\n"));
}	

/////////////////////////////////////////////////////////////////////////////
static void SelectRAIDExtents(
				StorageExtent **availExtents, 
				const CMPIUint16 numExtents,
				const CMPIUint64 reqExtSize, 
				StorageExtent **outExtents, 
				CMPICount *numOutExtents)
{
	StorageExtent *tempExtent;
	CMPIUint64 extSize;
	CMPICount numAvailExtents = *numOutExtents;

	_SMI_TRACE(1,("\nSelectRAIDExtents() called, numReqExtents = %d, reqExtSize = %lld", numExtents, reqExtSize));

	// NOTE: For any of the extents to be used 
	// that are oversized by MIN_VOLUME_SIZE bytes or more we will need to split the
	// oversize extent into used and free parts and perform Extent Conservation

	CMPICount i, outIdx = 0;;
	for (i = 0; i < numAvailExtents; i++)
	{
		StorageExtent *extent = availExtents[i];
		extSize = extent->size;

		if (extSize >= reqExtSize)
		{
			// We found an extent that will work, see if we need to partition
			// it first.
			if ((extSize - reqExtSize) > MIN_VOLUME_SIZE || ExtentIsFreespace(extent))
			{
				// We need to slice up this extent to avoid wasted space
				_SMI_TRACE(1,("Splitting oversize extent %s", extent->deviceID));

				StorageExtent *remExtent;
				ExtentPartition(extent, reqExtSize, &tempExtent, &remExtent);

				_SMI_TRACE(1,("New resized extent = %s", tempExtent->deviceID));
				if (remExtent != NULL)
				{
					_SMI_TRACE(1,("Remaining extent = %s", remExtent->deviceID));
				}

				outExtents[outIdx++] = tempExtent;
			}
			else
			{
				outExtents[outIdx++] = extent;
			}
					
			if (outIdx == numExtents)
			{
				// We have what we need, no need to look further
				break;
			}
		}
	}
	*numOutExtents = outIdx;

	_SMI_TRACE(1,("SelectRAIDExtents() finished, numOutExtents = %d", *numOutExtents));
	for (i = 0; i < *numOutExtents; i++)
	{
		_SMI_TRACE(1,("\tUsing extent %s", outExtents[i]->deviceID));
	}
	_SMI_TRACE(1,("\n"));
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateJBODComposition(
				StorageExtent **inExtents,
				CMPICount numInExtents,
				char *containingPoolName)
{
	CMPIUint32 rc = M_COMPLETED_OK , s_rc = 0;
	StorageExtent *antExtent;
	char nameBuf[256];
	deque<string> vg_devices;


	_SMI_TRACE(1,("\nCreateJBODComposition() called"));

	CMPICount i; 
	for (i = 0; i < numInExtents; i++)
	{
		antExtent = inExtents[i];
		_SMI_TRACE(1,("Looping thru input extents, processing extent %s", antExtent->name));
		vg_devices.push_back(antExtent->name);
	/*	if(strcasecmp(antExtent->name, "Freespace"))
		{
			ExtentGetContainerName(antExtent, nameBuf, 256);
			vg_devices.push_back(antExtent->name);
		}
		else
		{
			vg_devices.push_back(nameBuf);
		}
	*/
	}

	_SMI_TRACE(1,("Calling createLvmVg"));
	if(rc = s->createLvmVg(containingPoolName, 1024, false, vg_devices))
	{
		_SMI_TRACE(0,("Error creating LVM2 Volume Group for JBOD pool: rc = %s", rc));
		rc = M_FAILED;
	}
exit:
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
	_SMI_TRACE(1,("CreateJBODComposition() done, rc = %d\n", rc));
	return rc;
}


/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateRAID0Composition(
				StorageExtent **inExtents,
				CMPICount numExtents,
				char *containingPoolName,
				CMPIUint64 userDataStripeDepth,
				StorageExtent **outExtent)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char nameBuf[256] = {0};
	StorageExtent *composite;
	const char *compositeName;
	int evmsrc;
	deque<string> md_devices;
	deque<string> vg_devices;
	string md_name;

	_SMI_TRACE(1,("\nCreateRAID0Composition() called"));

	CMPICount i;
	for (i = 0; i < numExtents; i++)
	{
		StorageExtent* antExtent = inExtents[i];

		_SMI_TRACE(1,("Looping thru input extents, processing extent %s", antExtent->deviceID));
		md_devices.push_back(antExtent->name);
	}
/***********************	LVM code	**************************/
	
	_SMI_TRACE(1,("Create MD Raid 0 region"));
	if(rc = s->createMdAny((MdType)RAID0, md_devices, md_name))
	{
		_SMI_TRACE(0,("Error creating MD Raid 0 region, rc = %u", rc));
		rc = M_FAILED;
		goto exit;
	}
	md_devices.clear();
	// Create MD raid0 region using goal UserDataStripeDepth as "Chunk Size"
	if(rc = s->changeMdChunk(md_name, userDataStripeDepth / 1024))
	{
		_SMI_TRACE(0,("Error setting Chunk size of MD Raid 0 region, rc = %u", rc));
		rc = M_FAILED;
		goto exit;
	}
	 
//	compositeName = infoHandle->info.region.name;
	compositeName = md_name.c_str(); 
	vg_devices.push_back(md_name);

	// Allocate/initialize composite extent
	if (outExtent != NULL)
	{
		_SMI_TRACE(1,("Allocate composite extent %s", compositeName));
		composite = ExtentAlloc(compositeName, compositeName);
		ExtentsAdd(composite);
		*outExtent = composite;
	}
	if (containingPoolName != NULL && strlen(containingPoolName) != 0)
	{

	/**************		LVM code	***********/						
		_SMI_TRACE(1,("Create LVM VG for RAID0 pool"));
		if(rc = s->createLvmVg(containingPoolName, 1024, true, vg_devices))
		{
			_SMI_TRACE(0,("Error creating LVM2 Volume Group for JBOD pool: rc = %s", rc));
			rc = M_FAILED;
		}
		vg_devices.clear();
	}


exit:
	_SMI_TRACE(1,("CreateRAID0Composition() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
		
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateRAID1Composition(
				StorageExtent **inExtents,
				CMPICount numExtents,
				char *containingPoolName,
				StorageExtent **outExtent)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char nameBuf[256] = {0};
	StorageExtent *composite;
	const char *compositeName;
	int evmsrc;
	deque<string> md_devices;
	deque<string> vg_devices;
	string md_name;

	_SMI_TRACE(1,("\nCreateRAID1Composition() called"));


	size_t i;
	for (i = 0; i < numExtents; i++)
	{
		StorageExtent* antExtent = inExtents[i];

		_SMI_TRACE(1,("Looping thru input extents, processing extent %s", antExtent->deviceID));
		md_devices.push_back(antExtent->name);
	}

/***********************	LVM code	**************************/
	
	_SMI_TRACE(1,("Create MD Raid 1 region"));
	if(rc = s->createMdAny((MdType)RAID1, md_devices, md_name))
	{
		_SMI_TRACE(0,("Error creating MD Raid 1 region, rc = %u", rc));
		rc = M_FAILED;
		goto exit;
	}
	md_devices.clear();
	 
//	compositeName = infoHandle->info.region.name;
	compositeName = md_name.c_str(); 
	vg_devices.push_back(md_name);

	// Allocate/initialize composite extent
	if (outExtent != NULL)
	{
		_SMI_TRACE(1,("Allocate composite extent %s", compositeName));
		composite = ExtentAlloc(compositeName, compositeName);
		ExtentsAdd(composite);

		StorageExtent *antExtent = inExtents[0];
		_SMI_TRACE(1,("Initialize composite from underlying extent %s", antExtent->deviceID));

		composite->size = antExtent->size;
		composite->primordial = 0;
		composite->composite = 1;
		*outExtent = composite;
	}
	if (containingPoolName != NULL && strlen(containingPoolName) != 0)
	{
		_SMI_TRACE(1,("Create LVM VG for RAID1 pool"));
		if(rc = s->createLvmVg(containingPoolName, 1024, true, vg_devices))
		{
			_SMI_TRACE(0,("Error creating LVM2 Volume Group for RAID pool: rc = %s", rc));
			rc = M_FAILED;
		}
		vg_devices.clear();
	}

exit:
	_SMI_TRACE(1,("CreateRAID1Composition() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static void DeleteRAID1Composition(StorageExtent *r1CompExtent)
{
	_SMI_TRACE(1,("\nDeleteRAID1Composition() called"));

	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;

	if (r1CompExtent == NULL)
		return;

	// Delete the corresponding Md RAID region (including the superblocks)
	char *regionName = r1CompExtent->name;

	if(rc = s->removeMd(regionName, true))
	{
		_SMI_TRACE(1,("Unable to delete RAID 1 composition, rc = %d", rc));
	}
	_SMI_TRACE(1,("DeleteRAID1Composition() done\n"));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
}


/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateRAID10Composition(
				StorageExtent **inExtents,
				CMPICount numExtents,
				char *containingPoolName,
				CMPIUint16 dataRedundancy,
				CMPIUint16 extentStripeLength,
				CMPIUint64 userDataStripeDepth,
				StorageExtent **outExtent)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	StorageExtent **raid1InExtents;
	StorageExtent **raid1CompExtents;

	_SMI_TRACE(1,("\nCreateRAID10Composition() called"));

	// Allocate our intermediate extent arrays
	raid1InExtents = (StorageExtent **)malloc(dataRedundancy * sizeof(StorageExtent *));
	if (raid1InExtents == NULL)
	{
		_SMI_TRACE(0,("Out of memory!!!"));
		rc = M_FAILED;
		goto exit;
	}

	raid1CompExtents = (StorageExtent **)malloc(extentStripeLength * sizeof(StorageExtent *));
	if (raid1CompExtents == NULL)
	{
		_SMI_TRACE(0,("Out of memory!!!"));
		rc = M_FAILED;
		goto exit;
	}

	// First create the lower level RAID 1 compositions
	size_t i;
	for (i = 0; i < extentStripeLength; i++)
	{
		size_t j;
		for(j = 0; j < dataRedundancy; j++)
		{
			raid1InExtents[j] = inExtents[j+i*dataRedundancy]; 
		}

		StorageExtent *r1CompExtent;
		rc = CreateRAID1Composition(raid1InExtents, dataRedundancy, NULL, &r1CompExtent);

		if (rc != M_COMPLETED_OK || r1CompExtent == NULL)
		{
			_SMI_TRACE(0,("RAID10: Unable to create underlying mirrors"));
			goto exitErrorCleanup;
		}

		_SMI_TRACE(1,("Created RAID 1 Composite extent = %s", r1CompExtent->deviceID));

		raid1CompExtents[i] = r1CompExtent;
	}

	// Now create higher level RAID 0 composition made up of lower level mirrors
	rc = CreateRAID0Composition(
					raid1CompExtents, 
					extentStripeLength, 
					containingPoolName, 
					userDataStripeDepth,
					outExtent);

	if (rc != M_COMPLETED_OK)
	{
		_SMI_TRACE(0,("RAID10: Unable to create RAID0 top level extents"));
		goto exitErrorCleanup;
	}

exit:
	if (raid1InExtents)
	{
		free(raid1InExtents);
	}
	if (raid1CompExtents)
	{
		free(raid1CompExtents);
	}

	_SMI_TRACE(1,("CreateRAID10Composition() done, rc = %d\n", rc));
	return rc;

exitErrorCleanup:
	_SMI_TRACE(1,("RAID10: Unable to create RAID 10 composition"));

	// Delete any RAID 1 compositions we may have done
	for (i = 0; i < extentStripeLength; i++)
	{
		if (raid1CompExtents[i])
		{
			DeleteRAID1Composition(raid1CompExtents[i]);
		}
	}
	goto exit;
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateRAID5Composition(
				StorageExtent **inExtents,
				CMPICount numExtents,
				char *containingPoolName,
				CMPIUint64 userDataStripeDepth,
				StorageExtent **outExtent)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char nameBuf[256] = {0};
	StorageExtent *composite;
	const char *compositeName;
	int evmsrc;
	deque<string> md_devices;
	deque<string> vg_devices;
	string md_name;

	_SMI_TRACE(1,("\nCreateRAID5Composition() called"));


	CMPICount i;
	for (i = 0; i < numExtents; i++)
	{
		StorageExtent* antExtent = inExtents[i];

		_SMI_TRACE(1,("Looping thru input extents, processing extent %s", antExtent->name));
		md_devices.push_back(antExtent->name);
	}

/***********************	LVM code	**************************/
	
	_SMI_TRACE(1,("Create MD Raid 5 region"));
	if(rc = s->createMdAny((MdType)RAID5, md_devices, md_name))
	{
		_SMI_TRACE(0,("Error creating MD Raid 5 region, rc = %u", rc));
		rc = M_FAILED;
		goto exit;
	}
	md_devices.clear();
	// Create MD raid5 region using goal UserDataStripeDepth as "Chunk Size"
	if(rc = s->changeMdChunk(md_name, userDataStripeDepth / 1024))
	{
		_SMI_TRACE(0,("Error setting Chunk size of MD Raid 5 region, rc = %u", rc));
		rc = M_FAILED;
		goto exit;
	}
	 
	// Set the Parity of the RAID 5 device
	if(rc = s->changeMdParity(md_name, (MdParity)LEFT_ASYMMETRIC))
	{
		_SMI_TRACE(0,("Error setting parity of MD Raid 5 region, rc = %u", rc));
		rc = M_FAILED;
		goto exit;
	}
	 
//	compositeName = infoHandle->info.region.name;
	compositeName = md_name.c_str(); 
	vg_devices.push_back(md_name);

	// Allocate/initialize composite extent
	if (outExtent != NULL)
	{
		_SMI_TRACE(1,("Allocate composite extent %s", compositeName));
		composite = ExtentAlloc(compositeName, compositeName);
		ExtentsAdd(composite);
		*outExtent = composite;
	}
	if (containingPoolName != NULL && strlen(containingPoolName) != 0)
	{
		_SMI_TRACE(1,("Create LVM VG for RAID5 pool"));
		if(rc = s->createLvmVg(containingPoolName, 1024, true, vg_devices))
		{
			_SMI_TRACE(0,("Error creating LVM2 Volume Group for RAID pool: rc = %s", rc));
			rc = M_FAILED;
		}
		vg_devices.clear();
	}
exit:
	_SMI_TRACE(1,("CreateRAID5Composition() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static void DeleteComposition(StorageExtent *compExtent)
{
	_SMI_TRACE(1,("\nDeleteComposition() called: extent = %s", compExtent->name));

	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;

	char *regionName = compExtent->name;

	if(rc = s->removeMd(regionName, true))
	{
		_SMI_TRACE(1,("Unable to delete RAID 1 composition, rc = %d", rc));
	}
	
	CMPICount i;
	for (i = 0; i < compExtent->maxNumAntecedents; i++)
	{
		if (compExtent->antecedents[i])
		{
			BasedOn *antBO = compExtent->antecedents[i];
			StorageExtent *antExtent = antBO->extent;
			_SMI_TRACE(1,("Examine underlying extent %s", antExtent->name));

			if (!antExtent->composite && ExtentGetPool(antExtent)->primordial)
			{
				_SMI_TRACE(1,("Call unpartition for underlying primordial extent %s", antExtent->name));
				ExtentUnpartition(antExtent);
			}
			else if (antExtent->composite)
			{
				_SMI_TRACE(1,("Call DeleteComposition for underlying extent %s", antExtent->name));
				DeleteComposition(antExtent);
			}
		}
	}

	_SMI_TRACE(1,("DeleteComposition() done\n"));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d",s_rc));
}

/////////////////////////////////////////////////////////////////////////////
/*static CMPIUint32 RenameStoragePool(StoragePool *pool, const char *name)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	object_handle_t evmsHandle;
	option_array_t options;

	_SMI_TRACE(1,("\nRenameStoragePool() called: pool = %s, new name = %s", pool->name, name));

	char nameBuf[256];
	sprintf(nameBuf, "lvm2/", pool->name);
	evmsHandle = SCSGetEVMSObject(nameBuf);

	options.count = 1;
	options.option[0].is_number_based = FALSE;
	options.option[0].name = "name";
	options.option[0].type = EVMS_Type_String;
	options.option[0].flags = 0;
	options.option[0].value.s = (char *)name;
	
	_SMI_TRACE(1,("Call evms_set_info"));
	if (evms_set_info(evmsHandle, &options) != 0)
	{
		_SMI_TRACE(0,("Error returned from evms_set_info!"));
		rc = M_FAILED;
	}

	_SMI_TRACE(1,("RenameStoragePool() done, rc = %d\n", rc));
	return rc;
}*/

static CMPIUint32 RenameStoragePool(StoragePool *pool, const char *name)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char command[256];

	_SMI_TRACE(1,("\nRenameStoragePool() called: pool = %s, new name = %s", pool->name, name));

	_SMI_TRACE(1,("Trying to build the rename command"));
	sprintf(command, "%s %s %s", "vgrename", pool->name, name);
	_SMI_TRACE(1,("command = %s", command));
	if (rc = system(command))
	{
		_SMI_TRACE(0,("Error returned when renaming pool !"));
		rc = M_FAILED;
	}

	_SMI_TRACE(1,("RenameStoragePool() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d",s_rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static CMPIUint32 ModifyConcretePool(
						StoragePool *pool, 
						const char *ns,
						const char *name,
						StorageSetting *goal,
						const CMPIUint64 goalSize,
						StoragePool *inPool)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	char nameBuf[256];
	StorageExtent **availExtents = NULL;
	StorageExtent **raidExtents = NULL;
	handle_array_t *handles = NULL;
	handle_array_t *shrinkHandles = NULL;
	object_handle_t	evmsHandle;
	char currName[256];
	CMPIUint64 currSize, expandSize = 0, shrinkSize = 0;
	CMPIUint64 minSize, maxSize, divisor;

	_SMI_TRACE(1,("\nModifyConcretePool() called: pool = %s", name));

	if (pool == NULL)
	{
		_SMI_TRACE(0,("Invalid modify pool specified"));
		rc = M_INVALID_PARAM;
		goto exit;
	}
	strcpy(currName, pool->name);

	// Check for invalid/unsupported input parameters
	if (goal != NULL)
	{
		// If goal is specified we currently require it to match inPool defaults
		_SMI_TRACE(1,("Input goal specified, compare to defaults"));
		if (!PoolGoalMatchesDefault(pool, goal))
		{
			_SMI_TRACE(0,("Input goal does not match pool defaults"));
			rc = M_NOT_SUPPORTED;
			goto exit;
		}
	}

	// Before we get too far, make sure evms will even allow us to do the resize
	sprintf(nameBuf, "%s%s", "lvm2/", currName); 
	evmsHandle = SCSGetEVMSObject(nameBuf);
	currSize = pool->totalSize;
	
	if (goalSize > currSize)
	{
		expandSize = goalSize - currSize;

		// See if we can expand pool/container
		if (evms_can_expand(evmsHandle) != 0)
		{
			_SMI_TRACE(0,("Pool cannot be expanded using evms_expand!"));
			rc = M_NOT_SUPPORTED;
			goto exit;
		}

		// Determine how we can best expand the pool based on inPools/inExtents
		// Check to see if any InPool specified
		if (inPool == NULL)
		{
			// No InPool specified, search for pool that has adequate free space
			// to meet the expansion goal/size requirement. 
			_SMI_TRACE(1,("No Input pool specified, search for pool that meets expansion goal requirements"));

			inPool = PoolsFindByGoal(goal, &expandSize);
			if (inPool == NULL)
			{
				// Couldn't find pool to support the requested Goal/Size, fail.
				_SMI_TRACE(0,("Unable to find pool that meets goal/size requirements!!!"));
				rc = M_SIZE_NOT_SUPPORTED;	
				goto exit;
			}
		}
		else
		{
			// InPool was specified, make sure we can meet expansion Goal/Size requirements

			if (PoolGetSupportedSizeRange(inPool, goal, &minSize, &maxSize, &divisor) == GAE_COMPLETED_OK)
			{
				_SMI_TRACE(1,("Max supported size of inPool is %lld", maxSize));
				if (maxSize < expandSize || maxSize == 0)
				{
					// Input pool doesn't cut it, fail.
					_SMI_TRACE(0,("Specified input pool does not meet goal/size requirements!!!"));
					rc = M_SIZE_NOT_SUPPORTED;	
					goto exit;
				}
			}
			else
			{
				// Something wrong with input pool, fail
				_SMI_TRACE(0,("Problem with specified input pool, unable to meet goal/size requirements!!!"));
				rc = M_SIZE_NOT_SUPPORTED;	
				goto exit;
			}

		}

		// At this point we know we can do the pool create as requested, get list of
		// sorted available extents we can use to create the pool. 

		_SMI_TRACE(1,("Have valid inPool, expandSize = %lld,  getting available extents....", expandSize));
		CMPICount numAvailExtents = PoolExtentsSize(inPool);
		availExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
		if (availExtents == NULL)
		{
			_SMI_TRACE(0,("Out of memory!!!"));
			rc = M_FAILED;
			goto exit;
		}

		PoolGetAvailableExtents(inPool, goal, availExtents, &numAvailExtents);
		ExtentsSortBySize(availExtents, numAvailExtents);

		if (PoolDefaultCapabilitiesMatch(pool, inPool))
		{
			_SMI_TRACE(1,("Concrete pool to modify is JBOD or based on underlying non-JBOD primordial pool"));
			// Expand a JBOD concrete pool or non-JBOD pool based on non-JBOD primordial.
			// Choose extents to be used to meet the goal expansion size. 
			SelectJBODExtents(expandSize, availExtents, &numAvailExtents);

			// Now do the expansion
			size_t numExtents = numAvailExtents;
			handles = (handle_array_t *)malloc(sizeof(handle_array_t) + numExtents*sizeof(object_handle_t));
			if (handles == NULL)
			{
				_SMI_TRACE(0,("Unable to alloc memory for evms handle/option arrays"));
				rc = M_FAILED;
				goto exit;
			}

			handles->count = numExtents;
			size_t i;
			for (i = 0; i < numExtents; i++)
			{
				StorageExtent* antExtent = availExtents[i];

				_SMI_TRACE(1,("Looping thru input extents, processing extent %s", antExtent->name));

				if (antExtent->primordial)
				{
					handles->handle[i] = SCSGetEVMSObject(antExtent->name);
				}
				else
				{
					ExtentGetContainerName(antExtent, nameBuf, 256);
					if (strlen(nameBuf) == 0)
					{
						handles->handle[i] = SCSGetEVMSObject(antExtent->name);
					}
					else
					{
						strcat(nameBuf, "/");
						strcat(nameBuf, antExtent->name);
						_SMI_TRACE(1,("nameBuf = %s", nameBuf));
						handles->handle[i] = SCSGetEVMSObject(nameBuf);
					}
				}
			}
		}
		else 
		{
			_SMI_TRACE(1,("Pool is software RAID (composition)"));
			// RAID case
			handles = (handle_array_t *)malloc(sizeof(handle_array_t) + sizeof(object_handle_t));
			if (handles == NULL)
			{
				_SMI_TRACE(0,("Unable to alloc memory for evms handle/option arrays"));
				rc = M_FAILED;
				goto exit;
			}

			StorageExtent* antExtent;

			if (CapabilityDefaultIsRAID0(pool->capability))
			{
				// Expand a RAID0 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->extentStripe,
							expandSize / pool->capability->extentStripe,
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID0 composite by which we'll expand the existing pool/container
				rc = CreateRAID0Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									pool->capability->userDataStripeDepth,
									&antExtent);
			}
			else if (CapabilityDefaultIsRAID1(pool->capability))
			{
				// Expand a RAID1 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->dataRedundancy,
							expandSize,
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID0 composite by which we'll expand the existing pool/container
				rc = CreateRAID1Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									&antExtent);
			}
			else if (CapabilityDefaultIsRAID10(pool->capability))
			{
				// Expand a RAID10 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->dataRedundancy * pool->capability->extentStripe,
							expandSize / pool->capability->extentStripe,
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID10 composite by which we'll expand the existing pool/container
				rc = CreateRAID10Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									pool->capability->dataRedundancy,
									pool->capability->extentStripe,
									pool->capability->userDataStripeDepth,
									&antExtent);
			}
			else if (CapabilityDefaultIsRAID5(pool->capability))
			{
				// Expand a RAID5 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->extentStripe,
							expandSize / (pool->capability->extentStripe-1),
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID5 composite by which we'll expand the existing pool/container
				rc = CreateRAID5Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									pool->capability->userDataStripeDepth,
									&antExtent);
			}

			_SMI_TRACE(1,("Processing expand input extent %s", antExtent->name));

			handles->count = 1;
			handles->handle[0] = SCSGetEVMSObject(antExtent->name);
		}

		_SMI_TRACE(1,("Call evms_expand"));
		if (evms_expand(evmsHandle, handles, NULL) != 0)
		{
			_SMI_TRACE(0,("Error returned from evms_expand!"));
			rc = M_FAILED;
			goto exit;
		}
	}
	else
	{
		shrinkSize = currSize - goalSize;

		// See if we can shrink
		if (evms_can_shrink(evmsHandle) != 0)
		{
			_SMI_TRACE(0,("Pool cannot be shrunk using evms_shrink!"));
			rc = M_NOT_SUPPORTED;
			goto exit;
		}

		// Determine how we are going to shrink our pool by listing shrinkable pool extents.
		// Once we decide which extents to adjust/remove to satisfy the shrink then call
		// evms_shrink to to the evms dirty work.

		// Choose extents to be used to meet the goal shrinkage size
		CMPIUint64 excessShrinkage;
	
		rc = PoolGetShrinkExtents(pool, evmsHandle, &shrinkSize, &excessShrinkage, &shrinkHandles);

		if (rc != M_COMPLETED_OK)
		{
			_SMI_TRACE(0,("Pool has no component extents that can be shrunk!"));
			goto exit;
		}
		_SMI_TRACE(1,("Back from getShrinkExtents, shrinkSize = %llu, excess = %llu", shrinkSize, excessShrinkage));
		_SMI_TRACE(1,("  Number of shrink objects = %d", shrinkHandles->count));

		_SMI_TRACE(1,("Call evms_shrink"));
		if (evms_shrink(evmsHandle, shrinkHandles, NULL) != 0)
		{
			_SMI_TRACE(0,("Error returned from evms_shrink!"));
			rc = M_FAILED;
			goto exit;
		}

		pool->totalSize -= shrinkSize + excessShrinkage;
		pool->remainingSize -= shrinkSize + excessShrinkage;

		// Do any additional cleanup required for the extents we just deleted from the pool
		uint i;
		for (i = 0; i < shrinkHandles->count; i++)
		{
			handle_object_info_t *infoHandle;
			if (evms_get_info(shrinkHandles->handle[i], &infoHandle) != 0)
			{
				_SMI_TRACE(0,("Unable to get EVMS info for shrink handle!"));
				continue;
			}

			char *extName = strrchr(infoHandle->info.object.name, '/');
			if (extName != NULL && !omcStrStartsWith(infoHandle->info.object.name, "md/"))
			{
				extName++;
			}
			else
			{
				extName = infoHandle->info.object.name;
			}
			_SMI_TRACE(1,("Shrink extent name = %s", extName));
			
			StorageExtent *ext = ExtentsFind(extName);
			if (ext == NULL)
			{
				_SMI_TRACE(0,("Unable to find extent object for %s", extName));
				evms_free(infoHandle);
				continue;
			}

			if (!ext->composite && ExtentGetPool(ext)->primordial)
			{
				_SMI_TRACE(1,("Call unpartition for %s", ext->name));
				ExtentUnpartition(ext);
			}
			else if (ext->composite)
			{
				_SMI_TRACE(1,("Call DeleteComposition for %s", ext->name));
				DeleteComposition(ext);
			}
			evms_free(infoHandle);
		}

		// Now--because of the way EVMS shrinks a container/pool by removing whole objects
		// we may need to expand the container a bit to account for any excess shrinkage 
		// that may have occurred.
		if (excessShrinkage > MIN_VOLUME_SIZE)
		{
			char inPoolName[256];
			if (inPool != NULL)
			{
				strcpy(inPoolName, inPool->name);
			}

			evms_commit_changes();
			SCSScanStorage(ns);
			_SMI_TRACE(1,("Expanding pool to account for excess shrinkage"));

			// We need to make sure we get current/valid pool and inPool pointers
			// before we call ModifyConcretePool
			pool = PoolsFindByName(currName);
			if (inPool != NULL)
			{
				inPool = PoolsFindByName(inPoolName);
			}

			ModifyConcretePool(pool, ns, currName, NULL, goalSize, inPool);	
		}
	}

	// Lastly see if we need to do a rename
	if (name != NULL && strlen(name) > 0 && (strcmp(name, currName) != 0))
	{
		rc = RenameStoragePool(pool, name);

		if (rc != M_COMPLETED_OK)
		{
			_SMI_TRACE(1,("Unable to rename storage pool"));
		}
	}

exit:
	if (availExtents)
	{
		free(availExtents);
	}
	if (raidExtents)
	{
		free(raidExtents);
	}
	if (handles)
	{
		free(handles);
	}
	if (shrinkHandles)
	{
		free(shrinkHandles);
	}

	_SMI_TRACE(1,("ModifyConcretePool() done, rc = %d\n", rc));
	return rc;
}
*/
static CMPIUint32 ModifyConcretePool(
						StoragePool *pool, 
						const char *ns,
						const char *name,
						StorageSetting *goal,
						const CMPIUint64 goalSize,
						StoragePool *inPool)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char nameBuf[256];
	StorageExtent **availExtents = NULL;
	StorageExtent **raidExtents = NULL;
	char currName[256];
	CMPIUint64 currSize, expandSize = 0, shrinkSize = 0;
	CMPIUint64 minSize, maxSize, divisor;

	_SMI_TRACE(1,("\nModifyConcretePool() called: pool = %s", name));


	if (pool == NULL)
	{
		_SMI_TRACE(0,("Invalid modify pool specified"));
		rc = M_INVALID_PARAM;
		goto exit;
	}
	strcpy(currName, pool->name);

	// Check for invalid/unsupported input parameters
	if (goal != NULL)
	{
		// If goal is specified we currently require it to match inPool defaults
		_SMI_TRACE(1,("Input goal specified, compare to defaults"));
		if (!PoolGoalMatchesDefault(pool, goal))
		{
			_SMI_TRACE(0,("Input goal does not match pool defaults"));
			rc = M_NOT_SUPPORTED;
			goto exit;
		}
	}

	if(goalSize == 0)
	{
		_SMI_TRACE(1,("Goal size not specified, only trying to do rename"));
		goto exit_rename_only;
	}
	// Before we get too far, make sure evms will even allow us to do the resize
	currSize = pool->totalSize;
	
	if (goalSize > currSize)
	{
		expandSize = goalSize - currSize;

		// See if we can expand pool/container

		// Determine how we can best expand the pool based on inPools/inExtents
		// Check to see if any InPool specified
		if (inPool == NULL)
		{
			// No InPool specified, search for pool that has adequate free space
			// to meet the expansion goal/size requirement. 
			_SMI_TRACE(1,("No Input pool specified, search for pool that meets expansion goal requirements"));

			inPool = PoolsFindByGoal(goal, &expandSize);
			if (inPool == NULL)
			{
				// Couldn't find pool to support the requested Goal/Size, fail.
				_SMI_TRACE(0,("Unable to find pool that meets goal/size requirements!!!"));
				rc = M_SIZE_NOT_SUPPORTED;	
				goto exit;
			}
		}
		else
		{
			// InPool was specified, make sure we can meet expansion Goal/Size requirements

			if (PoolGetSupportedSizeRange(inPool, goal, &minSize, &maxSize, &divisor) == GAE_COMPLETED_OK)
			{
				_SMI_TRACE(1,("Max supported size of inPool is %lld", maxSize));
				if (maxSize < expandSize || maxSize == 0)
				{
					// Input pool doesn't cut it, fail.
					_SMI_TRACE(0,("Specified input pool does not meet goal/size requirements!!!"));
					rc = M_SIZE_NOT_SUPPORTED;	
					goto exit;
				}
			}
			else
			{
				// Something wrong with input pool, fail
				_SMI_TRACE(0,("Problem with specified input pool, unable to meet goal/size requirements!!!"));
				rc = M_SIZE_NOT_SUPPORTED;	
				goto exit;
			}

		}

		// At this point we know we can do the pool create as requested, get list of
		// sorted available extents we can use to create the pool. 

		_SMI_TRACE(1,("Have valid inPool, expandSize = %lld,  getting available extents....", expandSize));
		CMPICount numAvailExtents = PoolExtentsSize(inPool);
		availExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
		if (availExtents == NULL)
		{
			_SMI_TRACE(0,("Out of memory!!!"));
			rc = M_FAILED;
			goto exit;
		}

		PoolGetAvailableExtents(inPool, goal, availExtents, &numAvailExtents);
		ExtentsSortBySize(availExtents, numAvailExtents);
		
		deque<string> vg_devices;

		if (PoolDefaultCapabilitiesMatch(pool, inPool))
		{
			_SMI_TRACE(1,("Concrete pool to modify is JBOD or based on underlying non-JBOD primordial pool"));
			// Expand a JBOD concrete pool or non-JBOD pool based on non-JBOD primordial.
			// Choose extents to be used to meet the goal expansion size. 
			SelectJBODExtents(expandSize, availExtents, &numAvailExtents);

			// Now do the expansion
			size_t numExtents = numAvailExtents;
			size_t i;
			for (i = 0; i < numExtents; i++)
			{
				StorageExtent* antExtent = availExtents[i];

				_SMI_TRACE(1,("Looping thru input extents, processing extent %s", antExtent->name));
				vg_devices.push_back(antExtent->name);
			}
		}
		else 
		{
			_SMI_TRACE(1,("Pool is software RAID (composition)"));
			// RAID case
			vg_devices.clear();

			StorageExtent* antExtent;

			if (CapabilityDefaultIsRAID0(pool->capability))
			{
				// Expand a RAID0 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->extentStripe,
							expandSize / pool->capability->extentStripe,
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID0 composite by which we'll expand the existing pool/container
				rc = CreateRAID0Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									pool->capability->userDataStripeDepth,
									&antExtent);
			}
			else if (CapabilityDefaultIsRAID1(pool->capability))
			{
				// Expand a RAID1 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->dataRedundancy,
							expandSize,
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID0 composite by which we'll expand the existing pool/container
				rc = CreateRAID1Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									&antExtent);
			}
			else if (CapabilityDefaultIsRAID10(pool->capability))
			{
				// Expand a RAID10 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->dataRedundancy * pool->capability->extentStripe,
							expandSize / pool->capability->extentStripe,
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID10 composite by which we'll expand the existing pool/container
				rc = CreateRAID10Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									pool->capability->dataRedundancy,
									pool->capability->extentStripe,
									pool->capability->userDataStripeDepth,
									&antExtent);
			}
			else if (CapabilityDefaultIsRAID5(pool->capability))
			{
				// Expand a RAID5 pool
				// Choose extents to be used to meet the goal expansion size. 
				raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
				if (raidExtents == NULL)
				{
					_SMI_TRACE(0,("Out of memory!!!"));
					rc = M_FAILED;
					goto exit;
				}

				SelectRAIDExtents(
							availExtents, 
							pool->capability->extentStripe,
							expandSize / (pool->capability->extentStripe-1),
							raidExtents,
							&numAvailExtents);
				
				// Build the new RAID5 composite by which we'll expand the existing pool/container
				rc = CreateRAID5Composition(
									raidExtents, 
									numAvailExtents, 
									NULL, 
									pool->capability->userDataStripeDepth,
									&antExtent);
			}

			_SMI_TRACE(1,("Processing expand input extent %s", antExtent->name));

			vg_devices.push_back(antExtent->name);
		}

		_SMI_TRACE(1,("Call expandLvmVg"));
		if(rc = s->extendLvmVg(currName, vg_devices))
		{
			_SMI_TRACE(0,("Error returned from expandLvmVg!"));
			rc = M_FAILED;
			goto exit;
		}
		vg_devices.clear();
	}
	else
	{
		shrinkSize = currSize - goalSize;

		// See if we can shrink

		// Determine how we are going to shrink our pool by listing shrinkable pool extents.
		// Once we decide which extents to adjust/remove to satisfy the shrink then call
		// evms_shrink to to the evms dirty work.

		// Choose extents to be used to meet the goal shrinkage size
		CMPIUint64 excessShrinkage;
/*	
		rc = PoolGetShrinkExtents(pool, evmsHandle, &shrinkSize, &excessShrinkage, &shrinkHandles);

		if (rc != M_COMPLETED_OK)
		{
			_SMI_TRACE(0,("Pool has no component extents that can be shrunk!"));
			goto exit;
		}
		_SMI_TRACE(1,("Back from getShrinkExtents, shrinkSize = %llu, excess = %llu", shrinkSize, excessShrinkage));
		_SMI_TRACE(1,("  Number of shrink objects = %d", shrinkHandles->count));

		_SMI_TRACE(1,("Call evms_shrink"));
		if (evms_shrink(evmsHandle, shrinkHandles, NULL) != 0)
		{
			_SMI_TRACE(0,("Error returned from evms_shrink!"));
			rc = M_FAILED;
			goto exit;
		}

		pool->totalSize -= shrinkSize + excessShrinkage;
		pool->remainingSize -= shrinkSize + excessShrinkage;

		// Do any additional cleanup required for the extents we just deleted from the pool
		uint i;
		for (i = 0; i < shrinkHandles->count; i++)
		{
			handle_object_info_t *infoHandle;
			if (evms_get_info(shrinkHandles->handle[i], &infoHandle) != 0)
			{
				_SMI_TRACE(0,("Unable to get EVMS info for shrink handle!"));
				continue;
			}

			char *extName = strrchr(infoHandle->info.object.name, '/');
			if (extName != NULL && !omcStrStartsWith(infoHandle->info.object.name, "md/"))
			{
				extName++;
			}
			else
			{
				extName = infoHandle->info.object.name;
			}
			_SMI_TRACE(1,("Shrink extent name = %s", extName));
			
			StorageExtent *ext = ExtentsFind(extName);
			if (ext == NULL)
			{
				_SMI_TRACE(0,("Unable to find extent object for %s", extName));
				evms_free(infoHandle);
				continue;
			}

			if (!ext->composite && ExtentGetPool(ext)->primordial)
			{
				_SMI_TRACE(1,("Call unpartition for %s", ext->name));
				ExtentUnpartition(ext);
			}
			else if (ext->composite)
			{
				_SMI_TRACE(1,("Call DeleteComposition for %s", ext->name));
				DeleteComposition(ext);
			}
			evms_free(infoHandle);
		}

		// Now--because of the way EVMS shrinks a container/pool by removing whole objects
		// we may need to expand the container a bit to account for any excess shrinkage 
		// that may have occurred.
		if (excessShrinkage > MIN_VOLUME_SIZE)
		{
			char inPoolName[256];
			if (inPool != NULL)
			{
				strcpy(inPoolName, inPool->name);
			}

			evms_commit_changes();
			SCSScanStorage(ns);
			_SMI_TRACE(1,("Expanding pool to account for excess shrinkage"));

			// We need to make sure we get current/valid pool and inPool pointers
			// before we call ModifyConcretePool
			pool = PoolsFindByName(currName);
			if (inPool != NULL)
			{
				inPool = PoolsFindByName(inPoolName);
			}

			ModifyConcretePool(pool, ns, currName, NULL, goalSize, inPool);	
		}
*/	}

exit_rename_only:
	// Lastly see if we need to do a rename
	if (name != NULL && strlen(name) > 0 && (strcmp(name, currName) != 0))
	{
		rc = RenameStoragePool(pool, name);

		if (rc != M_COMPLETED_OK)
		{
			_SMI_TRACE(1,("Unable to rename storage pool"));
		}
	}

exit:
	if (availExtents)
	{
		free(availExtents);
	}
	if (raidExtents)
	{
		free(raidExtents);
	}

	_SMI_TRACE(1,("ModifyConcretePool() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc =%d", s_rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static CMPIUint32 CreateVolume(char *name, StorageExtent *dummyExt)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	CMPIBoolean closeEVMS = !EVMSopen;
	object_handle_t evmsHandle;

	_SMI_TRACE(1,("\nCreateVolume() called: name = %s, dummyExt = %s", name, dummyExt->name));

	StorageVolume *vol = VolumesFind(name);
	if (vol != NULL)
	{
		_SMI_TRACE(1,("Volume already exists!!"));
		rc = M_INVALID_PARAM;
		goto exit;
	}

	// Create the new evms volume
	if (!EVMSopen)
	{
		if (evms_open_engine(NULL, ((engine_mode_t)(ENGINE_READ | ENGINE_WRITE)), 
			NULL, WARNING, NULL) != 0)
		{
			_SMI_TRACE(1,("Error opening the evms engine, unable to create evms volume!"));
			rc = M_FAILED;
			goto exit;
		}

		closeEVMS = TRUE;
		_SMI_TRACE(1,("Engine open"));
	}

	char nameBuf[256];
	ExtentGetContainerName(dummyExt, nameBuf, 256);
	strcat(nameBuf, "/");
	strcat(nameBuf, dummyExt->name);
	_SMI_TRACE(1,("Get EVMS object for %s", nameBuf));

	evmsHandle = SCSGetEVMSObject(nameBuf);

	_SMI_TRACE(1,("Call evms_can_create_volume"));
	if (evms_can_create_volume(evmsHandle) == 0)
	{
		evms_create_volume(evmsHandle, name);
	}
	else
	{
		_SMI_TRACE(0,("Can't create evms volume for %s!", name));
		rc = M_FAILED;
		goto exit;
	}

exit:
	if (closeEVMS)
	{
		evms_close_engine();
		_SMI_TRACE(1,("Engine closed"));
	}			

	_SMI_TRACE(1,("CreateVolume() done, rc = %d\n", rc));
	return rc;
}
*/
//	**************** 	Create Volume for Array implementation 		******************
static CMPIUint32 CreateVolume(char *name, StoragePool *pool, CMPIUint64 size)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc;
	unsigned long long sizeM = size / (1024*1024);
	string vol_name;
/* 	if(type == ET_LOGICALDISK)
		vol_name.append("LD_");
	else
		vol_name.append("SV_");
	vol_name.append(name);
	strcpy(name, vol_name.c_str());
*/	
	_SMI_TRACE(1,("\nCreateVolume() called: pool = %s", pool->name));
	
	StorageVolume *vol = VolumesFind(name);
	if (vol != NULL)
	{
		_SMI_TRACE(1,("Volume already exists!!"));
		rc = M_INVALID_PARAM;
		goto exit;
	}
	if(rc = s->createLvmLv((const string)pool->name, name, sizeM, 1, vol_name))
	{
		_SMI_TRACE(0,("Can't create volume %s, rc = %d", vol_name.c_str(), rc));
		rc = M_FAILED;
		goto exit;
	}

exit:
	_SMI_TRACE(1,("CreateVolume() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed rc = %d", s_rc));
	vol_name.clear();
	return rc;
}
/*
static CMPIUint32 CreateVolume(char *name, StoragePool *pool, CMPIUint64 size)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	unsigned long long sizeM = size / (1024*1024);
	string vol_name;

	_SMI_TRACE(1,("\nCreateVolume() called: pool = %s", pool->name));
	
	StorageVolume *vol = VolumesFind(name);
	if (vol != NULL)
	{
		_SMI_TRACE(1,("Volume already exists!!"));
		rc = M_INVALID_PARAM;
		goto exit;
	}
	if(rc = s->createLvmLv((const string)pool->name, name, sizeM, 1, vol_name))
	{
		_SMI_TRACE(0,("Can't create volume %s, rc = %d", name, rc));
		rc = M_FAILED;
		goto exit;
	}

exit:
	_SMI_TRACE(1,("CreateVolume() done, rc = %d\n", rc));
	if(s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed"));
	vol_name.clear();
	return rc;
}
*/
/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 ResizeVolume(
						StorageVolume *vol, 
						const CMPIUint64 currSize,
						const CMPIUint64 goalSize)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	unsigned long long sizeM = goalSize / (1024*1024);
	char device[256];
//	object_handle_t evmsHandle;
//	option_array_t *options = NULL;

	_SMI_TRACE(1,("\nResizeVolume() called, vol = %s, currSize = %llu, goalSize = %llu", vol->name, currSize, goalSize));

	//TODO Should resizeVolumeNoFs() be used for shrinking? Since there is possiblity of data loss

	StoragePool *antPool = (StoragePool *) vol->antecedentPool.element;
	sprintf(device, "%s%s%s%s", "/dev/", antPool->name, "/", vol->name);
	_SMI_TRACE(1,("Call resizeVolume, device name = %s", device));
	if(rc = s->resizeVolumeNoFs(device, sizeM))
	{
		_SMI_TRACE(0,("Can't resize volume, rc = %d", rc));
		rc = M_FAILED;
		goto exit;
	}
	
exit:
	_SMI_TRACE(1,("ResizeVolume() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc =%d", s_rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static CMPIUint32 RenameVolume(StorageVolume *vol, const char *name)
{
	CMPIUint32 rc = M_COMPLETED_OK;
	object_handle_t evmsHandle;

	_SMI_TRACE(1,("\nRenameVolume() called, vol = %s, new name = %s", vol->name, name));

	char nameBuf[256];
	sprintf(nameBuf, "%s%s", "/dev/evms/", vol->name);
	evmsHandle = SCSGetEVMSObject(nameBuf);

	if (evms_can_set_volume_name(evmsHandle) != 0)
	{
		_SMI_TRACE(0,("Element cannot be renamed using evms_set_volume_name!"));
		rc = M_FAILED;
		goto exit;
	}

	if (evms_set_volume_name(evmsHandle, (char *)name) != 0)
	{
		_SMI_TRACE(0,("Error returned from evms_set_volume_name!"));
		rc = M_FAILED;
		goto exit;
	}

exit:
	_SMI_TRACE(1,("RenameVolume() done, rc = %d\n", rc));
	return rc;
}
*/
static CMPIUint32 RenameVolume(StorageVolume *vol, const char *name)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;
	char command[256];
	StoragePool *antPool = (StoragePool *)vol->antecedentPool.element;

	_SMI_TRACE(1,("\nRenameVolume() called, vol = %s, new name = %s", vol->name, name));

	_SMI_TRACE(1,("Trying to build the rename command"));
	sprintf(command, "%s %s %s %s", "lvrename", antPool->name, vol->name, name);
	_SMI_TRACE(1,("command = %s", command));
	if(system(command))
	{
		_SMI_TRACE(1,("Cannot rename Logical Volume %s", vol->name));
		rc = M_FAILED;
	}

exit:
	_SMI_TRACE(1,("RenameVolume() done, rc = %d\n", rc));
	s->rescanEverything();
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
	return rc;
}


/////////////////////////////////////////////////////////////////////////////
//TODO Process Snapshot to be done later


/*static int EVMSProcessSnapshot(
				StorageExtent *childExtent,
				const handle_object_info_t *snapInfo)
{
	int rc = 0;

	_SMI_TRACE(1,("\nEVMSProcessSnapshot() called for object: name = %s, size = %llu", snapInfo->info.object.name, snapInfo->info.object.size));
	_SMI_TRACE(1,("\t start = %llu, volHandle = %u", snapInfo->info.object.start, snapInfo->info.object.volume));
	_SMI_TRACE(1,("\t dataType = %d", snapInfo->info.object.data_type));

	// Handle any EVMS volume to which this snapshot object might belong
	if (snapInfo->info.object.volume != 0)
	{
		handle_object_info_t *volInfo = NULL;

		rc = evms_get_info(snapInfo->info.object.volume, &volInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evms_get_info for EVMS volume"));
			goto exit;
		}

		rc = EVMSProcessVolume(ExtentGetPool(childExtent), childExtent,	volInfo);

		evms_free(volInfo);

		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evmsProcessVolume"));
			goto exit;
		}
	}

exit:
	_SMI_TRACE(1,("EVMSProcessSnapshot() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static int EVMSProcessEVMSObject(
				StorageExtent *childExt,
				const handle_object_info_t *objInfo)
{
	int rc = 0;
	char *pluginName;

	_SMI_TRACE(1,("\nEVMSProcessEVMSObject() called"));

	// Determine what type of evms object we are dealing with
	handle_object_info_t *pluginInfo;
	rc = evms_get_info(objInfo->info.object.plugin, &pluginInfo);
	_SMI_TRACE(1,("Back from evms_get_info, rc = %d", rc));
	if (rc != 0)
	{
		_SMI_TRACE(1,("Error calling evms_get_info (pluginInfo) for object %s", objInfo->info.object.name));
		goto exit;
	}

	pluginName = pluginInfo->info.plugin.short_name;
	_SMI_TRACE(1,("Plugin short_name = %s", pluginName));
	evms_free(pluginInfo);
	if (strcasecmp(pluginName, "Snapshot") == 0)
	{
		_SMI_TRACE(1,("Object is Snapshot"));
		rc = EVMSProcessSnapshot(childExt, objInfo);
	}
	else
	{
		_SMI_TRACE(1,("EVMS Object is UNSUPPORTED"));
	}

exit:
	_SMI_TRACE(1,("\nEVMSProcessEVMSObject() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static int EVMSProcessVolume(
				StoragePool *pool,
				StorageExtent *dummyExtent,
				const handle_object_info_t *volInfo)
{
	int rc = 0;
	char nameBuf[256];
	StorageSetting *ss;

	_SMI_TRACE(1,("\nEVMSProcessVolume() called, pool = %s, dummyExtent = %s", pool->name, dummyExtent->name));
	_SMI_TRACE(1,("\t name = %s, size = %llu", volInfo->info.volume.name, volInfo->info.volume.vol_size));
	_SMI_TRACE(1,("\t maxsize = %llu, fs_size = %llu", volInfo->info.volume.max_vol_size, volInfo->info.volume.fs_size));

	// Create a StorageVolume object that maps to this EVMS volume
	const char *volEVMSName = volInfo->info.volume.name;
	CMPIUint64 volTotalBytes = volInfo->info.volume.vol_size * 512;
	CMPIUint64 volTotalBlocks = volTotalBytes / dummyExtent->blockSize;
	CMPIUint64 volConsumableBlocks = volTotalBlocks;
	if (volInfo->info.volume.vol_size % 8)
	{
		volTotalBlocks++;
	}
	_SMI_TRACE(1,("Vol NumBlocks = %llu, ConsumableBlocks = %llu, Size(bytes) = %llu", volTotalBlocks, volConsumableBlocks, volTotalBytes));

	char *volName = strrchr(volEVMSName, '/') + 1;
	_SMI_TRACE(1,("volName = %s", volName));

	StorageVolume *vol = VolumesFind(volName);
	if (vol != NULL)
	{
		_SMI_TRACE(1,("Already processed %s", volName));
		goto exit;
	}

	vol = VolumeAlloc(volName, volName);
	VolumesAdd(vol);

	// Create various associations between new volume and other objects.

	_SMI_TRACE(1,("Create AllocatedFromStoragePool assn"));
	// Create AllocatedFromStoragePool association
	VolumeCreateAllocFromAssociation(
						pool,
						vol,
						volTotalBlocks * dummyExtent->blockSize);

	_SMI_TRACE(1,("Create BasedOn assn to dummy extent %s", dummyExtent->name));
	// Create BasedOn association to underlying dummy extent
	VolumeCreateBasedOnAssociation(
						vol,
						dummyExtent,
						0,
						volTotalBlocks * dummyExtent->blockSize -1,
						1);

	_SMI_TRACE(1,("Initialize output element properties"));
	// Initialize output element properties based on underyling dummy extent

	vol->blockSize = dummyExtent->blockSize;
	vol->numberOfBlocks = volTotalBlocks;
	vol->consumableBlocks = volConsumableBlocks;

	_SMI_TRACE(1,("Create StorageSetting and StorageElementSettingData assn"));
	// Create storage setting to be associated to element
	sprintf(nameBuf, "%s%s", volName, "Setting");
	ss = SettingAlloc(nameBuf);
	SettingsAdd(ss);

	// Create association to volume
	vol->setting = ss;
	ss->volume = vol;

	// Initialize SettingData
	ss->capability = pool->capability;
	SettingInitFromCapability(ss, SCST_DEFAULT);

exit:
	_SMI_TRACE(1,("EVMSProcessVolume() done, rc = %d\n", rc));
	return rc;
}
*/
static int ProcessVolume(
				StoragePool *pool,
				StorageExtent *compExtent,
				const LvmLvInfo lvInfo)
{
	int rc = 0;
	char nameBuf[256];
	StorageSetting *ss;

	_SMI_TRACE(1,("\nProcessVolume() called, pool = %s, compExtent = %s", pool->name, compExtent->name));
	_SMI_TRACE(1,("\t name = %s, size = %llu", lvInfo.v.name.c_str(), lvInfo.v.sizeK * 1024));

	// Create a StorageVolume object that maps to this EVMS volume
	const char *volName = lvInfo.v.name.c_str();
	CMPIUint64 volTotalBytes = lvInfo.v.sizeK * 1024;

	StorageVolume *vol = VolumesFind(volName);
	if (vol != NULL)
	{
		_SMI_TRACE(1,("Already processed %s", volName));
		goto exit;
	}

	vol = VolumeAlloc(volName, volName);
	VolumesAdd(vol);

	// Create various associations between new volume and other objects.

	_SMI_TRACE(1,("Create AllocatedFromStoragePool assn"));
	// Create AllocatedFromStoragePool association
	VolumeCreateAllocFromAssociation(
						pool,
						vol,
						volTotalBytes);

	_SMI_TRACE(1,("Create BasedOn assn to comp extent %s", compExtent->name));
	// Create BasedOn association to underlying dummy extent
	VolumeCreateBasedOnAssociation(
						vol,
						compExtent,
						0,
						volTotalBytes -1,
						1);

	_SMI_TRACE(1,("Initialize output element properties"));
	// Initialize output element properties based on underyling dummy extent

	vol->size = volTotalBytes;
	if(cmpiutilStrStartsWith(volName, "SV_"))
		vol->IsLD = false;
	else if(cmpiutilStrStartsWith(volName, "LD_"))
		vol->IsLD = true;


	_SMI_TRACE(1,("Create StorageSetting and StorageElementSettingData assn"));
	// Create storage setting to be associated to element
	sprintf(nameBuf, "%s%s", volName, "Setting");
	ss = SettingAlloc(nameBuf);
	SettingsAdd(ss);

	// Create association to volume
	vol->setting = ss;
	ss->volume = vol;

	// Initialize SettingData
	ss->capability = pool->capability;
	SettingInitFromCapability(ss, SCST_DEFAULT);

exit:
	_SMI_TRACE(1,("ProcessVolume() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static int EVMSProcessSegment(
				StorageExtent *childExt,
				const handle_object_info_t *segInfo)
{
	int rc = 0;
	CMPIUint64 segSize = 0;
	char nameBuf[256];

	_SMI_TRACE(1,("\nEVMSProcessSegment() called for segment %s", segInfo->info.segment.name));

	// If we have devices with parent segments, these must be for devices that have
	// been formatted/configured outside of our control since we don't currently use segment
	// managers in our  provider. In this case we will allow our-style pools/volumes
	// to be created from any available freespace segments. Currently we don't model
	// any of the other data segments and/or consuming evms or compatibility volumes.

	// If we haven't done so already, we need to create the remaining/freespace
	// extent that corresponds to the sum of all this device's freespace segments

	sprintf(nameBuf, "%s%s", childExt->name, "Freespace");
	StorageExtent *remaining = ExtentsFind(nameBuf);
	if (remaining == NULL)
	{
		_SMI_TRACE(1,("No remaining extent created yet for segmented disk, do 1st time init"));
		remaining = ExtentAlloc(nameBuf, nameBuf);
		ExtentsAdd(remaining);

		remaining->blockSize = childExt->blockSize;
		remaining->consumableBlocks = childExt->consumableBlocks;
		remaining->numberOfBlocks = childExt->numberOfBlocks;
		remaining->pool = childExt->pool;
		PoolExtentsAdd(childExt->pool, remaining);

		ExtentCreateBasedOnAssociation(
							remaining,
							childExt,
							0,
							childExt->numberOfBlocks * childExt->blockSize - 1,
							1);
	}

		segSize = segInfo->info.segment.size * 512; 

	// Determine what type of segment (metadata, data, freespace) we are dealing with
	if (segInfo->info.segment.data_type == META_DATA_TYPE)
	{
		_SMI_TRACE(1,("Segment is META_DATA type, size(bytes) = %llu", segSize));
		remaining->consumableBlocks -= (segSize / remaining->blockSize);
		remaining->numberOfBlocks -= (segSize / remaining->blockSize);
		if (segSize % remaining->blockSize)
		{
			remaining->consumableBlocks--;
			remaining->numberOfBlocks--;
		}

		BasedOn *antBO = remaining->antecedents[0];
		antBO->startingAddress += segSize;
		childExt->pool->remainingSize -= segSize;
	}
	else if (segInfo->info.segment.data_type == DATA_TYPE)
	{
		_SMI_TRACE(1,("Segment is DATA type, size(bytes) = %llu", segSize));

		BasedOn *antBO = remaining->antecedents[0];

		// See if this data segment is one that we support in our model. Currently
		// we don't support segments that have immediate consuming volumes or that
		// have no parents.
		uint numParents = segInfo->info.segment.parent_objects->count;
		if (segInfo->info.segment.volume == 0 && numParents > 0)
		{
			_SMI_TRACE(1,("Segment is supported, create dummy extent to model"));

			const char *dummyName = segInfo->info.segment.name;
			CMPIUint64 segSizeBlocks = segSize / remaining->blockSize;
			_SMI_TRACE(1,("dummyName = %s", dummyName));

			StorageExtent *dummyExtent = ExtentsFind(dummyName);
			if (dummyExtent != NULL)
			{
				_SMI_TRACE(1,("Already processed %s, DONE", dummyName));
				goto exit;
			}
				
			// We haven't processed this one yet, allocate/init dummy extent
			dummyExtent	= ExtentAlloc(dummyName, dummyName);
			ExtentsAdd(dummyExtent);

			_SMI_TRACE(1,("Phys Cop = %s, FreeCop = %s", childExt->name, remaining->name));
			_SMI_TRACE(1,("DummyCop = %s", dummyExtent->name));

			dummyExtent->blockSize = childExt->blockSize;
			dummyExtent->numberOfBlocks = dummyExtent->consumableBlocks = segSizeBlocks;

			// Create BasedOn assn to lower level component extent
			ExtentCreateBasedOnAssociation(
								dummyExtent,
								childExt,
								antBO->startingAddress,
								antBO->startingAddress + segSize - 1,
								1);


			// See if this region/dummy-extent is consumed by any container/pool
			if (segInfo->info.segment.consuming_container != 0)
			{
				handle_object_info_t *contInfo = NULL;

				rc = evms_get_info(segInfo->info.segment.consuming_container, &contInfo);
				if (rc != 0)
				{
					_SMI_TRACE(1,("Error calling evms_get_info for segment/dummy extent"));
					goto exit;
				}

				rc = EVMSProcessContainer(dummyExtent, childExt->pool->capability, contInfo);
				evms_free(contInfo);
				if (rc != 0)
				{
					_SMI_TRACE(1,("Error calling evmsProcessContainer for segment/dummy extent"));
					goto exit;
				}
			}
			// Handle any parent regions
			else if (numParents > 0)
			{
				// Handle parent objects
				_SMI_TRACE(1,("Segment/Extent has %d parents", numParents));

				uint k;
				for (k = 0; k < numParents; k++)
				{
					handle_object_info_t *parentInfo = NULL;
					rc = evms_get_info(segInfo->info.segment.parent_objects->handle[k], &parentInfo);
					if (rc != 0)
					{
						_SMI_TRACE(1,("Error calling evms_get_info for dummy extent parent object"));
						continue;
					}
					if (parentInfo->type == REGION)
					{
						_SMI_TRACE(1,("Extent parent is a REGION, name = %s", parentInfo->info.region.name));
						rc = EVMSProcessRegion(dummyExtent, parentInfo);
					}
					else
					{
						rc = 0;
					}
					evms_free(parentInfo);
					if (rc != 0)
					{
						_SMI_TRACE(1,("Error calling evmsProcess<parent> for dummy extent parent object"));
						continue;
					}
				}
			}
		}
		else
		{
			_SMI_TRACE(1,("Segment is UNSUPPORTED (has a volume or has no parents)"));
			childExt->pool->remainingSize -= segSize;
		}

		remaining->consumableBlocks -= (segSize / remaining->blockSize);
		remaining->numberOfBlocks -= (segSize / remaining->blockSize);
		if (segSize % remaining->blockSize)
		{
			remaining->consumableBlocks--;
			remaining->numberOfBlocks--;
		}

		antBO->startingAddress += segSize;
	}

	_SMI_TRACE(1,("Freespace (bytes) = %llu", remaining->numberOfBlocks * remaining->blockSize));
	_SMI_TRACE(1,("Pool remaining (bytes) = %llu", childExt->pool->remainingSize));

exit:
	_SMI_TRACE(1,("EVMSProcessSegment() done, rc = %d\n", rc));
	return rc;
}
*/
static int ProcessPartition(
				StorageExtent *childExt,
				const PartitionInfo partInfo)
{
	int rc = 0;
	CMPIUint64 partSize = 0;
	char nameBuf[256];
	VolumeInfo volInfo;

	_SMI_TRACE(1,("\nProcessPartition() called for partition %s", partInfo.v.name.c_str()));


	// If we have devices with parent segments, these must be for devices that have
	// been formatted/configured outside of our control since we don't currently use segment
	// managers in our  provider. In this case we will allow our-style pools/volumes
	// to be created from any available freespace segments. Currently we don't model
	// any of the other data segments and/or consuming evms or compatibility volumes.

	// If we haven't done so already, we need to create the remaining/freespace
	// extent that corresponds to the sum of all this device's freespace segments

	sprintf(nameBuf, "%s%s", childExt->name, "Freespace");
	StorageExtent *remaining = ExtentsFind(nameBuf);
	if (remaining == NULL)
	{
		_SMI_TRACE(1,("No remaining extent created yet for segmented disk, do 1st time init"));
		remaining = ExtentAlloc(nameBuf, nameBuf);
		ExtentsAdd(remaining);

		remaining->size = childExt->size;
		remaining->pool = childExt->pool;
		PoolExtentsAdd(childExt->pool, remaining);

		ExtentCreateBasedOnAssociation(
							remaining,
							childExt,
							0,
							childExt->size - 1,
							1);
	}

	partSize = partInfo.v.sizeK * 1024; 

	BasedOn *antBO = remaining->antecedents[0];

	const char *dummyName = partInfo.v.name.c_str();
	_SMI_TRACE(1,("dummyName = %s", dummyName));

	_SMI_TRACE(1,("partition Used by type = %d: name = %s", partInfo.v.usedByType, partInfo.v.usedByName.c_str()));
//	if(partInfo.v.usedBy != UB_NONE)
	if(partInfo.v.usedByType == UB_LVM || partInfo.v.usedByType == UB_MD)
	{
		StorageExtent *dummyExtent = ExtentsFind(dummyName);
		if (dummyExtent != NULL)
		{
			_SMI_TRACE(1,("Already processed %s, DONE", dummyName));
			goto exit;
		}
		
		// We haven't processed this one yet, allocate/init dummy extent
		dummyExtent	= ExtentAlloc(dummyName, dummyName);
		ExtentsAdd(dummyExtent);
	
		_SMI_TRACE(1,("Phys Cop = %s, FreeCop = %s", childExt->name, remaining->name));
		_SMI_TRACE(1,("DummyCop = %s", dummyExtent->name));
	
		dummyExtent->size = partSize;

		// Create BasedOn assn to lower level component extent
		ExtentCreateBasedOnAssociation(
							dummyExtent,
							childExt,
							antBO->startingAddress,
							antBO->startingAddress + partSize - 1,
							1);


		_SMI_TRACE(1,("partition Used by type = %d: name = %s", partInfo.v.usedByType, partInfo.v.usedByName.c_str()));
		// See if this region/dummy-extent is consumed by any container/pool
		if (partInfo.v.usedByType == UB_LVM)
		{
			ContainerInfo vg_contInfo;
			LvmVgInfo vgInfo;
			_SMI_TRACE(1,("Calling getLvmVgInfo for partition/dummy extent consuming container"));
			if(rc = s->getContLvmVgInfo(partInfo.v.usedByName, vg_contInfo, vgInfo))
			{	
				_SMI_TRACE(1,("Error calling getLvmVgInfo for partition/dummy extent consuming container, rc = %d", rc));
				goto exit;
			}
			rc = ProcessContainer(dummyExtent, childExt->pool->capability, vg_contInfo, vgInfo);

			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling ProcessContainer for partition/dummy extent consuming container, rc = %d", rc));
				goto exit;
			}
		}
		// Handle any parent regions
		else
		{
			deque<MdInfo> mdInfoList;
			deque<MdInfo>::iterator i;
			MdInfo mdInfo;
			_SMI_TRACE(1,("Getting MdInfo"));
			if(rc = s->getMdInfo(mdInfoList))
			{
				_SMI_TRACE(1,("Error calling getMdInfo, rc =%d", rc));
				goto exit;
			}
   			for(i = mdInfoList.begin(); i != mdInfoList.end(); ++i )
			{
				_SMI_TRACE(1,("Comparing MD %s with %s", partInfo.v.usedByName.c_str(), i->v.name.c_str()));
				if(!strcasecmp(partInfo.v.usedByName.c_str(), i->v.name.c_str()))
				{
					mdInfo = *i;
					break;
				}
			}
			_SMI_TRACE(1,("Calling ProcessRegion"));
			if(i != mdInfoList.end())
			{
				if(rc = ProcessRegion(dummyExtent, mdInfo))
				{
					_SMI_TRACE(1,("Error calling ProcessRegion, rc =%d", rc));
					goto exit;
				}
			}
			else
			{
				_SMI_TRACE(1,("Unable to get MdInfo for %s, rc = %d", volInfo.usedByName.c_str(), rc));
				goto exit;
			}
			mdInfoList.clear();
		}
	}
	else
	{
		_SMI_TRACE(1,("Partition is not processed, rc = %d", rc));
		childExt->pool->remainingSize -= partSize;
	}

	remaining->size -= partSize ;

	antBO->startingAddress += partSize;

	_SMI_TRACE(1,("Freespace (bytes) = %llu", remaining->size));
	_SMI_TRACE(1,("Pool remaining (bytes) = %llu", childExt->pool->remainingSize));

exit:
	_SMI_TRACE(1,("ProcessPartition() done, rc = %d\n", rc));
	return rc;
}


/////////////////////////////////////////////////////////////////////////////
/*static int EVMSProcessRegion(
				StorageExtent *childExt,
				const handle_object_info_t *regionInfo)
{
	int rc = 0;

	_SMI_TRACE(1,("\nEVMSProcessRegion() called for region %s", regionInfo->info.region.name));

	// Determine what type of region we are dealing with
	handle_object_info_t *pluginInfo;
	rc = evms_get_info(regionInfo->info.region.plugin, &pluginInfo);
	_SMI_TRACE(1,("Back from evms_get_info, rc = %u", rc));
	if (rc != 0)
	{
		_SMI_TRACE(1,("Error calling evms_get_info (pluginInfo) for region %s", regionInfo->info.region.name));
		goto exit;
	}

	const char *pluginName = pluginInfo->info.plugin.short_name;
	_SMI_TRACE(1,("Plugin short_name = %s", pluginName));
	evms_free(pluginInfo);
	if (strcasecmp(pluginName, "MDRaid0RegMgr") == 0)
	{
		_SMI_TRACE(1,("Region is MDRaid0"));
		rc = EVMSProcessMDRaid0Region(childExt, regionInfo);
	}
	else if (strcasecmp(pluginName, "MDRaid1RegMgr") == 0)
	{
		_SMI_TRACE(1,("Region is MDRaid1"));
		rc = EVMSProcessMDRaid1Region(childExt, regionInfo);
	}
	else if (strcasecmp(pluginName, "MDRaid5RegMgr") == 0)
	{
		_SMI_TRACE(1,("Region is MDRaid5"));
		rc = EVMSProcessMDRaid5Region(childExt, regionInfo);
	}
	else
	{
		_SMI_TRACE(1,("Region is UNSUPPORTED"));
	}

exit:
	_SMI_TRACE(1,("EVMSProcessRegion() done, rc = %d\n", rc));
	return rc;
}
*/
static int ProcessRegion(
				StorageExtent *childExt,
				const MdInfo mdInfo)
{
	int rc = 0;
	unsigned md_type;

	_SMI_TRACE(1,("\nProcessRegion() called for region %s", mdInfo.v.name.c_str()));

	// Determine what type of region we are dealing with
	
	_SMI_TRACE(1,("Region type RAID%d", mdInfo.type));
	md_type = mdInfo.type;
	if (md_type == 1)
	{
		_SMI_TRACE(1,("Region is MDRaid0"));
		rc = ProcessMDRaid0Region(childExt, mdInfo);
	}
	else if (md_type == 2)
	{
		_SMI_TRACE(1,("Region is MDRaid1"));
		rc = ProcessMDRaid1Region(childExt, mdInfo);
	}
	else if (md_type == 3)
	{
		_SMI_TRACE(1,("Region is MDRaid5"));
		rc = ProcessMDRaid5Region(childExt, mdInfo);
	}
	else
	{
		_SMI_TRACE(1,("Region is UNSUPPORTED"));
	}

exit:
	_SMI_TRACE(1,("EVMSProcessRegion() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static int EVMSProcessMDRaid0Region(
				StorageExtent *childExt,
				const handle_object_info_t *regInfo)
{
	int rc = 0;
	const char *state;

	_SMI_TRACE(1,("\nEVMSProcessMDRaid0Region() called for region %s, size = %lld", regInfo->info.region.name, regInfo->info.region.size));
	_SMI_TRACE(1,("\t start = %lld, volHandle = %x", regInfo->info.region.start, regInfo->info.region.volume));
	_SMI_TRACE(1,("\t dataType = %d", regInfo->info.region.data_type));

	// If we haven't already, create a composite extent that maps to this Raid0 region 
	const char *regionName = regInfo->info.region.name;
	CMPIUint64 regionSizeBlocks = (regInfo->info.region.size * 512) / childExt->blockSize;

	StorageExtent *compExtent = ExtentsFind(regionName);
	if (compExtent == NULL)	
	{
		_SMI_TRACE(1,("No composite extent exists that maps to Raid0 region, create new"));
		compExtent = ExtentAlloc(regionName, regionName);
		ExtentsAdd(compExtent);
		compExtent->composite = 1;

		// Get extended info for RAID0 region to tell us all the specifics for this
		// region. First get general state information
		extended_info_array_t *extInfo = NULL;
		rc = evms_get_extended_info(regInfo->info.region.handle, NULL, &extInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evms_get_extended_info for MDRaid0 region"));
			goto exit;
		}

		CMPIBoolean isCorrupted = 0;
		CMPIBoolean isDegraded = 0;
		u_int32_t i;
		for (i = 0; i < extInfo->count; i++)
		{
			// Get state of this raid
			if (strcmp(extInfo->info[i].name, "state") == 0)
			{
				state = extInfo->info[i].value.s;
				_SMI_TRACE(1,("RAID0 State = %s", state));

				if (strstr(state, "Corrupt") != NULL)
				{
					isCorrupted = TRUE;
				}
				if (strstr(state, "Degraded") != NULL)
				{
					isDegraded = TRUE;
				}
			}
		}
		evms_free(extInfo);

		// Now get superblock information
		extInfo = NULL;
		rc = evms_get_extended_info(regInfo->info.region.handle, "superblock", &extInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evms_get_extended_info (superblock) for MDRaid0 region"));
			goto exit;
		}

		CMPIUint64 stripeChunkSize;
		CMPIUint16 numStripes;
		for (i = 0; i < extInfo->count; i++)
		{
			// Get region chunk_size
			if (strcmp(extInfo->info[i].name, "chunk_size") == 0)
			{
				stripeChunkSize = extInfo->info[i].value.ui32;
				_SMI_TRACE(1,("Chunk size = %lld", stripeChunkSize));
			}

			// Get number of disks/stripes in this raid0 set
			if (strcmp(extInfo->info[i].name, "raid_disks") == 0)
			{
				numStripes = (CMPIUint16)(extInfo->info[i].value.ui32);
				_SMI_TRACE(1,("Num stripes = %d", numStripes));
			}
		}
		evms_free(extInfo);
		
		compExtent->blockSize = childExt->blockSize;
		compExtent->numberOfBlocks = numStripes * childExt->numberOfBlocks;
		compExtent->consumableBlocks = regionSizeBlocks;

		if (compExtent->consumableBlocks > compExtent->numberOfBlocks)
		{
			compExtent->numberOfBlocks = compExtent->consumableBlocks;
		}

		if (isCorrupted)
		{
			_SMI_TRACE(1,("RAID0 composite is in a corrupted state!!!"));
			compExtent->ostatus = OSTAT_ERROR;
			compExtent->estatus = ESTAT_DATA_LOST;
		}
		else if (isDegraded)
		{
			_SMI_TRACE(1,("RAID0 composite is in a degraded state!!!"));
			compExtent->ostatus = OSTAT_DEGRADED;
			compExtent->estatus = ESTAT_BROKEN;
		}

		// Create BasedOn assn to lower level component extent
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->numberOfBlocks * childExt->blockSize - 1,
							1);

		// See if this region/composite-extent is consumed by any container/pool
		if (regInfo->info.region.consuming_container != 0)
		{
			handle_object_info_t *contInfo = NULL;
			rc = evms_get_info(regInfo->info.region.consuming_container, &contInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling evms_get_info for region/dummy extent"));
				goto exit;
			}

			// Flag minimum required capabilites to denote RAID 0 so that processContainer
			// knows what to do.
			StorageCapability cap;
			
			// Check for case where we are actually a RAID10
			if (childExt->composite)
			{
				// We are a RAID10
				cap.dataRedundancy = cap.dataRedundancyMax = 2;
				cap.packageRedundancy = cap.packageRedundancyMax = 1;
				cap.extentStripe = numStripes;
				cap.parity = 0;
				cap.userDataStripeDepth = stripeChunkSize;
				cap.noSinglePointOfFailure = 1;
			}
			else
			{
				// We are a normal RAID0
				cap.dataRedundancy = cap.dataRedundancyMax = 1;
				cap.packageRedundancy = cap.packageRedundancyMax = 0;
				cap.extentStripe = numStripes;
				cap.parity = 0;
				cap.userDataStripeDepth = stripeChunkSize;
				cap.noSinglePointOfFailure = 0;
			}
			cap.name="temp";

			rc = EVMSProcessContainer(compExtent, &cap, contInfo);
			evms_free(contInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling evmsProcessContainer for region/composite Raid0 extent"));
				goto exit;
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("Composite extent already exists for this Raid0 region"));

		// Create BasedOn assn to lower level component extent
		CMPIUint16 orderIdx = compExtent->numAntecedents + 1;
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->numberOfBlocks * childExt->blockSize - 1,
							orderIdx);

		if (childExt->composite)
		{
			if (compExtent->pool == NULL)
			{
				// Check for case where we are an intermediate composite extent and our parent is NOT 
				// another mdraid type of composite. This is the case when we have multiple RAID10 
				// compositions all under a single composite as a result of RAID10 pool expansion.
				if (compExtent->numDependents == 1)
				{
					BasedOn *depBO = compExtent->dependents[0];
					StorageExtent* depExtent = depBO->extent;

					if (StrEndsWith(depExtent->name, "Composite"))
					{
						_SMI_TRACE(1,("RAID10 composite concatenation case"));

						ExtentGetPool(childExt)->remainingSize -= (compExtent->numberOfBlocks * compExtent->blockSize);

						PoolModifyAllocFromAssociation(
												ExtentGetPool(childExt),
												ExtentGetPool(depExtent),
												compExtent->numberOfBlocks * compExtent->blockSize,
												0);
					}
				}
			}
		}
	}

exit:
	_SMI_TRACE(1,("EVMSProcessMDRaid0Region() done, rc = %d\n", rc));
	return rc;
}
*/
static int ProcessMDRaid0Region(
				StorageExtent *childExt,
				const MdInfo mdInfo)
{
	int rc = 0;
	const char *state;

	_SMI_TRACE(1,("\nProcessMDRaid0Region() called for region %s, size = %lld", mdInfo.v.name.c_str(), mdInfo.v.sizeK * 1024));


	// If we haven't already, create a composite extent that maps to this Raid0 region 
	const char *regionName = mdInfo.v.name.c_str();
	CMPIUint64 regionSize = mdInfo.v.sizeK * 1024;

	StorageExtent *compExtent = ExtentsFind(regionName);
	if (compExtent == NULL)	
	{
		_SMI_TRACE(1,("No composite extent exists that maps to Raid0 region, create new"));
		compExtent = ExtentAlloc(regionName, regionName);
		ExtentsAdd(compExtent);
		compExtent->composite = 1;

		CMPIBoolean isCorrupted = 0;
		CMPIBoolean isDegraded = 0;

		// Get extended info for RAID0 region to tell us all the specifics for this
		// region. First get general state information
//TODO: MD State verification
/*	
		MdStateInfo md_stInfo;
		if(rc = s->getMdStateInfo(regionName, md_stInfo))
		{
			_SMI_TRACE(1,("Unable to get the state of the MD RAID device, rc = %d", rc));
			goto exit();
		}
		if(md_stInfo.degraded)
		{
			isDegraded = true;
		}
		if(!md_stInfo.active)
		{
			isCorrupted = true;
		}
*/
		CMPIUint64 stripeChunkSize;
		CMPIUint16 numStripes = 0;
		stripeChunkSize = mdInfo.chunk * 1024;
		char *token = strtok((char *)mdInfo.devices.c_str(), " ");
		while(token != NULL)
		{
			_SMI_TRACE(1,("token = %s", token));
			numStripes++;
			token = strtok( NULL , " ");
		}
		_SMI_TRACE(1,("Chunk size = %lld", stripeChunkSize));
		_SMI_TRACE(1,("Num stripes = %d", numStripes));
		compExtent->size = numStripes * childExt->size;

		if (regionSize > compExtent->size)
		{
			compExtent->size = regionSize;
		}

		if (isCorrupted)
		{
			_SMI_TRACE(1,("RAID0 composite is in a corrupted state!!!"));
			compExtent->ostatus = OSTAT_ERROR;
			compExtent->estatus = ESTAT_DATA_LOST;
		}
		else if (isDegraded)
		{
			_SMI_TRACE(1,("RAID0 composite is in a degraded state!!!"));
			compExtent->ostatus = OSTAT_DEGRADED;
			compExtent->estatus = ESTAT_BROKEN;
		}

		// Create BasedOn assn to lower level component extent
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->size - 1,
							1);

		// See if this region/composite-extent is consumed by any container/pool
		_SMI_TRACE(1,("RAID Used by = %d: name = %s", mdInfo.v.usedByType, mdInfo.v.usedByName.c_str()));
		if (mdInfo.v.usedByType == (UsedByType)UB_LVM)
		{
			ContainerInfo contInfo;
			LvmVgInfo vgInfo;
			if (rc = s->getContLvmVgInfo(mdInfo.v.usedByName, contInfo, vgInfo))
			{
				_SMI_TRACE(1,("Error calling getLvmVgInfo for region/dummy extent"));
				goto exit;
			}

			// Flag minimum required capabilites to denote RAID 0 so that processContainer
			// knows what to do.
			StorageCapability cap;
			
			// Check for case where we are actually a RAID10
			if (childExt->composite)
			{
				// We are a RAID10
				cap.dataRedundancy = cap.dataRedundancyMax = 2;
				cap.packageRedundancy = cap.packageRedundancyMax = 1;
				cap.extentStripe = numStripes;
				cap.parity = 0;
				cap.userDataStripeDepth = stripeChunkSize;
				cap.noSinglePointOfFailure = 1;
			}
			else
			{
				// We are a normal RAID0
				cap.dataRedundancy = cap.dataRedundancyMax = 1;
				cap.packageRedundancy = cap.packageRedundancyMax = 0;
				cap.extentStripe = numStripes;
				cap.parity = 0;
				cap.userDataStripeDepth = stripeChunkSize;
				cap.noSinglePointOfFailure = 0;
			}
			cap.name="temp";

			rc = ProcessContainer(compExtent, &cap, contInfo, vgInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling ProcessContainer for region/composite Raid0 extent"));
				goto exit;
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("Composite extent already exists for this Raid0 region"));

		// Create BasedOn assn to lower level component extent
		CMPIUint16 orderIdx = compExtent->numAntecedents + 1;
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->size - 1,
							orderIdx);

		if (childExt->composite)
		{
			if (compExtent->pool == NULL)
			{
				// Check for case where we are an intermediate composite extent and our parent is NOT 
				// another mdraid type of composite. This is the case when we have multiple RAID10 
				// compositions all under a single composite as a result of RAID10 pool expansion.
				if (compExtent->numDependents == 1)
				{
					BasedOn *depBO = compExtent->dependents[0];
					StorageExtent* depExtent = depBO->extent;

					if (cmpiutilStrEndsWith(depExtent->name, "Composite"))
					{
						_SMI_TRACE(1,("RAID10 composite concatenation case"));

						ExtentGetPool(childExt)->remainingSize -= compExtent->size;

						PoolModifyAllocFromAssociation(
											ExtentGetPool(childExt),
											ExtentGetPool(depExtent),
											compExtent->size,
											0);
					}
				}
			}
		}
	}

exit:
	_SMI_TRACE(1,("ProcessMDRaid0Region() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static int EVMSProcessMDRaid1Region(
				StorageExtent *childExt,
				const handle_object_info_t *regInfo)
{
	int rc = 0;

	_SMI_TRACE(1,("\nEVMSProcessMDRaid1Region() called for region %s, size = %lld", regInfo->info.region.name, regInfo->info.region.size));
	_SMI_TRACE(1,("\t start = %lld, volHandle = %x", regInfo->info.region.start, regInfo->info.region.volume));
	_SMI_TRACE(1,("\t dataType = %d", regInfo->info.region.data_type));

	// If we haven't already, create a composite extent that maps to this Raid1 region 
	const char *regionName = regInfo->info.region.name;
	CMPIUint64 regionSizeBlocks = (regInfo->info.region.size * 512) / childExt->blockSize;

	StorageExtent *compExtent = ExtentsFind(regionName);
	if (compExtent == NULL)	
	{
		_SMI_TRACE(1,("No composite extent exists that maps to Raid1 region, create new"));
		compExtent = ExtentAlloc(regionName, regionName);
		ExtentsAdd(compExtent);
		compExtent->composite = 1;
	
		// Get extended info for RAID1 region to tell us the state
		extended_info_array_t *extInfo = NULL;
		rc = evms_get_extended_info(regInfo->info.region.handle, NULL, &extInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evms_get_extended_info for MDRaid1 region"));
			goto exit;
		}

		CMPIBoolean isCorrupted = 0;
		CMPIBoolean isDegraded = 0;
		u_int32_t i;
		for (i = 0; i < extInfo->count; i++)
		{
			// Get state of this raid
			if (strcmp(extInfo->info[i].name, "state") == 0)
			{
				const char *state = extInfo->info[i].value.s;
				_SMI_TRACE(1,("RAID1 State = %s", state));

				if (strstr(state, "Corrupt") != NULL)
				{
					isCorrupted = TRUE;
				}
				if (strstr(state, "Degraded") != NULL)
				{
					isDegraded = TRUE;
				}
			}
		}
		evms_free(extInfo);

		compExtent->blockSize = childExt->blockSize;
		compExtent->numberOfBlocks = childExt->numberOfBlocks;
		compExtent->consumableBlocks = regionSizeBlocks;

		if (isCorrupted)
		{
			_SMI_TRACE(1,("RAID1 composite is in a corrupted state!!!"));
			compExtent->ostatus = OSTAT_ERROR;
			compExtent->estatus = ESTAT_DATA_LOST;
		}
		else if (isDegraded)
		{
			_SMI_TRACE(1,("RAID1 composite is in a degraded state!!!"));
			compExtent->ostatus = OSTAT_DEGRADED;
			compExtent->estatus = ESTAT_BROKEN;
		}

		// Create BasedOn assn to lower level component extent
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->numberOfBlocks * childExt->blockSize - 1,
							1);

		// See if this region/composite-extent is consumed by any container/pool
		if (regInfo->info.region.consuming_container != 0)
		{
			handle_object_info_t *contInfo = NULL;
			rc = evms_get_info(regInfo->info.region.consuming_container, &contInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling evms_get_info for region/dummy extent"));
				goto exit;
			}
			// Flag minimum required capabilites to denote RAID 1 so that processContainer
			// knows what to do.
			StorageCapability cap;
			cap.dataRedundancy = cap.dataRedundancyMax = 2;
			cap.packageRedundancy = cap.packageRedundancyMax = 1;
			cap.extentStripe = 1;
			cap.parity = 0;
			cap.noSinglePointOfFailure = 1;

			rc = EVMSProcessContainer(compExtent, &cap, contInfo);

			evms_free(contInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling EVMSProcessContainer for region/composite Raid1 extent"));
				goto exit;
			}
		}
		// Else see if this region/composite-extent is consumed by another region
		else if (regInfo->info.region.parent_objects->count > 0)
		{
			// Handle parent objects
			uint numParents = regInfo->info.region.parent_objects->count;
			_SMI_TRACE(1,("Extent has %u parents", numParents));

			uint k;
			for (k = 0; k < numParents; k++)
			{
				handle_object_info_t *parentInfo = NULL;
				rc = evms_get_info(regInfo->info.region.parent_objects->handle[k], &parentInfo);
				if (rc != 0)
				{
					_SMI_TRACE(1,("Error calling evms_get_info for raid1 extent parent object"));
					continue;
				}
				if (parentInfo->type == REGION)
				{
					_SMI_TRACE(1,("RAID1 Extent parent is a REGION, name = %s", parentInfo->info.region.name));
					rc = EVMSProcessRegion(compExtent, parentInfo);
				}
				else
				{
					rc = 0;
				}
				evms_free(parentInfo);
				if (rc != 0)
				{
					_SMI_TRACE(1,("Error calling EVMSProcess<parent> for raid1 extent parent object"));
					continue;
				}
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("Composite extent already exists for this Raid1 region"));

		// Create BasedOn assn to lower level component extent
		CMPIUint16 orderIdx = compExtent->numAntecedents + 1;
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->numberOfBlocks * childExt->blockSize - 1,
							orderIdx);

		if (compExtent->pool != NULL)
		{
			// Adjust AllocFrom properties between Raid1 pool and underlying/src pool
			ExtentGetPool(childExt)->remainingSize -= (childExt->numberOfBlocks * childExt->blockSize);

			PoolModifyAllocFromAssociation(
									ExtentGetPool(childExt),
									ExtentGetPool(compExtent),
									childExt->numberOfBlocks * childExt->blockSize,
									0);
		}
		else
		{
			// Check for case where we are an intermediate composite extent (i.e. RAID10 or
			// RAID 1 concatenation as result of pool expansion.
			if (compExtent->numDependents == 1)
			{
				BasedOn *depBO = compExtent->dependents[0];
				StorageExtent* depExtent = depBO->extent;

				// Adjust AllocFrom properties between Raid1 pool and underlying/src pool
				ExtentGetPool(childExt)->remainingSize -= (childExt->numberOfBlocks * childExt->blockSize);

				PoolModifyAllocFromAssociation(
										ExtentGetPool(childExt),
										ExtentGetPool(depExtent),
										childExt->numberOfBlocks * childExt->blockSize,
										0);
			}
		}

		// Update Capabilites of Raid1 pool if we have more than 1 mirror
		if (compExtent->numAntecedents > 2)
		{
			if (compExtent->pool != NULL)
			{
				compExtent->pool->capability->dataRedundancy = compExtent->numAntecedents;
				compExtent->pool->capability->dataRedundancyMax = compExtent->numAntecedents;
			}
			else
			{
				// We are part of a RAID 10, update parents capability
				BasedOn *parentBO = compExtent->dependents[0];
				StoragePool *parentPool = parentBO->extent->pool;
				parentPool->capability->dataRedundancy = compExtent->numAntecedents;
				parentPool->capability->dataRedundancyMax = compExtent->numAntecedents;
			}
		}
	}

exit:
	_SMI_TRACE(1,("EVMSProcessMDRaid1Region() done, rc = %d\n", rc));
	return rc;
}
*/
static int ProcessMDRaid1Region(
				StorageExtent *childExt,
				const MdInfo mdInfo)
{
	int rc = 0;

	_SMI_TRACE(1,("\nProcessMDRaid1Region() called for region %s, size = %lld", mdInfo.v.name.c_str(), mdInfo.v.sizeK * 1024));


	// If we haven't already, create a composite extent that maps to this Raid1 region 
	const char *regionName = mdInfo.v.name.c_str();
	CMPIUint64 regionSize = mdInfo.v.sizeK * 1024;

	StorageExtent *compExtent = ExtentsFind(regionName);
	if (compExtent == NULL)	
	{
		_SMI_TRACE(1,("No composite extent exists that maps to Raid1 region, create new"));
		compExtent = ExtentAlloc(regionName, regionName);
		ExtentsAdd(compExtent);
		compExtent->composite = 1;

		CMPIBoolean isCorrupted = 0;
		CMPIBoolean isDegraded = 0;

		compExtent->size = childExt->size;

		if (regionSize > compExtent->size)
		{
			compExtent->size = regionSize;
		}

		if (isCorrupted)
		{
			_SMI_TRACE(1,("RAID1 composite is in a corrupted state!!!"));
			compExtent->ostatus = OSTAT_ERROR;
			compExtent->estatus = ESTAT_DATA_LOST;
		}
		else if (isDegraded)
		{
			_SMI_TRACE(1,("RAID1 composite is in a degraded state!!!"));
			compExtent->ostatus = OSTAT_DEGRADED;
			compExtent->estatus = ESTAT_BROKEN;
		}

		// Create BasedOn assn to lower level component extent
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->size - 1,
							1);

		_SMI_TRACE(1,("RAID Used by = %d: name = %s", mdInfo.v.usedByType, mdInfo.v.usedByName.c_str()));
		// See if this region/composite-extent is consumed by any container/pool
		if (mdInfo.v.usedByType == (UsedByType)UB_LVM)
		{
			ContainerInfo contInfo;
			LvmVgInfo vgInfo;
			_SMI_TRACE(1,("Calling getLvmVgInfo for region/dummy extent %s", mdInfo.v.usedByName.c_str()));
			if (rc = s->getContLvmVgInfo(mdInfo.v.usedByName, contInfo, vgInfo))
			{
				_SMI_TRACE(1,("Error calling getLvmVgInfo for region/dummy extent, rc = %d", rc));
				goto exit;
			}

			// Flag minimum required capabilites to denote RAID 1 so that processContainer
			// knows what to do.
			StorageCapability cap;
			cap.dataRedundancy = cap.dataRedundancyMax = 2;
			cap.packageRedundancy = cap.packageRedundancyMax = 1;
			cap.extentStripe = 1;
			cap.parity = 0;
			cap.noSinglePointOfFailure = 1;

			cap.name="temp";
			rc = ProcessContainer(compExtent, &cap, contInfo, vgInfo);

			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling ProcessContainer for region/composite Raid1 extent, rc = %d", rc));
				goto exit;
			}
		}
		// Else see if this region/composite-extent is consumed by another region
		else if (mdInfo.v.usedByType == (UsedByType)UB_MD)
		{
			deque<MdInfo> mdInfoList;
			deque<MdInfo>::iterator i;
			MdInfo md1Info;
			if(rc = s->getMdInfo(mdInfoList))
			{
				_SMI_TRACE(1,("Error calling getMdInfo, rc =%d", rc));
				goto exit;
			}
	   		for(i = mdInfoList.begin(); i != mdInfoList.end(); ++i )
			{
				_SMI_TRACE(1,("Comparing MD %s with %s", mdInfo.v.usedByName.c_str(), i->v.name.c_str()));
				if(!strcasecmp(mdInfo.v.usedByName.c_str(), i->v.name.c_str()))
				{
					md1Info = *i;
					break;
				}
			}
			if(i != mdInfoList.end())
			{
				if(rc = ProcessRegion(compExtent, md1Info))
				{
					_SMI_TRACE(1,("Error calling ProcessRegion, rc =%d", rc));
					goto exit;
				}
			}
			else
			{
				_SMI_TRACE(1,("Unable to get MdInfo for %s, rc = %d", mdInfo.v.usedByName.c_str(), rc));
				goto exit;
			}
		}
	}
	else
	{
		_SMI_TRACE(1,("Composite extent already exists for this Raid1 region"));

		// Create BasedOn assn to lower level component extent
		CMPIUint16 orderIdx = compExtent->numAntecedents + 1;
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->size - 1,
							orderIdx);

		if (compExtent->pool != NULL)
		{
			// Adjust AllocFrom properties between Raid1 pool and underlying/src pool
			ExtentGetPool(childExt)->remainingSize -= (childExt->size);

			PoolModifyAllocFromAssociation(
									ExtentGetPool(childExt),
									ExtentGetPool(compExtent),
									childExt->size,
									0);
		}
		else
		{
			// Check for case where we are an intermediate composite extent (i.e. RAID10 or
			// RAID 1 concatenation as result of pool expansion.
			if (compExtent->numDependents == 1)
			{
				BasedOn *depBO = compExtent->dependents[0];
				StorageExtent* depExtent = depBO->extent;

				// Adjust AllocFrom properties between Raid1 pool and underlying/src pool
				ExtentGetPool(childExt)->remainingSize -= (childExt->size);

				PoolModifyAllocFromAssociation(
										ExtentGetPool(childExt),
										ExtentGetPool(depExtent),
										childExt->size,
										0);
			}
		}

		// Update Capabilites of Raid1 pool if we have more than 1 mirror
		if (compExtent->numAntecedents > 2)
		{
			if (compExtent->pool != NULL)
			{
				compExtent->pool->capability->dataRedundancy = compExtent->numAntecedents;
				compExtent->pool->capability->dataRedundancyMax = compExtent->numAntecedents;
			}
			else
			{
				// We are part of a RAID 10, update parents capability
				BasedOn *parentBO = compExtent->dependents[0];
				StoragePool *parentPool = parentBO->extent->pool;
				parentPool->capability->dataRedundancy = compExtent->numAntecedents;
				parentPool->capability->dataRedundancyMax = compExtent->numAntecedents;
			}
		}
	}

exit:
	_SMI_TRACE(1,("ProcessMDRaid1Region() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static int ProcessMDRaid5Region(
				StorageExtent *childExt,
				const MdInfo mdInfo)
{
	int rc = 0;

	_SMI_TRACE(1,("\nProcessMDRaid5Region() called for region %s, size = %lld", mdInfo.v.name.c_str(), mdInfo.v.sizeK * 1024));


	// If we haven't already, create a composite extent that maps to this Raid5 region 
	const char *regionName = mdInfo.v.name.c_str();
	CMPIUint64 regionSize = mdInfo.v.sizeK * 1024;

	StorageExtent *compExtent = ExtentsFind(regionName);
	if (compExtent == NULL)	
	{
		_SMI_TRACE(1,("No composite extent exists that maps to Raid5 region, create new"));
		compExtent = ExtentAlloc(regionName, regionName);
		ExtentsAdd(compExtent);
		compExtent->composite = 1;

		CMPIBoolean isCorrupted = 0;
		CMPIBoolean isDegraded = 0;

		CMPIUint64 stripeChunkSize;
		CMPIUint16 numStripes = 0;
		stripeChunkSize = mdInfo.chunk;
		char *token = strtok((char *)mdInfo.devices.c_str(), " ");
		while(token != NULL)
		{
			_SMI_TRACE(1,("token = %s", token));
			numStripes++;
			token = strtok( NULL , " ");
		}

		_SMI_TRACE(1,("Chunk size = %lld", stripeChunkSize));
		_SMI_TRACE(1,("Num stripes = %d", numStripes));

		compExtent->size = (numStripes-1) * childExt->size;

		if (regionSize > compExtent->size)
		{
			compExtent->size = regionSize;
		}

		if (isCorrupted)
		{
			_SMI_TRACE(1,("RAID5 composite is in a corrupted state!!!"));
			compExtent->ostatus = OSTAT_ERROR;
			compExtent->estatus = ESTAT_DATA_LOST;
		}
		else if (isDegraded)
		{
			_SMI_TRACE(1,("RAID5 composite is in a degraded state!!!"));
			compExtent->ostatus = OSTAT_DEGRADED;
			compExtent->estatus = ESTAT_BROKEN;
		}

		// Create BasedOn assn to lower level component extent
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->size - 1,
							1);

		// See if this region/composite-extent is consumed by any container/pool
		if (mdInfo.v.usedByType == UB_LVM)
		{
			ContainerInfo contInfo;
			LvmVgInfo vgInfo;
			if (rc = s->getContLvmVgInfo(mdInfo.v.usedByName, contInfo, vgInfo))
			{
				_SMI_TRACE(1,("Error calling getLvmVgInfo for region/dummy extent, rc = %d", rc));
				goto exit;
			}

			// Flag minimum required capabilites to denote RAID 5 so that processContainer
			// knows what to do.
			StorageCapability cap;
			cap.dataRedundancy = cap.dataRedundancyMax = 1;
			cap.packageRedundancy = cap.packageRedundancyMax = 1;
			cap.extentStripe = numStripes;
			cap.parity = 2;
			cap.userDataStripeDepth = stripeChunkSize;
			cap.noSinglePointOfFailure = 1;

			rc = ProcessContainer(compExtent, &cap, contInfo, vgInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling ProcessContainer for region/composite Raid5 extent, rc = %d", rc));
				goto exit;
			}

			// Adjust AllocFrom properties between Raid5 pool and underlying/src pool
			StoragePool *pool = compExtent->pool;
			if (pool == NULL)
			{
				// Case where we are an intermediate composite, have to get pool from our parent
				BasedOn *depBO = compExtent->dependents[0];
				pool = depBO->extent->pool;
			}

			PoolModifyAllocFromAssociation(
									ExtentGetPool(childExt),
									pool,
									childExt->size,
									0);

			ExtentGetPool(childExt)->remainingSize -= (childExt->size);
		}	
	}
	else
	{
		_SMI_TRACE(1,("Composite extent already exists for this Raid5 region"));
		
		// Create BasedOn assn to lower level component extent
		CMPIUint16 orderIdx = compExtent->numAntecedents + 1;
		ExtentCreateBasedOnAssociation(
							compExtent,
							childExt,
							0,
							childExt->size - 1,
							orderIdx);
	}

exit:
	_SMI_TRACE(1,("ProcessMDRaid5Region() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
/*static int EVMSProcessLVM2Region(
			StoragePool *pool,
			StorageExtent *compExtent,
			StorageExtent *freeExtent,
			const handle_object_info_t *regInfo,
			CMPIUint64 startAddress)
{
	int rc = 0;
	CMPICount i;

	_SMI_TRACE(1,("\nEVMSProcessLVM2Region() called, pool = %s, compExtent = %s, freeExtent = %s", pool->name, compExtent->name, freeExtent->name));
	_SMI_TRACE(1,("\t region = %s, size = %lld", regInfo->info.region.name, regInfo->info.region.size));
	_SMI_TRACE(1,("\t start = %lld, volHandle = %x", regInfo->info.region.start, regInfo->info.region.volume));
	_SMI_TRACE(1,("\t dataType = %d", regInfo->info.region.data_type));

	// Create a dummy/intermediate extent that maps to this LVM2 region 
	const char *regionName = regInfo->info.region.name;
	CMPIUint64 regionSizeBytes = regInfo->info.region.size * 512;
	CMPIUint64 regionSizeBlocks = regionSizeBytes / compExtent->blockSize;

	char *dummyName = strrchr(regionName, '/') + 1;
	_SMI_TRACE(1,("dummyName = %s", dummyName));

	StorageExtent *dummyExtent = ExtentsFind(dummyName);
	if (dummyExtent != NULL)
	{
		_SMI_TRACE(1,("Already processed %s, EXIT", dummyName));
		goto exit;
	}

	// We haven't processed this one yet, allocate/init dummy extent
	dummyExtent	= ExtentAlloc(dummyName, dummyName);
	ExtentsAdd(dummyExtent);

	_SMI_TRACE(1,("Comp/Phys Cop = %s,  FreeCop = %s", compExtent->name, freeExtent->name));
	_SMI_TRACE(1,("DummyCop = %s", dummyExtent->name));

	dummyExtent->blockSize = compExtent->blockSize;
	dummyExtent->numberOfBlocks = dummyExtent->consumableBlocks = regionSizeBlocks;

	// Create BasedOn assn to lower level component extent
	ExtentCreateBasedOnAssociation(
						dummyExtent,
						compExtent,
						startAddress,
						startAddress + regionSizeBytes - 1,
						1);

	startAddress += regionSizeBytes;

	// See if this region/dummy-extent is consumed by any container/pool
	if (regInfo->info.region.consuming_container != 0)
	{
		handle_object_info_t *contInfo = NULL;

		rc = evms_get_info(regInfo->info.region.consuming_container, &contInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evms_get_info for region/dummy extent"));
			goto exit;
		}
		rc = EVMSProcessContainer(dummyExtent, pool->capability, contInfo);
		evms_free(contInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evmsProcessContainer for region/dummy extent"));
			goto exit;
		}
	}
	// Handle any parent regions
	else if (regInfo->info.region.parent_objects->count > 0)
	{
		// Handle parent objects
		uint numParents = regInfo->info.region.parent_objects->count;
		_SMI_TRACE(1,("Extent has %d parents", numParents));
		uint k;
		for (k = 0; k < numParents; k++)
		{
			handle_object_info_t *parentInfo = NULL;
			rc = evms_get_info(regInfo->info.region.parent_objects->handle[k], &parentInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling evms_get_info for dummy extent parent object"));
				continue;
			}
			if (parentInfo->type == REGION)
			{
				_SMI_TRACE(1,("Extent parent is a REGION, name = %s", parentInfo->info.region.name));
				rc = EVMSProcessRegion(dummyExtent, parentInfo);
			}
			else if (parentInfo->type == EVMS_OBJECT)
			{
				_SMI_TRACE(1,("Extent parent is an EVMS_OBJECT, name = %s", parentInfo->info.object.name));
				rc = EVMSProcessEVMSObject(dummyExtent, parentInfo);
			}
			else
			{
				_SMI_TRACE(1,("UNSUPPORTED parent object, type = %d", parentInfo->type));
				rc = 0;
			}
			evms_free(parentInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling evmsProcess<parent> for dummy extent parent object"));
				continue;
			}
		}
	}
	// Handle any EVMS volume to which this region/dummy-extent might belong
	else if (regInfo->info.region.volume != 0)
	{
		handle_object_info_t *volInfo = NULL;

		rc = evms_get_info(regInfo->info.region.volume, &volInfo);
		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling evms_get_info for EVMS volume"));
			goto exit;
		}

		rc = EVMSProcessVolume(pool, dummyExtent, volInfo);

		evms_free(volInfo);

		if (rc != 0)
		{
			_SMI_TRACE(1,("Error calling EVMSProcessVolume"));
			goto exit;
		}
	}

exit:
	_SMI_TRACE(1,("EVMSProcessLVM2Region() done, rc = %d\n", rc));
	return rc;
}



/////////////////////////////////////////////////////////////////////////////
static int EVMSProcessContainer(
				StorageExtent *childExt,
				StorageCapability *childCap,
				const handle_object_info_t *contInfo)
{
	int rc = 0;
	CMPIUint64 contFreespace;
	CMPIUint32 numRegions;
	CMPIUint32 numConsumed;
	StorageExtent *remaining = NULL;
	StorageExtent *primary = NULL;
	const char *contName;
	CMPIUint64 contTotalSize;
	extended_info_array_t *extInfo = NULL;

	_SMI_TRACE(1,("\nEVMSProcessContainer() called, childExtent = %s, capability = %s", childExt->name, childCap->name));

	const char *contEVMSName = contInfo->info.container.name;

	// Make sure it's an LVM2 container, we don't deal with any other type.
	if (omcStrStartsWith(contEVMSName, "lvm2") == NULL)
	{
		_SMI_TRACE(0,("Container is NOT an LVM2 container"));
		rc = -1;
		goto exit;
	}

	// Get extended info for LVM2 container
	rc = evms_get_extended_info(contInfo->info.container.handle, NULL, &extInfo);
	if (rc != 0)
	{
		_SMI_TRACE(0,("Error calling evms_get_extended_info for LVM2 container"));
		goto exit;
	}

	contTotalSize = contInfo->info.container.size * 512;
	CMPIUint32 i;

	for (i = 0; i < extInfo->count; i++)
	{
		// Get container freespace
		if (strcmp(extInfo->info[i].name, "Freespace") == 0)
		{
			contFreespace = extInfo->info[i].value.ui64 * 512;
		}

		// Get number of objects consumed by container
		if (strcmp(extInfo->info[i].name, "Objects") == 0)
		{
			numConsumed = extInfo->info[i].value.ui32;
			_SMI_TRACE(1,("Container has %ld consumed Objects/PVs", numConsumed));
		}

		// Get number of regions produced by container
		if (strcmp(extInfo->info[i].name, "Regions") == 0)
		{
			numRegions = extInfo->info[i].value.ui32;
			_SMI_TRACE(1,("Container has %ld produced Regions/LVs", numRegions));
		}
	}
	evms_free(extInfo);

	_SMI_TRACE(1,("Container EVMS name = %s total size = %lld freespace = %lld",
				contEVMSName, contTotalSize, contFreespace));

	contName  = strchr(contEVMSName, '/') + 1;
	_SMI_TRACE(1,("Container short name = %s", contName));
	char remainingName[256];
	sprintf(remainingName, "%s%s", contName, "Freespace");
	StoragePool *pool;

	// We need determine if this container corresponds to a concrete pool 
	// OR if it one being used to slice up a physical device/disk. We can
	// tell if it's the latter case if the container name is the same as
	// the child extent's name
	if (strcasecmp(contName, childExt->name) == 0)
	{
		_SMI_TRACE(1,("Container does NOT correspond to a concrete pool"));
		// Need to create the intermediate/partition and remaining/freespace 
		// extents that correspond to this container.
		pool = childExt->pool;
		remaining = ExtentAlloc(remainingName, remainingName);
		ExtentsAdd(remaining);

		remaining->blockSize = childExt->blockSize;
		remaining->consumableBlocks = contFreespace / childExt->blockSize;
		remaining->numberOfBlocks = contFreespace / childExt->blockSize;
		remaining->pool = pool;
		PoolExtentsAdd(pool, remaining);

		// Need to create BasedOn association with the underlying extent
		CMPIUint64 startAddress = (childExt->numberOfBlocks * childExt->blockSize) 
								- (remaining->numberOfBlocks * remaining->blockSize);
		if (remaining->numberOfBlocks == 0)
			startAddress--;

		ExtentCreateBasedOnAssociation(
			remaining,
			childExt,
			startAddress,
			childExt->numberOfBlocks * childExt->blockSize - 1,
			1);

		// Also need to take into account metadata pulled from the pool and 
		// reflect that by adjusting pools RemainingSize accordingly.
		CMPIUint64 metaSize = (childExt->numberOfBlocks * childExt->blockSize) - contTotalSize;
		_SMI_TRACE(1,("MetaSize = %lld", metaSize));
		pool->remainingSize -= metaSize;
		primary = childExt;
	}		
	else
	{
		_SMI_TRACE(1,("Container corresponds to concrete pool"));

		// See if we've already created an object for this pool
		char compositeName[256];
		sprintf(compositeName, "%s%s", contName, "Composite");
		StorageExtent *composite = NULL;
		StorageCapability *cap = NULL;
		StoragePool *childPool = ExtentGetPool(childExt);

		pool = PoolsFindByName(contName);
		if (pool == NULL)
		{
			_SMI_TRACE(1,("No concrete pool created yet for container, do 1st time init"));

			// First time we've created this concrete pool object
			pool = PoolAlloc(contName);
			PoolsAdd(pool);
			pool->totalSize = contTotalSize;
			pool->remainingSize = contFreespace;

			// Create pool capabilities
			char capName[256];
			sprintf(capName, "%s%s", contName, "Capabilities");
			cap = CapabilityAlloc(capName);
			CapabilitiesAdd(cap);
			pool->capability = cap;

			_SMI_TRACE(1,("Initialize capabilities"));
			cap->pool = pool;

			// Create the component composite/freespace extents and associations
			// In the case of MD Raid pools the passed in childExtent is also
			// the composite extent.
			if (omcStrStartsWith(childExt->name, "md/") && numConsumed == 1)
			{
				_SMI_TRACE(1,("Container is for MDRaid composite (single MDRaid composite)"));
				composite = childExt;
			}
			else
			{
				_SMI_TRACE(1,("Allocate composite extent %s", compositeName));
				composite = ExtentAlloc(compositeName, compositeName);
				ExtentsAdd(composite);
				composite->blockSize = childExt->blockSize;
				composite->consumableBlocks = contTotalSize / childExt->blockSize;
				composite->numberOfBlocks = composite->consumableBlocks;

				ExtentCreateBasedOnAssociation(
									composite,
									childExt,
									0,
									childExt->numberOfBlocks * childExt->blockSize - 1,
									1);
			}

			PoolCreateAllocFromAssociation(
									ExtentGetPool(childExt), 
									pool,
									childExt->numberOfBlocks * childExt->blockSize);

			composite->pool = pool;
			PoolExtentsAdd(pool, composite);

			_SMI_TRACE(1,("\tAllocate remaining/freespace extent %s", remainingName));
			remaining = ExtentAlloc(remainingName, remainingName);
			ExtentsAdd(remaining);
			remaining->blockSize = childExt->blockSize;
			remaining->consumableBlocks = contFreespace / childExt->blockSize;
			remaining->numberOfBlocks = contFreespace / childExt->blockSize;
			remaining->pool = pool;
			PoolExtentsAdd(pool, remaining);

			CMPIUint64 startAddress = (composite->numberOfBlocks * composite->blockSize) - (remaining->numberOfBlocks * remaining->blockSize);
			if (remaining->numberOfBlocks == 0)
				startAddress--;

			ExtentCreateBasedOnAssociation(
									remaining,
									composite,
									startAddress,
									composite->numberOfBlocks * composite->blockSize - 1,
									1);

			// Update Capabilities based on pool type (JBOD, RAID0, RAID1, etc.) 
			if (CapabilityDefaultIsJBOD(childCap))
			{
				// JBOD case
				_SMI_TRACE(1,("Concrete pool is JBOD"));
				cap->dataRedundancyMax = 1;
				cap->dataRedundancy = 1;
				cap->packageRedundancyMax = 0;
				cap->packageRedundancy = 0;
				cap->extentStripe = 1;
				cap->parity = 0;
				cap->noSinglePointOfFailure = 0;
			}
			else 
			{
				// RAID 0/1/5
				_SMI_TRACE(1,("Concrete pool is RAID"));
				cap->dataRedundancyMax = childCap->dataRedundancyMax;
				cap->dataRedundancy = childCap->dataRedundancy;
				cap->packageRedundancyMax = childCap->packageRedundancyMax;
				cap->packageRedundancy = childCap->packageRedundancy;
				cap->extentStripe = childCap->extentStripe;
				cap->parity = childCap->parity;
				cap->noSinglePointOfFailure = childCap->noSinglePointOfFailure;
			}
		}
		else
		{
			// We already found/allocated pool for this container earlier, do whatever adjustments
			// needed to take this childExtent into account
			_SMI_TRACE(1,("Already have pool for container, add childExtent %s", childExt->name));
			composite = ExtentsFind(compositeName);
			remaining = ExtentsFind(remainingName);
			cap = pool->capability;

			PoolModifyAllocFromAssociation(
								ExtentGetPool(childExt),
								pool,
								childExt->numberOfBlocks * childExt->blockSize,
								0);

			CMPIUint16 orderIdx = composite->numAntecedents + 1;
			ExtentCreateBasedOnAssociation(
								composite,
								childExt,
								0,
								childExt->numberOfBlocks * childExt->blockSize - 1,
								orderIdx);

			composite->composite = 1;
		}

		// Check for problems that would cause us to be degraded
		if (childPool->ostatus  != OSTAT_OK || childExt->ostatus != OSTAT_OK)
		{
			_SMI_TRACE(1,("Concrete pool input pool op status != OK!!"));
			// There is a problem with the input primordial pool that could affect us
			if (contTotalSize > ExtentGetPool(childExt)->totalSize)
			{
				// One of our underlying disks must be bad for this to happen
				_SMI_TRACE(1,("Concrete Pool TotalSize > Underlying Pool TotalSize!!"));
				pool->ostatus = OSTAT_DEGRADED;
				composite->ostatus = OSTAT_DEGRADED;
				composite->estatus = ESTAT_BROKEN;
			}
			else if (childExt->ostatus != OSTAT_OK)
			{
				_SMI_TRACE(1,("Concrete Pool child component status not OK!!!"));
				pool->ostatus = OSTAT_DEGRADED;
				composite->ostatus = childExt->ostatus;
			}
		}

		childPool->remainingSize -= (childExt->numberOfBlocks * childExt->blockSize);
		primary = composite;
	}

	// Now handle case where we have LVM Region created from this container
	if (numRegions > 0)
	{
		CMPIUint64 startAddress = primary->numberOfBlocks * primary->blockSize - contTotalSize;

		for (i = 0; i < contInfo->info.container.objects_produced->count; i++)
		{
			handle_object_info_t *regInfo = NULL;

			rc = evms_get_info(contInfo->info.container.objects_produced->handle[i], &regInfo);
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling evms_get_info for LVM2 region"));
				goto exit;
			}
			if (regInfo->info.region.data_type == DATA_TYPE)
			{
				rc = EVMSProcessLVM2Region(
							pool,
							primary,
							remaining,
							regInfo,
							startAddress);

				evms_free(regInfo);

				if (rc != 0)
				{
					_SMI_TRACE(1,("Error calling EVMSProcessLVM2Region for LVM2 region"));
					goto exit;
				}
			}
		}
	}

exit:
	_SMI_TRACE(1,("EVMSProcessContainer() done, rc = %d\n", rc));
	return rc;
}
*/
static int ProcessContainer(
				StorageExtent *childExt,
				StorageCapability *childCap,
				const ContainerInfo contInfo,
				const LvmVgInfo vgInfo)
{
	int rc = 0;
	CMPIUint64 contFreespace;
	CMPIUint32 numRegions;
	CMPIUint32 numConsumed;
	StorageExtent *remaining = NULL;
	StorageExtent *primary = NULL;
	const char *contName;
	CMPIUint64 contTotalSize;

	_SMI_TRACE(1,("\nProcessContainer() called, childExtent = %s, capability = %s", childExt->name, childCap->name));

	
	contName = contInfo.name.c_str();
	contTotalSize = vgInfo.peCount * vgInfo.peSize * 1024;
	contFreespace = vgInfo.peFree * vgInfo.peSize *1024;
	numRegions = contInfo.volcnt;
	char *token = strtok((char *)vgInfo.devices.c_str(), " ");
	numConsumed = 0;
	while(token != NULL)
	{
		_SMI_TRACE(1,("token = %s", token));
		numConsumed++;
		token = strtok( NULL , " ");
	}
	
	_SMI_TRACE(1,("Container name = %s total size = %lld freespace = %lld",
				contName, contTotalSize, contFreespace));
	char remainingName[256];
	sprintf(remainingName, "%s%s", contName, "Freespace");
	StoragePool *pool;

	// See if we've already created an object for this pool
	char compositeName[256];
	sprintf(compositeName, "%s%s", contName, "Composite");
	StorageExtent *composite = NULL;
	StorageCapability *cap = NULL;
	StoragePool *childPool = ExtentGetPool(childExt);

	pool = PoolsFindByName(contName);
	if (pool == NULL)
	{
		_SMI_TRACE(1,("No concrete pool created yet for container, do 1st time init"));

		// First time we've created this concrete pool object
		pool = PoolAlloc(contName);
		PoolsAdd(pool);
		pool->totalSize = contTotalSize;
		pool->remainingSize = contFreespace;

		// Create pool capabilities
		char capName[256];
		sprintf(capName, "%s%s", contName, "Capabilities");
		cap = CapabilityAlloc(capName);
		CapabilitiesAdd(cap);
		pool->capability = cap;

		_SMI_TRACE(1,("Initialize capabilities"));
		cap->pool = pool;

		// Create the component composite/freespace extents and associations
		// In the case of MD Raid pools the passed in childExtent is also
		// the composite extent.
		if (cmpiutilStrStartsWith(childExt->name, "md") && numConsumed == 1)
		{
			_SMI_TRACE(1,("Container is for MDRaid composite (single MDRaid composite)"));
			composite = childExt;
		}
		else
		{
			_SMI_TRACE(1,("Allocate composite extent %s", compositeName));
			composite = ExtentAlloc(compositeName, compositeName);
			ExtentsAdd(composite);
			composite->size = childExt->size;
			ExtentCreateBasedOnAssociation(
								composite,
								childExt,
								0,
								childExt->size,
								1);
		}

		PoolCreateAllocFromAssociation(
								ExtentGetPool(childExt), 
								pool,
								childExt->size);

		composite->pool = pool;
		PoolExtentsAdd(pool, composite);

		_SMI_TRACE(1,("\tAllocate remaining/freespace extent %s", remainingName));
		remaining = ExtentAlloc(remainingName, remainingName);
		ExtentsAdd(remaining);
		remaining->size = contFreespace;
		remaining->pool = pool;
		PoolExtentsAdd(pool, remaining);

		CMPIUint64 startAddress = (composite->size) - (remaining->size);
		if (remaining->size == 0)
			startAddress--;

		ExtentCreateBasedOnAssociation(
								remaining,
								composite,
								startAddress,
								composite->size  - 1,
								1);

		// Update Capabilities based on pool type (JBOD, RAID0, RAID1, etc.) 
		if (CapabilityDefaultIsJBOD(childCap))
		{
			// JBOD case
			_SMI_TRACE(1,("Concrete pool is JBOD"));
			cap->dataRedundancyMax = 1;
			cap->dataRedundancy = 1;
			cap->packageRedundancyMax = 0;
			cap->packageRedundancy = 0;
			cap->extentStripe = 1;
			cap->parity = 0;
			cap->noSinglePointOfFailure = 0;
		}
		else 
		{
			// RAID 0/1/5
			_SMI_TRACE(1,("Concrete pool is RAID"));
			cap->dataRedundancyMax = childCap->dataRedundancyMax;
			cap->dataRedundancy = childCap->dataRedundancy;
			cap->packageRedundancyMax = childCap->packageRedundancyMax;
			cap->packageRedundancy = childCap->packageRedundancy;
			cap->extentStripe = childCap->extentStripe;
			cap->parity = childCap->parity;
			cap->noSinglePointOfFailure = childCap->noSinglePointOfFailure;
		}
	}
	else
	{
		// We already found/allocated pool for this container earlier, do whatever adjustments
		// needed to take this childExtent into account
		_SMI_TRACE(1,("Already have pool for container, add childExtent %s", childExt->name));
		composite = ExtentsFind(compositeName);
		remaining = ExtentsFind(remainingName);
		cap = pool->capability;

		PoolModifyAllocFromAssociation(
							ExtentGetPool(childExt),
							pool,
							childExt->size,
							0);

		CMPIUint16 orderIdx = composite->numAntecedents + 1;
		ExtentCreateBasedOnAssociation(
							composite,
							childExt,
							0,
							childExt->size - 1,
							orderIdx);

		composite->composite = 1;
	}

	// Check for problems that would cause us to be degraded
	if (childPool->ostatus  != OSTAT_OK || childExt->ostatus != OSTAT_OK)
	{
		_SMI_TRACE(1,("Concrete pool input pool op status != OK!!"));
		// There is a problem with the input primordial pool that could affect us
		if (contTotalSize > ExtentGetPool(childExt)->totalSize)
		{
			// One of our underlying disks must be bad for this to happen
			_SMI_TRACE(1,("Concrete Pool TotalSize > Underlying Pool TotalSize!!"));
			pool->ostatus = OSTAT_DEGRADED;
			composite->ostatus = OSTAT_DEGRADED;
			composite->estatus = ESTAT_BROKEN;
		}
		else if (childExt->ostatus != OSTAT_OK)
		{
			_SMI_TRACE(1,("Concrete Pool child component status not OK!!!"));
			pool->ostatus = OSTAT_DEGRADED;
			composite->ostatus = childExt->ostatus;
		}
	}

	childPool->remainingSize -= (childExt->size);
	primary = composite;

	// Now handle case where we have LVM LV created from this container
	if (numRegions > 0)
	{
		deque<LvmLvInfo> lvInfoList;
		if(rc = s->getLvmLvInfo(contInfo.name, lvInfoList))
		{
			_SMI_TRACE(1,("Unable to get Lvm Lv info, rc = %d\n", rc));
			goto exit;
		}
   		for(deque<LvmLvInfo>::iterator i=lvInfoList.begin(); i!=lvInfoList.end(); ++i )
		{
			LvmLvInfo lvInfo = *i;
			rc = ProcessVolume(
						pool,
						primary,
						lvInfo);

			if (rc != 0)
			{
				_SMI_TRACE(1,("Error calling ProcessVolume for LVM2 region"));
				goto exit;
			}
			 
		}
	}
exit:
	_SMI_TRACE(1,("ProcessContainer() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
//////////// CIM methods called by invokeMethod /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/*static int OpenSession()
{
	int rc;
	
	rc = evms_open_engine(NULL, ((engine_mode_t)(ENGINE_READ | ENGINE_WRITE)), 
		NULL, (debug_level_t)-1, NULL);
	if (rc == 0)
	{
		EVMSopen = TRUE;
	}
	return rc;
}
	
/////////////////////////////////////////////////////////////////////////////
static int CloseSession()
{
	int rc;
	
	rc = evms_close_engine();
	if (rc == 0)
	{
		EVMSopen = FALSE;
	}
	return rc;
}
*/
/////////////////////////////////////////////////////////////////////////////
#define ScanStorage	SCSScanStorage

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateOrModifyStoragePool(
					const char *ns, 
					char *poolName,
					StorageSetting *goal,
					CMPIUint64 *size,
					StoragePool *inPool,
					StoragePool **outPool,
					CMPIStatus *pStatus)
{
	CMPIUint32		rc = M_COMPLETED_OK;
	char			genPoolName[32] = {0};
	CMPIUint64		goalSize = *size;
	StoragePool		*modPool = *outPool;
	StorageExtent	**availExtents = NULL;
	StorageExtent	**raidExtents = NULL;
	CMPIBoolean		scanNeeded = 1;

	_SMI_TRACE(1,("\nCreateOrModifyStoragePool() called, poolName = %s, goal = %x, size = %llu, inPool = %x, &outPool = %x", poolName, goal, *size, inPool, outPool));

	// Check the input Goal setting to make sure it is one of the flavors that
	// we support. We currently support JBOD (concatenation), RAID0, RAID1,
	// RAID10 and RAID5.

	if (goal)
	{
		if (!SettingIsJBOD(goal) &&
			!SettingIsRAID0(goal) &&
			!SettingIsRAID1(goal) &&
			!SettingIsRAID10(goal) &&
			!SettingIsRAID5(goal))
		{
			_SMI_TRACE(0,("Specified goal is unsupported."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Specified goal is unsupported in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit_return_only;
		}
	}
	else
	{
		_SMI_TRACE(1,("Goal not specified, will assume input pool defaults"));
	}

	if (*size != 0 && *size < MIN_VOLUME_SIZE)
	{
		// Specified size is unsupported.
		_SMI_TRACE(0,("Specified size is invalid: too small!!!"));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
			"Specified storage pool size not supported (too small) in call to CreateOrModifyStoragePool method");
		rc = M_SIZE_NOT_SUPPORTED;	
		goto exit_return_only;
	}
	
	// Determine whether this is a create or modify request, if Pool
	// parameter is non-null then it's a modify
	if (modPool == NULL)
	{
//---------------------- Beginning of Pool creation code --------------------------------

		_SMI_TRACE(1,("Request is to CreatePool."));

		// Request is to create new pool, first check to see if any InPool specified.
		if (inPool == NULL)
		{
			// No InPool specified, search for pool that has adequate free space
			// to meet the input goal/size requirement. 

			_SMI_TRACE(1,("No Input pool specified, search for pool that meets goal requirements"));
			inPool = PoolsFindByGoal(goal, &goalSize);
			if (inPool == NULL)
			{
				// Couldn't find pool to support the requested Goal/Size, we need to return
				// "Size Not Supported", size specifies our largest supported size.
				_SMI_TRACE(0,("Unable to find pool that meets goal/size requirements!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
					"No pool found meeting goal requirements in call to CreateOrModifyStoragePool method");
				rc = M_SIZE_NOT_SUPPORTED;	
				goto exit;
			}
		}
		else
		{
			// InPool was specified, make sure we can meet Goal/Size requirements.

			CMPIUint64 minSize, maxSize, divisor;
			if (PoolGetSupportedSizeRange(inPool, goal, &minSize, &maxSize, &divisor) == GAE_COMPLETED_OK)
			{
				_SMI_TRACE(1,("Max supported size of inPool is %lld", maxSize));
				if (maxSize < goalSize || maxSize == 0)
				{
					// Input pool doesn't cut it, fail with goalSize specifying
					// largest supported size.
					goalSize = maxSize;
					_SMI_TRACE(0,("Specified input pool does not meet goal/size requirements!!!"));
					CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
						"Specified input pool does not meet goal requirements in call to CreateOrModifyStoragePool method");
					rc = M_SIZE_NOT_SUPPORTED;	
					goto exit;
				}
				if (goalSize == 0)
				{
					// If goalSize was 0 then set to maxSize
					goalSize = maxSize;
				}
			}
			else
			{
				// Something wrong with input pool, fail
				goalSize = 0;
				_SMI_TRACE(0,("Specified input pool does not meet goal/size requirements!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
					"Specified input pool does not meet goal requirements in call to CreateOrModifyStoragePool method");
				rc = M_SIZE_NOT_SUPPORTED;	
				goto exit;
			}
		}

		// At this point we know we can do the pool create as requested, get list of
		// sorted available extents we can use to create the pool. 
		_SMI_TRACE(1,("Have valid inPool, goalSize = %lld, getting available extents....", goalSize));
		CMPICount numAvailExtents = PoolExtentsSize(inPool);
		availExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
		if (availExtents == NULL)
		{
			_SMI_TRACE(0,("Out of memory!!!"));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
					"Unable to allocate memory in call to CreateOrModifyStoragePool method");
			goalSize = 0;
			rc = M_FAILED;
			goto exit;
		}

		PoolGetAvailableExtents(inPool, goal, availExtents, &numAvailExtents);
		ExtentsSortBySize(availExtents, numAvailExtents);

		if (goal == NULL || PoolGoalMatchesDefault(inPool, goal))
		{
			_SMI_TRACE(1,("Creating concrete pool matching default input primordial pool capabilities"));

			// This is the case when we are creating a concrete pool from a 
			// primordial pool that is already the correct type of QOS so we don't 
			// have to create a RAID composition from a bunch of underlying 
			// primordial JBOD disks. 

			if (poolName == NULL || strlen(poolName) == 0)
			{
				// No name specified, need to generate one based on goal type
				if (CapabilityDefaultIsRAID0(inPool->capability))
				{
					poolName = PoolGenerateName("RAID0Pool", genPoolName, 32);
				}
				else if (CapabilityDefaultIsRAID1(inPool->capability))
				{
					poolName = PoolGenerateName("RAID1Pool", genPoolName, 32);
				}
				else if (CapabilityDefaultIsRAID10(inPool->capability))
				{
					poolName = PoolGenerateName("RAID10Pool", genPoolName, 32);
				}
				else if (CapabilityDefaultIsRAID5(inPool->capability))
				{
					poolName = PoolGenerateName("RAID5Pool", genPoolName, 32);
				}
				else
				{
					poolName = PoolGenerateName("JBODPool", genPoolName, 32);
				}
				_SMI_TRACE(1,("Generated poolName = %s", poolName));

			}
			// Choose extents to be used to meet the goal size then create composition
			SelectJBODExtents(goalSize, availExtents, &numAvailExtents);
			rc = CreateJBODComposition(availExtents, numAvailExtents, poolName);
		}
		else if (SettingIsRAID0(goal))
		{
			// Handle software RAID 0 pool creation
			_SMI_TRACE(1,("Creating software RAID 0 concrete pool BEGIN "));

			// Generate a name if no poolName specified 
			if (poolName == NULL || strlen(poolName) == 0)
			{
				poolName = PoolGenerateName("RAID0Pool", genPoolName, 32);
				_SMI_TRACE(1,("Generated poolName = %s", poolName));
			}
			// Choose extents to use where goal ExtentStripeLength tells us how many disks 
			// will be required. The prefered size for each extent we want to use for Raid0
			// is the requested Size / ExtentStripeLength.  
			raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
			if (raidExtents == NULL)
			{
				_SMI_TRACE(0,("Out of memory!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
						"Unable to allocate memory in call to CreateOrModifyStoragePool method");
				goalSize = 0;
				rc = M_FAILED;
				goto exit;
			}

			SelectRAIDExtents(
						availExtents, 
						goal->extentStripeLength,
						(goalSize / goal->extentStripeLength),
						raidExtents,
						&numAvailExtents);

			rc = CreateRAID0Composition(
								raidExtents, 
								numAvailExtents, 
								poolName, 
								goal->userDataStripeDepth,
								NULL);
		}
		else if (SettingIsRAID1(goal))
		{
			// Handle software RAID 1 pool creation
			_SMI_TRACE(1,("Creating software RAID 1 concrete pool BEGIN "));

			// Generate a name if no poolName specified 
			if (poolName == NULL || strlen(poolName) == 0)
			{
				poolName = PoolGenerateName("RAID1Pool", genPoolName, 32);
				_SMI_TRACE(1,("Generated poolName = %s", poolName));
			}

			// Choose extents to use where goal DataRedundancy tells us how many disks 
			// will be required. The prefered size for each extent we want to use for Raid1
			// is equal to the requested Size. 

			raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
			if (raidExtents == NULL)
			{
				_SMI_TRACE(0,("Out of memory!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
						"Unable to allocate memory in call to CreateOrModifyStoragePool method");
				goalSize = 0;
				rc = M_FAILED;
				goto exit;
			}

			SelectRAIDExtents(
						availExtents, 
						goal->dataRedundancyGoal,
						goalSize,
						raidExtents,
						&numAvailExtents);

			rc = CreateRAID1Composition(
								raidExtents, 
								numAvailExtents, 
								poolName, 
								NULL);
		}
		else if (SettingIsRAID10(goal))
		{
			// Handle software RAID 10 pool creation
			_SMI_TRACE(1,("Creating software RAID 10 concrete pool BEGIN "));

			// Generate a name if no poolName specified 
			if (poolName == NULL || strlen(poolName) == 0)
			{
				poolName = PoolGenerateName("RAID10Pool", genPoolName, 32);
				_SMI_TRACE(1,("Generated poolName = %s", poolName));
			}

			// Choose extents to use where goal DataRedundancy*ExtentStripeLength tells us how 
			// many disks will be required. The prefered size for each extent we want to use for 
			// Raid10 is equal to the requested Size / ExtentStripeLength. 

			raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
			if (raidExtents == NULL)
			{
				_SMI_TRACE(0,("Out of memory!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
						"Unable to allocate memory in call to CreateOrModifyStoragePool method");
				goalSize = 0;
				rc = M_FAILED;
				goto exit;
			}

			SelectRAIDExtents(
						availExtents, 
						goal->dataRedundancyGoal * goal->extentStripeLength,
						goalSize / goal->extentStripeLength,
						raidExtents,
						&numAvailExtents);

			rc = CreateRAID10Composition(
						raidExtents, 
						numAvailExtents, 
						poolName,
						goal->dataRedundancyGoal,
						goal->extentStripeLength,
						goal->userDataStripeDepth,
						NULL);
		}
		else if (SettingIsRAID5(goal))
		{
			// Handle software RAID 5 pool creation
			_SMI_TRACE(1,("Creating software RAID 5 concrete pool BEGIN "));

			// Generate a name if no poolName specified 
			if (poolName == NULL || strlen(poolName) == 0)
			{
				poolName = PoolGenerateName("RAID5Pool", genPoolName, 32);
				_SMI_TRACE(1,("Generated poolName = %s", poolName));
			}
			// Choose extents to use where goal ExtentStripeLength tells us how many disks 
			// will be required. The prefered size for each extent we want to use for Raid0
			// is the requested Size / (ExtentStripeLength-1). 

			raidExtents = (StorageExtent **)malloc(numAvailExtents * sizeof(StorageExtent *));
			if (raidExtents == NULL)
			{
				_SMI_TRACE(0,("Out of memory!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
						"Unable to allocate memory in call to CreateOrModifyStoragePool method");
				goalSize = 0;
				rc = M_FAILED;
				goto exit;
			}

			SelectRAIDExtents(
						availExtents, 
						goal->extentStripeLength,
						(goalSize / (goal->extentStripeLength-1)),
						raidExtents,
						&numAvailExtents);

			rc = CreateRAID5Composition(
								raidExtents, 
								numAvailExtents, 
								poolName, 
								goal->userDataStripeDepth,
								NULL);
		}
		else
		{
			// WE SHOULD NEVER GET HERE!!!!
			_SMI_TRACE(0, ("Specified goal is unsupported."));
			rc = M_INVALID_PARAM;
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Specified goal is unsupported in call to CreateOrModifyStoragePool method");
			goto exit;
		}

	}
//---------------------- End of Pool creation code --------------------------------
	else
	{
		// Request is to modify existing pool
		_SMI_TRACE(1,("Request to modify existing pool %s", modPool->name));

		if (poolName == NULL || strlen(poolName) == 0)
		{
			poolName = genPoolName;
			strcpy(poolName, modPool->name);
		}

		// First try to take the simple route and see if we can just delete existing
		// pool and create new pool based on passed in parameters. This will work IF
		// we don't have any Pools/StorageVolumes/LogicalDisks allocated from the
		// original pool.

		char nameBuf[256] = {0};
		StorageCapability saveCap;
		if (goal != NULL)
		{
			saveCap.name = goal->name;
			saveCap.dataRedundancy = goal->dataRedundancyGoal;
			saveCap.extentStripe = goal->extentStripeLength;
			saveCap.packageRedundancy = goal->packageRedundancyGoal;
			saveCap.userDataStripeDepth = goal->userDataStripeDepth;
			saveCap.parity = goal->parityLayout;
		}
		else
		{
			if (modPool)
			{
				saveCap = *(modPool->capability);
				strcpy(nameBuf, modPool->capability->name);
				saveCap.name = nameBuf;
			}
		}
		_SMI_TRACE(1,("Capability name = %s", saveCap.name));

		char inPoolName[256];
		if (inPool != NULL)
		{
			strcpy(inPoolName, inPool->name);
		}

		rc = DeleteStoragePool(ns, modPool, pStatus);

		if (rc == M_COMPLETED_OK)
		{
			// Delete pool worked so go ahead and create new pool matching new
			// requirements. We need to recreate the goal setting object since
			// it may have been deleted during the deleteStoragePool.

			_SMI_TRACE(1,("Call CapabilityCreateSetting"));
			CapabilityCreateSetting(&saveCap, ns, SCST_DEFAULT, &goal);
			modPool = NULL;
			if (inPool != NULL)
			{
				inPool = PoolsFindByName(inPoolName);
				_SMI_TRACE(1,("inPool name is %s", inPool->name));
			}
			rc = CreateOrModifyStoragePool(ns, poolName, goal, &goalSize, inPool, &modPool, pStatus);
			scanNeeded = 0;
		}
		else if (rc == M_IN_USE)
		{
			// This pool has something allocated from it so we need to try and do a 
			// true modify without deleting anything that might trash extents/data that
			// someone allocating from the original pool is depending on.

			// We force goal to match pool default capabilities in this case
			_SMI_TRACE(1,("Call CapabilityCreateSetting"));
			CapabilityCreateSetting(&saveCap, ns, SCST_DEFAULT, &goal);
			
			rc = ModifyConcretePool(modPool, ns, poolName, goal, goalSize, inPool);
		}
		else
		{
			_SMI_TRACE(1,("Unable to modify pool, DeleteStoragePool failed."));
			goto exit;
		}
	}

exit:
/*	if (EVMSopen)
	{
		if (rc == M_COMPLETED_OK)
		{
			_SMI_TRACE(1,("Calling evms_commit_changes"));
			evms_commit_changes();
		}
		CloseSession();
		_SMI_TRACE(1,("Engine closed"));
	}
*/
	if (availExtents)
	{
		free(availExtents);
	}
	if (raidExtents)
	{
		free(raidExtents);
	}

	// Update our internal model if need be
	if (scanNeeded)
	{
		ScanStorage(ns, pStatus);
		if ((pStatus->rc != CMPI_RC_OK))
		{
			goto exit_return_only;
		}
	}

	// Set output pool/size
	*outPool = PoolsFindByName(poolName);
	if (*outPool)
	{
		*size = (*outPool)->totalSize;
	}
	else
	{
		*size = goalSize;
	}

exit_return_only:
	_SMI_TRACE(1,("CreateOrModifyStoragePool() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 CreateOrModifyElementFromStoragePool(
					const char *ns,
					char *elmName,
					CMPIUint16 elmType,
					StorageSetting *goal,
					CMPIUint64 *size,
					StoragePool *pool,
					StorageVolume **outVol,
					CMPIStatus *pStatus)
{
	CMPIUint32		rc = M_COMPLETED_OK;
	CMPIUint64		goalSize = *size;
	StorageVolume	*modVol = *outVol;
	
	char namebuf[256] = {0};
	char genVolName[256] = {0};
	strcpy(namebuf, elmName);
	CMPIUint64 currSize;

	_SMI_TRACE(1,("\nCreateOrModifyElementFromStoragePool() called: Validating input parameters"));

	// Check for invalid inputs

	if (pool != NULL && modVol != NULL)
	{
		// Not valid to specify both pool and modVol
		_SMI_TRACE(1,("Input pool cannot be non-NULL for Element Modify operation"));
		rc = M_INVALID_PARAM;
		goto exit_return_only;
	}

	if (pool == NULL && modVol == NULL)
	{
		_SMI_TRACE(1,("No input pool specified, try to find suitable one"));
		// NULL pool specified for a volume create, spec is vague on what to
		// do in this case. For now we just grab the first concrete pool we can
		// find that matches the goal. If we can't find a concrete pool we look
		// for a primordial to use.
		CMPICount i;
		for (i = 0; i < PoolsSize(); i++)
		{
			pool = PoolsGet(i);
			if (!pool->primordial)
			{
				if (goal == NULL)
				{
					// No goal specified, any pool with size remaining will do
					if (pool->remainingSize >= MIN_VOLUME_SIZE)
					{
						break;
					}
				}
				else if (PoolGoalMatchesDefault(pool, goal))
				{
					// Pool meets the goal
					break;
				}
			}
		}
		if (i == PoolsSize())
		{
			_SMI_TRACE(1,("Unable to find suitable concrete pool, look for a primordial"));
			for (i = 0; i < PoolsSize(); i++)
			{
				pool = PoolsGet(i);
				if (pool->primordial)
				{
					if (goal == NULL)
					{
						// No goal specified, any pool with size remaining will do
						if (pool->remainingSize >= MIN_VOLUME_SIZE)
						{
							break;
						}
					}
					else if (PoolGoalMatchesDefault(pool, goal))
					{
						// Pool meets the goal
						break;
					}
				}
			}
			if (i == PoolsSize())
			{
				_SMI_TRACE(1,("Unable to find suitable pool for element creation"));
				rc = M_INVALID_PARAM;
				goto exit_return_only;
			}
		}
	}
	else if (modVol)
	{
		// This is a modify, get our antecedent pool
		pool = (StoragePool *)modVol->antecedentPool.element;
	}

	// Make sure pool is valid
	if (pool == NULL)
	{
		_SMI_TRACE(1,("Input pool is invalid"));
		rc = M_INVALID_PARAM;
		goto exit_return_only;
	}
	if (elmType != ET_LOGICALDISK && elmType != ET_STORAGEVOLUME)
	{
		_SMI_TRACE(1,("Invalid element type specified, only Logical Disk type supported"));
		
		rc = M_INVALID_PARAM;
		goto exit_return_only;
	}

	if (goal != NULL)
	{
		// If goal is specified we currently require it to match inPool defaults
		// in the case where input pool is a concrete
		_SMI_TRACE(1,("Input goal specified, compare to defaults"));
		if (!pool->primordial && !PoolGoalMatchesDefault(pool, goal))
		{
			_SMI_TRACE(1,("Input goal does not match concrete pool defaults"));
			goalSize = 0;
			rc = M_SIZE_NOT_SUPPORTED;
			goto exit;
		}
	}
	
	if (modVol == NULL)
	{
		// This is a CREATE
		if (pool->primordial)
		{
			_SMI_TRACE(1,("Input pool is primordial, first creating concrete pool"));

			// If we're dealing with a primordial pool we have to create a concrete pool first,
			// then create the volume/logical-disk.
			StoragePool *outPool = NULL; 
			rc = CreateOrModifyStoragePool(ns, NULL, goal, &goalSize, pool, &outPool, pStatus);
		
			if (rc != M_COMPLETED_OK)
			{
				_SMI_TRACE(1,("Call to create concrete pool failed, rc = %d", rc));
				goto exit;
			}

			if (outPool == NULL)
			{
				_SMI_TRACE(1,("Concrete input pool is invalid"));
				rc = M_INVALID_PARAM;
				goto exit;
			}
			pool = outPool;
			goalSize = pool->remainingSize;
		}
/*
		// Initialize EVMS
		if (!EVMSopen)
		{
			rc = OpenSession();
			if (rc != 0)
			{
				_SMI_TRACE(1,("Error %d opening the evms engine", rc));
				rc = M_FAILED;
				goto exit;
			}
			_SMI_TRACE(1,("Engine open"));
		}
*/
		// Make sure we have the space required to meet goal
		if (pool->remainingSize < goalSize)
		{
			_SMI_TRACE(1,("Requested size(%lld) too large, pool available space = %lld", goalSize, pool->remainingSize));
			goalSize = pool->remainingSize;
			rc = M_SIZE_NOT_SUPPORTED;
			goto exit;
		}

		// If no goalSize specified we default to use entire pool RemainingSize
		if (goalSize == 0)
		{
			if (pool->remainingSize >= MIN_VOLUME_SIZE)
			{
				goalSize = pool->remainingSize;
			}
			else
			{
				rc = M_SIZE_NOT_SUPPORTED;
				goalSize = 0;
				goto exit;
			}
		}

		// Enum all our concrete components
		// NOTE: Currently we never have more than 2 component extents in our
		// concrete pools (original composite extent and possibly a remaining extent)
		StorageExtent *remExt = NULL;
//		StorageExtent *compExt = NULL;
		_SMI_TRACE(1,("Found %d concrete components for pool", PoolExtentsSize(pool)));

		if (PoolExtentsSize(pool) < 2)
		{
			_SMI_TRACE(1,("No concrete components found in pool"));
			goalSize = 0;
			rc = M_SIZE_NOT_SUPPORTED;
			goto exit;
		}

		CMPICount i;
		for (i = 0; i < 2; i++)
		{
			StorageExtent *se = PoolExtentsGet(pool, i);
			if (ExtentIsFreespace(se))
				remExt = se;
//			else
//				compExt = se;
		}
/*
		StorageExtent *dummyExt;
		if (remExt != NULL)
		{
			// We should already have a remaining/freespace extent to work with,
			// need to create usable dummy extent/region.
			ExtentPartition(remExt, goalSize, &dummyExt, &remExt);
			_SMI_TRACE(1,("Dummy = %s, Rem = %s", dummyExt->name, remExt->name));
		}
		else
		{
			_SMI_TRACE(1,("No remaining freespace found in pool"));
			goalSize = 0;
			rc = M_SIZE_NOT_SUPPORTED;
			goto exit;
		}
*/
		// Generate a name for our output storage element if none specified
		if (elmName == NULL || strlen(elmName) == 0)
		{
			char prefix[256] = {0};
			sprintf(prefix, "%s%s", pool->name, "Volume");
			elmName = VolumeGenerateName(prefix, genVolName, 256);
			_SMI_TRACE(1,("Generated volume name = %s", elmName));
		}

		if(elmType == ET_LOGICALDISK)
			sprintf(namebuf, "%s%s", "LD_", elmName);
		else
			sprintf(namebuf, "%s%s", "SV_", elmName);
		
		// Create it
//		rc = CreateVolume(elmName, dummyExt);

		rc = CreateVolume(namebuf, pool, goalSize);
		if(!rc)
		{
			//Adjust the remaining extent size
			remExt->size -= goalSize;
		}
	}
	else
	{
		// This is a MODIFY
		_SMI_TRACE(1,("Request to modify existing storage element"));

		// Compare goalSize to existing size to determine if we need to expand/shrink the element
		currSize = modVol->size;

		if (goalSize && goalSize != currSize && goalSize >= MIN_VOLUME_SIZE)
		{
			_SMI_TRACE(1,("Request to resize, goalSize = %llu, currSize = %llu", goalSize, currSize));

			rc = ResizeVolume(modVol, currSize, goalSize);

			if (rc != 0)
			{
				_SMI_TRACE(0,("Error %d attempting to resize volume", rc));
				rc = M_NOT_SUPPORTED;
				goalSize = currSize;
				goto exit;
			}
		}

		// See if we need to do a rename
		if ( (elmName != NULL) && (strlen(elmName) > 0) && (strcmp(elmName, modVol->name) != 0) )
		{
			_SMI_TRACE(1,("Request to rename, new name = %s, current name = %s", elmName, modVol->name));

				if(elmType == ET_LOGICALDISK)
					sprintf(namebuf, "%s%s" ,"LD_", elmName);
				else
					sprintf(namebuf, "%s%s", "SV_", elmName);
			
			rc = RenameVolume(modVol, namebuf);

			if (rc != 0)
			{
				_SMI_TRACE(0,("Unable to rename storage element"));
				rc = M_FAILED;
				goto exit;
			}
		}
		else
		{
			elmName = genVolName;
			strcpy(elmName, modVol->name);
			_SMI_TRACE(1,("No rename required, elmName = %s", elmName));
		}
	}

exit:
	// Update our internal model
	ScanStorage(ns, pStatus);
	if ((pStatus->rc != CMPI_RC_OK))
	{
		goto exit_return_only;
	}

	// Set output pool/size
	*outVol = VolumesFind(namebuf);
	*size = goalSize;

exit_return_only:
	_SMI_TRACE(1,("CreateOrModifyElementFromStoragePool() done, rc = %d, outSize = %llu", rc, *size));
	if (*outVol)
	{
		_SMI_TRACE(1,("\t outVol = %s", (*outVol)->name));
	}
	return rc;
	_SMI_TRACE(1,("\n"));
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 ReturnToStoragePool(
					const char *ns,
					StorageVolume *vol,
					CMPIStatus *pStatus)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;

	_SMI_TRACE(1,("\nReturnToStoragePool() called"));

	StoragePool *antecedentPool; 

	if (vol == NULL)
	{
		_SMI_TRACE(0,("Unable to find input element instance."));
		rc = M_INVALID_PARAM;
		goto exit_return_only;
	}

	antecedentPool = (StoragePool *)vol->antecedentPool.element;
	_SMI_TRACE(1,("Call removeLvmLv inside Pool %s", antecedentPool->name));
	if(rc = s->removeLvmLv(antecedentPool->name, vol->deviceID))
	{
		_SMI_TRACE(1,("Can't delete LVM Logical volume %s, rc = %d", vol->deviceID, rc));
		rc = M_FAILED;
		goto exit;
	}	
	
exit:
/*	if (closeEVMS)
	{
		if (rc == M_COMPLETED_OK)
		{
			_SMI_TRACE(1,("Calling evms_commit_changes"));
			evms_commit_changes();
		}
		CloseSession();
		_SMI_TRACE(1,("Engine closed"));
	}
*/
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));

	// Update our internal model
	ScanStorage(ns, pStatus);

exit_return_only:
	_SMI_TRACE(1,("ReturnToStoragePool() done, rc = %d\n", rc));
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
static CMPIUint32 DeleteStoragePool(
					const char *ns,
					StoragePool *pool,
					CMPIStatus *pStatus)
{
	CMPIUint32 rc = M_COMPLETED_OK, s_rc = 0;

	_SMI_TRACE(1,("\nDeleteStoragePool() called"));


	if (pool == NULL)
	{
		_SMI_TRACE(0,("Unable to find input pool %s", pool->name));
		rc = M_INVALID_PARAM;
		goto exit_return_only;
	}

	// See if we have any pool/volumes allocated from us, if so we return "In Use".
	if (pool->numDepPools > 0 || pool->numDepVolumes > 0)
	{
		_SMI_TRACE(0,("Can't delete pool that has pools/volumes allocated from it."));
		rc = M_IN_USE;
		goto exit_return_only;
	}
	// Delete ConcreteComponent extents 

	_SMI_TRACE(1,("Found %d concrete components for pool", PoolExtentsSize(pool)));
		
	int i;
	for (i = PoolExtentsSize(pool) - 1; i >= 0; i--)
	{
		StorageExtent *ce = PoolExtentsGet(pool, i);
		if (ExtentIsFreespace(ce))
		{
			_SMI_TRACE(1,("Component is a remaining extent"));
			// For remaining extents we need to delete the corresponding
			// EVMS LVM2 container. 
			// *** NOTE: Deletion of any LVM2 regions exported by the
			// container will have occured during returnToStoragePool
			if(rc = s->removeLvmVg(pool->name))
			{
				_SMI_TRACE(1,("Unable to remove VG %s, rc = %d", pool->name, rc));
			}
		}
		else
		{
			_SMI_TRACE(1,("Component is NOT a remaining extent"));
			// For non-remaining extent need to delete any corresponding evms region
			char *regionName = ce->name;
			if(strstr(regionName, "md"))
			{
				_SMI_TRACE(1,("Calling removeMd for %s", regionName));
				if(rc = s->removeMd(regionName, true))
				{
					_SMI_TRACE(1,("Unable to remove Md %s, rc = %d", regionName, rc));
				}
			}
			else
			{
				_SMI_TRACE(0,("Unable to delete %s", regionName));
			}

		}
		_SMI_TRACE(1,("Found %d underlying BasedOns for %s", ce->numAntecedents, ce->name));


		CMPICount j;
		for (j = 0; j < ce->numAntecedents; j++)
		{
			BasedOn *antBO = ce->antecedents[j];
			StorageExtent *se = antBO->extent;
			CMPIBoolean deleteSrcExt = 0;

			_SMI_TRACE(1,("Examine underlying extent %s", se->name));

			// We need to deal with RAID10 case where we have intermediate
			// underlying composite extents that are based on other lowest level
			// underlying physical/primordial extents. In this case we need to delete
			// any corresponding EVMS region, delete BasedOn's with physical extents
			// and delete the intermediate composite extent itself.
			// NOTE: We only need to worry about this with non-remaining component extents

			if (!ExtentIsFreespace(ce) && se->composite)
			{
				_SMI_TRACE(1,("Underlying BasedOn is non-primordial, need to do additional cleanup"));
				char *regionName = se->name;
				if(strstr(regionName, "md"))
				{
					if(rc = s->removeMd(regionName, true))
					{
						_SMI_TRACE(1,("Unable to remove Md %s, rc = %d", regionName, rc));
					}
				}
				else
				{
					_SMI_TRACE(0,("Unable to delete %s", regionName));
					rc = M_FAILED;
				}

				CMPICount k;
				for (k = 0; k < se->numAntecedents; k++)
				{
					BasedOn *antBO = se->antecedents[k];
					StorageExtent *pe = antBO->extent;
					if (!pe->composite && ExtentGetPool(pe)->primordial)
					{
						_SMI_TRACE(1,("Examine underlying primordial extent %s", pe->name));
						ExtentUnpartition(pe);
					}
					else if (pe->composite)
					{
						_SMI_TRACE(1,("Underlying BasedOn is non-primordial, need to do additional cleanup"));
						char *regionName = pe->name;
						if(strstr(regionName, "md"))
						{
							if(rc = s->removeMd(regionName, true))
							{
								_SMI_TRACE(1,("Unable to remove Md %s, rc = %d", regionName, rc));
							}
						}
						else
						{
							_SMI_TRACE(0,("Unable to delete %s", regionName));
						}

						CMPICount l;
						for (l = 0; l < pe->numAntecedents; l++)
						{
							BasedOn *antBO = pe->antecedents[l];
							StorageExtent *pe2 = antBO->extent;
							_SMI_TRACE(1,("Examine underlying primordial extent %s", pe2->name));
							ExtentUnpartition(pe2);
						}
					}
				}
			}

			// We need to check the underlying extent which we are based on and perform any extent
			// conservation tasks that might be required.  We only need to do this for primordial extents.

			_SMI_TRACE(1,("Check for additional extent conservation required..."));
			if (!se->composite && ExtentGetPool(se)->primordial)
			{
				_SMI_TRACE(1,("Call unpartition(2)"));
				ExtentUnpartition(se);
			}
		}
	}

exit:
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc=%d", s_rc));

	// Update our internal model
	ScanStorage(ns, pStatus);

exit_return_only:
	_SMI_TRACE(1,("DeleteStoragePool() done, rc = %d\n", rc));
	return rc;
}


/////////////////////////////////////////////////////////////////////////////
//////////// Public functions ///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SCSCreateInstance(
					const char *ns,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[1024];
	CMPIValue val;
	CMPIArray *arr;
	
	_SMI_TRACE(1,("SCSCreateInstance() called"));

	ci = CMNewInstance(
				_BROKER,
				CMNewObjectPath(_BROKER, ns, StorageConfigurationServiceClassName, status),
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(1,("SCSCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, "SystemCreationClassName", 
		cmpiutilGetComputerSystemClassName(), CMPI_chars);
	CMSetProperty(ci, "SystemName", 
		cmpiutilGetComputerSystemName(buf, 1024), CMPI_chars);
	CMSetProperty(ci, "CreationClassName", StorageConfigurationServiceClassName, CMPI_chars);
	CMSetProperty(ci, "Name", "StorageConfigurationService", CMPI_chars);
	CMSetProperty(ci, "Caption", "StorageConfigurationService", CMPI_chars);
	val.uint16 = 2;
	CMSetProperty(ci, "EnabledState", &val, CMPI_uint16);
	CMSetProperty(ci, "EnabledDefault", &val, CMPI_uint16);
	val.uint16 = 12;
	CMSetProperty(ci, "RequestedState", &val, CMPI_uint16);
	val.dateTime = CMNewDateTime(_BROKER, NULL);
	CMSetProperty(ci, "TimeOfLastStateChange", &val, CMPI_dateTime);
	CMSetProperty(ci, "InstallDate", &val, CMPI_dateTime);
	val.boolean = 1;
	CMSetProperty(ci, "Started", &val, CMPI_boolean);
	arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
	val.uint16 = 2;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, "OperationalStatus", &val, CMPI_uint16A);
	CMSetProperty(ci, "Status", "OK", CMPI_chars);
	val.uint16 = 5;
	CMSetProperty(ci, "HealthState", &val, CMPI_uint16);
	CMSetProperty(ci, "StartMode", "Automatic", CMPI_chars);

exit:
	_SMI_TRACE(1,("SCSCreateInstance() done"));
	return ci;
}


/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SCSCreateObjectPath(
					const char *ns,
					CMPIStatus *status)
{
	CMPIObjectPath *cop;
	char buf[1024];

	_SMI_TRACE(1,("SCSCreateObjectPath() called"));

	cop = CMNewObjectPath(
				_BROKER, ns,
				StorageConfigurationServiceClassName,
				status);

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(1,("SCSCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
		goto exit;
	}

	CMAddKey(cop, "SystemCreationClassName", cmpiutilGetComputerSystemClassName(), CMPI_chars);
	CMAddKey(cop, "SystemName", cmpiutilGetComputerSystemName(buf, 1024), CMPI_chars);
	CMAddKey(cop, "CreationClassName", StorageConfigurationServiceClassName, CMPI_chars);
	CMAddKey(cop, "Name", "StorageConfigurationService", CMPI_chars);

exit:
	_SMI_TRACE(1,("SCSCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SCSCreateHostedAssocInstance(
					const char *ns,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { "Antecedent",  "Dependent", NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("SCSCreateHostedAssocInstance() called"));

	// Create and populate cs object path
	CMPIObjectPath *cscop = cmpiutilCreateCSObjectPath(_BROKER, ns, pStatus);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create ComputerSystem cop");
		return NULL;
	}

	// Create and populate StorageConfigurationService object path (RIGHT)
	CMPIObjectPath *scscop = SCSCreateObjectPath(ns, &status);
	if (CMIsNullObject(scscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationService cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											HostedStorageConfigurationServiceClassName,
											classKeys,
											properties,
											"Antecedent",
											"Dependent",
											cscop,
											scscop,
											pStatus);

	_SMI_TRACE(1,("Leaving SCSCreateHostedAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SCSCreateHostedAssocObjectPath(
					const char *ns,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("SCSCreateHostedAssocObjectPath() called"));

	CMPIObjectPath *cscop = cmpiutilCreateCSObjectPath( _BROKER, ns, pStatus);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
			"Could not create ComputerSystem cop");
		return NULL;
	}

	// Create and populate StorageConfigurationService object path (RIGHT)
	CMPIObjectPath *scscop = SCSCreateObjectPath(ns, &status);
	if (CMIsNullObject(scscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create StorageConfigurationService cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									HostedStorageConfigurationServiceClassName,
									"Antecedent",
									"Dependent",
									cscop,
									scscop,
									pStatus);

	_SMI_TRACE(1,("Leaving SCSCreateHostedAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}


/////////////////////////////////////////////////////////////////////////////
void SCSInvokeMethod(
		const char *ns,
		const char *methodName,
		const CMPIArgs *in,
		CMPIArgs *out,
		const CMPIResult* results,
		CMPIStatus *pStatus,
		const CMPIContext *context,
		const CMPIObjectPath *cop)
{
	CMPIUint32 rc; 

	_SMI_TRACE(1,("\nSCSInvokeMethod() called"));

	if (strcasecmp(methodName, "ScanStorage") == 0)
	{
/*		rc = ScanStorage(ns);
		if (rc != 0)
		{
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
				"Call to CloseSession() failed");
		}
*/
		ScanStorage(ns, pStatus);
	}
	else if (strcasecmp(methodName, "CreateOrModifyStoragePool") == 0)
	{
		// Validate input/output parameters

		const char			*poolName = NULL;
		StorageSetting	*goal = NULL;
		CMPIUint64		goalSize = 0;
		StoragePool		*inPool = NULL;
		StoragePool		*modPool = NULL;

		CMPICount inArgCount = CMGetArgCount(in, pStatus);
	/*	if ( (pStatus->rc != CMPI_RC_OK) || (inArgCount < 6) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to CreateOrModifyStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}

		CMPICount outArgCount = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) ) 
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateOrModifyStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}


		
		if ( (pStatus->rc != CMPI_RC_OK) || (outArgCount < 3) )
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateOrModifyStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}*/

		CMPIData inData = CMGetArg(in, "ElementName", NULL);
		if (inData.state == CMPI_goodValue)
		{
			poolName = CMGetCharsPtr(inData.value.string, NULL);
			_SMI_TRACE(1,("inPoolName = %s", poolName));
		}

		inData = CMGetArg(in, "Goal", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath	*goalCop = inData.value.ref;
			if (goalCop != NULL)
			{
				CMPIData keyData = CMGetKey(goalCop, "InstanceID", NULL);
				const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Input goal setting has instanceID = %s", instanceID));
				goal = SettingsFind(instanceID);
			}
			if (goal == NULL)
			{
				_SMI_TRACE(0,("CreateOrModifyStoragePool(): Input goal object not found!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
					"Input settings goal object not found in call to CreateOrModifyStoragePool method");
				rc = M_INVALID_PARAM;
				goto exit;
			}
		}

		inData = CMGetArg(in, "Size", NULL);
		if (inData.state == CMPI_goodValue)
		{
			goalSize = inData.value.uint64;
			_SMI_TRACE(1,("goalSize = %lld", goalSize));
		}

		inData = CMGetArg(in, "InPools", NULL);
		_SMI_TRACE(0,("CreateOrModifyStoragePool(): Checking the In Pools paramter"));
		if (inData.state == CMPI_goodValue)
		{
			_SMI_TRACE(0,("CreateOrModifyStoragePool(): Checking for multiple In Pools"));
			// NOTE: We currently only support a single input pool
			CMPIArray *inPools = inData.value.array;
			CMPICount numInPools = CMGetArrayCount(inPools, NULL);
			if (numInPools > 1)
			{
				_SMI_TRACE(0,("CreateOrModifyStoragePool(): Multiple input pools not supported!!!"));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
					"Multiple input pools not supported in call to CreateOrModifyStoragePool method");
				rc = M_INVALID_PARAM;
				goto exit;
			}

			inData = CMGetArrayElementAt(inPools, 0, NULL);
			const char *inPoolCopStr = CMGetCharsPtr(inData.value.string, NULL);
			_SMI_TRACE(0,("CreateOrModifyStoragePool(): Trying to get the instance ID"));
			if (inPoolCopStr != NULL)
			{
				_SMI_TRACE(1,("inPool[0] = %s", inPoolCopStr));
				char *instanceID = strchr(inPoolCopStr, '=');
				_SMI_TRACE(0,("CreateOrModifyStoragePool():instance ID = %s", instanceID));
				if (instanceID)
				{
					instanceID++;	// skip '='
					instanceID++;	// skip '"'
					instanceID[strlen(instanceID)-1] = '\0'; // Get rid of end '"'
					_SMI_TRACE(1,("inPool InstanceID = %s", instanceID));
					inPool = PoolsFind(instanceID);
				}
				if (inPool == NULL)
				{
					_SMI_TRACE(0,("CreateOrModifyStoragePool(): Input pool object not found!!!"));
					CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
						"Input pool object not found in call to CreateOrModifyStoragePool method");
					rc = M_INVALID_PARAM;
					goto exit;
				}
			}
		}

		inData = CMGetArg(in, "InExtents", NULL);
		if (inData.state == CMPI_goodValue)
		{
			// NOTE: We currently do not support InExtents
			_SMI_TRACE(0,("CreateOrModifyStoragePool(): Input extents not currently supported!!!"));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Input extents not supported in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}

		inData = CMGetArg(in, "Pool", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath *modPoolCop = inData.value.ref;
			if (modPoolCop != NULL)
			{
				CMPIData keyData = CMGetKey(modPoolCop, "InstanceID", NULL);
				const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Modify pool has instanceID = %s", instanceID));
				modPool = PoolsFind(instanceID);
				if (modPool == NULL)
				{
					_SMI_TRACE(0,("CreateOrModifyStoragePool(): Input modify pool object not found!!!"));
					CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
						"Input modify pool object not found in call to CreateOrModifyStoragePool method");
					rc = M_INVALID_PARAM;
					goto exit;
				}
			}
		}

		rc = CreateOrModifyStoragePool(ns, (char *)poolName, goal, &goalSize, inPool, &modPool, pStatus);

		// Set output params (job, size, pool)
		CMAddArg(out, "Job", NULL, CMPI_ref);
		CMAddArg(out, "Size", &goalSize, CMPI_uint64);

		if (modPool)
		{
			CMPIObjectPath *outPoolCop = PoolCreateObjectPath(modPool, ns, pStatus);;
			CMPIValue val;
			val.ref = outPoolCop;
			CMAddArg(out, "Pool", &val, CMPI_ref);
		}
		else
		{
			CMAddArg(out, "Pool", NULL, CMPI_ref);
		}
	}
	else if (strcasecmp(methodName, "CreateOrModifyElementFromStoragePool") == 0)
	{
		// Validate input/output parameters
		const char			*elmName = NULL;
		CMPIUint16		elmType = ET_STORAGEVOLUME;
		StorageSetting	*goal = NULL;
		CMPIUint64		goalSize = 0;
		StoragePool		*pool = NULL;
		StorageVolume	*modVol = NULL;

		CMPICount inArgCount = CMGetArgCount(in, pStatus);
/*		if ( (pStatus->rc != CMPI_RC_OK) || (inArgCount < 6) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to CreateOrModifyElementFromStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to CreateOrModifyElementFromStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}

		CMPICount outArgCount = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) ) 
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateOrModifyStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}
		if ( (pStatus->rc != CMPI_RC_OK) || (outArgCount < 3) )
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateOrModifyElementFromStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateOrModifyElementFromStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		} */

		CMPIData inData = CMGetArg(in, "ElementName", NULL);
		if (inData.state == CMPI_goodValue)
		{
			elmName = CMGetCharsPtr(inData.value.string, NULL);
			_SMI_TRACE(1,("inElmName = %s", elmName));
		}

		inData = CMGetArg(in, "ElementType", NULL);
		if (inData.state == CMPI_goodValue)
		{
			elmType = inData.value.uint16;
			_SMI_TRACE(1,("inElmType = %d", elmType));
		}

		inData = CMGetArg(in, "Goal", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath	*goalCop = inData.value.ref;
			if (goalCop != NULL)
			{
				CMPIData keyData = CMGetKey(goalCop, "InstanceID", NULL);
				const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Input goal setting has instanceID = %s", instanceID));
				goal = SettingsFind(instanceID);
			}
		}

		inData = CMGetArg(in, "Size", NULL);
		if (inData.state == CMPI_goodValue)
		{
			goalSize = inData.value.uint64;
			_SMI_TRACE(1,("goalSize = %lld", goalSize));
		}

		inData = CMGetArg(in, "InPool", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath	*inPoolCop = inData.value.ref;
			if (inPoolCop != NULL)
			{
				CMPIData keyData = CMGetKey(inPoolCop, "InstanceID", NULL);
				const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Input pool has instanceID = %s", instanceID));
				pool = PoolsFind(instanceID);
			}
		}

		inData = CMGetArg(in, "TheElement", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath *modVolCop = inData.value.ref;
			if (modVolCop != NULL)
			{
				CMPIData keyData = CMGetKey(modVolCop, "DeviceID", NULL);
				const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Modify element has deviceID = %s", deviceID));
				modVol = VolumesFind(deviceID);
			}
		}

		rc = CreateOrModifyElementFromStoragePool(ns, (char *)elmName, elmType, goal, &goalSize, pool, &modVol, pStatus);

		// Set output params (job, size, pool)
		//Vijay
		CMAddArg(out, "Job", NULL, CMPI_ref);
		CMAddArg(out, "Size", &goalSize, CMPI_uint64); 

		if (modVol)
		{
			CMPIObjectPath *outVolCop;

			if (elmType == 4 )
			{
				_SMI_TRACE(1,("inElmType = %d", elmType));
				outVolCop = VolumeCreateObjectPath(LogicalDiskClassName,modVol, ns, pStatus);
			}
			else if (elmType == 2 )
			{
				_SMI_TRACE(1,("inElmType = %d", elmType));
				outVolCop = VolumeCreateObjectPath(StorageVolumeClassName,modVol, ns, pStatus);
			}
				
			CMPIValue val;
			val.ref = outVolCop;
			CMAddArg(out, "TheElement", &val, CMPI_ref);
		}
		else
		{
			CMAddArg(out, "TheElement", NULL, CMPI_ref);
		}
	}
	else if (strcasecmp(methodName, "ReturnToStoragePool") == 0)
	{
		// Validate input/output parameters
		StorageVolume *vol = NULL;

		CMPICount inArgCount = CMGetArgCount(in, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (inArgCount < 1) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to ReturnToStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to ReturnToStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}

		CMPICount outArgCount = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) ) 
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateOrModifyStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}
		//Vijay
		/*if (outArgCount == 0 )
		{
			out = CMNewArgs(_BROKER, pStatus);
			_SMI_TRACE(1,("After CMNewArgs"));
			*pStatus = CMAddArg(out, "Job", NULL, CMPI_ref);
                	if(pStatus->rc != CMPI_RC_OK)
			{
					rc = M_FAILED; 
					goto exit;
			}
		}
		CMPICount outArgCount1 = CMGetArgCount(out, pStatus);
		_SMI_TRACE(1,("---outArgCount : %d",outArgCount1));
		
		if ( (pStatus->rc != CMPI_RC_OK) || (outArgCount1 < 1) )
		{
			_SMI_TRACE(0,("Required output parameter missing in call to ReturnToStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to ReturnToStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		} */

		CMPIData inData = CMGetArg(in, "TheElement", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath *volCop = inData.value.ref;
			if (volCop != NULL)
			{
				CMPIData keyData = CMGetKey(volCop, "DeviceID", NULL);
				const char *deviceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Input element/volume has deviceID = %s", deviceID));
				vol = VolumesFind(deviceID);
			}
		}

		rc = ReturnToStoragePool(ns, vol, pStatus);

		// Set output params (job)
		CMAddArg(out, "Job", NULL, CMPI_ref);
	}
	else if (strcasecmp(methodName, "DeleteStoragePool") == 0)
	{
		// Validate input/output parameters
		StoragePool *pool = NULL;

		CMPICount inArgCount = CMGetArgCount(in, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) || (inArgCount < 1) )
		{
			_SMI_TRACE(0,("Required input parameter missing in call to DeleteStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to DeleteStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}

		CMPICount outArgCount = CMGetArgCount(out, pStatus);
		if ( (pStatus->rc != CMPI_RC_OK) ) 
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateOrModifyStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateOrModifyStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		}
		/*if ( (pStatus->rc != CMPI_RC_OK) || (outArgCount < 1) )
		{
			_SMI_TRACE(0,("Required output parameter missing in call to DeleteStoragePool method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to DeleteStoragePool method");
			rc = M_INVALID_PARAM;
			goto exit;
		} */

		CMPIData inData = CMGetArg(in, "Pool", NULL);
		if (inData.state == CMPI_goodValue)
		{
			CMPIObjectPath *poolCop = inData.value.ref;
			if (poolCop != NULL)
			{
				CMPIData keyData = CMGetKey(poolCop, "InstanceID", NULL);
				const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);
				_SMI_TRACE(1,("Input pool has instanceID = %s", instanceID));
				pool = PoolsFind(instanceID);
			}
		}

		rc = DeleteStoragePool(ns, pool, pStatus);

		// Set output params (job)
		CMAddArg(out, "Job", NULL, CMPI_ref);
	}
	else if (strcasecmp(methodName, "CreateReplica") == 0)
	{
		rc = CopyServicesCreateReplica( _BROKER, ns, context, cop, methodName, in, out, results, pStatus);
		//CMAddArg(in2, "ElementType", elmType, CMPI_uint16);
	}
/*	else if (strcasecmp(methodName,"ModifySynchronization") == 0)
	{
		rc = CopyServicesModifySynchronization( _BROKER, ns, context, cop, methodName, in, out, results, pStatus);
	}	
*/
	else
	{
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_METHOD_NOT_AVAILABLE,
			"Method not supported by OMC_StorageConfigurationService");
	}

exit:
	CMReturnData(results, &rc, CMPI_uint32);
	_SMI_TRACE(1,("SCSInvokeMethod() finished, rc = %d\n", rc));
}

/////////////////////////////////////////////////////////////////////////////
// Scan all of the EVMS devices and match the lowest level disks to their 
// matching persistent CIM primordial storage extent objects. Resize CIM objects if 
// the sizes have changed. Continue up the evms tree and build the in-memory CIM 
// representation of the evms storage object tree.
/*int SCSScanStorage(const char *ns)
{
	int rc = 0;
	CMPICount i, numPools;
	CMPIBoolean closeEVMS = !EVMSopen;
	StoragePool *pool;
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("\nSCSScanStorage() called"));

	// Make sure we start with a clean slate
	VolumesFree();
	ExtentsFree();
	PoolsFree();
	CapabilitiesFree();
	SettingsFree();

	// Read primordial pool/extent/capability information from our
	// persistent store
	rc = LoadPersistentStorageObjects(ns);
	if (rc != 0)
	{
		_SMI_TRACE(0,("Error %d loading persistent storage objects", rc));
		return rc;
	}
	numPools = PoolsSize();
	if (numPools == 0)
	{
		// If we have no pools we don't need to do anything more.
		_SMI_TRACE(1,("No StoragePools found"));
		return 0;
	} 
	
	// Initialize EVMS
	if (!EVMSopen)
	{
		rc = OpenSession();
		if (rc != 0)
		{
			_SMI_TRACE(0,("Error %d opening the evms engine", rc));
			return rc;
		}
		closeEVMS = TRUE;
		_SMI_TRACE(1,("Engine open"));
	}

	_SMI_TRACE(1,("Found %d hosted primordial pools on this computer", numPools));
	for (i = 0; i < numPools; i++)
	{
		pool = PoolsGet(i);
		if (pool)
		{
			_SMI_TRACE(1,("Examining pool %x:%s", pool, pool->instanceID));
			
			// Scan all concrete component extents for this primordial pool
			CMPICount numComps = PoolExtentsSize(pool);
			CMPICount numAvailableComps = numComps;
			if (numComps > 0)
			{
				_SMI_TRACE(1,("Found %d concrete components for pool %s", numComps, pool->instanceID));

				// Recalculate pool sizes on every scan in case things have changed
				pool->totalSize = 0;
				pool->remainingSize = 0;

				CMPICount j;
				for (j = 0; j < numComps; j++)
				{
					StorageExtent *ce = pool->concreteComps[j];
					_SMI_TRACE(1,("Examining extent %s", ce->deviceID));

					object_handle_t evmsHandle = SCSGetEVMSObject(ce->name);
					handle_object_info_t *objectInfo = NULL;

					rc = evms_get_info(evmsHandle, &objectInfo);
					if (rc != 0)
					{
						_SMI_TRACE(0,("Error calling evms_get_info for primordial extent"));
						ce->estatus = ESTAT_UNKNOWN;
						ce->ostatus = OSTAT_ERROR;
						ce->consumableBlocks = 0;
						ce->numberOfBlocks = 0;
						pool->ostatus = OSTAT_DEGRADED;
						continue;
					}

					CMPIUint64 blockSize = objectInfo->info.disk.geometry.block_size;
					CMPIUint64 numBlocks = (objectInfo->info.disk.size * 512) / blockSize;
					_SMI_TRACE(1,("Extent blkSize = %llu, numBlocks = %llu", blockSize, numBlocks));

					ce->blockSize = blockSize;
					ce->numberOfBlocks = numBlocks;
					ce->consumableBlocks = numBlocks;
					pool->totalSize += (numBlocks * blockSize);
					pool->remainingSize += (numBlocks * blockSize);

					// See if this device has been consumed by a concrete pool (i.e. has LVM2 container)
					if (objectInfo->info.disk.consuming_container != 0)
					{
						handle_object_info_t *contInfo = NULL;
						rc = evms_get_info(objectInfo->info.disk.consuming_container, &contInfo);
						if (rc != 0)
						{
							_SMI_TRACE(1,("Error calling evms_get_info for primordial extent consuming container"));
							continue;
						}

						rc = EVMSProcessContainer(ce, pool->capability, contInfo);

						evms_free(contInfo);
						if (rc != 0)
						{
							_SMI_TRACE(1,("Error calling evmsProcessContainer for primordial extent consuming container"));
							continue;
						}
					}
					else if (objectInfo->info.disk.parent_objects->count > 0)
					{
						// Handle parent objects
						uint numParents = objectInfo->info.disk.parent_objects->count;
						_SMI_TRACE(1,("Device has %d parents", numParents));
	
						uint k;
						for (k = 0; k < numParents; k++)
						{
							handle_object_info_t *parentInfo = NULL;
							rc = evms_get_info(objectInfo->info.disk.parent_objects->handle[k], &parentInfo);
							if (rc != 0)
							{
								_SMI_TRACE(1,("Error calling evms_get_info for primordial extent parent object"));
								continue;
							}
							if (parentInfo->type == REGION)
							{
								_SMI_TRACE(1,("Device parent is a REGION, name = %s", parentInfo->info.region.name));

								rc = EVMSProcessRegion(ce, parentInfo);

								numAvailableComps--;
							}
							else if (parentInfo->type == SEGMENT)
							{
								_SMI_TRACE(1,("Device parent is a SEGMENT, name = %s", parentInfo->info.segment.name));
								_SMI_TRACE(1,("Segment data_type = %d", parentInfo->info.segment.data_type));

								rc = EVMSProcessSegment(ce, parentInfo);

								if (parentInfo->info.segment.data_type == FREE_SPACE_TYPE)
								{
									if ((parentInfo->info.segment.size * 512) <  MIN_VOLUME_SIZE)
									{
										_SMI_TRACE(1,("Device with parent segments has no available freespace"));
										numAvailableComps--;
									}
								}
							}
							else
							{
								rc = 0;
							}
							evms_free(parentInfo);
							if (rc != 0)
							{
								_SMI_TRACE(1,("Error calling evmsProcess<parent> for primordial extent parent object"));
								continue;
							}
						}
					}
					evms_free(objectInfo);
				}

				// Check for bogus case of remainingSpace > totalSpace, in this case set Remaining to 0
				if (pool->totalSize < pool->remainingSize)
				{
					_SMI_TRACE(1,("Pool RemainingSize (%llu) is greater than TotalSize (%llu), setting RemainingSize to ZERO!!!", pool->remainingSize, pool->totalSize));
					pool->remainingSize = 0;
				}

				_SMI_TRACE(1,("Pool sizes: Total = %llu, Remaining = %llu", pool->totalSize, pool->remainingSize));

				// Also fixup the pool capabilities
				_SMI_TRACE(1,("Update capabilities, numAvailableComps = %d", numAvailableComps));
				if (numAvailableComps > 1 || pool->capability->parity == 2)
				{
					pool->capability->noSinglePointOfFailure = 1;
				}
				else
				{
					pool->capability->noSinglePointOfFailure = 0;
				}

				if (numAvailableComps > 0)
				{
					pool->capability->dataRedundancyMax = numAvailableComps;
					pool->capability->packageRedundancyMax = numAvailableComps-1;
				}
			}
			else
			{
				_SMI_TRACE(0,("NO concrete components for pool %s!!!", pool->name));
			}
		}
		else
		{
			_SMI_TRACE(0,("Found NULL pool in pools array!!!"));
		}
	}

	if (closeEVMS)
	{
		CloseSession();
		_SMI_TRACE(1,("Engine closed"));
	}			

	_SMI_TRACE(1,("SCSScanStorage() done, rc = %d\n", rc));
	SCSNeedToScan = 0;
	return rc;
}
*/


int SCSScanStorage(const char *ns, CMPIStatus *pStatus)
{
	int rc = 0;
	CMPICount i, numPools;
	StoragePool *pool;
	const char *deviceID = NULL;
	char nameBuf[256];
	char diskID[10];

	_SMI_TRACE(1,("\nSCSScanStorage() called"));


	// Make sure we start with a clean slate
	VolumesFree();
	ExtentsFree();
	PoolsFree();
	CapabilitiesFree();
	SettingsFree();

	// Read primordial pool/extent/capability information from our
	// persistent store
	rc = LoadPersistentStorageObjects(ns);
	if (rc != 0)
	{
		_SMI_TRACE(0,("\nError:Configuration file smsetup.conf not found under /etc directory."));
		_SMI_TRACE(0,("A sample file is available at /usr/share/doc/packages/<package>. Refer to README for more details\n"));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
			"Configuration file smsetup.conf not found under /etc directory. A sample file is available at /usr/share/doc/packages/<package>. Refer to README for more details");
		return rc;
	}
	numPools = PoolsSize();
	if (numPools == 0)
	{
		// If we have no pools we don't need to do anything more.
		_SMI_TRACE(1,("No StoragePools found"));
		return 0;
	} 

	_SMI_TRACE(1,("Found %d hosted primordial pools on this computer", numPools));
	for (i = 0; i < numPools; i++)
	{
		pool = PoolsGet(i);
		if (pool)
		{
			_SMI_TRACE(1,("Examining pool %x:%s", pool, pool->instanceID));
			
			// Scan all concrete component extents for this primordial pool
			CMPICount numComps = PoolExtentsSize(pool);
			CMPICount numAvailableComps = numComps;
			if (numComps > 0)
			{
				_SMI_TRACE(1,("Found %d concrete components for pool %s", numComps, pool->instanceID));

				// Recalculate pool sizes on every scan in case things have changed
				pool->totalSize = 0;
				pool->remainingSize = 0;

				CMPICount j;
				for (j = 0; j < numComps; j++)
				{
					StorageExtent *ce = pool->concreteComps[j];
					//Generate device name for the extent by prefixing "/dev/" to its name
					deviceID = ExtentCreateDeviceID(ce->deviceID);
					_SMI_TRACE(1,("Examining extent %s", deviceID));

					DiskInfo diskInfo;
					ContainerInfo contInfo;
					VolumeInfo volInfo;
				/*	if(deviceID.c_str() != NULL)
					{
						deviceID.append("/dev/");
						deviceID.append(ce->name);
					}
					_SMI_TRACE(1,("DeviceID %s", deviceID.c_str()));
				*/	if (rc = s->getContDiskInfo(deviceID, contInfo, diskInfo))
					{
						_SMI_TRACE(0,("Device is not a disk. Checking if its a extended partition, rc = %d", rc));
						if(rc = s->getVolume(deviceID, volInfo))
						{
							_SMI_TRACE(0,("Error calling getInfo for primordial extent, rc = %d", rc));
							ce->estatus = ESTAT_UNKNOWN;
							ce->ostatus = OSTAT_ERROR;
							pool->ostatus = OSTAT_DEGRADED;
							ce->size = 0;
							continue;
						}
						else
						{
							deque<PartitionInfo> partInfoList;
							ce->size = volInfo.sizeK * oneKB; 
							pool->totalSize += (volInfo.sizeK * oneKB);
							pool->remainingSize += (volInfo.sizeK * oneKB);
							_SMI_TRACE(0,("Pool total size: %d, remaining size: %d", pool->totalSize, pool->remainingSize));
							// See if this device is an extended partition
//							_SMI_TRACE(1,("Checking if %s is an extended partition", deviceID.c_str()));
							_SMI_TRACE(1,("Checking if %s is an extended partition", deviceID));
							if(!IsExtendedPartition(deviceID, diskID))
							{
								_SMI_TRACE(0,("Error: Primordial extent is not a extended partition, rc = %d", rc));
								continue;
							}
							else
							{
								sprintf(nameBuf, "%s%s", ce->name, "Freespace");
								StorageExtent *remaining = ExtentsFind(nameBuf);
								if (remaining == NULL)
								{
									_SMI_TRACE(1,("No remaining extent created yet for segmented disk, do 1st time init"));
									remaining = ExtentAlloc(nameBuf, nameBuf);
									ExtentsAdd(remaining);
		
									remaining->size = ce->size;
									remaining->pool = ce->pool;
									PoolExtentsAdd(ce->pool, remaining);
				
									ExtentCreateBasedOnAssociation(
													remaining,
													ce,
													0,
													ce->size - 1,
													1);
								}

								//Handling logical partitions
								_SMI_TRACE(1,("Handling logical partitions of %s", diskID));
								if(rc = s->getPartitionInfo(diskID, partInfoList))
								{
									_SMI_TRACE(0,("Error getting info on extended partition's parent disk, rc = %d", rc));
									continue;
								}
   								for(deque<PartitionInfo>::iterator i=partInfoList.begin(); i!=partInfoList.end(); ++i )
								{
									if(i->partitionType == LOGICAL)
									{
										PartitionInfo partInfo = *i;
										if(rc = ProcessPartition(ce, partInfo))
										{	
											_SMI_TRACE(1,("Error calling ProcessPartition for primordial extent parent object, rc = %d", rc));
											continue;
										}
									}	
								}
							}
				/*			if (volInfo.usedBy == (UsedByType)UB_LVM)
							{
								_SMI_TRACE(0,("Pool consumed by container %s", volInfo.usedByName.c_str()));
								ContainerInfo vg_contInfo;
								LvmVgInfo vgInfo;
								if(rc = s->getContLvmVgInfo(volInfo.usedByName, vg_contInfo, vgInfo))
								{
									_SMI_TRACE(1,("Error calling getLvmVgInfo for primordial extent consuming container, rc = %d", rc));
									continue;
								}
								rc = ProcessContainer(ce, pool->capability, vg_contInfo, vgInfo);
		
								if (rc != 0)
								{
									_SMI_TRACE(1,("Error calling ProcessContainer for primordial extent consuming container, rc =%d", rc));
									continue;
								}
							}
							else
							{
				
								_SMI_TRACE(1,("Handling logical partitions"));
								deque<PartitionInfo> partInfoList;
								char *disk;
								volInfo.name.copy(disk, 3);
								if(rc = s->getPartitionInfo(disk, partInfoList))
								{
									_SMI_TRACE(1,("Error calling getDiskInfo for primordial extent parent object, rc =%d", rc));
									continue;
								}
								sprintf(nameBuf, "%s%s", ce->name, "Freespace");
								StorageExtent *remaining = ExtentsFind(nameBuf);
								if (remaining == NULL)
								{
									_SMI_TRACE(1,("No remaining extent created yet for segmented disk, do 1st time init"));
									remaining = ExtentAlloc(nameBuf, nameBuf);
									ExtentsAdd(remaining);
		
									remaining->size = ce->size;
									remaining->pool = ce->pool;
									PoolExtentsAdd(ce->pool, remaining);
				
									ExtentCreateBasedOnAssociation(
													remaining,
													ce,
													0,
													ce->size - 1,
													1);
								}
   								for(deque<PartitionInfo>::iterator i=partInfoList.begin(); i!=partInfoList.end(); ++i )
								{
									if(i->partitionType == LOGICAL)
									{
										PartitionInfo partInfo = *i;
										if(rc = ProcessPartition(ce, partInfo))
										{	
											_SMI_TRACE(1,("Error calling ProcessPartition for primordial extent parent object, rc = %d", rc));
											continue;
										}
									}	
								}
							}
					*/	}
					}
					else
					{
						ce->size = diskInfo.sizeK * oneKB; 
						pool->totalSize += (diskInfo.sizeK * oneKB);
						pool->remainingSize += (diskInfo.sizeK * oneKB);
						_SMI_TRACE(0,("Pool total size: %llu, remaining size: %llu", pool->totalSize, pool->remainingSize));
						// See if this device has been consumed by a concrete pool (i.e. has container)
						if (contInfo.usedByType == UB_LVM)
						{
							_SMI_TRACE(0,("Pool consumed by container %s", contInfo.usedByName.c_str()));
							ContainerInfo vg_contInfo;
							LvmVgInfo vgInfo;
							if(rc = s->getContLvmVgInfo(contInfo.usedByName, vg_contInfo, vgInfo))
							{
								_SMI_TRACE(1,("Error calling getLvmVgInfo for primordial extent consuming container, rc = %d", rc));
								continue;
							}
							rc = ProcessContainer(ce, pool->capability, vg_contInfo, vgInfo);
	
							if (rc != 0)
							{
								_SMI_TRACE(1,("Error calling ProcessContainer for primordial extent consuming container, rc =%d", rc));
								continue;
							}
						}
						else 
						{
							// Handle disk partitions
							_SMI_TRACE(1,("Handling disk partitions for %s", ce->name));
							deque<PartitionInfo> partInfoList;
							if(rc = s->getPartitionInfo(deviceID, partInfoList))
							{
								_SMI_TRACE(1,("Error calling getDiskInfo for primordial extent parent object, rc = %d", rc));
								continue;
							}
							_SMI_TRACE(0,("Disk has %d partitions", partInfoList.size()));
							sprintf(nameBuf, "%s%s", ce->name, "Freespace");
							StorageExtent *remaining = ExtentsFind(nameBuf);
							if (remaining == NULL)
							{
								_SMI_TRACE(1,("No remaining extent created yet for segmented disk, do 1st time init"));
								remaining = ExtentAlloc(nameBuf, nameBuf);
								ExtentsAdd(remaining);

								remaining->size = ce->size;
								remaining->pool = ce->pool;
								PoolExtentsAdd(ce->pool, remaining);
			
								ExtentCreateBasedOnAssociation(
												remaining,
												ce,
												0,
												ce->size - 1,
												1);
							}
	   						for(deque<PartitionInfo>::iterator i=partInfoList.begin(); i!=partInfoList.end(); ++i )
							{
								PartitionInfo partInfo = *i;
								if(partInfo.partitionType != EXTENDED)
								{
									if(rc = ProcessPartition(ce, partInfo))
									{	
										_SMI_TRACE(1,("Error calling ProcessPartition for primordial extent parent object, rc = %d", rc));
										continue;
									}	
								}
							}
						}
					}
				}
					
				// Check for bogus case of remainingSpace > totalSpace, in this case set Remaining to 0
				if (pool->totalSize < pool->remainingSize)
				{
					_SMI_TRACE(1,("Pool RemainingSize (%llu) is greater than TotalSize (%llu), setting RemainingSize to ZERO!!!", pool->remainingSize, pool->totalSize));
					pool->remainingSize = 0;
				}

				_SMI_TRACE(1,("Pool sizes: Total = %llu, Remaining = %llu", pool->totalSize, pool->remainingSize));

				// Also fixup the pool capabilities
				_SMI_TRACE(1,("Update capabilities, numAvailableComps = %d", numAvailableComps));
				if (numAvailableComps > 1 || pool->capability->parity == 2)
				{
					pool->capability->noSinglePointOfFailure = 1;
				}
				else
				{
					pool->capability->noSinglePointOfFailure = 0;
				}

				if (numAvailableComps > 0)
				{
					pool->capability->dataRedundancyMax = numAvailableComps;
					pool->capability->packageRedundancyMax = numAvailableComps-1;
				}
			}
			else
			{
				_SMI_TRACE(0,("NO concrete components for pool %s!!!", pool->name));
			}
		}
		else
		{
			_SMI_TRACE(0,("Found NULL pool in pools array!!!"));
		}
	}
	_SMI_TRACE(1,("SCSScanStorage() done, rc = %d\n", rc));
	SCSNeedToScan = 0;
//	if(deviceID.c_str())
//		deviceID.clear();
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
void StorageInterfaceFree()
{
	_SMI_TRACE(1,("StorageInterfacefree() called"));
	int s_rc = 0;
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d\n", s_rc));
	_SMI_TRACE(1,("Deleting Yast2 StorageInterface"));
	destroyStorageInterface(s);
}
