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
#ifndef OMC_STORAGEREPLICATIONCAPABILITIES_H_
#define OMC_STORAGEREPLICATIONCAPABILITIES_H_
 




#define StorageReplicationCapabilitiesName 		"StorageReplicationCapabilities"
#define StorageReplicationCapabilitiesAsync 		"Novell:StorageReplicationCapabilities_Async" 
#define StorageReplicationCapabilitiesSync 		"Novell:StorageReplicationCapabilities_Sync" 
#define StorageReplicationCapabilitiesUnsyncAssocDelta 	"Novell:StorageReplicationCapabilities_UnsyncAssocDelta"
 

//Properties
#define InstanceIDProperty 				"InstanceID"
#define SupportedSynchronousActionsProperty 		"SupportedSynchronousActions"
#define SupportedAsynchronousActionsProperty 		"SupportedAsynchronousActions"
#define DescriptionProperty 				"Description"
#define SupportedSynchronizationTypeProperty 		"SupportedSynchronizationType"
#define CaptionProperty 				"Caption"
#define ElementNameProperty 				"ElementName"
#define HostAccessibleStateProperty 			"HostAccessibleStateName"
#define SupportedModifyOperationsProperty 		"SupportedModifyOperations"
#define InitialReplicationStateProperty 		"InitialReplicationState"
#define ReplicaHostAccessibilityProperty 		"ReplicaHostAccessibility"
#define MaximumReplicasPerSourceProperty 		"MaximumReplicasPerSource"
#define PersistentReplicasSupportedProperty 		"PersistentReplicasSupported"
#define MaximumLocalReplicationDepthProperty 		"MaximumLocalReplicationDepth"
#define PeerConnectionProtocolProperty 			"PeerConnectionProtocol"
#define BidirectionalConnectionsSupportedProperty 	"BidirectionalConnectionsSupported"
#define RemoteReplicationServicePointAccessProperty 	"RemoteReplicationServicePointAccess"
#define RemoteBufferHostProperty 			"RemoteBufferHost"
#define RemoteBufferLocationProperty 			"RemoteBufferLocation"
#define RemoteBufferElementTypeProperty 		"RemoteBufferElementType"
	
#define SupportedSpecializedElementsProperty 		"SupportedSpecializedElements"
#define SpaceLimitSupportedProperty 			"SpaceLimitSupported"
#define SpaceReservationSupportedProperty 		"SpaceReservationSupported"
#define LocalMirrorSnapshotSupportedProperty 		"LocalMirrorSnapshotSupported"
#define RemoteMirrorSnapshotSupportedProperty 		"RemoteMirrorSnapshotSupported"
#define IncrementalDeltasSupportedProperty 		"IncrementalDeltasSupported"
#define DeltaReplicaPoolAccessProperty 			"DeltaReplicaPoolAccess"
 



/* LATER VERSION OF COPY SERVICES TALKS ABT THESE VALUES.
//PUTTING THESE IN SO WE CAN CHANGE QUICKLY IF NEEDED

enum CopyType
{

	CT_ASYNCLOCAL		 = 2,
	CT_ASYNCREMOTE		 = 3,
	CT_SYNCLOCAL		 = 4,
	CT_SYNCREMOTE		 = 5,
	CT_UNSYNC_ASSOC_FULL 	 = 6,
	CT_UNSYNC_ASSOC_DELTA	 = 7,
	CT_UNSYNC_UNASSOC    	 = 8,
	CT_MIGRATE	    	 = 9

};

enum Operations
{
	OP_REPLICA_CREATION		= 2,
	OP_REPLICA_MODIFICATION		= 3,
	OP_REPLICA_ATTACHMENT		= 4,
	OP_BUFFER_CREATION		= 5, 
	OP_MIGRATION			= 6, 
	OP_REPLICATIONPIPE_CREATION	= 7

};


*/



enum CopyType
{

	CT_ASYNC		 = 2,
	CT_SYNC			 = 3,
	CT_UNSYNC_ASSOC_FULL 	 = 4,
	CT_UNSYNC_ASSOC_DELTA	 = 5,
	CT_UNSYNC_UNASSOC    	 = 6

};



enum Operations
{
	OP_LOCALREPLICA_CREATION		= 2,
	OP_REMOTEREPLICA_CREATION		= 3,
	OP_LOCALREPLICA_MODIFICATION		= 4,
	OP_REMOTEREPLICA_MODIFICATION		= 5,
	OP_LOCALREPLICA_ATTACHMENT		= 6,
	OP_REMOTEREPLICA_ATTACHMENT		= 7,
	OP_BUFFER_CREATION			= 8, 
	OP_NETWORKPIPE_CREATION			= 9

};


enum InitialReplicationState
{

	RS_INITIALIZED   = 2,
	RS_PREPARED      = 3,
	RS_SYNCHRONIZED  = 4,
	RS_IDLE          = 5

};


enum SpecializedElements
{
	
	SE_DELTA_POOL 		= 2,
	SE_DELTA_POOL_COMPONENT = 3,
	SE_REMOTE_MIRROR	= 4,
	SE_LOCAL_MIRROR		= 5,
	SE_FULL_SNAPSHOT	= 6,
	SE_DELTA_SNAPSHOT	= 7,
	SE_REPLICATION_BUFFER	= 8

};



enum ModifySynchronizationOperations
{

	MS_DETACH		= 2,         
	MS_FRACTURE		= 3,			
	MS_RESYNC		= 4,
	MS_RESTORE		= 5,
	MS_PREPARE		= 6,
	MS_UNPREPARE		= 7,
	MS_QUIESCE		= 8,
	MS_UNQUIESCE		= 9,
	MS_RESET_TO_SYNC	= 10,
	MS_RESET_TO_ASYNC	= 11,
	MS_START_COPY		= 12,
	MS_STOP_COPY		= 13

};


enum ReplicaSynchronizationStates
{
		
	RSS_INITIALIZED			= 2,
	RSS_PREPARE_IN_PROGRESS		= 3,
	RSS_PREPARED			= 4,
	RSS_RESYNC_IN_PROGRESS		= 5,
	RSS_SYNCHRONIZED		= 6,
	RSS_FRACTURE_IN_PROGRESS	= 7,
	RSS_QUIESCE_IN_PROGRESS		= 8,
	RSS_QUIESCED			= 9,
	RSS_RESTORE_IN_PROGRESS		= 10,
	RSS_IDLE			= 11,
	RSS_BROKEN			= 12,
	RSS_FRACTURED			= 13,
	RSS_FROZEN			= 14,
	RSS_COPY_IN_PROGRESS		= 15

};


enum ReplicaHostAccessibility
{

	RHA_NOT_ACCESSIBLE		= 2,
	RHA_NO_RESTRICTIONS		= 3,
	RHA_SOURCE_HOSTS_ONLY		= 4,
	RHA_SOURCE_HOSTS_EXCLUDED	= 5

};


enum RemoteReplicationServicePointAccess
{
	RSPA_NOT_SPECIFIED	= 2,
	RSPA_SOURCE		= 3,
	RSPA_TARGET		= 4,
	RSPA_PROXY		= 5

};

enum RemoteBufferLocation
{
 	RBL_SOURCE	= 2,
	RBL_TARGET	= 3,
	RBL_BOTH	= 4
};


enum RemoteBufferHost
{
	RBH_TOPLEVEL		= 2,
	RBH_COMPONENTCS		= 3,
	RBH_PIPE		= 4

};

enum BufferElementType
{
	BEL_NOT_SPECIFIED	= 2,
	BEL_INEXTENT		= 3,
	BEL_INPOOL		= 4

};

enum RemoteBufferSupported
{

	RBS_NOT_SUPPORTED	= 0,
	RBS_REQUIRED		= 2,
	RBS_OPTIONAL		= 3
	

};




CMPIObjectPath* OMC_CreateSRCObjectPath(
		int syncType, 
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		CMPIStatus *rc); 


CMPIInstance* OMC_CreateSRCInstance(
		int syncType, 
		const CMPIBroker *broker,
		const CMPIObjectPath *op,
		const char** properties,
		CMPIStatus *rc); 

#endif
