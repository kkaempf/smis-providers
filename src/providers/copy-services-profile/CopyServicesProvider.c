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

#include <storage/StorageInterface.h>
#include "CopyServicesProvider.h"
#include "StorageReplicationCapabilities.h"
#include "StorageReplicationElementCapabilities.h"
#include "StorageSynchronized.h"
#include "Array-Profile/StorageConfigurationService.h"
#include "Array-Profile/ArrayProvider.h"
//#include "Array-Profile/StorageConfigurationCapabilities.h"
//#include "Array-Profile/StorageConfigurationElementCapabilities.h"
 

using namespace storage;

//static const CMPIBroker *omc_csp_broker;
const CMPIBroker *omc_csp_broker;

//const StorageInterface* st_interface = createDefaultStorageInterface();


void LogMsg(const char *msg)
{
	/* Replace printf with messages logging of cmpi */
	printf("%s", msg);
}

void DebugMsg(const char *msg)
{
	/* Replace printf with debug message logging of cmpi */
	printf("%s", msg);
}


/*
 *
 *	Instance Provider Functions
 * 
 */

  
CMPIStatus CopyServicesProviderCleanup(
                CMPIInstanceMI* mi,
		const CMPIContext* ctx,
		CMPIBoolean terminating
		)
{
	CMReturn(CMPI_RC_OK);
}


CMPIStatus CopyServicesProviderEnumInstanceNames(
		CMPIInstanceMI* thisMI,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op
		)
{
	const char *className;
	CMPIStatus rc={CMPI_RC_OK, NULL};
        CMPIObjectPath * oPath=NULL;
	
	DebugMsg("CopyServices EnumInstanceNames\n");

	//Vijay className = CMGetClassName(op, &rc);
	className = CMGetCharPtr(CMGetClassName(op, &rc));
	printf ("CopyServicesProviderEIN:className %s\n",className);

	if(rc.rc != CMPI_RC_OK)
	{
		DebugMsg("Unable to get class name\n");
		CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return rc;
	}

	if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
	{
        	oPath = OMC_CreateSRCObjectPath(CT_UNSYNC_ASSOC_DELTA, omc_csp_broker, op, &rc);
		if(CMIsNullObject(oPath))
			return rc;
		CMReturnObjectPath(rslt, oPath);
		CMRelease(oPath);
	}
	else if(!strcasecmp(className, StorageReplicationElementCapabilitiesAssnName))
	{
		OMC_CreateSRECObjectPaths(-1, omc_csp_broker, ctx, rslt, op, &rc);
		if(rc.rc != CMPI_RC_OK)
			return rc;
 	}
	else if(!strcasecmp(className, StorageSynchronizedAssnName))
	{
		DebugMsg("In side StorageSynchronized\n");
		OMC_EnumSSObjectPaths(LogicalDiskClassName, omc_csp_broker, ctx, rslt, op, &rc);
		if(rc.rc != CMPI_RC_OK)
			return rc;
 	}
	else if(!strcasecmp(className, SVStorageSynchronizedAssnName))
	{
		OMC_EnumSSObjectPaths(StorageVolumeClassName, omc_csp_broker, ctx, rslt, op, &rc);
		if(rc.rc != CMPI_RC_OK)
			return rc;
 	}
	if(thisMI)
		CMReturnDone(rslt);

	return rc;	
}


CMPIStatus CopyServicesProviderEnumInstances(
		CMPIInstanceMI* thisMI,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const char** properties
		)
{
	const char *className;
	CMPIInstance *inst=NULL;
	CMPIStatus rc={CMPI_RC_OK, NULL};
	
	DebugMsg("CopyServices EnumInstances\n");
	
	//className = CMGetClassName(op, &rc);
	className = CMGetCharPtr(CMGetClassName(op, &rc));
	printf ("CopyServicesProviderEIN:className %s\n",className);

	if(rc.rc != CMPI_RC_OK)
	{
		DebugMsg("Unable to get class name\n");
		CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return rc;
	}

	if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
	{
		inst = OMC_CreateSRCInstance(CT_UNSYNC_ASSOC_DELTA, omc_csp_broker, op, properties, &rc);
		if(CMIsNullObject(inst))
			return rc;
		CMReturnInstance(rslt, inst);
		CMRelease(inst);
 	}
	else if(!strcasecmp(className, StorageReplicationElementCapabilitiesAssnName))
	{
		OMC_CreateSRECInstances(-1, omc_csp_broker, ctx, rslt, op, properties, &rc);
		if(rc.rc != CMPI_RC_OK)
			return rc;
		
	}
	else if(!strcasecmp(className, StorageSynchronizedAssnName))
	{
		OMC_EnumSSInstances(LogicalDiskClassName, omc_csp_broker, ctx, rslt, op, properties, &rc);
		if(rc.rc != CMPI_RC_OK)
			return rc;
		
	}
	else if(!strcasecmp(className, SVStorageSynchronizedAssnName))
	{
		OMC_EnumSSInstances(StorageVolumeClassName, omc_csp_broker, ctx, rslt, op, properties, &rc);
		if(rc.rc != CMPI_RC_OK)
			return rc;
		
	}
	
 	
	if(thisMI)
		CMReturnDone(rslt);

	return rc;	
}

CMPIStatus CopyServicesProviderGetInstance(
		CMPIInstanceMI* thisMI,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const char** properties
		)
{
	const char *className;
	CMPIInstance *inst=NULL;
	CMPIString *name=NULL;
	CMPIObjectPath *oPath=NULL, *oPath2=NULL;
	CMPIStatus rc={CMPI_RC_OK, NULL};

	DebugMsg("CopyServices GetInstance\n");
	
	className = CMGetCharPtr(CMGetClassName(op, &rc));

	if(rc.rc != CMPI_RC_OK)
	{
		DebugMsg("Unable to get class name\n");
		CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return rc;
	}

	if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
	{
		name = CMGetKey(op, InstanceIDProperty, &rc).value.string;
		if(name == NULL)
		{
			DebugMsg("Unable to get key from OP\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(CMGetCharPtr(name), StorageReplicationCapabilitiesUnsyncAssocDelta))
		{
      	     		inst = OMC_CreateSRCInstance(CT_UNSYNC_ASSOC_DELTA, omc_csp_broker, op, properties, &rc);
			if(CMIsNullObject(inst))
				return rc;
			CMReturnInstance(rslt, inst);
			CMRelease(inst);
		}
	}
	else if(!strcasecmp(className, StorageReplicationElementCapabilitiesAssnName))
	{
		oPath = CMGetKey(op, CapabilitiesProperty, &rc).value.ref;
		name = CMGetKey(oPath, InstanceIDProperty, &rc).value.string;
		if(name == NULL)
		{
			DebugMsg("Unable to get key from SRC OP\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
                if(!strcasecmp(CMGetCharPtr(name), StorageReplicationCapabilitiesUnsyncAssocDelta)) 
		{
                        OMC_CreateSRECInstances(CT_UNSYNC_ASSOC_DELTA, omc_csp_broker, ctx, rslt, op, properties, &rc);
			if(rc.rc != CMPI_RC_OK)
				return rc;
		}
	}
	else if(!strcasecmp(className, StorageSynchronizedAssnName))
	{
		char isReturned = 0;
		oPath = CMGetKey(op, SystemElementProperty, &rc).value.ref;
		oPath2 = CMGetKey(op, SyncedElementProperty, &rc).value.ref;
 	       	OMC_CreateSSInstances(LogicalDiskClassName, omc_csp_broker, ctx, rslt, oPath, oPath2, &isReturned, properties, &rc); 
		if(rc.rc != CMPI_RC_OK)
			return rc;
		if(!isReturned)
		{
			DebugMsg("Unable to get SS association inst for this OP\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
	}
 
	 else if(!strcasecmp(className, SVStorageSynchronizedAssnName))
 	{
		char isReturned = 0;
		oPath = CMGetKey(op, SystemElementProperty, &rc).value.ref;
		oPath2 = CMGetKey(op, SyncedElementProperty, &rc).value.ref;
		OMC_CreateSSInstances(StorageVolumeClassName, omc_csp_broker, ctx, rslt, oPath, oPath2, &isReturned, properties, &rc); 
	 	if(rc.rc != CMPI_RC_OK)
			return rc;
		if(!isReturned)
		{
			DebugMsg("Unable to get SVStorageSynchronizedAssnName association inst for this OP\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
	 	}
 	}
	CMReturnDone(rslt);
        return rc;
}



CMPIStatus CopyServicesProviderModifyInstance
		(CMPIInstanceMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const CMPIInstance* inst,
		const char** properties
		)
{
	CMPIStatus rc={CMPI_RC_OK, NULL};
	CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_NOT_SUPPORTED, "Function not supported");
	return rc;
} 

CMPIStatus CopyServicesProviderSetInstance
		(CMPIInstanceMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const CMPIInstance* inst,
		const char** properties
		)
{
	return CopyServicesProviderModifyInstance(mi, ctx, rslt, op, inst, properties);
} 
 
CMPIStatus CopyServicesProviderCreateInstance(
		CMPIInstanceMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const CMPIInstance* inst
		)
{
	CMPIStatus rc={CMPI_RC_OK, NULL};
	CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_NOT_SUPPORTED, "Function not supported");
	return rc;
}

CMPIStatus CopyServicesProviderDeleteInstance(
		CMPIInstanceMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op
		)
{
	const char *className;
	CMPIStatus rc={CMPI_RC_OK, NULL};
	CMPIObjectPath *oPath=NULL, *oPath2=NULL;

	DebugMsg("CopyServices Delete Instance\n");
	
	className = CMGetCharPtr(CMGetClassName(op, &rc));

	if(rc.rc != CMPI_RC_OK)
	{
		DebugMsg("Unable to get class name\n");
		CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
		return rc;
	}

	if(!strcasecmp(className, StorageSynchronizedAssnName))
	{
		oPath = CMGetKey(op, SystemElementProperty, &rc).value.ref;
		oPath2 = CMGetKey(op, SyncedElementProperty, &rc).value.ref;
		OMC_DeleteSSInstance(LogicalDiskClassName, omc_csp_broker, ctx, rslt, oPath, oPath2, &rc);
	}
	else if(!strcasecmp(className, SVStorageSynchronizedAssnName))
	{
		oPath = CMGetKey(op, SystemElementProperty, &rc).value.ref;
		oPath2 = CMGetKey(op, SyncedElementProperty, &rc).value.ref;
		OMC_DeleteSSInstance(StorageVolumeClassName,omc_csp_broker, ctx, rslt, oPath, oPath2, &rc);
	}

        if(rc.rc == CMPI_RC_OK)
	{
		CMReturnDone(rslt);
	}
	return rc;
}

CMPIStatus CopyServicesProviderExecQuery(
		CMPIInstanceMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const char* query,
		const char* lang
		)
{
	CMPIStatus rc={CMPI_RC_OK, NULL};
	CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_NOT_SUPPORTED, "Function not supported");
	return rc;
} 


/*
 *
 *	Association Provider Functions
 * 
 */


CMPIStatus CopyServicesProviderAssociatorNames(
		CMPIAssociationMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const char* assocClass,
		const char* resultClass,
		const char* role,
		const char* resultRole
		)
{
	char isReturned=0;
	const char *className;
        CMPIStatus rc={CMPI_RC_OK, NULL};
        CMPIObjectPath *srcOP=NULL;

	className = CMGetCharPtr(CMGetClassName(op, &rc));

	if(!assocClass || !strcasecmp(assocClass, StorageReplicationElementCapabilitiesAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageConfigurationServiceClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, StorageReplicationCapabilitiesClassName)) &&
       				(!role || !strcasecmp(role, ManagedElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, CapabilitiesProperty)))
			{
				srcOP = CMNewObjectPath(omc_csp_broker, CMGetCharPtr(CMGetNameSpace(op, &rc)), 
						StorageReplicationCapabilitiesClassName, &rc);
				if(CMIsNullObject(srcOP))
				{
					DebugMsg("Unable to create SRC Object Path\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				rc = CopyServicesProviderEnumInstanceNames(NULL, ctx, rslt, srcOP);
				CMRelease(srcOP);
				isReturned = 1;
			}
// 			else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		else if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, StorageConfigurationServiceClassName)) &&
       				(!role || !strcasecmp(role, CapabilitiesProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, ManagedElementProperty)))
			{
				CMPIObjectPath *oPath=NULL;
				CMPIEnumeration *en=NULL;
				CMPIData scsOPData;
				
				oPath = CMNewObjectPath(omc_csp_broker, CMGetCharPtr(CMGetNameSpace(op, &rc)), 
						StorageConfigurationServiceClassName, &rc);
				if(CMIsNullObject(oPath))
				{
					DebugMsg("Unable to create SCS Object Path\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				en = omc_csp_broker->bft->enumerateInstanceNames(omc_csp_broker, ctx, oPath, &rc);
				if(rc.rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to enumInstanceName SCS\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				if(!CMHasNext(en, &rc))
				{
					DebugMsg("No OPs for SCS\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}

				scsOPData = CMGetNext(en, &rc);
				CMReturnObjectPath(rslt, scsOPData.value.ref);
				CMRelease(en);

        			isReturned = 1;
			}
// 			else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
//		else if(assocClass) 
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	}

	else if(!assocClass || !strcasecmp(assocClass, StorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, LogicalDiskClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, LogicalDiskClassName)) &&
       				(!role || !strcasecmp(role, SystemElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SyncedElementProperty)))
			{
				rc = OMC_SS_ReturnSyncedElementsOPs(LogicalDiskClassName, omc_csp_broker, op, ctx, rslt, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if ((!resultClass || !strcasecmp(resultClass, LogicalDiskClassName)) &&
       				(!role || !strcasecmp(role, SyncedElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SystemElementProperty)))
			{
				rc = OMC_SS_ReturnSourceElementOP(LogicalDiskClassName, omc_csp_broker, op, ctx, rslt, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
//  		else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		
//	  else if(assocClass) 
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	}
	else if(!assocClass || !strcasecmp(assocClass, SVStorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageVolumeClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, StorageVolumeClassName)) &&
       				(!role || !strcasecmp(role, SystemElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SyncedElementProperty)))
			{
				rc = OMC_SS_ReturnSyncedElementsOPs(StorageVolumeClassName, omc_csp_broker, op, ctx, rslt, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if ((!resultClass || !strcasecmp(resultClass, StorageVolumeClassName)) &&
       				(!role || !strcasecmp(role, SyncedElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SystemElementProperty)))
			{
				rc = OMC_SS_ReturnSourceElementOP(StorageVolumeClassName, omc_csp_broker, op, ctx, rslt, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
  //		else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		
//	  else if(assocClass) 
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	}	
//        if(!isReturned)
//		CMReturn(CMPI_RC_ERR_INVALID_CLASS);

	if(rc.rc == CMPI_RC_OK)
		CMReturnDone(rslt);

	return rc;
}

CMPIStatus CopyServicesProviderAssociators(
		CMPIAssociationMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* objName,
		const char* assocClass,
		const char* resultClass,
		const char* role,
		const char* resultRole,
		const char** properties
		)
{
	char isReturned=0;
	const char *className;
        CMPIStatus rc={CMPI_RC_OK, NULL};
        CMPIObjectPath *srcOP=NULL;
		
	className = CMGetCharPtr(CMGetClassName(objName, &rc));

	if(!assocClass || !strcasecmp(assocClass, StorageReplicationElementCapabilitiesAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageConfigurationServiceClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, StorageReplicationCapabilitiesClassName)) &&
       				(!role || !strcasecmp(role, ManagedElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, CapabilitiesProperty)))
			{
				srcOP = CMNewObjectPath(omc_csp_broker, CMGetCharPtr(CMGetNameSpace(objName, &rc)), 
						StorageReplicationCapabilitiesClassName, &rc);
				if(CMIsNullObject(srcOP))
				{
					DebugMsg("Unable to create SRC Object Path\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				rc = CopyServicesProviderEnumInstances(NULL, ctx, rslt, srcOP, properties);
				CMRelease(srcOP);
	       			
				isReturned = 1;
			}
 //			else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
		else if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, StorageConfigurationServiceClassName)) &&
       				(!role || !strcasecmp(role, CapabilitiesProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, ManagedElementProperty)))
			{
				CMPIObjectPath *oPath=NULL;
				CMPIEnumeration *en=NULL;
				CMPIData scsOPData;
				
				oPath = CMNewObjectPath(omc_csp_broker, CMGetCharPtr(CMGetNameSpace(objName, &rc)), 
						StorageConfigurationServiceClassName, &rc);
				if(CMIsNullObject(oPath))
				{
					DebugMsg("Unable to create SCS Object Path\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				en = omc_csp_broker->bft->enumerateInstances(omc_csp_broker, ctx, oPath, properties, &rc);
				if(rc.rc != CMPI_RC_OK)
				{
					DebugMsg("Unable to enumInstance SCS\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				if(!CMHasNext(en, &rc))
				{
					DebugMsg("No instance for SCS\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}

				scsOPData = CMGetNext(en, &rc);
				
				CMReturnInstance(rslt, scsOPData.value.inst);
				CMRelease(en);

	       			isReturned = 1;
			}
// 			else if(assocClass) 
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	}

	else if(!assocClass || !strcasecmp(assocClass, StorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, LogicalDiskClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, LogicalDiskClassName)) &&
       				(!role || !strcasecmp(role, SystemElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SyncedElementProperty)))
			{
				rc = OMC_SS_ReturnSyncedElementsInsts(className, omc_csp_broker, objName, ctx, rslt, properties, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if ((!resultClass || !strcasecmp(resultClass, LogicalDiskClassName)) &&
       				(!role || !strcasecmp(role, SyncedElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SystemElementProperty)))
			{
				rc = OMC_SS_ReturnSourceElementInst(className, omc_csp_broker, objName, ctx, rslt, properties, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
//  			else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
//	       	else if(assocClass) 
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	}
	else if(!assocClass || !strcasecmp(assocClass, SVStorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageVolumeClassName))
		{
			if ((!resultClass || !strcasecmp(resultClass, StorageVolumeClassName)) &&
       				(!role || !strcasecmp(role, SystemElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SyncedElementProperty)))
			{
				rc = OMC_SS_ReturnSyncedElementsInsts(className,omc_csp_broker, objName, ctx, rslt, properties, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if ((!resultClass || !strcasecmp(resultClass, StorageVolumeClassName)) &&
       				(!role || !strcasecmp(role, SyncedElementProperty)) &&
       				(!resultRole || !strcasecmp(resultRole, SystemElementProperty)))
			{
				rc = OMC_SS_ReturnSourceElementInst(className, omc_csp_broker, objName, ctx, rslt, properties, &isReturned);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
  //			else if(assocClass)
//				CMReturn(CMPI_RC_ERR_INVALID_CLASS);
		}
//	       	else if(assocClass) 
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	}
 
//	if(!isReturned)
//		CMReturn(CMPI_RC_ERR_INVALID_CLASS);
	
	if(rc.rc == CMPI_RC_OK)
		CMReturnDone(rslt);

       	return rc;
}
 
CMPIStatus CopyServicesProviderReferenceNames(
		CMPIAssociationMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* op,
		const char* resultClass, 
		const char* role
		)
{
	char isReturned=0;
	const char *assocClass = resultClass; 
	const char *className, *name;
        CMPIStatus rc={CMPI_RC_OK, NULL};

	className = CMGetCharPtr(CMGetClassName(op, &rc));

	if(!assocClass || !strcasecmp(assocClass, StorageReplicationElementCapabilitiesAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageConfigurationServiceClassName))
		{
			if(!role || !strcasecmp(role, ManagedElementProperty))
			{
				OMC_CreateSRECObjectPaths(-1, omc_csp_broker, ctx, rslt, op, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
				
				isReturned = 1;	
        		}
 			else if(assocClass)
				CMReturn(CMPI_RC_ERR_NO_SUCH_PROPERTY);
		}
		else if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
		{
			if(!role || !strcasecmp(role, CapabilitiesProperty))
			{
				name = CMGetCharPtr(CMGetKey(op, InstanceIDProperty, &rc).value.string);
				if(name == NULL)
				{
					DebugMsg("Unable to get key from OP\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				if(!strcasecmp(name, StorageReplicationCapabilitiesUnsyncAssocDelta))
				{
					OMC_CreateSRECObjectPaths(CT_UNSYNC_ASSOC_DELTA, omc_csp_broker, ctx, rslt, op, &rc);
					if(rc.rc != CMPI_RC_OK)
						return rc;
					isReturned = 1;	
	      			}
			 	else if(assocClass) 
					CMReturn(CMPI_RC_ERR_NO_SUCH_PROPERTY);
			}
 			else if(assocClass)
				CMReturn(CMPI_RC_ERR_NO_SUCH_PROPERTY);
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
       	}
	else if(!assocClass || !strcasecmp(assocClass, StorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, LogicalDiskClassName))
		{
			if(!role || !strcasecmp(role, SystemElementProperty))
			{
				OMC_CreateSSObjectPaths(className, omc_csp_broker, ctx, rslt, op, NULL, &isReturned, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if(!role || !strcasecmp(role, SyncedElementProperty))
			{
				OMC_CreateSSObjectPaths(className, omc_csp_broker, ctx, rslt, NULL, op, &isReturned, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}       
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
       	}
	
	if(!assocClass || !strcasecmp(assocClass, SVStorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageVolumeClassName))
		{
			if(!role || !strcasecmp(role, SystemElementProperty))
			{
				OMC_CreateSSObjectPaths(className, omc_csp_broker, ctx, rslt, op, NULL, &isReturned, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if(!role || !strcasecmp(role, SyncedElementProperty))
			{
				OMC_CreateSSObjectPaths(className, omc_csp_broker, ctx, rslt, NULL, op, &isReturned, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}       
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
       	}
 	
//	if(!isReturned)
//		CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	
	return rc;	
}


CMPIStatus CopyServicesProviderReferences(
		CMPIAssociationMI* mi,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath* objName,
		const char* resultClass, 
		const char* role,
		const char** properties
		)
{
	char isReturned=0;
	const char *assocClass = resultClass; 
	const char *className, *name;
        CMPIStatus rc={CMPI_RC_OK, NULL};

	className = CMGetCharPtr(CMGetClassName(objName, &rc));

	if(!assocClass || !strcasecmp(assocClass, StorageReplicationElementCapabilitiesAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageConfigurationServiceClassName))
		{
			if(!role || !strcasecmp(role, ManagedElementProperty))
			{
				OMC_CreateSRECInstances(-1, omc_csp_broker, ctx, rslt, objName, properties, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
				isReturned = 1;	
        		}
 			else if(assocClass)
				CMReturn(CMPI_RC_ERR_NO_SUCH_PROPERTY);
		}
		else if(!strcasecmp(className, StorageReplicationCapabilitiesClassName))
		{
			if(!role || !strcasecmp(role, CapabilitiesProperty))
			{
				name = CMGetCharPtr(CMGetKey(objName, InstanceIDProperty, &rc).value.string);
				if(name == NULL)
				{
					DebugMsg("Unable to get key from OP\n");
					CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
					return rc;
				}
				if(!strcasecmp(name, StorageReplicationCapabilitiesUnsyncAssocDelta))
				{
					OMC_CreateSRECInstances(CT_UNSYNC_ASSOC_DELTA, omc_csp_broker, ctx, rslt, objName, 
							properties, &rc);
					if(rc.rc != CMPI_RC_OK)
						return rc;
					isReturned = 1;	
	      			}
			 	else if(assocClass) 
					CMReturn(CMPI_RC_ERR_NO_SUCH_PROPERTY);
			}
 //			else if(assocClass)
//				CMReturn(CMPI_RC_ERR_NO_SUCH_PROPERTY);
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
       	}

	else if(!assocClass || !strcasecmp(assocClass, StorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, LogicalDiskClassName))
		{
			if(!role || !strcasecmp(role, SystemElementProperty))
			{
				OMC_CreateSSInstances(className, omc_csp_broker, ctx, rslt, objName, NULL, &isReturned, properties, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if(!role || !strcasecmp(role, SyncedElementProperty))
			{
				OMC_CreateSSInstances(className, omc_csp_broker, ctx, rslt, NULL, objName, &isReturned, properties, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}       
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
       	} 
	if(!assocClass || !strcasecmp(assocClass, SVStorageSynchronizedAssnName))
	{
		if(rc.rc != CMPI_RC_OK)
		{
			DebugMsg("Unable to get class name\n");
			CMSetStatusWithChars(omc_csp_broker, &rc, CMPI_RC_ERR_FAILED, "CMPI_RC_ERR_FAILED");
			return rc;
		}
		if(!strcasecmp(className, StorageVolumeClassName))
		{
			if(!role || !strcasecmp(role, SystemElementProperty))
			{
				OMC_CreateSSInstances(className, omc_csp_broker, ctx, rslt, objName, NULL, &isReturned, properties, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}
			if(!role || !strcasecmp(role, SyncedElementProperty))
			{
				OMC_CreateSSInstances(className, omc_csp_broker, ctx, rslt, NULL, objName, &isReturned, properties, &rc);
				if(rc.rc != CMPI_RC_OK)
					return rc;
			}       
		}
//		else if(assocClass)
//			CMReturn(CMPI_RC_ERR_INVALID_CLASS);
       	}

//	if(!isReturned)
//		CMReturn(CMPI_RC_ERR_INVALID_CLASS);
 	
	return rc;	
}
 
CMPIStatus CopyServicesProviderAssociationCleanup(
		CMPIAssociationMI* mi, 
		const CMPIContext* ctx, 
		CMPIBoolean terminating
		)
{
	CMReturn(CMPI_RC_OK);
}


/*
 *
 * 	Provider Factory Functions
 * 
 */ 

CMInstanceMIStub(
		CopyServicesProvider, 
		omc_smi_snapshot, 
		omc_csp_broker, 
		CMNoHook);
CMAssociationMIStub(
		CopyServicesProvider, 
		omc_smi_snapshot, 
		omc_csp_broker, 
		CMNoHook);

/*			-:END OF PROVIDER:-			*/

