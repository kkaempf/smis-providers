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
#include "StorageReplicationElementCapabilities.h"
 
void  OMC_CreateSRECObjectPaths(
		int syncTypes,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		CMPIStatus *rc)
{
	CMPIObjectPath *oPath=NULL, *srecOP=NULL, *srcOP=NULL;
	CMPIEnumeration *en=NULL;
	CMPIData scsOPData;
	
	oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), StorageConfigurationServiceClassName, rc);
	if(CMIsNullObject(oPath))
	{
		DebugMsg("Unable to create SCS Object Path\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	en = broker->bft->enumerateInstanceNames(broker, ctx, oPath, rc);
	if(rc->rc != CMPI_RC_OK)
	{
		DebugMsg("Unable to enumInstanceName SCS\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	if(!CMHasNext(en, rc))
	{
		DebugMsg("No OPs for SCS\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}

	scsOPData = CMGetNext(en, rc);

	srecOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), StorageReplicationElementCapabilitiesAssnName, rc);
	if(CMIsNullObject(srecOP))
	{
		DebugMsg("Unable to create SREC Object Path\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}

	if(syncTypes == CT_UNSYNC_ASSOC_DELTA || syncTypes == -1)
	{
		srcOP = OMC_CreateSRCObjectPath(CT_UNSYNC_ASSOC_DELTA, broker, op, rc);
		if(CMIsNullObject(srcOP))
		{
			goto Exit;
		}
		
		*rc = CMAddKey(srecOP, ManagedElementProperty, &scsOPData.value.ref, CMPI_ref);
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to add key to SREC\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			if(srcOP)
				CMRelease(srcOP);
			goto Exit;
		}
		*rc = CMAddKey(srecOP, CapabilitiesProperty, &srcOP, CMPI_ref);
		if(rc->rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to add key to SREC\n");
			CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			if(srcOP)
				CMRelease(srcOP);
			goto Exit;
		}
		CMReturnObjectPath(rslt, srecOP);

		if(srcOP)
			CMRelease(srcOP);
	}

Exit:
	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en);
	if(srecOP)
		CMRelease(srecOP);
	return;
}



void OMC_CreateSRECInstances(
		int  syncTypes, 
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		const char** properties,
		CMPIStatus *rc)
{
	DebugMsg("In side Create SREC Instances \n");
	CMPIObjectPath *oPath=NULL, *srecOP=NULL, *srcOP=NULL;
	CMPIEnumeration *en=NULL;
	CMPIInstance *inst=NULL;
	CMPIData scsOPData;
	
	oPath = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), StorageConfigurationServiceClassName, rc);
	if(CMIsNullObject(oPath))
	{
		DebugMsg("Unable to create SCS Object Path\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	en = broker->bft->enumerateInstanceNames(broker, ctx, oPath, rc);
	if(rc->rc != CMPI_RC_OK)
	{
		DebugMsg("Unable to enumInstanceName SCS\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	if(!CMHasNext(en, rc))
	{
		DebugMsg("No OPs for SCS\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}

	scsOPData = CMGetNext(en, rc);

	srecOP = CMNewObjectPath(broker, CMGetCharPtr(CMGetNameSpace(op, rc)), StorageReplicationElementCapabilitiesAssnName, rc);
	if(CMIsNullObject(srecOP))
	{
		DebugMsg("Unable to create SREC Object Path\n");
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		goto Exit;
	}
	inst = CMNewInstance(broker, srecOP, rc);
	if(CMIsNullObject(inst))
	{
		CMSetStatusWithChars(broker, rc, CMPI_RC_ERR_FAILED, "Unable to create SREC instance");
		DebugMsg(CMGetCharPtr(rc->msg));
		goto Exit;
	} 	
	CMSetProperty(inst, ManagedElementProperty, &scsOPData.value.ref, CMPI_ref);

	if(syncTypes == CT_UNSYNC_ASSOC_DELTA || syncTypes == -1)
	{
		DebugMsg("In side Unsync Assoc \n");
		srcOP = OMC_CreateSRCObjectPath(CT_UNSYNC_ASSOC_DELTA, broker, op, rc);
		if(CMIsNullObject(srcOP))
		{
			goto Exit;
		}
		
		CMSetProperty(inst, CapabilitiesProperty, &srcOP, CMPI_ref);
		CMReturnInstance(rslt, inst);
		
		if(srcOP)
			CMRelease(srcOP);
	}


Exit:
	if(oPath)
		CMRelease(oPath);
	if(en)
		CMRelease(en);
	if(srecOP)
		CMRelease(srecOP);
	if(inst)
		CMRelease(inst);
	return; 	
}	

 


 
