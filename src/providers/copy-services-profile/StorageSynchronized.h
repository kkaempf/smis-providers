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
#ifndef STORAGE_SYNCHRONIZED_H_
#define STORAGE_SYNCHRONIZED_H_ 


#define SystemElementProperty 				"SystemElement"
#define SyncedElementProperty 				"SyncedElement"
#define DeviceIDProperty 				"DeviceID"
#define CopyTypeProperty 				"CopyType"
#define ReplicaTypeProperty 				"ReplicaType"
#define SyncStateProperty 				"SyncState"
#define CopyPriorityProperty 				"CopyPriority"
 

void  OMC_EnumSSObjectPaths(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		CMPIStatus *rc);

void OMC_EnumSSInstances(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		const char** properties,
		CMPIStatus *rc);

void OMC_CreateSSInstances(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		const CMPIObjectPath *oPath, //source element
		const CMPIObjectPath *oPath2, //synced element
		char *isReturned,
		const char** properties,
		CMPIStatus *rc);
 
CMPIStatus OMC_SS_ReturnSyncedElementsOPs(
		const char * className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op, 
		const CMPIContext *ctx, 
		const CMPIResult *rslt,  
		char *isReturned);

CMPIStatus OMC_SS_ReturnSourceElementOP(
		const char * className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		char *isReturned);

CMPIStatus OMC_SS_ReturnSyncedElementsInsts(
		const char * className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		const char** properties,
		char *isReturned);

CMPIStatus OMC_SS_ReturnSourceElementInst(
		const char * className,
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		const char** properties,
		char *isReturned);

void OMC_CreateSSObjectPaths(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext *ctx,
		const CMPIResult *rslt,
		const CMPIObjectPath *oPath, //source element
		const CMPIObjectPath *oPath2, //synced element
		char *isReturned,
		CMPIStatus *rc); 

void OMC_DeleteSSInstance(
		const char * className,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *oPath, //source element
		const CMPIObjectPath *oPath2, //synced element
		CMPIStatus *rc); 

#endif /* STORAGE_SYNCHRONIZED_H_ */
