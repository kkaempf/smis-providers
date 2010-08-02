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
 |	 OMC SMI-S Server profile provider include file
 |
 |---------------------------------------------------------------------------
 |
 | $Id:
 |
 +-------------------------------------------------------------------------*/
#ifndef	SERVERPROVIDER_H_
#define SERVERPROVIDER_H_

/* A simple stderr logging/tracing facility. */
#ifndef _SMI_TRACE
#include <stdarg.h>
extern void _logstderr(char *fmt,...);
#define _SMI_TRACE(tracelevel,args) _logstderr args 
#endif

// Exports
extern CMPIObjectPath *GetObjectManagerObjectPath(
						const CMPIContext * ctx,
						const char *ns);

#define InstanceIDName "InstanceID"
#define ManagedElementName "ManagedElement"
#define ConformantStandardName "ConformantStandard"
#define AntecedentName "Antecedent"
#define DependentName "Dependent"
#define ParamName "Name"
#define ElementName "ElementName"
#define Caption "Caption"
#define Description "Description"
#define RegisteredName "RegisteredName"
#define RegisteredOrg "RegisteredOrganization"
#define RegisteredVersion "RegisteredVersion"
#define RegisteredVersionValue "1.2.0"
#define AdvertiseTypes "AdvertiseTypes"
#define AdvertiseTypeDesc "AdvertiseTypeDescriptions"
#define NotAdvertised "Not Advertised"
#define VersionString "VersionString"
#define VersionStringValue "v0.1.0"
#define Manufacturer "Manufacturer"
#define ManufacturerName "Novell"
#define Status "Status"
#define OK "OK"
#define HealthState "HealthState"
#define Classifications "Classifications"
#define OperationalStatus "OperationalStatus"

#define VMProfileName "OMC SMI-S Volume Management"
#define ArrayProfileName "OMC SMI-S Array"

#define ServerName "Server"
#define ArrayName "Array"
#define VolumeManagementName "Volume Management"
#define BlockSevicesName "Block Services"
#define CopyServicesName "Copy Services"
#define ExtentCompositionName "Extent Composition"
#define SMIVolumeManagementName "SMIVolumeManagement"
#define SMIArrayName "SMIArray"
#define ServerInstanceIDName "cmpiutil:Server"
#define VMInstanceIDName "cmpiutil:Volume Management"
#define ArrayInstanceIDName "cmpiutil:Array"
#define BlockServicesInstanceIDName "cmpiutil:Block Services"
#define ExtentCompositionInstanceIDName "cmpiutil:Extent Composition"
#define CopyServicesInstanceIDName "cmpiutil:Copy Services"
#define VMSoftwareInstanceIDName "cmpiutil:SMIVolumeManagement"
#define ArraySoftwareInstanceIDName "cmpiutil:SMIArray"

#define ComputerSystemName "Linux_ComputerSystem"
#define ObjectManagerName "CIM_ObjectManager"
#define RegisteredProfileName "OMC_RegisteredSMIProfile"
#define RegisteredSubProfileName "OMC_RegisteredSMISubProfile"
#define VolumeManagementSoftwareName "OMC_SMIVolumeManagementSoftware"
#define ArraySoftwareName "OMC_SMIArraySoftware"
#define ElementConformsToProfileName "OMC_ElementConformsToSMIProfile"
#define SubProfileRequiresProfileName "OMC_SMISubProfileRequiresProfile"
#define ElementSoftwareIdentityName "OMC_SMIElementSoftwareIdentity"

#endif /*SERVERPROVIDER_H_*/
