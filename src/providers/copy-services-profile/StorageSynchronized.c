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
 |********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#ifdef __cplusplus
}
#endif

#include "CopyServicesProvider.h"
#include "StorageSynchronized.h"
//#include "LvmCopyServices.h"
#include "Array-Profile/LvmCopyServices.h"
 
void  OMC_EnumSSObjectPaths(
		const char *className,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		CMPIStatus *rc)
{
	printf("className : %s\n",className);
	DebugMsg("Inside EnumSSObjectPaths\n");
	CMPIObjectPath *oPath=NULL, *ssOP=NULL;
	CMPIEnumeration *en=NULL, *en2=NULL;
	CMPIData ldOPData, ldOPData2, keyData, keyData2;
	char **snapshot_volumes_list=NULL; 
	int no_of_volumes=0, rv, i, k;

	oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), className, rc);
	DebugMsg("Inside EnumSSObjectPaths 1\n");
	if(CMIsNullObject(oPath))
	{
		DebugMsg("Unable to create Logical Disk/StorageVolume Object Path\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	DebugMsg("Inside EnumSSObjectPaths 2\n");
	en = CBEnumInstanceNames(broker, ctx, oPath, rc); 
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to enumInstanceName LD\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		}
//	en = broker->bft->enumerateInstanceNames(broker, ctx, oPath, rc);
/*	if(broker->bft->enumerateInstanceNames)
	{
		en = CBEnumerateInstanceNames(broker, ctx, oPath, rc); 
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to enumInstanceName LD\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		}
	}
	else
	{
		DebugMsg("broker->bft->enumerateInstanceNames NULL\n");
	
		return ;
	} 
*/
	DebugMsg("Inside EnumSSObjectPaths 3\n");
	while(CMHasNext(en, rc))
	{
		DebugMsg("Inside While\n");
		ldOPData = CMGetNext(en, rc);
		keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, rc);
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get key for LD\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		} 
                no_of_volumes=0;
		rv = cs_return_associated_snapshot_volumes(CMGetCharPtr(keyData.value.string), 
				&snapshot_volumes_list, &no_of_volumes);
		if(rv)
		{
			DebugMsg("Error in getting associated snapshot volumes\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		}
		else if(no_of_volumes > 0)
		{
			en2 = broker->bft->enumerateInstanceNames(broker, ctx, oPath, rc);
			if(rc->rc != CMPI_RC_OK)
			{
				DebugMsg("Unable to enumInstanceName LD\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			while(CMHasNext(en2, rc))
			{
				ldOPData2 = CMGetNext(en2, rc);
				keyData2 = CMGetKey(ldOPData2.value.ref, DeviceIDProperty, rc);

				if(rc->rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to get key for LD\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 

				for(k=0; k<no_of_volumes; k++)
				{
					if(!strcasecmp(CMGetCharPtr(keyData2.value.string), snapshot_volumes_list[k]))
					{
						if (strcasecmp( className, LogicalDiskClassName) == 0)
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
								StorageSynchronizedAssnName, rc);
						else if (strcasecmp( className, StorageVolumeClassName) == 0)
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
								SVStorageSynchronizedAssnName, rc);
						
						if(CMIsNullObject(ssOP))
						{
							DebugMsg("Unable to create SS Object Path\n");
							CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
							goto Exit;
						}
						CMAddKey(ssOP, SystemElementProperty, &ldOPData.value.ref, CMPI_ref);
						CMAddKey(ssOP, SyncedElementProperty, &ldOPData2.value.ref, CMPI_ref);
						CMReturnObjectPath(rslt, ssOP);
						CMRelease(ssOP);
					}
				}
			}

			if(snapshot_volumes_list)
			{
				for(i=0; i<no_of_volumes; i++)
					free(snapshot_volumes_list[i]);
				free(snapshot_volumes_list);
				snapshot_volumes_list = 0;
			}
 		}
	}
	DebugMsg("Inside EnumSSObjectPaths 5\n");

Exit:
	if(snapshot_volumes_list)
	{
		for(i=0; i<no_of_volumes; i++)
			free(snapshot_volumes_list[i]);
		free(snapshot_volumes_list);
		snapshot_volumes_list = 0;
	}

 	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en);
 	if(en2)
		CMRelease(en2);
       	return;
}


void OMC_EnumSSInstances(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		const char** properties,
		CMPIStatus *rc)
{
	DebugMsg("Inside EnumSSInstances\n");
	printf ("className : %s\n",className);
	CMPIObjectPath *oPath=NULL, *ssOP=NULL;
	CMPIEnumeration *en=NULL, *en2=NULL;
	CMPIData ldOPData, ldOPData2, keyData, keyData2;
	CMPIInstance *inst=NULL;
	char **snapshot_volumes_list=NULL; 
	int no_of_volumes=0, rv, i, k;
	//Vijay
	//broker->bft->enumerateInstanceNames = 0x2b6db2483EA3;	

	oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), className, rc);
	if(CMIsNullObject(oPath))
	{
		DebugMsg("Unable to create Logical Disk Object Path\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	DebugMsg("Inside EnumSSInstances 1\n");
	//en = broker->bft->enumerateInstanceNames(broker, ctx, oPath, rc);
		/* Get the list of all target class object paths from the CIMOM. */ 
	en = CBEnumInstanceNames(broker, ctx, oPath, NULL); 
//	if(rc->rc != CMPI_RC_OK)
	if (CMIsNullObject(en))
	{
		DebugMsg("Unable to enumInstanceName LD\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	/*if( broker->bft->enumerateInstanceNames )
	{

		en = CBEnumInstanceNames(broker, ctx, oPath, rc); 
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to enumInstanceName LD\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		}
	}
	else
	{
			DebugMsg("broker->bft->enumerateInstanceNames\n");
			return;
	} */
		
	while(CMHasNext(en, rc))
	{
		DebugMsg("Inside While\n");
		ldOPData = CMGetNext(en, rc);
		keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, rc);
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get key for LD\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		} 
                
		no_of_volumes=0;
		rv = cs_return_associated_snapshot_volumes(CMGetCharPtr(keyData.value.string), 
				&snapshot_volumes_list, &no_of_volumes);
		if(rv)
		{
			DebugMsg("Error in getting associated snapshot volumes\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
		}
		else if(no_of_volumes > 0)
		{
			en2 = broker->bft->enumerateInstanceNames(broker, ctx, oPath, rc);
			if(rc->rc != CMPI_RC_OK)
			{
				DebugMsg("Unable to enumInstanceName LD\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			while(CMHasNext(en2, rc))
			{
				ldOPData2 = CMGetNext(en2, rc);
				keyData2 = CMGetKey(ldOPData2.value.ref, DeviceIDProperty, rc);

				if(rc->rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to get key for LD\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 

				for(k=0; k<no_of_volumes; k++)
				{
					if(!strcasecmp(CMGetCharPtr(keyData2.value.string), snapshot_volumes_list[k]))
					{
						short val;
						
						if (strcasecmp( className, LogicalDiskClassName) == 0)
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
									StorageSynchronizedAssnName, rc);
						else if (strcasecmp( className, StorageVolumeClassName) == 0)
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
									SVStorageSynchronizedAssnName, rc);
						
						if(CMIsNullObject(ssOP))
						{
							DebugMsg("Unable to create SS Object Path\n");
							CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
							goto Exit;
						}
						inst = CMNewInstance(broker, ssOP, rc);
						if(CMIsNullObject(inst))
						{
							CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
									"Unable to create SS instance");
							DebugMsg(CMGetCharPtr(rc->msg));
							CMRelease(ssOP);
							goto Exit;
						}
						
						do{
							const char **keys;
							keys = (const char **)malloc(3 * sizeof(char*));  
							keys[0] = SystemElementProperty;
							keys[1] = SyncedElementProperty;
							keys[2] = NULL;
							CMSetPropertyFilter(inst, properties, keys);
							free(keys);
						}while(0);


						CMSetProperty(inst, SystemElementProperty, &ldOPData.value.ref, CMPI_ref);
						CMSetProperty(inst, SyncedElementProperty, &ldOPData2.value.ref, CMPI_ref);

						val = 4;
						CMSetProperty(inst, CopyTypeProperty, &val, CMPI_uint16);
						CMSetProperty(inst, ReplicaTypeProperty, &val, CMPI_uint16);

						val = 11;
						CMSetProperty(inst, SyncStateProperty, &val, CMPI_uint16);

						val = 0;
						CMSetProperty(inst, CopyPriorityProperty, &val, CMPI_uint16);        
					 
						CMReturnInstance(rslt, inst);
						CMRelease(inst);
						CMRelease(ssOP);
					}
				}
			}
			DebugMsg("outside While\n");

			if(snapshot_volumes_list)
			{
				for(i=0; i<no_of_volumes; i++)
					free(snapshot_volumes_list[i]);
				free(snapshot_volumes_list);
				snapshot_volumes_list = 0;
			}
 		}
	}

Exit:
	if(snapshot_volumes_list)
	{
		for(i=0; i<no_of_volumes; i++)
			free(snapshot_volumes_list[i]);
		free(snapshot_volumes_list);
		snapshot_volumes_list = 0;
	}

 	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en);
 	if(en2)
		CMRelease(en2);
       	return;
}
 
 

void OMC_CreateSSInstances(
		const char *className,
		const CMPIBroker *broker,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		const CMPIObjectPath *oPath, //source element
		const CMPIObjectPath *oPath2, //synced element
		char *isReturned,
		const char** properties,
		CMPIStatus *rc) 
{
	const char *name=NULL, *name2=NULL;
	CMPIObjectPath *op=NULL, *ssOP=NULL, *ldOP=NULL;
	CMPIInstance *inst=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData ldOPData, keyData;
	char **snapshot_volumes_list=NULL, *source_volume=NULL;
	int no_of_volumes = 0, rv, i, k;
	char is_snapshot_exists = 0;

	if(oPath != NULL)
		op = (CMPIObjectPath *)oPath;
	else
		op = (CMPIObjectPath *)oPath2;

	if(oPath)
		name = CMGetCharPtr(CMGetKey(oPath, DeviceIDProperty, rc).value.string);
	if(oPath2)
		name2 = CMGetCharPtr(CMGetKey(oPath2, DeviceIDProperty, rc).value.string);

	if(name == NULL && name2 == NULL)
	{
		DebugMsg("Unable to get key from LD OP\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return;
	}

	if(name && name2)
	{
		if(1) //DRBD guys must pass the local nodes case here
		{
			no_of_volumes=0;
			rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
			if(rv)
			{
				DebugMsg("Error in getting associated snapshot volumes\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				return;
			}
			else if(no_of_volumes)
			{
				for(i=0; i<no_of_volumes; i++)
				{
					if(!strcasecmp(name2, snapshot_volumes_list[i]))
					{
						is_snapshot_exists = 1;
						break;
					}
				}
				if(snapshot_volumes_list)
				{
					for(i=0; i<no_of_volumes; i++)
						free(snapshot_volumes_list[i]);
					if(snapshot_volumes_list)
						free(snapshot_volumes_list);
					snapshot_volumes_list = 0;
				}
			}

			if(is_snapshot_exists)
			{
				short val;
				if (strcasecmp( className, LogicalDiskClassName) == 0)				
					ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
						StorageSynchronizedAssnName, rc);
				else if (strcasecmp( className, StorageVolumeClassName) == 0)				
					ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
						SVStorageSynchronizedAssnName, rc);
				if(CMIsNullObject(ssOP))
				{
					DebugMsg("Unable to create SS Object Path\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return;
				}
				inst = CMNewInstance(broker, ssOP, rc);
				if(CMIsNullObject(inst))
				{
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
							"Unable to create SS instance");
					DebugMsg(CMGetCharPtr(rc->msg));
					CMRelease(ssOP);
					return;
				} 	
				
				do{
					const char **keys;
					keys = (const char **)malloc(3 * sizeof(char*));  
					keys[0] = SystemElementProperty;
					keys[1] = SyncedElementProperty;
					keys[2] = NULL;
					CMSetPropertyFilter(inst, properties, keys);
					free(keys);
				}while(0);


				CMSetProperty(inst, SystemElementProperty, &oPath, CMPI_ref);
				CMSetProperty(inst, SyncedElementProperty, &oPath2, CMPI_ref);

				val = 4;
				CMSetProperty(inst, CopyTypeProperty, &val, CMPI_uint16);
				CMSetProperty(inst, ReplicaTypeProperty, &val, CMPI_uint16);

				val = 11;
				CMSetProperty(inst, SyncStateProperty, &val, CMPI_uint16);

				val = 0;
				CMSetProperty(inst, CopyPriorityProperty, &val, CMPI_uint16);        

				CMReturnInstance(rslt, inst);
				CMRelease(inst);
				CMRelease(ssOP);
				*isReturned = 1;
			}
		}
	}
	else if(name)
	{
		if(1) //DRBD guys must pass the local nodes case here
		{
			no_of_volumes=0;
			rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
			if(rv)
			{
				DebugMsg("Error in getting associated snapshot volumes\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				return;
			}
			else if(no_of_volumes > 0)
			{
				ldOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), className, rc);
				if(CMIsNullObject(ldOP))
				{
					DebugMsg("Unable to create Logical Disk Object Path\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 				
				
				en = broker->bft->enumerateInstanceNames(broker, ctx, ldOP, rc);
				if(rc->rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to enumInstanceName LD\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				}
				while(CMHasNext(en, rc))
				{
					ldOPData = CMGetNext(en, rc);
					keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, rc);

					if(rc->rc != CMPI_RC_OK)
					{
						DebugMsg("Unable to get key for LD\n");
						CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
						goto Exit;
					} 

					for(k=0; k<no_of_volumes; k++)
					{
						if(!strcasecmp(CMGetCharPtr(keyData.value.string), snapshot_volumes_list[k]))
						{
							short val;
							if (strcasecmp( className, LogicalDiskClassName) == 0)											
								ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
									StorageSynchronizedAssnName, rc);
							else if (strcasecmp( className, StorageVolumeClassName) == 0) 										
									ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
										SVStorageSynchronizedAssnName, rc);
							if(CMIsNullObject(ssOP))
							{
								DebugMsg("Unable to create SS Object Path\n");
								CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
										"CMPI_RC_ERR_FAILED");
								goto Exit;
							}
							inst = CMNewInstance(broker, ssOP, rc);
							if(CMIsNullObject(inst))
							{
								CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
										"Unable to create SS instance");
								DebugMsg(CMGetCharPtr(rc->msg));
								CMRelease(ssOP);
								goto Exit;
							} 	
							
							do{
								const char **keys;
								keys = (const char **)malloc(3 * sizeof(char*));  
								keys[0] = SystemElementProperty;
								keys[1] = SyncedElementProperty;
								keys[2] = NULL;
								CMSetPropertyFilter(inst, properties, keys);
								free(keys);
							}while(0);

	 
							CMSetProperty(inst, SystemElementProperty, &oPath, CMPI_ref);
							CMSetProperty(inst, SyncedElementProperty, &ldOPData.value.ref, CMPI_ref);

							val = 4;
							CMSetProperty(inst, CopyTypeProperty, &val, CMPI_uint16);
							CMSetProperty(inst, ReplicaTypeProperty, &val, CMPI_uint16);

							val = 11;
							CMSetProperty(inst, SyncStateProperty, &val, CMPI_uint16);

							val = 0;
							CMSetProperty(inst, CopyPriorityProperty, &val, CMPI_uint16);        

							CMReturnInstance(rslt, inst);
							CMRelease(inst);
							CMRelease(ssOP);
							*isReturned = 1;
						}
					}
				}

				if(snapshot_volumes_list)
				{
					for(i=0; i<no_of_volumes; i++)
						free(snapshot_volumes_list[i]);
					free(snapshot_volumes_list);
					snapshot_volumes_list = 0;
				}
			}
			else
				return;
		}
	}
	else if (name2)
	{
		if(1) //DRBD guys must pass the local nodes case here 
		{
			rv = cs_return_associated_source_volume(name2, &source_volume);
			if(rv)
			{
				DebugMsg("Error in getting associated source volumes\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			else if(source_volume)
			{
				ldOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), className, rc);
				if(CMIsNullObject(ldOP))
				{
					DebugMsg("Unable to create Logical Disk Object Path\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				}
				en = broker->bft->enumerateInstanceNames(broker, ctx, ldOP, rc);
				if(rc->rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to enumInstanceName LD\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				}
				while(CMHasNext(en, rc))
				{
					ldOPData = CMGetNext(en, rc);
					keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, rc);
					if(rc->rc != CMPI_RC_OK)
					{
						DebugMsg("Unable to get key for LD\n");
						CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
						goto Exit;
					} 
					if(!strcasecmp(CMGetCharPtr(keyData.value.string), source_volume))
					{
						short val;
						if (strcasecmp( className, LogicalDiskClassName) == 0)																	
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
								StorageSynchronizedAssnName, rc);
						else if (strcasecmp( className, StorageVolumeClassName) == 0)																	
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
								SVStorageSynchronizedAssnName, rc);						
						if(CMIsNullObject(ssOP))
						{
							DebugMsg("Unable to create SS Object Path\n");
							CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
									"CMPI_RC_ERR_FAILED");
							goto Exit;
						}
						inst = CMNewInstance(broker, ssOP, rc);
						if(CMIsNullObject(inst))
						{
							CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
									"Unable to create SS instance");
							DebugMsg(CMGetCharPtr(rc->msg));
							CMRelease(ssOP);
							goto Exit;
						} 	
						
						do{
							const char **keys;
							keys = (const char **)malloc(3 * sizeof(char*));  
							keys[0] = SystemElementProperty;
							keys[1] = SyncedElementProperty;
							keys[2] = NULL;
							CMSetPropertyFilter(inst, properties, keys);
							free(keys);
						}while(0);

 
						CMSetProperty(inst, SystemElementProperty, &ldOPData.value.ref, CMPI_ref);
						CMSetProperty(inst, SyncedElementProperty, &oPath2, CMPI_ref);

						val = 4;
						CMSetProperty(inst, CopyTypeProperty, &val, CMPI_uint16);
						CMSetProperty(inst, ReplicaTypeProperty, &val, CMPI_uint16);

						val = 11;
						CMSetProperty(inst, SyncStateProperty, &val, CMPI_uint16);

						val = 0;
						CMSetProperty(inst, CopyPriorityProperty, &val, CMPI_uint16);        

						CMReturnInstance(rslt, inst);
						CMRelease(inst);
						CMRelease(ssOP);       
						*isReturned = 1;
						break;
					}
				}
			} 		
		}
	}


Exit:			
	if(snapshot_volumes_list)
	{
		for(i=0; i<no_of_volumes; i++)
			free(snapshot_volumes_list[i]);
		free(snapshot_volumes_list);
		snapshot_volumes_list = 0;
	} 
        if(source_volume)
		free(source_volume);
	return;
}



CMPIStatus OMC_SS_ReturnSyncedElementsOPs(
		const char * className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op, 
		const CMPIContext *ctx, 
		const CMPIResult *rslt, 
		char *isReturned)
{
	CMPIObjectPath *oPath=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData ldOPData, keyData;
        CMPIStatus rc={CMPI_RC_OK, NULL};
	const char *name=NULL;
	char **snapshot_volumes_list = NULL;
	int no_of_volumes = 0, rv, i;
 
	name = CMGetCharPtr(CMGetKey(op, DeviceIDProperty, &rc).value.string);
	if(name == NULL)
	{
		DebugMsg("Unable to get key from LD OP\n");
		CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
 
	if(1) //DRBD guys must pass the local nodes case here
	{
		rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
		if(rv)
		{
			DebugMsg("Error in getting associated snapshot volumes\n");
			CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
	       	}
		else
		{
			oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, &rc)), className, &rc);
			if(CMIsNullObject(oPath))
			{
				DebugMsg("Unable to create Logical Disk Object Path\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			en = broker->bft->enumerateInstanceNames(broker, ctx, oPath, &rc);
			if(rc.rc != CMPI_RC_OK)
			{
				DebugMsg("Unable to enumInstanceName LD\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			while(CMHasNext(en, &rc))
			{
				ldOPData = CMGetNext(en, &rc);
				keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, &rc);
				if(rc.rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to get key for LD\n");
					CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 
				for(i=0; i<no_of_volumes; i++)
				{
					if(!strcasecmp(CMGetCharPtr(keyData.value.string), snapshot_volumes_list[i]))
					{
						CMReturnObjectPath(rslt, ldOPData.value.ref);
						*isReturned = 1;
					}
				}
	       		}
		}
	}

Exit:
	if(snapshot_volumes_list)
	{	
		for(i=0; i<no_of_volumes; i++)
			free(snapshot_volumes_list[i]);
		if(snapshot_volumes_list)
			free(snapshot_volumes_list);  
		snapshot_volumes_list = 0;
	}
 	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en); 

	return rc;
}


CMPIStatus OMC_SS_ReturnSourceElementOP(
		const char *className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op, 
		const CMPIContext *ctx, 
		const CMPIResult *rslt, 
		char *isReturned)
{
	CMPIObjectPath *oPath=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData ldOPData, keyData;
        CMPIStatus rc={CMPI_RC_OK, NULL};
	const char *name=NULL;
	char *source_volume = NULL;
        int rv;
 
	name = CMGetCharPtr(CMGetKey(op, DeviceIDProperty, &rc).value.string);
	if(name == NULL)
	{
		DebugMsg("Unable to get key from LD OP\n");
		CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
 
	if(1) //DRBD guys must pass the local nodes case here
	{
		rv = cs_return_associated_source_volume(name, &source_volume);
		if(rv)
		{
			DebugMsg("Error in getting associated source volumes\n");
			CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
	       	}
		else
		{
			oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, &rc)), className, &rc);
			if(CMIsNullObject(oPath))
			{
				DebugMsg("Unable to create Logical Disk/Storage Volume Object Path\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			en = broker->bft->enumerateInstanceNames(broker, ctx, oPath, &rc);
			if(rc.rc != CMPI_RC_OK)
			{
				DebugMsg("Unable to enumInstanceName LD\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			while(CMHasNext(en, &rc))
			{
				ldOPData = CMGetNext(en, &rc);
				keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, &rc);
				if(rc.rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to get key for LD\n");
					CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 
				if(!strcasecmp(CMGetCharPtr(keyData.value.string), source_volume))
				{
					CMReturnObjectPath(rslt, ldOPData.value.ref);
					*isReturned = 1;
					break;
				}
	       		}
		}
	}

Exit:
	if(source_volume)
		free(source_volume);
 	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en); 

	return rc;
}
 
CMPIStatus OMC_SS_ReturnSyncedElementsInsts(
		const char *className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op, 
		const CMPIContext *ctx, 
		const CMPIResult *rslt, 
		const char** properties,
		char *isReturned)
{
	CMPIObjectPath *oPath=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData ldOPData, keyData;
        CMPIStatus rc={CMPI_RC_OK, NULL};
	const char *name=NULL;
	char **snapshot_volumes_list = NULL;
	int no_of_volumes = 0, rv, i;
 
	name = CMGetCharPtr(CMGetKey(op, DeviceIDProperty, &rc).value.string);
	if(name == NULL)
	{
		DebugMsg("Unable to get key from LD OP\n");
		CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
 
	if(1) //DRBD guys must pass the local nodes case here
	{
		rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
		if(rv)
		{
			DebugMsg("Error in getting associated snapshot volumes\n");
			CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
	       	}
		else
		{
			oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, &rc)), className, &rc);
			if(CMIsNullObject(oPath))
			{
				DebugMsg("Unable to create Logical Disk Object Path\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			en = broker->bft->enumerateInstances(broker, ctx, oPath, properties, &rc);
			if(rc.rc != CMPI_RC_OK)
			{
				DebugMsg("Unable to enumInstanceName LD\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			while(CMHasNext(en, &rc))
			{
				ldOPData = CMGetNext(en, &rc);
				keyData = CMGetProperty(ldOPData.value.inst, DeviceIDProperty, &rc);
				if(rc.rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to get key for LD\n");
					CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 
				for(i=0; i<no_of_volumes; i++)
				{
					if(!strcasecmp(CMGetCharPtr(keyData.value.string), snapshot_volumes_list[i]))
					{
						CMReturnInstance(rslt, ldOPData.value.inst);
						*isReturned = 1;
					}
				}
	       		}
		}
	}

Exit:
	if(snapshot_volumes_list)
	{	
		for(i=0; i<no_of_volumes; i++)
			free(snapshot_volumes_list[i]);
		if(snapshot_volumes_list)
			free(snapshot_volumes_list);  
		snapshot_volumes_list = 0;
	}
 	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en); 

	return rc;
}


CMPIStatus OMC_SS_ReturnSourceElementInst(
		const char *className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op, 
		const CMPIContext *ctx, 
		const CMPIResult *rslt, 
		const char** properties,
		char *isReturned)
{
	CMPIObjectPath *oPath=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData ldOPData, keyData;
        CMPIStatus rc={CMPI_RC_OK, NULL};
	const char *name=NULL;
	char *source_volume = NULL;
        int rv;
 
	name = CMGetCharPtr(CMGetKey(op, DeviceIDProperty, &rc).value.string);
	if(name == NULL)
	{
		DebugMsg("Unable to get key from LD OP\n");
		CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
 
	if(1) //DRBD guys must pass the local nodes case here
	{
		rv = cs_return_associated_source_volume(name, &source_volume);
		if(rv)
		{
			DebugMsg("Error in getting associated source volumes\n");
			CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			goto Exit;
	       	}
		else
		{
			oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, &rc)), className, &rc);
			if(CMIsNullObject(oPath))
			{
				DebugMsg("Unable to create Logical Disk Object Path\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			en = broker->bft->enumerateInstances(broker, ctx, oPath, properties, &rc);
			if(rc.rc != CMPI_RC_OK)
			{
				DebugMsg("Unable to enumInstanceName LD\n");
				CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			while(CMHasNext(en, &rc))
			{
				ldOPData = CMGetNext(en, &rc);
				keyData = CMGetProperty(ldOPData.value.inst, DeviceIDProperty, &rc);
				if(rc.rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to get key for LD\n");
					CMSetStatusWithChars(broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 
				if(!strcasecmp(CMGetCharPtr(keyData.value.string), source_volume))
				{
					CMReturnInstance(rslt, ldOPData.value.inst);
					*isReturned = 1;
					break;
				}
	       		}
		}
	}

Exit:
	if(source_volume)
		free(source_volume);
 	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en); 

	return rc;
}
 


void OMC_CreateSSObjectPaths(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		const CMPIObjectPath *oPath, //source element
		const CMPIObjectPath *oPath2, //synced element
		char *isReturned,
		CMPIStatus *rc) 
{
	const char *name=NULL, *name2=NULL;
	CMPIObjectPath *op=NULL, *ssOP=NULL, *ldOP=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData ldOPData, keyData;
	char **snapshot_volumes_list=NULL, *source_volume=NULL;
	int no_of_volumes = 0, rv, i, k;
	char is_snapshot_exists = 0;

	if(oPath != NULL)
		op = (CMPIObjectPath *)oPath;
	else
		op = (CMPIObjectPath *)oPath2;

	if(oPath)
		name = CMGetCharPtr(CMGetKey(oPath, DeviceIDProperty, rc).value.string);
	if(oPath2)
		name2 = CMGetCharPtr(CMGetKey(oPath2, DeviceIDProperty, rc).value.string);

	if(name == NULL && name2 == NULL)
	{
		DebugMsg("Unable to get key from SS OP\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return;
	}

	if(name && name2)
	{
		if(1) //DRBD guys must pass the local nodes case here
		{
			rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
			if(rv)
			{
				DebugMsg("Error in getting associated snapshot volumes\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				return;
			}
			else if(no_of_volumes)
			{
				for(i=0; i<no_of_volumes; i++)
				{
					if(!strcasecmp(name2, snapshot_volumes_list[i]))
					{
						is_snapshot_exists = 1;
						break;
					}
				}
				if(snapshot_volumes_list)
				{
					for(i=0; i<no_of_volumes; i++)
						free(snapshot_volumes_list[i]);
					if(snapshot_volumes_list)
						free(snapshot_volumes_list);
					snapshot_volumes_list = 0;
				}
			}

			if(is_snapshot_exists)
			{
				if (strcasecmp( className, LogicalDiskClassName) == 0)				
					ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
						StorageSynchronizedAssnName, rc);
				else if (strcasecmp( className, StorageVolumeClassName) == 0)
					ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
					SVStorageSynchronizedAssnName, rc);
				if(CMIsNullObject(ssOP))
				{
					DebugMsg("Unable to create SS Object Path\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return;
				}
				CMAddKey(ssOP, SystemElementProperty, &oPath, CMPI_ref);
				CMAddKey(ssOP, SyncedElementProperty, &oPath2, CMPI_ref);
				CMReturnObjectPath(rslt, ssOP);
				CMRelease(ssOP); 
				*isReturned = 1;
			}
		}
	}
	else if(name)
	{
		if(1) //DRBD guys must pass the local nodes case here
		{
			rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
			if(rv)
			{
				DebugMsg("Error in getting associated snapshot volumes\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				return;
			}
			else if(no_of_volumes > 0)
			{
				ldOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), className, rc);
				if(CMIsNullObject(ldOP))
				{
					DebugMsg("Unable to create Logical Disk Object Path\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				} 				
				
				en = broker->bft->enumerateInstanceNames(broker, ctx, ldOP, rc);
				if(rc->rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to enumInstanceName LD\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				}
				while(CMHasNext(en, rc))
				{
					ldOPData = CMGetNext(en, rc);
					keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, rc);

					if(rc->rc != CMPI_RC_OK)
					{
						DebugMsg("Unable to get key for LD\n");
						CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
						goto Exit;
					} 

					for(k=0; k<no_of_volumes; k++)
					{
						if(!strcasecmp(CMGetCharPtr(keyData.value.string), snapshot_volumes_list[k]))
						{
							if (strcasecmp( className, LogicalDiskClassName) == 0)		
								ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
									StorageSynchronizedAssnName, rc);
							else 
								if (strcasecmp( className, StorageVolumeClassName) == 0) 	
									ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
										SVStorageSynchronizedAssnName, rc);
							if(CMIsNullObject(ssOP))
							{
								DebugMsg("Unable to create SS Object Path\n");
								CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
										"CMPI_RC_ERR_FAILED");
								return;
							}
							CMAddKey(ssOP, SystemElementProperty, &oPath, CMPI_ref);
							CMAddKey(ssOP, SyncedElementProperty, &ldOPData.value.ref, CMPI_ref);
							CMReturnObjectPath(rslt, ssOP);
							CMRelease(ssOP); 
							*isReturned = 1;
						}
					}
				}

				if(snapshot_volumes_list)
				{
					for(i=0; i<no_of_volumes; i++)
						free(snapshot_volumes_list[i]);
					free(snapshot_volumes_list);
					snapshot_volumes_list = 0;
				}
			}
			else
				return;
		}
	}
	else if (name2)
	{
		if(1) //DRBD guys must pass the local nodes case here 
		{
			rv = cs_return_associated_source_volume(name2, &source_volume);
			if(rv)
			{
				DebugMsg("Error in getting associated source volumes\n");
				CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
				goto Exit;
			}
			else if(source_volume)
			{
				ldOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), className, rc);
				if(CMIsNullObject(ldOP))
				{
					DebugMsg("Unable to create Logical Disk Object Path\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				}
				en = broker->bft->enumerateInstanceNames(broker, ctx, ldOP, rc);
				if(rc->rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to enumInstanceName LD\n");
					CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					goto Exit;
				}
				while(CMHasNext(en, rc))
				{
					ldOPData = CMGetNext(en, rc);
					keyData = CMGetKey(ldOPData.value.ref, DeviceIDProperty, rc);
					if(rc->rc != CMPI_RC_OK)
					{
						DebugMsg("Unable to get key for LD\n");
						CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
						goto Exit;
					} 
					if(!strcasecmp(CMGetCharPtr(keyData.value.string), source_volume))
					{
					
						if (strcasecmp( className, LogicalDiskClassName) == 0) 	
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
							StorageSynchronizedAssnName, rc);
						else if (strcasecmp( className, StorageVolumeClassName) == 0) 	
							ssOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), 
								SVStorageSynchronizedAssnName, rc);
						if(CMIsNullObject(ssOP))
						{
							DebugMsg("Unable to create SS Object Path\n");
							CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, 
									"CMPI_RC_ERR_FAILED");
							return;
						}
						CMAddKey(ssOP, SystemElementProperty, &ldOPData.value.ref, CMPI_ref);
						CMAddKey(ssOP, SyncedElementProperty, &oPath2, CMPI_ref);
						CMReturnObjectPath(rslt, ssOP);
						CMRelease(ssOP); 
						*isReturned = 1;
						break;
					}
					
				}
			} 		
		}
	}


Exit:			
	if(snapshot_volumes_list)
	{
		for(i=0; i<no_of_volumes; i++)
			free(snapshot_volumes_list[i]);
		free(snapshot_volumes_list);
		snapshot_volumes_list = 0;
	} 
        if(source_volume)
		free(source_volume);
	return;
}

 
void OMC_DeleteSSInstance(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *oPath, //source element
		const CMPIObjectPath *oPath2, //synced element
		CMPIStatus *rc)
{
	const char *name=NULL, *name2=NULL;
	char **snapshot_volumes_list = NULL;
	int no_of_volumes = 0, rv, i;
	char is_snapshot_exists = 0;

       	name = CMGetCharPtr(CMGetKey(oPath, DeviceIDProperty, rc).value.string);
       	name2 = CMGetCharPtr(CMGetKey(oPath2, DeviceIDProperty, rc).value.string);

	if(name == NULL && name2 == NULL)
	{
		DebugMsg("Unable to get key from LD OP\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return;
	}

	rv = cs_return_associated_snapshot_volumes(name, &snapshot_volumes_list, &no_of_volumes);
	if(rv)
	{
		DebugMsg("Error in getting associated snapshot volumes\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return;
	}
	else if(no_of_volumes)
	{
		for(i=0; i<no_of_volumes; i++)
		{
			if(!strcasecmp(name2, snapshot_volumes_list[i]))
			{
				is_snapshot_exists = 1;
				break;
			}
		}
		if(snapshot_volumes_list)
		{
			for(i=0; i<no_of_volumes; i++)
				free(snapshot_volumes_list[i]);
			if(snapshot_volumes_list)
				free(snapshot_volumes_list);
			snapshot_volumes_list = 0;
		}
	}

	if(is_snapshot_exists)
	{
		rv = cs_delete_snapshot(name2);
		if(rv)
		{
			DebugMsg("Error in deleting snapshot. Volumes might be mounted. \n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return;
		}
	}	
}


