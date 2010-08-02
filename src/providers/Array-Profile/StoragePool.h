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
#ifndef STORAGEPOOL_H_
#define STORAGEPOOL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "StorageCapability.h"
#include "StorageExtent.h"

#define POOL_ARRAY_SIZE_DEFAULT	16
#define POOL_EXTENT_ARRAY_SIZE_DEFAULT 16
#define POOL_ALLOC_FROM_ARRAY_SIZE_DEFAULT 16

enum getSupportedSizesRetCodes_e
{
	GSS_COMPLETED_OK					= 0,
	GSS_NOT_SUPPORTED					= 1,
	GSS_USE_GET_SUPPORTED_SIZE_RANGE	= 2
};

enum getSupportedSizeRangeRetCodes_e
{
	GSSR_COMPLETED_OK				= 0,
	GSSR_NOT_SUPPORTED				= 1,
	GSSR_USE_GET_SUPPORTED_SIZES	= 2
};

enum getSupportedSizeElementTypes
{
	GSS_STORAGE_POOL			= 2,	
	GSS_STORAGE_VOLUME			= 3,	
	GSS_LOGICAL_DISK			= 4
};

enum getAvailableExtentsRetCodes_e
{
	GAE_COMPLETED_OK	= 0,
	GAE_NOT_SUPPORTED	= 1,
	GAE_UNKNOWN			= 2,
	GAE_TIMEOUT			= 3,
	GAE_FAILED			= 4,
	GAE_INVALID_PARAM	= 5,
	GAE_IN_USE			= 6
};
	
typedef struct _AllocFrom
{
	void*		element;
	CMPIUint64	spaceConsumed;
} AllocFrom;

typedef struct _StoragePool
{
	char*				name;
	char*				instanceID;
	CMPIBoolean			primordial;
	CMPIUint64			totalSize;
	CMPIUint64			remainingSize;
	CMPIUint16			ostatus;
	StorageCapability*	capability;

	StorageExtent**		concreteComps;
	CMPICount			numComps;
	CMPICount			maxNumComps;

	AllocFrom**			dependentPools;
	CMPICount			numDepPools;
	CMPICount			maxDepPools;

	AllocFrom**			antecedentPools;
	CMPICount			numAntPools;
	CMPICount			maxAntPools;

	AllocFrom**			dependentVolumes;
	CMPICount			numDepVolumes;
	CMPICount			maxDepVolumes;
} StoragePool;

// Exports

// Pool management
extern StoragePool*		PoolAlloc(const char *poolName);
extern void				PoolFree(StoragePool *pool);
extern char*			PoolGenerateName(const char *prefix, char *nameBuf, CMPICount bufSize);

/*extern CMPIUint32		PoolGetShrinkExtents(
								StoragePool *pool,
								const object_handle_t evmsContHandle,
								CMPIUint64 *shrinkSize,
								CMPIUint64 *excessShrinkage,
								handle_array_t **shrinkHandles);
*/
extern void				PoolCreateAllocFromAssociation(
								StoragePool *antecedentPool,
								StoragePool *dependentPool,
								CMPIUint64 spaceConsumed);

extern void				PoolModifyAllocFromAssociation(
								StoragePool *antecedent,
								StoragePool *dependent,
								CMPIUint64 spaceConsumedIncrement,
								CMPIUint64 spaceConsumedDecrement);

extern CMPIInstance*	PoolCreateInstance(
								StoragePool *pool, 
								const char *ns,
								CMPIStatus *status);

extern CMPIObjectPath*	PoolCreateObjectPath(
								StoragePool *pool, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	PoolCreateHostedAssocInstance(
								StoragePool *pool,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	PoolCreateHostedAssocObjectPath(
								StoragePool *pool,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	PoolCreateCapabilityAssocInstance(
								StoragePool *pool,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	PoolCreateCapabilityAssocObjectPath(
								StoragePool *pool,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	PoolCreateComponentAssocInstance(
								StoragePool *pool,
								StorageExtent *extent,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	PoolCreateComponentAssocObjectPath(
								StoragePool *pool,
								StorageExtent *extent,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	PoolCreateRemainingAssocInstance(
								StoragePool *pool,
								StorageExtent *extent,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	PoolCreateRemainingAssocObjectPath(
								StoragePool *pool,
								StorageExtent *extent,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	PoolCreateAllocFromAssocInstance(
								const char *classname,
								StoragePool *dependentPool,
								StoragePool *antecedentPool,
								CMPIUint64 spaceConsumed,
  								const char *ns, 
								const char **properties,
								CMPIStatus *status);

extern CMPIObjectPath*	PoolCreateAllocFromAssocObjectPath(
								const char *classname,
								StoragePool *dependentPool,
								StoragePool *antecedentPool,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIBoolean		PoolDefaultCapabilitiesMatch(	
								StoragePool *poolA,
								StoragePool *poolB);

extern CMPIBoolean		PoolGoalMatchesDefault(	
								StoragePool *pool,
								StorageSetting *goal);

extern void				PoolInvokeMethod(
								StoragePool *pool,
								const char *ns,
								const char *methodName,
								const CMPIArgs *in,
								CMPIArgs *out,
								const CMPIResult* results,
								CMPIStatus *pStatus);

#define PoolGetSupportedSizeRange GetSupportedSizeRange
#define PoolGetAvailableExtents GetAvailableExtents

// Pools array manipulation
extern CMPICount		PoolsSize();
extern StoragePool*		PoolsGet(const CMPICount index);
extern StoragePool*		PoolsFind(const char *instanceID);
extern StoragePool*		PoolsFindByGoal(StorageSetting *goal, CMPIUint64 *goalSize);
extern StoragePool*		PoolsFindByName(const char *name);
extern void				PoolsAdd(StoragePool *pool);
extern void				PoolsRemove(StoragePool *pool);
extern void				PoolsFree();

// Pool ConcreteComponent extents array manipulation
extern CMPICount		PoolExtentsSize(StoragePool *pool);
extern StorageExtent*	PoolExtentsGet(StoragePool *pool, const CMPICount index);
extern StorageExtent*	PoolExtentsFind(StoragePool *pool, const char *deviceID);
extern void				PoolExtentsAdd(StoragePool *pool, StorageExtent *extent);
extern void				PoolExtentsRemove(StoragePool *pool, StorageExtent *extent);
extern void				PoolExtentsFree(StoragePool *pool);

// Methods called by invokeMethod
extern CMPIUint32		GetSupportedSizeRange(
								StoragePool *pool,
								StorageSetting *goal,
								CMPIUint64 *minSize,
								CMPIUint64 *maxSize,
								CMPIUint64 *sizeDivisor);

extern CMPIUint32		GetAvailableExtents(
								StoragePool *pool,
								StorageSetting *goal,
								StorageExtent **availExtents,
								CMPICount *numAvailExtents);


#ifdef __cplusplus
}
#endif

#endif /*STORAGEPOOL_H_*/
