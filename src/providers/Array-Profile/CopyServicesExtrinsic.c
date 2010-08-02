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
 |	 OMC SMI-S Volume Management provider
 |
 |---------------------------------------------------------------------------
 |
 | $Id: 
 |
 |---------------------------------------------------------------------------
 | This module contains:
 |   Provider code dealing with extrinsic methods of Copy Services class
 |
 +-------------------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" {
#endif

#include<libintl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>

#include <cmpiutil/base.h>
#include <cmpiutil/modifyFile.h>
#include <cmpiutil/string.h>
#include <cmpiutil/cmpiUtils.h>

#ifdef __cplusplus
}
#endif

#include "CopyServicesExtrinsic.h" 
#include "Utils.h"
#include "LvmCopyServices.h"
#include "StorageConfigurationService.h"
#include "ArrayProvider.h"
 
#define MAX_NAME_SIZE 50

int CopyServicesCreateReplica(
        CMPIBroker *_BROKER,
	const char *ns,
	const CMPIContext* context,
	const CMPIObjectPath* cop,
	const char *methodName,
	const CMPIArgs *in,
	CMPIArgs *out,
	const CMPIResult* results,
	CMPIStatus *pStatus)
{
	CMPIStatus status = {CMPI_RC_OK, NULL};
        CMPICount inArgCount, outArgCount;
	CMPIData inData, keyData, retData, argData, keyData2, ldOPData;
	CMPIValue elmType,goalCop,targetPoolCop,elementCop;
        char *source_volume = NULL;
	const char *snapshot_volume = NULL;
	const char *elmName;
	//CMPIObjectPath  *goalCop, *sourceElmCop, *targetPoolCop, 
	//		*elementCop, *oPath;
	CMPIObjectPath *oPath,*sourceElmCop;
	CMPIUint16 copyType = 0;
	CMPIUint64 size = 0;
        CMPIString *className, *volume;
        CMPIUint32 rc = M_COMPLETED_OK;
        CMPIArgs *in2 = NULL, *out2 = NULL;
	CMPIEnumeration *en=NULL;
	elmType; 
/*	inArgCount = CMGetArgCount(in, pStatus);
	if ( (pStatus->rc != CMPI_RC_OK) || (inArgCount < 5) )
	{
		_SMI_TRACE(0,("Required input parameter missing in call to CreateReplica method."));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to CreateReplica method");
		rc = M_INVALID_PARAM;
		goto Exit;
	}

	outArgCount = CMGetArgCount(out, pStatus);
	if ( (pStatus->rc != CMPI_RC_OK) ) 
		{
			_SMI_TRACE(0,("Required output parameter missing in call to CreateReplica method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
					"Required output parameter missing in call to CreateReplica method");
			rc = M_INVALID_PARAM;
			goto Exit;
		}
	_SMI_TRACE(1,("Out Arg Count = %d", outArgCount));
	if (outArgCount == 0 )
	{
		out = CMNewArgs(_BROKER, pStatus);
		_SMI_TRACE(1,("After CMNewArgs"));
		*pStatus = CMAddArg(out, "Job", NULL, CMPI_ref);
               	if(pStatus->rc != CMPI_RC_OK)
		{
				rc = M_FAILED; 
				goto Exit;
		}
		*pStatus = CMAddArg(out, "TargetElement", NULL, CMPI_ref);
               	if(pStatus->rc != CMPI_RC_OK)
		{
				rc = M_FAILED; 
				goto Exit;
		}
	}
		CMPICount outArgCount1 = CMGetArgCount(out, pStatus);
		_SMI_TRACE(1,("---outArgCount : %d",outArgCount1));

	if ( (pStatus->rc != CMPI_RC_OK) || (outArgCount1 < 2) )
	{
		_SMI_TRACE(0,("Required output parameter missing in call to CreateReplica method."));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required output parameter missing in call to CreateReplica method");
		rc = M_INVALID_PARAM;
		goto Exit;
	} */

        inData = CMGetArg(in, "ElementName", NULL);
	if (inData.state == CMPI_goodValue)
	{
		elmName = CMGetCharsPtr(inData.value.string, NULL);
		_SMI_TRACE(1,("inElmName = %s", elmName));
	}
	
	inData = CMGetArg(in, "SourceElement", NULL);
	if (inData.state == CMPI_goodValue)
	{
		sourceElmCop = inData.value.ref;
	}

	inData = CMGetArg(in, "TargetSettingGoal", NULL);
	if (inData.state == CMPI_goodValue)
	{
		_SMI_TRACE(0,("Snapshots can be created only on the source pool, no Target Goal can be specified"));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Snapshots can be created only on the source pool, no Target Goal can be specified");
		rc = M_INVALID_PARAM;
		goto Exit;
//		goalCop.ref = inData.value.ref;
	}

	inData = CMGetArg(in, "TargetPool", NULL);
	if (inData.state == CMPI_goodValue)
	{
		_SMI_TRACE(0,("Snapshots can be created only on the source pool, no Target Pool can be specified"));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Snapshots can be created only on the source pool, no Target Pool can be specified");
		rc = M_INVALID_PARAM;
		goto Exit;
//		targetPoolCop.ref = inData.value.ref;
	}

	inData = CMGetArg(in, "CopyType", NULL);
	if (inData.state == CMPI_goodValue)
	{
		copyType = inData.value.uint16;
	}

	if(copyType == 4) //UnsyncAssoc
	{
		_SMI_TRACE(1,("In side UnsyncAssoc"));
		className = CMGetClassName(sourceElmCop, pStatus);
		
		
		if ((strcasecmp(CMGetCharsPtr(className, pStatus), LogicalDiskClassName))  &&  strcasecmp(CMGetCharsPtr(className, pStatus), StorageVolumeClassName))
		{
			_SMI_TRACE(0,("Expecting OMC_LogicalDisk or OMC_StorageVolume for source element in CreateReplica Method"));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
					"Expecting OMC_LogicalDisk or OMC_StorageVolume for source element in CreateReplica Method");
			rc = M_INVALID_PARAM;
			goto Exit;
		}
		keyData = CMGetKey(sourceElmCop, "DeviceID", NULL);
		rc = cs_return_associated_source_volume(CMGetCharPtr(keyData.value.string), &source_volume);
		if(!rc)
		{
			if(source_volume)
			{
				free(source_volume);
				source_volume = NULL;
				_SMI_TRACE(0,("CreateReplica: can not snapshot a snapshotted volume."));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
						"CreateReplica: can not snapshot a snapshotted volume.");
				rc = M_INVALID_PARAM;
				goto Exit; 
			}
		}
		snapshot_volume = (const char *) malloc (MAX_NAME_SIZE * sizeof(char));
		if (strcasecmp(CMGetCharsPtr(className, pStatus), LogicalDiskClassName))
		{
			sprintf((char *)snapshot_volume, "%s%s", "SV_", elmName);
		}	
  		else
		{
			sprintf((char *)snapshot_volume, "%s%s", "LD_", elmName);
		}	
/*		_SMI_TRACE(1,("Before CMNewArgs"));
		in2 = CMNewArgs(_BROKER, pStatus);
                if(pStatus->rc != CMPI_RC_OK)
			{
				rc = M_FAILED; goto Exit;}
		out2 = CMNewArgs(_BROKER, pStatus);
		_SMI_TRACE(1,("After CMNewArgs"));

		//_SMI_TRACE(1,("inElmName = %s", elmName.string));
		*pStatus = CMAddArg(in2, "ElementName", elmName, CMPI_chars);
                if(pStatus->rc != CMPI_RC_OK)
		{
			
			//_SMI_TRACE(1,("inElmName = %s", elmName.string));
			rc = M_FAILED;
			goto Exit;
		}

		if (strcasecmp(CMGetCharsPtr(className, pStatus), "OMC_LogicalDisk"))
			elmType.uint16 = 2;
  		else
			elmType.uint16 = 4;
		_SMI_TRACE(1,("inElmType = %d", elmType.uint16));
		*pStatus = CMAddArg(in2, "ElementType", &elmType, CMPI_uint16);
                if(pStatus->rc != CMPI_RC_OK)
		{
			_SMI_TRACE(1,("inElmType = %d", elmType.uint16));
			rc = M_FAILED; 
			goto Exit;
		}
		_SMI_TRACE(1,("Size"));
		*pStatus = CMAddArg(in2, "Size", &size, CMPI_uint64);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}
	
		_SMI_TRACE(1,("In Pool"));
		*pStatus = CMAddArg(in2, "InPool", &targetPoolCop, CMPI_ref);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

		_SMI_TRACE(1,("Goal"));
		*pStatus = CMAddArg(in2,"Goal",NULL,CMPI_ref);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

	
		*pStatus = CMAddArg(in2, "TheElement", NULL, CMPI_ref);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

		*pStatus = CMAddArg(out2, "Job", NULL, CMPI_ref);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

		*pStatus = CMAddArg(out2, "Size", NULL, CMPI_uint64);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

		*pStatus = CMAddArg(out2, "TheElement",NULL, CMPI_ref);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

		_SMI_TRACE(1,("Before InvokeMethod"));
		inArgCount = CMGetArgCount(in2, pStatus);
		_SMI_TRACE(1,("inArgCount to CreateOrModifyElementFromStoragePool is %d",inArgCount));
		outArgCount = CMGetArgCount(out2, pStatus);
		_SMI_TRACE(1,("inArgCount to CreateOrModifyElementFromStoragePool is %d",outArgCount));
		_SMI_TRACE(1,("cop  %u",*_BROKER->bft->enumerateInstanceNames));
		_SMI_TRACE(1,("cop  %u",*_BROKER->bft->invokeMethod));
		//_SMI_TRACE(1,("cop  String class Nmae %s",CMObjectPathToString(cop,NULL)));
		
  		 
                //retData = _BROKER->bft->invokeMethod(_BROKER, context, cop, 
		//		"CreateOrModifyElementFromStoragePool", in2, out2, pStatus);
		_SMI_TRACE(0,("method Name--------- %s",method));
		retData = CBInvokeMethod(_BROKER, context, cop, method, in2, out2, pStatus);
		_SMI_TRACE(0,("CBInvokeMethod ---- rc= %d",retData.value.uint32));
		_SMI_TRACE(1,("After InvokeMethod"));

		if(retData.value.uint32 != 0 || pStatus->rc !=CMPI_RC_OK)
		{
			//rc = M_FAILED; 
			rc = retData.value.uint32;
			goto Exit;
		}

		argData = CMGetArg(out2, "TheElement", pStatus);
                if(pStatus->rc != CMPI_RC_OK)
			{rc = M_FAILED; goto Exit;}

		keyData2 = CMGetKey(argData.value.ref, "DeviceID", NULL);
*/
		_SMI_TRACE(1,("Calling cs_evms_take_snapshot, snapshot name %s", snapshot_volume));
		rc = cs_take_snapshot(CMGetCharPtr(keyData.value.string), 
				snapshot_volume);
		
		_SMI_TRACE(1,("cs_evms_take_snapshot ---- rc= %d",rc));
		if(rc != 0)
		{
			_SMI_TRACE(0,("Unable to take snapshot."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
					"Unable to take snapshot.");
			rc = M_FAILED;
			goto Exit;       
		}	
/*		if (!strcasecmp(CMGetCharsPtr(className, pStatus), "OMC_LogicalDisk"))
		{
			oPath = CMNewObjectPath(_BROKER, CMGetCharPtr(CMGetNameSpace(cop, pStatus)), 
				"OMC_LogicalDisk", pStatus);

			en = _BROKER->bft->enumerateInstanceNames(_BROKER, context, oPath, pStatus);
			_SMI_TRACE(1,("CBEnumInstance ---- rc= %d",pStatus->rc));
			if(pStatus->rc != CMPI_RC_OK)
			{
				_SMI_TRACE(0,("Unable to enumerate OMC_LogicalDisk."));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
					"Unable to take snapshot.");
				rc = M_FAILED;
				goto Exit;       
			}
		}
		else if(!strcasecmp(CMGetCharsPtr(className, pStatus), "OMC_StorageVolume"))
		{
			oPath = CMNewObjectPath(_BROKER, CMGetCharPtr(CMGetNameSpace(cop, pStatus)), 
				"OMC_LogicalDisk", pStatus);

			en = _BROKER->bft->enumerateInstanceNames(_BROKER, context, oPath, pStatus);
			_SMI_TRACE(1,("CBEnumInstance ---- rc= %d",pStatus->rc));
			if(pStatus->rc != CMPI_RC_OK)
			{
				_SMI_TRACE(0,("Unable to enumerate OMC_LogicalDisk."));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
					"Unable to take snapshot.");
				rc = M_FAILED;
				goto Exit;       
			}
		}
		else
		{
			_SMI_TRACE(0,("Expecting OMC_LogicalDisk or OMC_StorageVolume for source element in CreateReplica Method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
					"Expecting OMC_LogicalDisk or OMC_StorageVolume for source element in CreateReplica Method.");
			rc = M_INVALID_PARAM;
			goto Exit;
		}
 
		while(CMHasNext(en, pStatus))
		{
			ldOPData = CMGetNext(en, pStatus);
			keyData = CMGetKey(ldOPData.value.ref, "DeviceID", pStatus);
			_SMI_TRACE(1,("Has next ---- rc= %d",pStatus->rc));
			//Vijay
			if(pStatus->rc != CMPI_RC_OK)
			{
				_SMI_TRACE(0,("Unable to enumerate OMC_LogicalDisk."));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
						"Unable to take snapshot.");
				rc = M_FAILED;
				CMRelease(en);
				goto Exit;       
			}
			_SMI_TRACE(0,("keyData--------- %s",CMGetCharPtr(keyData.value.string)));
			_SMI_TRACE(0,("keyData2--------- %s",CMGetCharPtr(keyData2.value.string)));
			if(!strcasecmp(CMGetCharPtr(keyData.value.string), CMGetCharPtr(keyData2.value.string)))
			{
				CMAddArg(out, "Job", CMPI_null, CMPI_ref);
				CMAddArg(out, "TargetElement", ldOPData.value.ref, CMPI_ref);
		//		CMAddArg(out, "TargetElement", NULL, CMPI_ref);
				_SMI_TRACE(0,("cs_  end return ---- rc= %d",rc));
				return rc;
			}
		}
		CMRelease(en);
*/
		
	}
  
	
Exit:
	// Update our internal model
	SCSScanStorage(ns, pStatus);

	CMAddArg(out, "Job", CMPI_null, CMPI_ref);
	CMAddArg(out, "TargetElement", NULL, CMPI_ref);
	//if(in2)
	//	CMRelease(in2);
	//if(out2)
	//	CMRelease(out2);

	//CMReturnDone(results);
	//_SMI_TRACE(1,("Leaving InvokeMethod(): %s", (status.rc == CMPI_RC_OK)? "succeeded":"failed"));
	_SMI_TRACE(1,("cs_  end return ---- rc= %d",rc));
	return rc;
}

/*
int CopyServicesModifySynchronization(
        CMPIBroker *_BROKER,
	const char *ns,
	const CMPIContext* context,
	const CMPIObjectPath* cop,
	const char *methodName,
	const CMPIArgs *in,
	CMPIArgs *out,
	const CMPIResult* results,
	CMPIStatus *pStatus)
{
        CMPICount inArgCount, outArgCount;
	CMPIData inData, keyData, keyData2, sourceData, syncData;
	char *source_volume = NULL;
	const char *source_vol, *sync_vol;
	CMPIUint16 operation;
        CMPIUint32 rc = M_COMPLETED_OK;
	int rv;
 
	
	inArgCount = CMGetArgCount(in, pStatus);
	if ( (pStatus->rc != CMPI_RC_OK) || (inArgCount < 2) )
	{
		_SMI_TRACE(0,("Required input parameter missing in call to ModifySynchronization method."));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Required input parameter missing in call to ModifySynchronization method");
		rc = M_INVALID_PARAM;
		goto Exit;
	}

	outArgCount = CMGetArgCount(out, pStatus);
	if ( (pStatus->rc != CMPI_RC_OK) ) 
		{
			_SMI_TRACE(0,("Required output parameter missing in call to ModifySynchronization method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
					"Required output parameter missing in call to ModifySynchronization method");
			rc = M_INVALID_PARAM;
			goto Exit;
		}
	//if ( (pStatus->rc != CMPI_RC_OK) || (outArgCount < 1) )
	//{
	//	_SMI_TRACE(0,("Required output parameter missing in call to ModifySynchronization method."));
	//	CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
	//			"Required output parameter missing in call to ModifySynchronization method");
	//	rc = M_INVALID_PARAM;
	//	goto Exit;
	//} 

        inData = CMGetArg(in, "Operation", NULL);
	if (inData.state == CMPI_goodValue)
	{
		operation = inData.value.uint16; 
		_SMI_TRACE(1,("operation = %d", operation));
	}

	inData = CMGetArg(in, "Synchronization", NULL);
	if (inData.state != CMPI_goodValue)
	{
		_SMI_TRACE(0,("Unable to extract Synchronization ref: ModifySynchronization method."));
		CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
				"Unable to extract Synchronization ref: ModifySynchronization method.");
		rc = M_INVALID_PARAM;
		goto Exit;
	}
	
	keyData = CMGetKey(inData.value.ref, "SystemElement", pStatus);
	keyData2 = CMGetKey(inData.value.ref, "SyncedElement", pStatus);
        sourceData = CMGetKey(keyData.value.ref, "DeviceID", pStatus);
        syncData = CMGetKey(keyData2.value.ref, "DeviceID", pStatus);
	source_vol = CMGetCharPtr(sourceData.value.string);
	sync_vol = CMGetCharPtr(syncData.value.string);

        if(1) // DRBD guys must pass local volumes case here
	{
		if(operation == 4 || operation == 5)
		{
			rv = cs_return_associated_source_volume(sync_vol, &source_volume); 
			if(rv || !source_vol || strcasecmp(source_volume, source_vol))
			{
				_SMI_TRACE(0,("Synchronization relation does not exist: ModifySynchronization method."));
				CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_INVALID_PARAMETER, 
						"Synchronization relation does not exist: ModifySynchronization method.");
				rc = M_INVALID_PARAM;
				goto Exit;
 			}
			if(operation == 4) //Resync
			{
				//Vijay rv = cs_evms_reset_snapshot(snap_vol);
				rv = cs_reset_snapshot(sync_vol);
				if(rv)
				{
					_SMI_TRACE(0,("Unable to reset/resync snapshot. The snapshot must be active "
								"but unmounted for it to be reinitialized"
								": ModifySynchronization method."));
					CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
							"Unable to reset/resync snapshot. The snapshot must be active "
							"but unmounted for it to be reinitialized"
							": ModifySynchronization method.");
					rc = M_FAILED;
					goto Exit;       
				}
			}
			else //Restore
			{
				rv = cs_rollback_snapshot(sync_vol);
				if(rv)
				{
					_SMI_TRACE(0,("Unable to rollback/restore snapshot. Both volumes must be unmounted."
								"There should be only one snapshot of source volume. "
								": ModifySynchronization method."));
					CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_FAILED, 
							"Unable to reset/resync snapshot. Both volumes must be unmounted."
							"There should be only one snapshot of source volume. "
							": ModifySynchronization method.");
					rc = M_FAILED;
					goto Exit;       
				}
        		}
			CMAddArg(out, "Job", CMPI_null, CMPI_ref);
		}
		else
		{
			_SMI_TRACE(0,("This operation is not supported: ModifySynchronization method."));
			CMSetStatusWithChars(_BROKER, pStatus, CMPI_RC_ERR_NOT_SUPPORTED, 
					"This operation is not supported: ModifySynchronization method.");
			rc = M_NOT_SUPPORTED;
			goto Exit;
		}
	
	}

Exit:
	if(source_volume)
		free(source_volume);
	CMReturnData(results, &rc, CMPI_uint32);
	return rc; 	
  
}
*/
