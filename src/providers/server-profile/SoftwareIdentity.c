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
 |   Provider code dealing with OMC_SoftwareIdentity class
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
#include "SoftwareIdentity.h"
#include "RegisteredProfile.h"

extern CMPIBroker * _BROKER;


/////////////////////////////////////////////////////////////////////////////
//////////// Public functions ///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SoftwareIdentityCreateInstance(
					const char *ns,
					const char *name,
					CMPIStatus *status)
{
	CMPIInstance *ci;
	char buf[256];
	CMPIValue val;

	_SMI_TRACE(1,("SoftwareIdentityCreateInstance() called"));

	if (strcasecmp(name, SMIVolumeManagementName) == 0)
	{
		ci = CMNewInstance(
					_BROKER,
					CMNewObjectPath(_BROKER, ns, VolumeManagementSoftwareName, status),
					status);
	
		if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			_SMI_TRACE(1,("SoftwareIdentityCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
			CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
			goto exit;
		}

		CMSetProperty(ci, InstanceIDName, 
			cmpiutilMakeInstanceID(name, buf, 256), CMPI_chars);

		CMSetProperty(ci, ElementName, VMProfileName, CMPI_chars);
		CMSetProperty(ci, ParamName, VMProfileName, CMPI_chars);
		CMSetProperty(ci, Caption, VMProfileName, CMPI_chars);
		CMSetProperty(ci, Description, VMProfileName, CMPI_chars);
		CMSetProperty(ci, VersionString, VersionStringValue, CMPI_chars);
		CMSetProperty(ci, Manufacturer, ManufacturerName, CMPI_chars);
		CMSetProperty(ci, Status, OK, CMPI_chars);

		val.uint16 = 5;
		CMSetProperty(ci, HealthState, &val, CMPI_uint16);

		CMPIArray *arr = CMNewArray(_BROKER, 2, CMPI_uint16, NULL);
		val.uint16 = 3;
		CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
		val.uint16 = 5;
		CMSetArrayElementAt(arr, 1, &val, CMPI_uint16);
		val.array = arr;
		CMSetProperty(ci, Classifications, &val, CMPI_uint16A);
	
		arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
		val.uint16 = 2;
		CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
		val.array = arr;
		CMSetProperty(ci, OperationalStatus, &val, CMPI_uint16A);
	}

	else if (strcasecmp(name, SMIArrayName) == 0)
	{
		ci = CMNewInstance(
					_BROKER,
					CMNewObjectPath(_BROKER, ns, ArraySoftwareName, status),
					status);
	
		if ((status->rc != CMPI_RC_OK) || CMIsNullObject(ci))
		{
			_SMI_TRACE(1,("SoftwareIdentityCreateInstance(): CMNewInstance() failed - %s", CMGetCharPtr(status->msg)));
			CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new instance");
			goto exit;
		}

		CMSetProperty(ci, InstanceIDName, 
			cmpiutilMakeInstanceID(name, buf, 256), CMPI_chars);

		CMSetProperty(ci, ElementName, ArrayProfileName, CMPI_chars);
		CMSetProperty(ci, ParamName, ArrayProfileName, CMPI_chars);
		CMSetProperty(ci, Caption, ArrayProfileName, CMPI_chars);
		CMSetProperty(ci, Description, ArrayProfileName, CMPI_chars);
		CMSetProperty(ci, VersionString, VersionStringValue, CMPI_chars);
		CMSetProperty(ci, Manufacturer, ManufacturerName, CMPI_chars);
		CMSetProperty(ci, Status, OK, CMPI_chars);

		val.uint16 = 5;
		CMSetProperty(ci, HealthState, &val, CMPI_uint16);

		CMPIArray *arr = CMNewArray(_BROKER, 2, CMPI_uint16, NULL);
		val.uint16 = 3;
		CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
		val.uint16 = 5;
		CMSetArrayElementAt(arr, 1, &val, CMPI_uint16);
		val.array = arr;
		CMSetProperty(ci, Classifications, &val, CMPI_uint16A);
	
		arr = CMNewArray(_BROKER, 1, CMPI_uint16, NULL);
		val.uint16 = 2;
		CMSetArrayElementAt(arr, 0, &val, CMPI_uint16);
		val.array = arr;
		CMSetProperty(ci, OperationalStatus, &val, CMPI_uint16A);
	}

exit:
	_SMI_TRACE(1,("SoftwareIdentityCreateInstance() done"));
	return ci;
}


/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SoftwareIdentityCreateObjectPath(
					const char *ns,
					const char *name,
					CMPIStatus *status)
{
	CMPIObjectPath *cop;
	char buf[256];

	_SMI_TRACE(1,("SoftwareIdentityCreateObjectPath() called"));

	if (strcasecmp(name, SMIArrayName) == 0)
	{
		cop = CMNewObjectPath(
					_BROKER, ns,
					ArraySoftwareName,
					status);
	}
	else if (strcasecmp(name, SMIVolumeManagementName) == 0)
	{
		cop = CMNewObjectPath(
					_BROKER, ns,
					VolumeManagementSoftwareName,
					status);
	}

	if ((status->rc != CMPI_RC_OK) || CMIsNullObject(cop))
	{
		_SMI_TRACE(1,("SoftwareIdentityCreateObjectPath(): CMNewObjectPath() failed - %s", CMGetCharPtr(status->msg)));
		CMSetStatusWithChars(_BROKER, status, CMPI_RC_ERROR_SYSTEM, "Cannot create new objectpath");
		goto exit;
	}

	CMAddKey(cop, InstanceIDName, cmpiutilMakeInstanceID(name, buf, 256), CMPI_chars);

exit:
	_SMI_TRACE(1,("SoftwareIdentityCreateObjectPath() done"));
	return cop;
}

/////////////////////////////////////////////////////////////////////////////
CMPIInstance *SoftwareIdentityCreateElementAssocInstance(
					const char *ns,
					const char *siname,
					const char *name,
					const char ** properties,
					CMPIStatus *pStatus)
{
	const char* classKeys[] = { AntecedentName, DependentName, NULL };
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("SoftwareIdentityCreateElementAssocInstance() called"));

	// Create software identity object path (LEFT)
	CMPIObjectPath *antcop;
	antcop = SoftwareIdentityCreateObjectPath(ns, siname, &status);

	if (CMIsNullObject(antcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create SoftwareIdentity cop");
		return NULL;
	}

	// Create registered profile object path (RIGHT)
	CMPIObjectPath *depcop = RegisteredProfileCreateObjectPath(ns, name, &status);
	if (CMIsNullObject(depcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredProfile cop");
		return NULL;
	}

	CMPIInstance *assocInst = cmpiutilCreateAssociationInst(_BROKER,
											ns,
											ElementSoftwareIdentityName,
											classKeys,
											properties,
											AntecedentName,
											DependentName,
											antcop,
											depcop,
											pStatus);

	_SMI_TRACE(1,("Leaving SoftwareIdentityCreateElementAssocInstance(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));
	return assocInst;
}

/////////////////////////////////////////////////////////////////////////////
CMPIObjectPath *SoftwareIdentityCreateElementAssocObjectPath(
					const char *ns,
					const char *siname,
					const char *name,
					CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};

	_SMI_TRACE(1,("SoftwareIdentityCreateElementAssocObjectPath() called"));

	// Create SoftwareIdentity object path (LEFT)
	CMPIObjectPath *antcop;
	antcop = SoftwareIdentityCreateObjectPath(ns, siname, &status);

	if (CMIsNullObject(antcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create SoftwareIdentity cop");
		return NULL;
	}

	// Create RegisteredProfile object path (RIGHT)
	CMPIObjectPath *depcop = RegisteredProfileCreateObjectPath(ns, name, &status);
	if (CMIsNullObject(depcop))
	{
		CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED,
			"Could not create RegisteredProfile cop");
		return NULL;
	}

	CMPIObjectPath *assocCop = cmpiutilCreateAssociationPath(_BROKER,
									ns,
									ElementSoftwareIdentityName,
									AntecedentName,
									DependentName,
									antcop,
									depcop,
									pStatus);

	_SMI_TRACE(1,("Leaving SoftwareIdentityCreateElementAssocObjectPath(): %s",
			(pStatus->rc == CMPI_RC_OK)? "succeeded":"failed"));

	return assocCop;
}
