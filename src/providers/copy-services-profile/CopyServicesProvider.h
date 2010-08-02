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
#ifndef COPYSERVICESPROVIDER_H_
#define COPYSERVICESPROVIDER_H_



// Copy service provider classes
#define StorageConfigurationServiceClassName 		"OMC_StorageConfigurationService"
#define StorageConfigurationCapabilitiesClassName 	"OMC_StorageConfigurationCapabilities"
#define StorageReplicationCapabilitiesClassName 	"OMC_StorageReplicationCapabilities"

// Copy service provider association classes
#define StorageReplicationElementCapabilitiesAssnName 	"OMC_StorageReplicationElementCapabilities"
#define StorageSynchronizedAssnName 			"OMC_LogicalDiskStorageSynchronized"
#define SVStorageSynchronizedAssnName 			"OMC_StorageVolumeStorageSynchronized"
#define LogicalDiskClassName 				"OMC_LogicalDisk"
#define StorageVolumeClassName				"OMC_StorageVolume"

void LogMsg(const char *msg);
void DebugMsg(const char *msg);



#endif  /* COPYSERVICESPROVIDER_H_ */




