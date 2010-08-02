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
 |***************************************************************************
 |
 |	 OMC SMI-S Volume Management provider include file
 |
 |---------------------------------------------------------------------------
 |
 | $Id:
 |
 +-------------------------------------------------------------------------*/
#ifndef STORAGEEXTENT_H_
#define STORAGEEXTENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EXTENT_ARRAY_SIZE_DEFAULT 64
#define EXTENT_BASED_ON_ARRAY_SIZE_DEFAULT 16

typedef struct _BasedOn
{
	struct _StorageExtent *extent;
	struct _StorageVolume *volume;
	CMPIUint64 startingAddress;
	CMPIUint64 endingAddress;
	CMPIUint16 orderIndex;
} BasedOn;

typedef struct _StorageExtent
{
	char					*name;
	char					*deviceID;
	CMPIBoolean				primordial;
	CMPIBoolean				composite;
	CMPIUint64				size;
/*	CMPIUint64				blockSize;
	CMPIUint64				numberOfBlocks;
	CMPIUint64				consumableBlocks;
*/	CMPIUint16				estatus;
	CMPIUint16				ostatus;

	struct _StoragePool*	pool;
	BasedOn**				antecedents;
	CMPICount				numAntecedents;
	CMPICount				maxNumAntecedents;
	BasedOn**				dependents;
	CMPICount				numDependents;
	CMPICount				maxNumDependents;
} StorageExtent;

//
// Exports
//

// Extent management
extern StorageExtent*		ExtentAlloc(
								const char *name, 
								const char *deviceID);

extern void					ExtentFree(
								StorageExtent *extent);

extern char*				ExtentGenerateName(
								const char *prefix, 
								char *nameBuf, 
								const CMPICount bufSize);

extern CMPIInstance*		ExtentCreateInstance(
								StorageExtent *extent, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIObjectPath*		ExtentCreateObjectPath(
								StorageExtent *extent, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*		ExtentCreateBasedOnAssocInstance(
								StorageExtent *dependent,
								StorageExtent *antecedent,
								BasedOn *basedOn,
								const char *ns, 
								const char **properties,
								const char *className,
								CMPIStatus *status);

extern CMPIObjectPath*		ExtentCreateBasedOnAssocObjectPath(
								StorageExtent *dependent,
								StorageExtent *antecedent,
  								const char *ns, 
								const char *className,
								CMPIStatus *status);

extern CMPIBoolean			ExtentIsFreespace(
								StorageExtent *extent);

extern void					ExtentGetContainerName(
								StorageExtent *extent, 
								char *nameBuf, 
								const CMPICount bufSize);

extern struct _StoragePool*	ExtentGetPool(
								StorageExtent *extent);

extern CMPIUint32			ExtentPartition(
								StorageExtent *extent, 
								CMPIUint64 partSize, 
								StorageExtent **partExtent,
								StorageExtent **remExtent);

extern CMPIUint32			ExtentUnpartition(
								StorageExtent *extent);

// Extent BasedOn Association array manipulation
extern void	ExtentCreateBasedOnAssociation(
				StorageExtent *dependent,
				StorageExtent *antecedent,
				CMPIUint64 startingAddress,
				CMPIUint64 endingAddress,
				CMPIUint16 orderIndex);

// Extents array manipulation
extern CMPICount		ExtentsSize();
extern StorageExtent*	ExtentsGet(const CMPICount index);
extern StorageExtent*	ExtentsFind(const char *deviceID);
extern void				ExtentsAdd(StorageExtent *extent);
extern void				ExtentsRemove(StorageExtent *extent);
extern void				ExtentsFree();
extern void				ExtentsSortBySize(StorageExtent **extArray, CMPICount extArraySize);
extern char* 		ExtentCreateDeviceID(const char *name);



#ifdef __cplusplus
}
#endif

#endif /*STORAGEEXTENT_H_*/
