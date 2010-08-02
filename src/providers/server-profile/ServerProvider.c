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
 |************************************************************
 |
 |	 OMC SMI-S Server profile provider
 |
 |---------------------------------------------------------------------------
 |
 | $Id: 
 |
 |---------------------------------------------------------------------------
 | This module contains:
 |   Provider initialization and CMPI function table code.
 |
 +-------------------------------------------------------------------------*/

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <cmpiutil/base.h>
#include <cmpiutil/cmpiUtils.h>
#include <cmpiutil/cmpiSimpleAssoc.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Local includes  */
#include "ServerProvider.h"
#include "RegisteredProfile.h"
#include "SoftwareIdentity.h"

/* A simple stderr logging/tracing facility. */
void _logstderr(char *fmt,...)
{
   va_list ap;
   va_start(ap,fmt);
   vfprintf(stderr,fmt,ap);
   va_end(ap);
   fprintf(stderr,"\n");
}

/* Global handle to the CIM broker. This is initialized by the CIMOM when the provider is loaded */
const CMPIBroker * _BROKER;

static const char* ClassKeysMC[] = { ManagedElementName, ConformantStandardName, NULL };
static const char* ClassKeysAD[] = { AntecedentName,  DependentName, NULL };


// ----------------------------------------------------------------------------
// HELPER FUNCTIONS
// ----------------------------------------------------------------------------

CMPIObjectPath *GetObjectManagerObjectPath(
					const CMPIContext * ctx,
					const char *ns)
{

	_SMI_TRACE(1,("GetObjectManagerObjectPath() called, namespace = %s", ns));
	CMPIObjectPath *cop = NULL;
	CMPIObjectPath *outCop = NULL;
	CMPIEnumeration *copEnum;
	
	cop = CMNewObjectPath(_BROKER, ns, ObjectManagerName, NULL);

	if (!CMIsNullObject(cop))
	{
		copEnum = CBEnumInstanceNames(_BROKER, ctx, cop, NULL);
		if (!CMIsNullObject(copEnum) && CMHasNext(copEnum, NULL))
		{
			CMPIData data = CMGetNext(copEnum, NULL);
			if (data.state == CMPI_goodValue)
			{
				outCop = data.value.ref;
			}
		}
	}
	_SMI_TRACE(1,("GetObjectManagerObjectPath() done"));
	return outCop;
}


// ----------------------------------------------------------------------------
// CMPI INSTANCE PROVIDER FUNCTIONS
// ----------------------------------------------------------------------------

/* EnumInstanceNames() - return a list of all the instances names (i.e. return their object paths only) */
static CMPIStatus EnumInstanceNames(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self') */
		const CMPIContext * context,		/* [in] Additional context info, if any */
		const CMPIResult * results,			/* [out] Results of this operation */
		const CMPIObjectPath * reference)	/* [in] Contains the CIM namespace and classname */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char * ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("EnumInstanceNames() called, className = %s", className));

	//
	// Handle object enumerations
	//
	if (strcasecmp(className, RegisteredProfileName) == 0)
	{
		CMPIObjectPath* cop = RegisteredProfileCreateObjectPath(ns, ServerName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop);

		cop = RegisteredProfileCreateObjectPath(ns, VolumeManagementName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop); 

		cop = RegisteredProfileCreateObjectPath(ns, ArrayName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop); 
		
	}
	else if (strcasecmp(className, RegisteredSubProfileName) == 0)
	{
		CMPIObjectPath* cop = RegisteredProfileCreateObjectPath(ns, BlockSevicesName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop);

		cop = RegisteredProfileCreateObjectPath(ns, CopyServicesName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop);

		cop = RegisteredProfileCreateObjectPath(ns, ExtentCompositionName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop); 

	}
	else if (strcasecmp(className, VolumeManagementSoftwareName) == 0)
	{
		CMPIObjectPath* cop = SoftwareIdentityCreateObjectPath(ns, SMIVolumeManagementName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop);
	}

	else if (strcasecmp(className, ArraySoftwareName) == 0)
	{
		CMPIObjectPath* cop = SoftwareIdentityCreateObjectPath(ns, SMIArrayName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(cop))
		{
			goto exit;
		}
	    CMReturnObjectPath(results, cop);
	}

	//
	// Handle association enumerations
	//

	else if (strcasecmp(className, ElementConformsToProfileName) == 0)
	{
		CMPIObjectPath *assocCop = RegisteredProfileCreateElementConformsAssocObjectPath(context, ns, ServerName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = RegisteredProfileCreateElementConformsAssocObjectPath(context, ns, VolumeManagementName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = RegisteredProfileCreateElementConformsAssocObjectPath(context, ns, ArrayName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 
		
	}
	else if (strcasecmp(className, SubProfileRequiresProfileName) == 0)
	{
		CMPIObjectPath *assocCop = RegisteredProfileCreateSubProfileRequiresAssocObjectPath(ns, VolumeManagementName, BlockSevicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop);

		assocCop = RegisteredProfileCreateSubProfileRequiresAssocObjectPath(ns, VolumeManagementName, CopyServicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = RegisteredProfileCreateSubProfileRequiresAssocObjectPath(ns, VolumeManagementName, ExtentCompositionName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = RegisteredProfileCreateSubProfileRequiresAssocObjectPath(ns, ArrayName, BlockSevicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = RegisteredProfileCreateSubProfileRequiresAssocObjectPath(ns, ArrayName, ExtentCompositionName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = RegisteredProfileCreateSubProfileRequiresAssocObjectPath(ns, ArrayName, CopyServicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 
		
	}
	else if (strcasecmp(className, ElementSoftwareIdentityName) == 0)
	{
		CMPIObjectPath *assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIVolumeManagementName, ServerName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIVolumeManagementName, VolumeManagementName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIVolumeManagementName, CopyServicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIVolumeManagementName, BlockSevicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIVolumeManagementName, ExtentCompositionName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIArrayName, ServerName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIArrayName, ArrayName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIArrayName, CopyServicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIArrayName, BlockSevicesName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 

		assocCop = SoftwareIdentityCreateElementAssocObjectPath(ns, SMIArrayName, ExtentCompositionName,  &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(assocCop))
		{
			goto exit;
		}
		CMReturnObjectPath(results, assocCop); 
	}

	/* Finished */
	CMReturnDone(results);
exit:
	_SMI_TRACE(1,("EnumInstanceNames() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* EnumInstances() - return a list of all the instances (i.e. return all the instance data) */
static CMPIStatus EnumInstances(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self') */
		const CMPIContext * context,		/* [in] Additional context info, if any */
		const CMPIResult * results,			/* [out] Results of this operation */
		const CMPIObjectPath * reference,	/* [in] Contains the CIM namespace and classname */
		const char ** properties)			/* [in] List of desired properties (NULL=all) */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char *ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("EnumInstances() called, className = %s", className));

	//
	// Handle object enumerations
	//
	if (strcasecmp(className, RegisteredProfileName) == 0)
	{
		CMPIInstance* ci = RegisteredProfileCreateInstance(ns, ServerName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);

		ci = RegisteredProfileCreateInstance(ns, VolumeManagementName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);

		ci = RegisteredProfileCreateInstance(ns, ArrayName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);		
	}
	else if (strcasecmp(className, RegisteredSubProfileName) == 0)
	{
		CMPIInstance* ci = RegisteredProfileCreateInstance(ns, BlockSevicesName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);

		ci = RegisteredProfileCreateInstance(ns, ExtentCompositionName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);

		ci = RegisteredProfileCreateInstance(ns, CopyServicesName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);		
	}
	else if (strcasecmp(className, VolumeManagementSoftwareName) == 0)
	{
		CMPIInstance* ci = SoftwareIdentityCreateInstance(ns, SMIVolumeManagementName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}

	else if (strcasecmp(className, ArraySoftwareName) == 0)
	{
		CMPIInstance* ci = SoftwareIdentityCreateInstance(ns, SMIArrayName, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
	}
	//
	// Handle association enumerations
	//

	else if (strcasecmp(className, ElementConformsToProfileName) == 0)
	{
		CMPIInstance *ci = RegisteredProfileCreateElementConformsAssocInstance(context, ns, ServerName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = RegisteredProfileCreateElementConformsAssocInstance(context, ns, VolumeManagementName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
		
		ci = RegisteredProfileCreateElementConformsAssocInstance(context, ns, ArrayName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 		
	}
	else if (strcasecmp(className, SubProfileRequiresProfileName) == 0)
	{
		CMPIInstance *ci = RegisteredProfileCreateSubProfileRequiresAssocInstance(ns, VolumeManagementName, BlockSevicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = RegisteredProfileCreateSubProfileRequiresAssocInstance(ns, VolumeManagementName, ExtentCompositionName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = RegisteredProfileCreateSubProfileRequiresAssocInstance(ns, VolumeManagementName, CopyServicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = RegisteredProfileCreateSubProfileRequiresAssocInstance(ns, ArrayName, BlockSevicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = RegisteredProfileCreateSubProfileRequiresAssocInstance(ns, ArrayName, ExtentCompositionName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = RegisteredProfileCreateSubProfileRequiresAssocInstance(ns, ArrayName, CopyServicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 		

	}
	else if (strcasecmp(className, ElementSoftwareIdentityName) == 0)
	{
		CMPIInstance *ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, ServerName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, VolumeManagementName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, BlockSevicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, CopyServicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, ExtentCompositionName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		//SMIArray Identity instances
		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ServerName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ArrayName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 		

		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, BlockSevicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci);
		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, CopyServicesName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

		ci = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ExtentCompositionName, properties, &status);
		if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			goto exit;
		}
		CMReturnInstance(results, ci); 

	}

	/* Finished */
	CMReturnDone(results);
exit:
	_SMI_TRACE(1,("EnumInstances() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
return status;
}


// ----------------------------------------------------------------------------


/* GetInstance() -  return the instance data for the specified instance only */
static CMPIStatus GetInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self') */
		const CMPIContext * context,		/* [in] Additional context info, if any */
		const CMPIResult * results,			/* [out] Results of this operation */
		const CMPIObjectPath * reference,	/* [in] Contains the CIM namespace, classname and desired object path */
		const char ** properties)			/* [in] List of desired properties (NULL=all) */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
	const char * ns = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Our current CIM namespace */
	const char *className = CMGetCharPtr(CMGetClassName(reference, NULL));

	_SMI_TRACE(1,("GetInstance() called"));

	//
	// Get object instance
	//
	if (strcasecmp(className, RegisteredProfileName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, InstanceIDName, NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);

		if (strcasecmp(instanceID, ServerInstanceIDName) == 0)
		{
			CMPIInstance* ci = RegisteredProfileCreateInstance(ns, ServerName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else if (strcasecmp(instanceID, VMInstanceIDName) == 0)
		{
			CMPIInstance* ci = RegisteredProfileCreateInstance(ns, VolumeManagementName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else if (strcasecmp(instanceID, ArrayInstanceIDName) == 0)
		{
			CMPIInstance* ci = RegisteredProfileCreateInstance(ns, ArrayName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}		
	}
	else if (strcasecmp(className, RegisteredSubProfileName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, InstanceIDName, NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);

		if (strcasecmp(instanceID, BlockServicesInstanceIDName) == 0)
		{
			CMPIInstance* ci = RegisteredProfileCreateInstance(ns, BlockSevicesName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else if (strcasecmp(instanceID, ExtentCompositionInstanceIDName) == 0)
		{
			CMPIInstance* ci = RegisteredProfileCreateInstance(ns, ExtentCompositionName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
		else if (strcasecmp(instanceID, CopyServicesInstanceIDName) == 0)
		{
			CMPIInstance* ci = RegisteredProfileCreateInstance(ns, CopyServicesName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}		
	}
	else if (strcasecmp(className, VolumeManagementSoftwareName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, InstanceIDName, NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);

		if (strcasecmp(instanceID, VMSoftwareInstanceIDName) == 0)
		{
			CMPIInstance* ci = SoftwareIdentityCreateInstance(ns, SMIVolumeManagementName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
			CMReturnInstance(results, ci);
		}
	}
	else if (strcasecmp(className, ArraySoftwareName) == 0)
	{
		CMPIData keyData = CMGetKey(reference, InstanceIDName, NULL);
		const char *instanceID = CMGetCharsPtr(keyData.value.string, NULL);

		if (strcasecmp(instanceID, ArraySoftwareInstanceIDName) == 0)
		{
			CMPIInstance* ci = SoftwareIdentityCreateInstance(ns, SMIArrayName, &status);
			if ((status.rc != CMPI_RC_OK) || CMIsNullObject(ci))
			{
				goto exit;
			}
		CMReturnInstance(results, ci);
		}		
	}

	//
	// Get association instance
	//

	else if (strcasecmp(className, ElementConformsToProfileName) == 0)
	{
		CMPIData leftkey = CMGetKey(reference, ManagedElementName, &status);
		CMPIData rightkey = CMGetKey(reference, ConformantStandardName, &status);
		if (!CMIsNullValue(leftkey) && !CMIsNullValue(rightkey))
		{
			CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
												ns,
												className,
												ClassKeysMC,
												properties,
												ManagedElementName,
												ConformantStandardName,
												leftkey.value.ref,
												rightkey.value.ref,
												&status);
			CMReturnInstance(results, assocInst);
		}
	}
	else if (strcasecmp(className, SubProfileRequiresProfileName) == 0 ||
			 strcasecmp(className, ElementSoftwareIdentityName) == 0)
	{
		CMPIData leftkey = CMGetKey(reference, AntecedentName, &status);
		CMPIData rightkey = CMGetKey(reference, DependentName, &status);
		if (!CMIsNullValue(leftkey) && !CMIsNullValue(rightkey))
		{
			CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
												ns,
												className,
												ClassKeysAD,
												properties,
												AntecedentName,
												DependentName,
												leftkey.value.ref,
												rightkey.value.ref,
												&status);
			CMReturnInstance(results, assocInst);
		}
	}
	else
	{
		CMSetStatusWithChars(
			_BROKER, 
			&status, 
			CMPI_RC_ERR_NOT_FOUND, "Specified object not found in system");
		goto exit;
	}

	/* Finished */
	CMReturnDone(results);
exit:
	_SMI_TRACE(1,("GetInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* CreateInstance() - create a new instance from the specified instance data. */
static CMPIStatus CreateInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference,	/* [in] Contains the target namespace, classname and objectpath. */
		const CMPIInstance * newinstance)	/* [in] Contains all the new instance data. */
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */
   
	_SMI_TRACE(1,("CreateInstance() called"));
	/* Instance creation not supported. */

	/* Finished. */
exit:
	_SMI_TRACE(1,("CreateInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* SetInstance() - save modified instance data for the specified instance. */
static CMPIStatus SetInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference,	/* [in] Contains the target namespace, classname and objectpath. */
		const CMPIInstance * newinstance,	/* [in] Contains all the new instance data. */
		const char ** properties)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("SetInstance() called"));
	/* Instance modification not supported. */

	/* Finished. */
exit:
	_SMI_TRACE(1,("SetInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* DeleteInstance() - delete/remove the specified instance. */
static CMPIStatus DeleteInstance(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference)	/* [in] Contains the target namespace, classname and objectpath. */
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */

	_SMI_TRACE(1,("DeleteInstance() called"));
	/* Instance deletion not supported. */
	
	/* Finished. */
exit:
	_SMI_TRACE(1,("DeleteInstance() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------


/* ExecQuery() - return a list of all the instances that satisfy the desired query filter. */
static CMPIStatus ExecQuery(
		CMPIInstanceMI * self,				/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,		/* [in] Additional context info, if any. */
		const CMPIResult * results,			/* [out] Results of this operation. */
		const CMPIObjectPath * reference,	/* [in] Contains the target namespace and classname. */
		const char * language,				/* [in] Name of the query language (e.g. "WQL"). */ 
		const char * query)					/* [in] Text of the query, written in the query language. */ 
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */

	_SMI_TRACE(1,("ExecQuery() called"));

	/* Query filtering is not supported */

	/* Finished. */
exit:
	_SMI_TRACE(1,("ExecQuery() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------
// CMPI ASSOCIATION PROVIDER FUNCTIONS
// ----------------------------------------------------------------------------

// ****************************************************************************
// doReferences()
// 		This is the callback called from the SimpleAssociators helper functions
//		It "handles" one or more instances of the association class
// 		to be filtered by the SimpleAssociatior helper functions to return
// 		the correct object (instance or object path)
// ****************************************************************************
static CMPIStatus
doReferences(
		cmpiutilSimpleAssocCtx ctx,
		CMPIAssociationMI* self,
		const CMPIBroker *broker,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *assocClass,
		const char *resultClass,
		const char *role,
		const char *resultRole,
		const char** properties)
{
	_SMI_TRACE(1,("doReferences() called, assocClass: %s", assocClass));

	CMPIStatus status = {CMPI_RC_OK, NULL};

	char key[128] = {0};
	const char *objClassName;
	const char * ns = CMGetCharPtr(CMGetNameSpace(cop, NULL));

	if (assocClass == NULL || strcasecmp(assocClass, ElementConformsToProfileName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, ComputerSystemName) == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate RegisteredProfile for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(RegisteredProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, ConformantStandardName) != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					cmpiutilGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, ParamName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return the instances we have:
								CMPIInstance * instance = RegisteredProfileCreateElementConformsAssocInstance(context, ns, VolumeManagementName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementConformsToProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
								instance = RegisteredProfileCreateElementConformsAssocInstance(context, ns, ArrayName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementConformsToProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcasecmp(objClassName, ObjectManagerName) == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate RegisteredProfile for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(RegisteredProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, ConformantStandardName) != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					const char *key = NULL;
					CMPIObjectPath *omCop = GetObjectManagerObjectPath(context, ns);
					if (!CMIsNullObject(omCop))
					{
						CMPIData keyData = CMGetKey(omCop, ParamName, NULL);
						if (!CMIsNullValue(keyData))
						{
							key = CMGetCharPtr(keyData.value.string);
						}
					}

					CMPIData data = CMGetKey(cop, ParamName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return the one and only instance we have:
								CMPIInstance * instance = RegisteredProfileCreateElementConformsAssocInstance(context, ns, ServerName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementConformsToProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, RegisteredProfileName) == 0)
			{
				// this is a RegisteredProfile
				// need to get appropriate UnitaryComputerSystem/ObjectManager for assocInst

				CMPIData data = CMGetKey(cop, InstanceIDName, &status);
				const char *inName = CMGetCharPtr(data.value.string);
				_SMI_TRACE(1,("inName = %s", inName));
				int bHaveCSMatch = 1; // TRUE
				int bHaveOMMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					_SMI_TRACE(1,("Result class = %s", resultClass));

					// check
					if (!cmpiutilClassIsDerivedFrom(ComputerSystemName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveCSMatch = 0; // FALSE
					}
					else
					{
						bHaveOMMatch = 0;
					}
					if (!cmpiutilClassIsDerivedFrom(ObjectManagerName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveOMMatch = 0; // FALSE
					}
					else
					{
						bHaveCSMatch = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, ManagedElementName) != 0)
					{
						bHaveCSMatch = 0; // FALSE
						bHaveOMMatch = 0;
					}
				}

				if (bHaveCSMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (strcasecmp(inName, VMInstanceIDName) == 0)
						{
							// it matches, return the one and only instance we have:
							CMPIInstance * instance = RegisteredProfileCreateElementConformsAssocInstance(context, ns, VolumeManagementName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementConformsToProfile assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else if (strcasecmp(inName, ArrayInstanceIDName) == 0)
						{
							// it matches, return the one and only instance we have:
							CMPIInstance * instance = RegisteredProfileCreateElementConformsAssocInstance(context, ns, ArrayName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementConformsToProfile assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
				if (bHaveOMMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (strcasecmp(inName, ServerInstanceIDName) == 0)
						{

							// it matches, return the one and only instance we have:
							CMPIInstance * instance = RegisteredProfileCreateElementConformsAssocInstance(context, ns, ServerName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementConformsToProfile assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_ElementConformsToSMIProfile"

	if (assocClass == NULL || strcasecmp(assocClass, SubProfileRequiresProfileName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, RegisteredProfileName) == 0)
			{
				// this is a RegisteredProfile
				// need to get approprate RegisteredSubProfile for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(RegisteredSubProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, DependentName) != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					CMPIData data = CMGetKey(cop, InstanceIDName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, VMInstanceIDName) == 0)
							{
								// it matches, return all applicable instances:
								CMPIInstance * instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, VolumeManagementName, BlockSevicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, VolumeManagementName, ExtentCompositionName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, VolumeManagementName, CopyServicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfilee instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);								
							}
							else if (strcasecmp(inName, ArrayInstanceIDName) == 0)
							{
								// it matches, return all applicable instances:
								CMPIInstance * instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, ArrayName, BlockSevicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, ArrayName, CopyServicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, ArrayName, ExtentCompositionName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, RegisteredSubProfileName) == 0)
			{
				// this is a RegisteredSubProfile
				// need to get appropriate RegisteredProfile for assocInst

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(RegisteredProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, AntecedentName) != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					CMPIData data = CMGetKey(cop, InstanceIDName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, BlockServicesInstanceIDName) == 0)
							{
								CMPIInstance * instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, VolumeManagementName, BlockSevicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
								
								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, ArrayName, BlockSevicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);								
							}
							else if (strcasecmp(inName, ExtentCompositionInstanceIDName) == 0)
							{
								// it matches, return all applicable instances:
								CMPIInstance * instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, VolumeManagementName, ExtentCompositionName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, ArrayName, ExtentCompositionName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);								
							}
							else if (strcasecmp(inName, CopyServicesInstanceIDName) == 0)
							{
								// it matches, return all applicable instances:
								CMPIInstance * instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, VolumeManagementName, CopyServicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
								instance = RegisteredProfileCreateSubProfileRequiresAssocInstance(
																ns, ArrayName, CopyServicesName, properties, &status);

								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create SubProfileRequiresProfile instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);								
							}							
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_SMISubProfileRequiresProfile"

	if (assocClass == NULL || strcasecmp(assocClass, ElementSoftwareIdentityName) == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

//Raman TODO
			if(strcasecmp(objClassName, VolumeManagementSoftwareName) == 0)
			{
				// this is a SoftwareIdentity object
				// need to get approprate RegisteredProfiles/SubProfiles for assoc inst
				// but if resultClass is set, it must match

				int bHaveRPMatch = 1; // TRUE
				int bHaveSPMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(RegisteredProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveRPMatch = 0; // FALSE
					}
					else
					{
						bHaveSPMatch = 0;
					}
					if (!cmpiutilClassIsDerivedFrom(RegisteredSubProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveSPMatch = 0; // FALSE
					}
					else
					{
						bHaveRPMatch = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, DependentName) != 0)
					{
						bHaveSPMatch = 0; // FALSE
						bHaveRPMatch = 0;
					}
				}

				if (bHaveRPMatch)
				{
					CMPIData data = CMGetKey(cop, InstanceIDName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s", inName));
							if (strcasecmp(inName, VMSoftwareInstanceIDName) == 0)
							{
								// it matches, return the instances we have:
								CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, ServerName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, VolumeManagementName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
								
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
				if (bHaveSPMatch)
				{
					CMPIData data = CMGetKey(cop, InstanceIDName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s", inName));
							if (strcasecmp(inName, VMSoftwareInstanceIDName) == 0)
							{
								// it matches, return the instances we have:
								CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, BlockSevicesName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, ExtentCompositionName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = SoftwareIdentityCreateElementAssocInstance(ns,SMIVolumeManagementName, CopyServicesName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);								
							}
					
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcasecmp(objClassName, ArraySoftwareName) == 0)
			{
				// this is a SoftwareIdentity object
				// need to get approprate RegisteredProfiles/SubProfiles for assoc inst
				// but if resultClass is set, it must match

				int bHaveRPMatch = 1; // TRUE
				int bHaveSPMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!cmpiutilClassIsDerivedFrom(RegisteredProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveRPMatch = 0; // FALSE
					}
					else
					{
						bHaveSPMatch = 0;
					}
					if (!cmpiutilClassIsDerivedFrom(RegisteredSubProfileName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveSPMatch = 0; // FALSE
					}
					else
					{
						bHaveRPMatch = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, DependentName) != 0)
					{
						bHaveSPMatch = 0; // FALSE
						bHaveRPMatch = 0;
					}
				}

				if (bHaveRPMatch)
				{
					CMPIData data = CMGetKey(cop, InstanceIDName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s", inName));
							if (strcasecmp(inName, ArraySoftwareInstanceIDName) == 0)
							{
								// it matches, return the instances we have:
								CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ServerName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ArrayName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);
							}
								
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
				if (bHaveSPMatch)
				{
					CMPIData data = CMGetKey(cop, InstanceIDName, &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s", inName));
							if (strcasecmp(inName, ArraySoftwareInstanceIDName) == 0)
							{
								// it matches, return the instances we have:
								CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, BlockSevicesName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName,  ExtentCompositionName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);

								instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, CopyServicesName, properties, &status);
								if ((instance == NULL) || (CMIsNullObject(instance)))
								{
									CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
										"Could not create ElementSoftwareIdentity instance");
									return status;
								}
								cmpiutilSimpleAssocResults(ctx, instance, &status);								
							}
					
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, RegisteredProfileName) == 0)
			{
				// this is a RegisteredProfile
				// need to get appropriate SoftwareIdentity for assocInst

				CMPIData data = CMGetKey(cop, InstanceIDName, &status);
				const char *inName = CMGetCharPtr(data.value.string);
				_SMI_TRACE(1,("inName = %s", inName));
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					_SMI_TRACE(1,("Result class = %s", resultClass));

					// check
					if (!cmpiutilClassIsDerivedFrom(VolumeManagementSoftwareName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
					else if (!cmpiutilClassIsDerivedFrom(ArraySoftwareName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, AntecedentName) != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (strcasecmp(inName, ServerInstanceIDName) == 0)
						{
							// it matches, return the VM and Array instances
							CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, ServerName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);

							instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ServerName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else if (strcasecmp(inName, VMInstanceIDName) == 0)
						{
							// it matches, return the one and only instance we have:
							CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, VolumeManagementName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						else if (strcasecmp(inName, ArrayInstanceIDName) == 0)
						{
							// it matches, return the one and only instance we have:
							CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ArrayName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						
					}
				}
			}
			else if(strcmp(objClassName, RegisteredSubProfileName) == 0)
			{
				// this is a RegisteredSubProfile
				// need to get appropriate SoftwareIdentity for assocInst

				CMPIData data = CMGetKey(cop, InstanceIDName, &status);
				const char *inName = CMGetCharPtr(data.value.string);
				_SMI_TRACE(1,("inName = %s", inName));
				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					_SMI_TRACE(1,("Result class = %s", resultClass));

					// check
					if (!cmpiutilClassIsDerivedFrom(VolumeManagementSoftwareName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
					else if (!cmpiutilClassIsDerivedFrom(ArraySoftwareName,
												  resultClass,
												  broker,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}					
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, AntecedentName) != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (strcasecmp(inName, BlockServicesInstanceIDName) == 0)
						{
							// it matches, return the VM and Array instances we have:
							CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, BlockSevicesName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
					
							instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, BlockSevicesName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						if (strcasecmp(inName, CopyServicesInstanceIDName) == 0)
						{
							// it matches, return the VM and Array instances we have:
							CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, CopyServicesName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
					
							instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, CopyServicesName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
						
						else if (strcasecmp(inName, ExtentCompositionInstanceIDName) == 0)
						{
							// it matches, return the VM and Array instances we have:
							CMPIInstance * instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIVolumeManagementName, ExtentCompositionName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
					
							instance = SoftwareIdentityCreateElementAssocInstance(ns, SMIArrayName, ExtentCompositionName, properties, &status);
							if ((instance == NULL) || (CMIsNullObject(instance)))
							{
								CMSetStatusWithChars(broker, &status, CMPI_RC_ERR_FAILED,
									"Could not create ElementSoftwareIdentity assoc instance");
								return status;
							}
							cmpiutilSimpleAssocResults(ctx, instance, &status);
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_SMIElementSoftwareIdentity"

	//close return handler
	CMReturnDone(results);

	_SMI_TRACE(1,("Leaving doReferences"));
	return status;
}


// ****************************************************************************
// Associators()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname
// 											and desired objectpath
//             char *assocClass
//             char *resultClass
//             char *role
//             char *resultRole
//             char **properties
// ****************************************************************************
static CMPIStatus
Associators(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *assocClass,
		const char *resultClass,
		const char *role,
		const char *resultRole,
		const char** properties)
{
	_SMI_TRACE(1,("Associators() called.  assocClass: %s", assocClass));

//	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIStatus status = cmpiutilSimpleAssociators( doReferences, self,
				 _BROKER, context, results, cop, assocClass,
				resultClass, role, resultRole, properties);

/*
	char key[128] = {0};
	const char * objClassName;
	const char *ns = CMGetCharPtr(CMGetNameSpace(cop, NULL));
	CMPIInstance *srcInstance=NULL;
	CMPIObjectPath *srcOP=NULL;
	CMPIData scsOPData;

	if (strcasecmp(assocClass, "OMC_ElementConformsToSMIProfile") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_UnitaryComputerSystem") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate RegisteredProfile for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_RegisteredSMIProfile",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ConformantStandard") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					omcGetComputerSystemName(key, 128);

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return the instances we have:
								srcInstance = RegisteredProfileCreateInstance(ns, "Volume Management", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Array", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcasecmp(objClassName, "OpenWBEM_ObjectManager") == 0 ||
					strcasecmp(objClassName, "CIM_ObjectManager") == 0)
			{
				// this is a UnitaryComputerSystem
				// need to get approprate RegisteredProfile for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_RegisteredSMIProfile",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ConformantStandard") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					const char *key = NULL;
					CMPIObjectPath *omCop = GetObjectManagerObjectPath(ns);
					if (!CMIsNullObject(omCop))
					{
						CMPIData keyData = CMGetKey(omCop, "Name", NULL);
						if (!CMIsNullValue(keyData))
						{
							key = CMGetCharPtr(keyData.value.string);
						}
					}

					CMPIData data = CMGetKey(cop, "Name", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, key) == 0)
							{
								// it matches, return the one and only instance we have:
								srcInstance = RegisteredProfileCreateInstance(ns, "Server", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_RegisteredSMIProfile") == 0)
			{
				// this is a RegisteredProfile
				// need to get appropriate UnitaryComputerSystem/ObjectManager for assocInst

				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				_SMI_TRACE(1,("inName = %s", inName));
				int bHaveCSMatch = 1; // TRUE
				int bHaveOMMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					_SMI_TRACE(1,("Result class = %s", resultClass));

					// check
					if (!omccmpiClassIsDerivedFrom("OMC_UnitaryComputerSystem",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveCSMatch = 0; // FALSE
					}
					else
					{
						bHaveOMMatch = 0;
					}
					if (!omccmpiClassIsDerivedFrom("CIM_ObjectManager",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveOMMatch = 0; // FALSE
					}
					else
					{
						bHaveCSMatch = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "ManagedElement") != 0)
					{
						bHaveCSMatch = 0; // FALSE
						bHaveOMMatch = 0;
					}
				}

				if (bHaveCSMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (!strcasecmp(inName, "SNIA:Volume Management")
						|| !strcasecmp(inName, "SNIA:Array"))
						{
							// it matches, return the one and only instance we have:
							srcOP = omccmpiCreateCSObjectPath (_BROKER, CMGetCharPtr(CMGetNameSpace(cop, &status)), &status);
							if (CMIsNullObject(srcOP))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
								"Could not create ComputerSystem object path");
								return status;
							}
						
							CMPIEnumeration * instances = CBEnumInstances(_BROKER, context, srcOP,NULL, &status);
							if((status.rc != CMPI_RC_OK) || CMIsNullObject(instances))
							{ 
								_SMI_TRACE(1,("--- CBEnumInstanceNames() failed - %s", CMGetCharPtr(status.msg)));
					 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
					  			return status;
							}
				
							scsOPData = CMGetNext(instances , &status);
							srcInstance = (CMPIInstance *) scsOPData.value.ref;
							CMReturnInstance(results, srcInstance);
							CMRelease(instances);
						}
					}
				}
				if (bHaveOMMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (strcasecmp(inName, "SNIA:Server") == 0)
						{

							// it matches, return the one and only instance we have:
							srcOP = CMNewObjectPath (_BROKER, ns, "CIM_ObjectManager",&status);
							if (CMIsNullObject(srcOP))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
								"Could not create ComputerSystem object path");
								return status;
							}
						
							CMPIEnumeration * instances = CBEnumInstances(_BROKER, context, srcOP,NULL, &status);
							if((status.rc != CMPI_RC_OK) || CMIsNullObject(instances))
							{ 
								_SMI_TRACE(1,("--- CBEnumInstances() failed - %s", CMGetCharPtr(status.msg)));
					 			CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERROR, "Cannot enumerate target class");
					  			return status;
							}
				
							scsOPData = CMGetNext(instances , &status);
							srcInstance = (CMPIInstance *) scsOPData.value.ref;
							CMReturnInstance(results, srcInstance);
							CMRelease(instances);
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_ElementConformsToSMIProfile"

	else if (strcasecmp(assocClass, "OMC_SMISubProfileRequiresProfile") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

			if(strcasecmp(objClassName, "OMC_RegisteredSMIProfile") == 0)
			{
				// this is a RegisteredProfile
				// need to get approprate RegisteredSubProfile for assoc inst
				// but if resultClass is set, it must match

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_RegisteredSMISubProfile",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Dependent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, "SNIA:Server") == 0)
							{
								// it matches, return all applicable instances:
								srcInstance = RegisteredProfileCreateInstance(ns, "Indication", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
							else if (!strcasecmp(inName, "SNIA:Array") || !strcasecmp(inName, "SNIA:Volume Management"))
							{
								// it matches, return all applicable instances:
								srcInstance = RegisteredProfileCreateInstance(ns, "Block Services", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Copy Services", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Extent Composition", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Indication", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_RegisteredSMISubProfile") == 0)
			{
				// this is a RegisteredSubProfile
				// need to get appropriate RegisteredProfile for assocInst

				int bHaveMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_RegisteredSMIProfile",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveMatch = 0; // FALSE
					}
				}

				if (bHaveMatch)
				{
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s   with name: %s", inName, key));
							if (strcasecmp(inName, "SNIA:Indication") == 0)
							{
								// it matches, return all applicable instances:
								srcInstance = RegisteredProfileCreateInstance(ns, "Server", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Volume Management", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Array", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

							}
							else if (!strcasecmp(inName, "SNIA:Block Services")
								|| !strcasecmp(inName, "SNIA:Copy Services")
								|| !strcasecmp(inName, "SNIA:Extent Composition"))
							{
								// it matches, return all applicable instances:
								srcInstance = RegisteredProfileCreateInstance(ns, "Volume Management", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Array", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}							
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_SMISubProfileRequiresProfile"

	if (strcasecmp(assocClass, "OMC_SMIElementSoftwareIdentity") == 0)
	{
		objClassName = CMGetCharPtr(CMGetClassName(cop, NULL));
		if (objClassName)
		{
			_SMI_TRACE(1,("  Incoming className: %s", objClassName));

//Raman TODO
			if(!strcasecmp(objClassName, "OMC_SMIVolumeManagementSoftware")
			|| !strcasecmp(objClassName, "OMC_SMIArraySoftware"))
			{
				// this is a SoftwareIdentity object
				// need to get approprate RegisteredProfiles/SubProfiles for assoc inst
				// but if resultClass is set, it must match

				int bHaveRPMatch = 1; // TRUE
				int bHaveSPMatch = 1;
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					// check
					if (!omccmpiClassIsDerivedFrom("OMC_RegisteredSMIProfile",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveRPMatch = 0; // FALSE
					}
					else
					{
						bHaveSPMatch = 0;
					}
					if (!omccmpiClassIsDerivedFrom("OMC_RegisteredSMISubProfile",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveSPMatch = 0; // FALSE
					}
					else
					{
						bHaveRPMatch = 0;
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Dependent") != 0)
					{
						bHaveSPMatch = 0; // FALSE
						bHaveRPMatch = 0;
					}
				}

				if (bHaveRPMatch)
				{
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s", inName));
							if (strcasecmp(inName, "SNIA:SMIVolumeManagement") == 0)
							{
								// it matches, return the instances we have:
								srcInstance = RegisteredProfileCreateInstance(ns, "Server", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Volume Management", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
							else if(strcasecmp(inName, "SNIA:SMIArray") == 0)
							{
								// it matches, return the instances we have:
								srcInstance = RegisteredProfileCreateInstance(ns, "Server", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Array", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMIProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
								
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
				if (bHaveSPMatch)
				{
					CMPIData data = CMGetKey(cop, "InstanceID", &status);
					if (!CMIsNullValue(data))
					{
						const char *inName = CMGetCharPtr(data.value.string);
						if (inName)
						{
							_SMI_TRACE(1,("Comparing inName: %s", inName));
							if (!strcasecmp(inName, "SNIA:SMIVolumeManagement")
							|| !strcasecmp(inName, "SNIA:SMIArray"))
							{
								// it matches, return the instances we have:
								srcInstance = RegisteredProfileCreateInstance(ns, "Block Services", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Extent Composition", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Indication", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 

								srcInstance = RegisteredProfileCreateInstance(ns, "Copy Services", &status);

								if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
								{
									CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
										"Could not create RegisteredSMISubProfile instance");
									return status;
								}
								CMReturnInstance(results, srcInstance); 
							}
					
						}
						else
						{
							_SMI_TRACE(1,("Got empty value for key Name"));
						}
					}
					else
					{
						_SMI_TRACE(1,("Got null value for key Name"));
					}
				}
			}
			else if(strcmp(objClassName, "OMC_RegisteredSMIProfile") == 0)
			{
				// this is a RegisteredProfile
				// need to get appropriate SoftwareIdentity for assocInst

				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				_SMI_TRACE(1,("inName = %s", inName));
				int bHaveVMMatch = 1; // TRUE
				int bHaveARMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					_SMI_TRACE(1,("Result class = %s", resultClass));

					// check
					if (!omccmpiClassIsDerivedFrom("OMC_SMIVolumeManagementSoftware",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveVMMatch = 0; // FALSE
					}
					else if (!omccmpiClassIsDerivedFrom("OMC_SMIArraySoftware",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveARMatch = 0; // FALSE
					}
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveVMMatch = 0; // FALSE
						bHaveARMatch = 0; // FALSE
					}
				}

				if (bHaveVMMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (!strcasecmp(inName, "SNIA:Server")
						|| !strcasecmp(inName, "SNIA:Volume Management"))
						{
							// it matches, return the one and only instance we have:
							srcInstance = SoftwareIdentityCreateInstance(ns, "SMIVolumeManagement", &status);

							if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create Software Identity instance");
								return status;
							}
							CMReturnInstance(results, srcInstance); 
						}
					}
				}
				else if (bHaveARMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (!strcasecmp(inName, "SNIA:Server")
						|| !strcasecmp(inName, "SNIA:Array"))
						{
							// it matches, return the one and only instance we have:
							srcInstance = SoftwareIdentityCreateInstance(ns, "SMIArray", &status);

							if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create Software Identity instance");
								return status;
							}
							CMReturnInstance(results, srcInstance); 
						}
					}
				}
			}
			else if(strcmp(objClassName, "OMC_RegisteredSMISubProfile") == 0)
			{
				// this is a RegisteredSubProfile
				// need to get appropriate SoftwareIdentity for assocInst

				CMPIData data = CMGetKey(cop, "InstanceID", &status);
				const char *inName = CMGetCharPtr(data.value.string);
				_SMI_TRACE(1,("inName = %s", inName));
				int bHaveVMMatch = 1; // TRUE
				int bHaveARMatch = 1; // TRUE
				if ((resultClass != NULL) && (*resultClass != 0))
				{
					_SMI_TRACE(1,("Result class = %s", resultClass));

					// check
					if (!omccmpiClassIsDerivedFrom("OMC_SMIVolumeManagementSoftware",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveVMMatch = 0; // FALSE
					}
					else if (!omccmpiClassIsDerivedFrom("OMC_SMIArraySoftware",
												  resultClass,
												  _BROKER,ns,&status))
					{
						bHaveARMatch = 0; // FALSE
					}					
				}

				if ((resultRole != NULL) && (*resultRole != 0))
				{
					// check
					if (strcasecmp(resultRole, "Antecedent") != 0)
					{
						bHaveVMMatch = 0; // FALSE
						bHaveARMatch = 0; // FALSE
					}
				}

				if (bHaveVMMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (!strcasecmp(inName, "SNIA:Block Services")
						|| (!strcasecmp(inName, "SNIA:Copy Services"))
						|| (!strcasecmp(inName, "SNIA:Extent Composition"))
						|| (!strcasecmp(inName, "SNIA:Indication")))
						{
							// it matches, return the one and only instance we have:
							srcInstance = SoftwareIdentityCreateInstance(ns, "SMIVolumeManagement", &status);

							if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create Software Identity instance");
								return status;
							}
							CMReturnInstance(results, srcInstance); 
						}
					}
				}
				else if (bHaveARMatch)
				{
					if (inName)
					{
						_SMI_TRACE(1,("Comparing inName: %s", inName));
						if (!strcasecmp(inName, "SNIA:Block Services")
						|| (!strcasecmp(inName, "SNIA:Copy Services"))
						|| (!strcasecmp(inName, "SNIA:Extent Composition"))
						|| (!strcasecmp(inName, "SNIA:Indication")))
						{
							// it matches, return the one and only instance we have:
							srcInstance = SoftwareIdentityCreateInstance(ns, "SMIArray", &status);

							if ((srcInstance == NULL) || (CMIsNullObject(srcInstance)))
							{
								CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
									"Could not create Software Identity instance");
								return status;
							}
							CMReturnInstance(results, srcInstance); 
						}
					}
				}
			}
			else
			{
				_SMI_TRACE(1,("!!! Object type unknown: %s\n", objClassName));
			}
		}
		else
		{
			_SMI_TRACE(1,("!!! Object type unknown: No ObjectClassName determined"));
		}
	}// END--if assocClass == "OMC_SMIElementSoftwareIdentity"
*/
	//close return handler
	CMReturnDone(results);

	_SMI_TRACE(1,("Leaving Associatiors(): %s",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ****************************************************************************
// AssociatorNames()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname
// 											and desired objectpath
//             char *assocClass
//             char *resultClass
//             char *role
//             char *resultRole
// ****************************************************************************
static CMPIStatus
AssociatorNames(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *assocClass,
		const char *resultClass,
		const char *role,
		const char *resultRole)
{
	_SMI_TRACE(1,("AssociatorNames() called. assocClass: %s", assocClass));

	CMPIStatus status = cmpiutilSimpleAssociatorNames( doReferences, self,
				 _BROKER, context, results, cop, assocClass,
				resultClass, role, resultRole);

	_SMI_TRACE(1,("Leaving AssociatiorNames(): \n",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ****************************************************************************
// References()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname
// 											and desired objectpath
//             char *resultClass
//             char *role
//             char **properties
// ****************************************************************************
static CMPIStatus
References(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char *resultClass,
		const char *role ,
		const char** properties)
{
	_SMI_TRACE(1,("References() called"));

	CMPIStatus status = cmpiutilSimpleReferences( doReferences, self, _BROKER,
					context, results, cop, resultClass, role, properties);

	_SMI_TRACE(1,("Leaving References(): %s",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ****************************************************************************
// ReferenceNames()
//    params:  CMPIAssociationMI* self:  [in] Handle to this provider
//             CMPIContext* context:  [in] any additional context info
//             CMPIResult* results:   [out] Results
//             CMPIObjectPath* cop:   [in] target namespace and classname,
// 											and desired objectpath
//             char *resultClass
//             char *role
// ****************************************************************************
static CMPIStatus
ReferenceNames(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		const CMPIResult* results,
		const CMPIObjectPath* cop,
		const char* resultClass,
		const char* role)
{
	_SMI_TRACE(1,("ReferenceNames() called"));

	CMPIStatus status = cmpiutilSimpleReferenceNames( doReferences, self,
					_BROKER, context, results, cop, resultClass, role);

	_SMI_TRACE(1,("Leaving ReferenceNames(): %s",
			(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

// ----------------------------------------------------------------------------
// CMPI INDICATION PROVIDER FUNCTIONS
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

/* AuthorizeFilter() - verify whether this filter is allowed. */
static CMPIStatus AuthorizeFilter(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx,
		const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op, 
		const char* user)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations. */
   
	_SMI_TRACE(1,("AuthorizeFilter() called"));
	/* AuthorizeFilter not supported. */

	/* Finished. */
exit:
	_SMI_TRACE(1,("AuthorizeFilter() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}


// ----------------------------------------------------------------------------

/* MustPoll() - report whether polling mode should be used */
static CMPIStatus MustPoll(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx, 
		const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("MustPoll() called"));


	/* Finished. */
exit:
	_SMI_TRACE(1,("MustPool() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;

}

// ----------------------------------------------------------------------------

/* ActivateFilter() - begin monitoring a resource */
static CMPIStatus ActivateFilter(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx,
		const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op, 
		CMPIBoolean first)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("ActivateFilter() called"));
	/* ActivateFilter not supported. */


	/* Finished. */
exit:
	_SMI_TRACE(1,("ActivateFilter() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;

}

/* DeActivateFilter() - end monitoring a resource */
static CMPIStatus DeActivateFilter(
		CMPIIndicationMI* mi, 
		const CMPIContext* ctx, 
        	const CMPISelectExp* se, 
		const char* ns, 
		const CMPIObjectPath* op, 
		CMPIBoolean last)
{
	CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};	/* Return status of CIM operations */

	_SMI_TRACE(1,("DeActivateFilter() called"));
	/* DeActivateFilter not supported. */


	/* Finished. */
exit:
	_SMI_TRACE(1,("DeActivateFilter() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;

}

// ----------------------------------------------------------------------------
// INITIALIZE/CLEANUP FUNCTIONS
// ----------------------------------------------------------------------------

/* Cleanup() - perform any necessary cleanup immediately before this provider is unloaded. */
static CMPIStatus Cleanup(
		CMPIInstanceMI * self,			/* [in] Handle to this provider (i.e. 'self'). */
		const CMPIContext * context,	/* [in] Additional context info, if any. */
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations. */

	_SMI_TRACE(1,("Cleanup() called"));
   
	/* Do any cleanup required */
 
   /* Finished. */
exit:
	_SMI_TRACE(1,("Cleanup() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

static CMPIStatus AssociationCleanup(
		CMPIAssociationMI* self,
		const CMPIContext* context,
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("AssociationCleanup() called"));

	/* Do work here if necessary */

	/* Finished */
exit:
	_SMI_TRACE(1,("AssociationCleanup() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

static CMPIStatus IndicationCleanup(
		CMPIIndicationMI* self,
		const CMPIContext* context,
		CMPIBoolean terminating)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("IndicationCleanup() called"));

	/* Do work here if necessary */

	/* Finished */
exit:
	_SMI_TRACE(1,("IndicationCleanup() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	return status;
}

// ----------------------------------------------------------------------------


/* Initialize() - perform any necessary initialization immediately after this provider is loaded. */
static void Initialize(
		CMPIInstanceMI * self)		/* [in] Handle to this provider (i.e. 'self'). */
{
	CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations. */	

	_SMI_TRACE(1,("Initialize() called"));

	/* Do any general init stuff here */

	/* Finished. */
exit:
	_SMI_TRACE(1,("Initialize() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
}

static  void EnableIndications (CMPIIndicationMI* mi,
                                       const CMPIContext *ctx)
{
        CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};  /* Return status of CIM operations */

        _SMI_TRACE(1,("EnableIndications() called"));
        /* DeActivateFilter not supported. */


        /* Finished. */
exit:
        _SMI_TRACE(1,("EnableIndications() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
//        return status;
}

static  void DisableIndications (CMPIIndicationMI* mi,
                                       const CMPIContext *ctx)
{
        CMPIStatus status = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};  /* Return status of CIM operations */

        _SMI_TRACE(1,("DisableIndications() called"));
        /* DeActivateFilter not supported. */


        /* Finished. */
exit:
        _SMI_TRACE(1,("DisableIndications() %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
//        return status;
}


// ----------------------------------------------------------------------------
// SETUP CMPI PROVIDER FUNCTION TABLES
// ----------------------------------------------------------------------------

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 * ------------------------------------------------------------------ */

/* Factory method that creates the handle to this provider, specifically
   setting up the instance provider function table:
   - 1st param is an optional prefix for the function names in the table.
     It is blank in this sample provider because the instance provider
     function names do not need a unique prefix.
   - 2nd param is the name to call this provider within the CIMOM. It is
     recommended to call providers "<_CLASSNAME>Provider". This name must be
     unique among all providers. Make sure to use the same name when
     registering the provider with the hosting CIMOM.
   - 3rd param is the local static variable acting as a handle to the CIMOM.
     This will be initialized by the CIMOM when the provider is loaded. 
   - 4th param specifies the provider's initialization function to be called
     immediately after loading the provider. Specify "CMNoHook" if no special
     initialization routine is required.
*/

CMInstanceMIStub( , omc_smi_server, _BROKER, Initialize(&mi));

/* ------------------------------------------------------------------ *
 * Association MI Factory
 * ------------------------------------------------------------------ */

CMAssociationMIStub( , omc_smi_server, _BROKER, CMNoHook);

/* ------------------------------------------------------------------ *
 * Indication MI Factory
 * ------------------------------------------------------------------ */

CMIndicationMIStub( , omc_smi_server, _BROKER, CMNoHook);
