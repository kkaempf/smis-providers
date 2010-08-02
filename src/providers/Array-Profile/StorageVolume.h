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
#ifndef STORAGEVOLUME_H_
#define STORAGEVOLUME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "StorageExtent.h"
#include "StoragePool.h"

#define VOLUME_ARRAY_SIZE_DEFAULT 64
	
typedef struct _StorageVolume
{
	char				*name;
	char				*deviceID;
	CMPIUint64			size;
/*	CMPIUint64			blockSize;
	CMPIUint64			numberOfBlocks;
	CMPIUint64			consumableBlocks;
*/	CMPIUint16			estatus;
	CMPIUint16			ostatus;
						
	StorageSetting			*setting;
	AllocFrom			antecedentPool;
	BasedOn				antecedentExtent;
	bool				IsLD;
} StorageVolume;

// Exports

extern StorageVolume*	VolumeAlloc(const char *name, const char *deviceID);
extern void				VolumeFree(StorageVolume *volume);
extern char*			VolumeGenerateName(
								const char *prefix, 
								char *nameBuf, 
								const CMPICount bufSize);

extern void				VolumeCreateAllocFromAssociation(
								StoragePool *antecedentPool,
								StorageVolume *dependentVolume,
								CMPIUint64 spaceConsumed);

extern void				VolumeCreateBasedOnAssociation(
								StorageVolume *dependent,
								StorageExtent *antecedent,
								CMPIUint64 startingAddress,
								CMPIUint64 endingAddress,
								CMPIUint16 orderIndex);

extern CMPIInstance*	VolumeCreateInstance(
								const char *classname,
								StorageVolume *volume, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIObjectPath*	VolumeCreateObjectPath(
								const char *classname,
								StorageVolume *volume, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	VolumeCreateDeviceAssocInstance(
								const char *classname,
								StorageVolume *vol,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	VolumeCreateDeviceAssocObjectPath(
								const char *classname,
								StorageVolume *vol,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	VolumeCreateBasedOnAssocInstance(
								const char *classname,
								StorageVolume *dependent,
								StorageExtent *antecedent,
								BasedOn *basedOn,
								const char *ns, 
								const char **properties,
								CMPIStatus *status);

extern CMPIObjectPath*	VolumeCreateBasedOnAssocObjectPath(
								const char *classname,
								StorageVolume *dependent,
								StorageExtent *antecedent,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	VolumeCreateAllocFromAssocInstance(
								const char *classname,
								StorageVolume *dependentVolume,
								StoragePool *antecedentPool,
								CMPIUint64 spaceConsumed,
  								const char *ns, 
								const char **properties,
								CMPIStatus *status);

extern CMPIObjectPath*	VolumeCreateAllocFromAssocObjectPath(
								const char *classname,
								StorageVolume *dependentVolume,
								StoragePool *antecedentPool,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	VolumeCreateSettingAssocInstance(
								const char *classname,
								StorageVolume *vol,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	VolumeCreateSettingAssocObjectPath(
								const char *classname,
								StorageVolume *vol,
  								const char *ns, 
								CMPIStatus *status);

extern CMPICount		VolumesSize();
extern StorageVolume*	VolumesGet(const CMPICount index);
extern StorageVolume*	VolumesFind(const char *deviceID);
extern void				VolumesAdd(StorageVolume *volume);
extern void				VolumesRemove(StorageVolume *volume);
extern void				VolumesFree();



#ifdef __cplusplus
}
#endif

#endif /*STORAGEVOLUME_H_*/
