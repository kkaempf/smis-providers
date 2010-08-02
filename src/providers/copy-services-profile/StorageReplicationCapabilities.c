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
#include "StorageReplicationCapabilities.h"



CMPIObjectPath* OMC_CreateSRCObjectPath(
		int syncType, 
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		CMPIStatus *rc)
{
	CMPIObjectPath *oPath=NULL;
	
	oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), StorageReplicationCapabilitiesClassName, rc);
	if(CMIsNullObject(oPath))
	{
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create SRC Object Path");
		DebugMsg(CMGetCharPtr(rc->msg));
		return oPath;
	}
	if(syncType == CT_UNSYNC_ASSOC_DELTA);
	{
		CMAddKey(oPath, InstanceIDProperty, StorageReplicationCapabilitiesUnsyncAssocDelta, CMPI_chars);
	}

	return oPath;
}


CMPIInstance* OMC_CreateSRCInstance(
		int syncType, 
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		const char** properties, 
		CMPIStatus *rc)
{
	CMPIObjectPath *oPath=NULL;
	CMPIInstance *inst=NULL;
	printf("Inside SRC Instance\n");
	
	oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), StorageReplicationCapabilitiesClassName, rc);
	if(CMIsNullObject(oPath))
	{
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create SRC Object Path");
		DebugMsg(CMGetCharPtr(rc->msg));
		return inst;
	}
	inst = CMNewInstance(broker, oPath, rc);
	if(CMIsNullObject(inst))
	{
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create SRC instance");
		DebugMsg(CMGetCharPtr(rc->msg));
		return inst;
	}
	if(syncType == CT_UNSYNC_ASSOC_DELTA)
	{
		printf("in side unsyncassoc\n");
		CMPIValue val, arr;
		CMPIUint16 uint16Val;
		CMPIBoolean boolVal;

		do{
			const char **keys;
			keys = (const char **)malloc(2 * sizeof(char*));  
			keys[0] = InstanceIDProperty;
			keys[1] = NULL;
			CMSetPropertyFilter(inst, properties, keys);
			free(keys);
		}while(0);
		
 		CMSetProperty(inst, InstanceIDProperty, StorageReplicationCapabilitiesUnsyncAssocDelta, CMPI_chars);
	 	CMSetProperty(inst, DescriptionProperty, "Asynchronous Association Delta Storage Replication Capability", CMPI_chars);
		CMSetProperty(inst, CaptionProperty, StorageReplicationCapabilitiesName, CMPI_chars);
		CMSetProperty(inst, ElementNameProperty, StorageReplicationCapabilitiesName, CMPI_chars);

       		//MANDATORY PROPERTIES:
		CMSetProperty(inst, SupportedSynchronizationTypeProperty, &syncType, CMPI_uint16);

 		arr.array = CMNewArray(broker, 2, CMPI_uint16, rc);
		if (rc->rc != CMPI_RC_OK) 
		{
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create uint16 array");
			DebugMsg(CMGetCharPtr(rc->msg));
			CMRelease(inst);
			return NULL;
		}
       	
		val.uint16 = OP_LOCALREPLICA_MODIFICATION;
		CMSetArrayElementAt(arr.array, 0, &val, CMPI_uint16);
		
		val.uint16 = OP_LOCALREPLICA_ATTACHMENT;
		CMSetArrayElementAt(arr.array, 1, &val, CMPI_uint16);
		
		CMSetProperty(inst, SupportedSynchronousActionsProperty, &arr, CMPI_uint16A);
		
       		CMRelease(arr.array);
		
 		arr.array = CMNewArray(broker, 3, CMPI_uint16, rc);
		if (rc->rc != CMPI_RC_OK) 
		{
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create uint16 array");
			DebugMsg(CMGetCharPtr(rc->msg));
			CMRelease(inst);
			return NULL;
		}
       	
		val.uint16 = RSS_RESYNC_IN_PROGRESS;
		CMSetArrayElementAt(arr.array, 0, &val, CMPI_uint16);
		
		val.uint16 = RSS_IDLE;
		CMSetArrayElementAt(arr.array, 1, &val, CMPI_uint16);
		
		val.uint16 = RSS_FROZEN;
		CMSetArrayElementAt(arr.array, 2, &val, CMPI_uint16);
		
		CMSetProperty(inst, HostAccessibleStateProperty, &arr, CMPI_uint16A);

		arr.array = CMNewArray(broker, 2, CMPI_uint16, rc);
		if (rc->rc != CMPI_RC_OK) 
		{
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create uint16 array");
			DebugMsg(CMGetCharPtr(rc->msg));
			CMRelease(inst);
			return NULL;
		}
       	
		val.uint16 = MS_RESYNC;
		CMSetArrayElementAt(arr.array, 0, &val, CMPI_uint16);
		
		val.uint16 = MS_RESTORE;
		CMSetArrayElementAt(arr.array, 1, &val, CMPI_uint16);
		
		CMSetProperty(inst, SupportedModifyOperationsProperty, &arr, CMPI_uint16A);
		
                uint16Val = RS_IDLE;
		CMSetProperty(inst, InitialReplicationStateProperty, &uint16Val, CMPI_uint16);

		uint16Val = RHA_NO_RESTRICTIONS;
		CMSetProperty(inst, ReplicaHostAccessibilityProperty, &uint16Val, CMPI_uint16);

		uint16Val = 16;
		CMSetProperty(inst, MaximumReplicasPerSourceProperty, &uint16Val, CMPI_uint16);
 
		boolVal = 1;
		CMSetProperty(inst, PersistentReplicasSupportedProperty, &boolVal, CMPI_boolean);
		
		uint16Val = 1;
		CMSetProperty(inst, MaximumLocalReplicationDepthProperty, &uint16Val, CMPI_uint16);
 

		// OPTIONAL PROPERTIES:
		CMSetProperty(inst, PeerConnectionProtocolProperty, NULL, CMPI_chars);
		
		boolVal = 0;
		CMSetProperty(inst, BidirectionalConnectionsSupportedProperty, &boolVal, CMPI_boolean);
		
		uint16Val = RSPA_NOT_SPECIFIED;
		CMSetProperty(inst, RemoteReplicationServicePointAccessProperty, &uint16Val, CMPI_uint16);

		uint16Val = RBH_TOPLEVEL;
		CMSetProperty(inst, RemoteBufferHostProperty, &uint16Val, CMPI_uint16);

		uint16Val = RBL_BOTH;
		CMSetProperty(inst, RemoteBufferLocationProperty, &uint16Val, CMPI_uint16);

		uint16Val = BEL_NOT_SPECIFIED;
		CMSetProperty(inst, RemoteBufferElementTypeProperty, &uint16Val, CMPI_uint16);

		boolVal = 0;
		CMSetProperty(inst, SpaceLimitSupportedProperty, &boolVal, CMPI_boolean);
		
		boolVal = 0;
		CMSetProperty(inst, SpaceReservationSupportedProperty, &boolVal, CMPI_boolean);
		
		boolVal = 0;
		CMSetProperty(inst, LocalMirrorSnapshotSupportedProperty, &boolVal, CMPI_boolean);
		
		boolVal = 0;
		CMSetProperty(inst, RemoteMirrorSnapshotSupportedProperty, &boolVal, CMPI_boolean);
		
		boolVal = 0;
		CMSetProperty(inst, IncrementalDeltasSupportedProperty, &boolVal, CMPI_boolean);
 		
	}

	return inst;
}
 

