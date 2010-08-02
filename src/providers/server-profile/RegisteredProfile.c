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
 |	 OMC SMI-S Server profile provider
 |
 |---------------------------------------------------------------------------
 |
 | $Id: 
 |
 |---------------------------------------------------------------------------
 | This module contains:
 |   Provider code dealing with OMC_RegisteredSMIProfile class
 |
 +-------------------------------------------------------------------------*/

#include <cmpiutil/base.h>
#include <cmpiutil/string.h>
#include <cmpiutil/cmpiUtils.h>

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include "ServerProvider.h"
#include "RegisteredProfile.h"

extern CMPIBroker * _BROKER;

/////////////////////////////////////////////////////////////////////////////
//////////// Public functions ///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *RegisteredProfileCreateInstance(
					const char *ns,
					const char *name,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[256];
	CMPIValue val;

	_SMI_TRACE(1,("RegisteredProfileCreateInstance() called"));

	if (strcasecmp(name, ArrayName) == 0 ||
		strcasecmp(name, VolumeManagementName) == 0 ||
		strcasecmp(name, ServerName) == 0)
	{
		ci = CMNewInstance(
					_BROKER,
					CMNewObjectPath(_BROKER, ns, RegisteredProfileName, status),
					status);
	}
	else
	{
		ci = CMNewInstance(
					_BROKER,
					CMNewObjectPath(_BROKER, ns, RegisteredSubProfileName, status),
					status);
	}

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
	{
		_SMI_TRACE(1,("RegisteredProfileCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
		goto exit;
	}

	CMSetProperty(ci, InstanceIDName, 
		cmpiutilMakeInstanceID(name, buf, 256), CMPI_chars);

	CMSetProperty(ci, ElementName, name, CMPI_chars);
	CMSetProperty(ci, ParamName, name, CMPI_chars);
	CMSetProperty(ci, Caption, name, CMPI_chars);

	CMSetProperty(ci, RegisteredName, name, CMPI_chars);
	val.uint16 = 11;
	CMSetProperty(ci, RegisteredOrg, &val, CMPI_uint16);
	CMSetProperty(ci, RegisteredVersion, RegisteredVersionValue, CMPI_chars);

	CMPIArray *arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
	val.uint16 = 2;
	CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
	val.array = arr;
	CMSetProperty(ci, AdvertiseTypes, &val, CMPI_uint16A);

	arr = CMNewArray(_BROKER, 1, CMPI_string, NULL);
	val.string = CMNewString(_BROKER, NotAdvertised, NULL);
	CMSetArrayElementAt(arr, 0, &val, CMPI_string);
	val.array = arr;
	CMSetProperty(ci, AdvertiseTypeDesc, &val, CMPI_stringA);

exit:
	_SMI_TRACE(1,("RegisteredProfileCreateInstance() done"));
	return ci;
}


/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *RegisteredProfileCreateObjectPath(
					const char *ns,
					const char *name,
					CMPIStatus *status)
{
	CMPIObjectPath *cop;
	char buf[256];

	_SMI_TRACE(1,("RegisteredProfileCreateObjectPath() called"));

	if (strcasecmp(name, VolumeManagementName) == 0 ||
		strcasecmp(name, ArrayName) == 0 ||
		strcasecmp(name, ServerName) == 0)
	{
		cop = CMNewObjectPath(
					_BROKER, ns,
					RegisteredProfileName,
					status);
	}
	else
	{
		cop = CMNewObjectPath(
					_BROKER, ns,
					RegisteredSubProfileName,
					status);
	}

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(1,("RegisteredProfileCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
		goto exit;
	}

	CMAddKey(cop, InstanceIDName, cmpiutilMakeInstanceID(name, buf, 256), CMPI_chars);
/*	*buf = 0;
	strncat(buf, "SNIA:", 256);
	strncat(buf, name , 256);
	CMAddKey(cop, "InstanceID", buf , CMPI_chars);
*/
exit:
	_SMI_TRACE(1,("RegisteredProfileCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *RegisteredProfileCreateElementConformsAssocInstance(
					const CMPIContext * ctx,
					const char *ns,
					const char *name,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { ManagedElementName, ConformantStandardName, NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *mecop;

	_SMI_TRACE(1,("RegisteredProfileCreateElementConformsAssocInstance() called"));

	if (strcasecmp(name, ServerName) == 0)
	{
		// For Server profile ManagedElement is the Object Manager
		mecop = GetObjectManagerObjectPath(ctx, ns);
		if (CMIsNullObject(mecop))
		{
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
				"Could not create ObjectManager cop");
			return NULL;
		}
		_SMI_TRACE(1,("ObjectManagerObjectPath() called"));
	}
	else
	{
		// For all other profiles ManagedElement is the Computer System
		_SMI_TRACE(1,("CreateCSObjectPath() called"));
		mecop = cmpiutilCreateCSObjectPath(_BROKER, ns, pStatus);
		if (CMIsNullObject(mecop))
		{
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
				"Could not create ComputerSystem cop");
			return NULL;
		}
		_SMI_TRACE(1,("CreateCSObjectPath() called"));
	}

	// Create registered profile object path (RIGHT)
	CMPIObjectPath *cscop = RegisteredProfileCreateObjectPath(ns, name, &status);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredProfile cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											ElementConformsToProfileName,
											classKeys,
											properties,
											ManagedElementName,
											ConformantStandardName,
											mecop,
											cscop,
											pStatus);

	_SMI_TRACE(1,("Leaving RegisteredProfileCreateElementConformsAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *RegisteredProfileCreateElementConformsAssocObjectPath(
					const CMPIContext * ctx,
					const char *ns,
					const char *name,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};
	CMPIObjectPath *mecop;

	_SMI_TRACE(1,("RegisteredProfileCreateElementConformsAssocObjectPath() called"));

	if (strcasecmp(name, ServerName) == 0)
	{
		// For Server profile ManagedElement is the Object Manager
		_SMI_TRACE(1,("GetOMObjectPath() called"));
		mecop = GetObjectManagerObjectPath(ctx, ns);
		if (CMIsNullObject(mecop))
		{
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
				"Could not create ObjectManager cop");
			return NULL;
		}
		_SMI_TRACE(1,("GetOMObjectPath() done"));
	}
	else
	{
		// For all other profiles ManagedElement is the Computer System
		_SMI_TRACE(1,("CreateCSObjectPath() called"));
		mecop = cmpiutilCreateCSObjectPath( _BROKER, ns, pStatus);
		if (CMIsNullObject(mecop))
		{
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED,
				"Could not create ComputerSystem cop");
			return NULL;
		}
		_SMI_TRACE(1,("CreateCSObjectPath() done"));
	}

	// Create RegisteredProfile object path (RIGHT)
	CMPIObjectPath *cscop = RegisteredProfileCreateObjectPath(ns, name, &status);
	if (CMIsNullObject(cscop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredProfile cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									ElementConformsToProfileName,
									ManagedElementName,
									ConformantStandardName,
									mecop,
									cscop,
									pStatus);

	_SMI_TRACE(1,("Leaving RegisteredProfileCreateElementConformsAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *RegisteredProfileCreateSubProfileRequiresAssocInstance(
					const char *ns,
					const char *profName,
					const char *subProfName,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { DependentName, AntecedentName, NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("RegisteredProfileCreateSubProfileRequiresAssocInstance() called"));

	// Create registered sub-profile object path (LEFT)
	CMPIObjectPath *rspcop = RegisteredProfileCreateObjectPath(ns, subProfName, &status);
	if (CMIsNullObject(rspcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredSubProfile cop");
		return NULL;
	}



	// Create registered profile object path (RIGHT)
	CMPIObjectPath *rpcop = RegisteredProfileCreateObjectPath(ns, profName, &status);
	if (CMIsNullObject(rpcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredProfile cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											SubProfileRequiresProfileName,
											classKeys,
											properties,
											DependentName,
											AntecedentName,
											rspcop,
											rpcop,
											pStatus);

	_SMI_TRACE(1,("Leaving RegisteredProfileCreateSubProfileRequiresAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *RegisteredProfileCreateSubProfileRequiresAssocObjectPath(
					const char *ns,
					const char *profName,
					const char *subProfName,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("RegisteredProfileCreateSubProfileRequiresAssocObjectPath() called"));

	// Create RegisteredSubProfile object path (LEFT)
	CMPIObjectPath *rspcop = RegisteredProfileCreateObjectPath(ns, subProfName, &status);
	if (CMIsNullObject(rspcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredSubProfile cop");
		return NULL;
	}

	// Create RegisteredProfile object path (RIGHT)
	CMPIObjectPath *rpcop = RegisteredProfileCreateObjectPath(ns, profName, &status);
	if (CMIsNullObject(rpcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredProfile cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									SubProfileRequiresProfileName,
									DependentName,
									AntecedentName,
									rspcop,
									rpcop,
									pStatus);

	_SMI_TRACE(1,("Leaving RegisteredProfileCreateSubProfileRequiresAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;

}
