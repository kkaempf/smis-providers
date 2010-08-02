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
 |   Provider code dealing with Copy Services extrinsic functions
 |
 +-------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include <libintl.h>
#include <cmpiutil/base.h>
#include <cmpiutil/modifyFile.h>
#include <cmpiutil/string.h>
#include <cmpiutil/cmpiUtils.h>

/* Include the required CMPI macros, data types, and API function headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#ifdef __cplusplus
}
#endif

#include <storage/StorageInterface.h>
#include "LvmCopyServices.h"
#include "StorageConfigurationService.h"
#include "StorageVolume.h"
#include "StoragePool.h"
#include "Utils.h"

//Raman commented
//#define EVMS_VERSION_MAJOR	10
//#define EVMS_VERSION_MINOR	1
//#define EVMS_VERSION_PATCH	0

#define MAX_NAME_SIZE 20
#define SOURCE_VOLUME 1
#define PERCENT_FULL 2

using namespace storage;

extern StorageInterface* s;


/* Get the Volume Group name corresponding to the volume
*/

int get_VG_name(const char* volume, char *vg_name)
{
	int rc=0;
//	ContVolInfo conVolInfo;
	StoragePool *antPool = NULL;
	StorageVolume *vol;
	_SMI_TRACE(0,("get_VG_name called for volume %s", volume));

	/* Get the container info corresponding to the volume
	
	if(rc = s->getContVolInfo (volume, conVolInfo))
	{	
		rc = M_FAILED;
		_SMI_TRACE(1,("Error calling getContVolInfo for getting the container info corresponding to the snapshot_volume , rc = %d", rc));
		return rc;
	}
	*/

	vol = VolumesFind(volume);
	if(vol == NULL)
	{
		_SMI_TRACE(1,("Volume cannot be found"));
		rc = M_FAILED;
		goto exit;
	}
	antPool = (StoragePool *)vol->antecedentPool.element;
	strcpy(vg_name, antPool->name);
exit:
	_SMI_TRACE(0,("Exiting get_VG_name, returning %s", vg_name));
	return rc;	

}


#if 0
int open_engine(engine_mode_t mode)
{
        evms_version_t ver;
	int rc=0;
	evms_get_api_version(&ver);
	if(ver.major != EVMS_VERSION_MAJOR ||
		ver.minor < EVMS_VERSION_MINOR ||
		(ver.minor == EVMS_VERSION_MINOR && ver.patchlevel < EVMS_VERSION_PATCH))
	{
		printf("Evms engine version not matched\n");
		return -1;
	}
        
	rc = evms_open_engine(NULL, mode, NULL, EVERYTHING, NULL);
	if(rc)
	{
		printf("evms error %d in opening engine\n", rc);
		return -1;
	}
	return 0;
 
}


void close_engine(void)
{
	evms_close_engine();
}

#endif


/* Get the Volume size corresponding to the volume name
*/

int get_vol_size(const char *volume_name, unsigned long long &return_info)
{
	int rc = 0;
//	deque<VolumeInfo> volInfoList;
	_SMI_TRACE(0,("Entered get_vol_size for volume %s", volume_name));
	
	/* Get the Information for all volumes on the system
	
	s->getVolumes(volInfoList);

	for(deque<VolumeInfo>::iterator i=volInfoList.begin(); i!=volInfoList.end(); ++i )
		{
			VolumeInfo volInfo = *i;	
			if(!strcmp(i->name.c_str(), volume_name))
			{
				return_info = i->sizeK;
			}
			_SMI_TRACE(0,("Exiting get_vol_size \n"));			
		}
*/
	
	StorageVolume *vol = VolumesFind(volume_name);
	if(vol == NULL)
	{
		_SMI_TRACE(1,("Volume cannot be found"));
		rc = M_FAILED;
		goto exit;
	}
	return_info = vol->size;
exit:
	_SMI_TRACE(0,("Exiting get_vol_size \n"));
	return rc;
}

/* Create the Snapshot
*/
int cs_take_snapshot(const char *source_volume, const char *snapshot_volume)
{
	int rc=0, s_rc = 0; 
	string snap_device;
	char *vg_name = NULL;
	unsigned long long volSize;
	StoragePool *antPool = NULL;

	_SMI_TRACE(0,("Entered cs_take_snapshot"));

	// Get the Volume Group Name for the Snapshot volume 
	
	vg_name = (char *) malloc (MAX_NAME_SIZE * sizeof(char));
	rc = get_VG_name(source_volume, vg_name);
	if (rc)
	{
		_SMI_TRACE(1,("Unable to get VG name, rc = %d", rc));
		goto exit;		
	}
	
	_SMI_TRACE(1,("VG name = %s", vg_name));

	// Get the size for the source volume
	
	rc = get_vol_size(source_volume, volSize);
	if(rc)
	{
		_SMI_TRACE(1,("Unable to get volume size, rc = %d", rc));
		goto exit;		
	}
	_SMI_TRACE(1,("volume size = %llu", volSize));
/*
	vg_name = (char *) malloc (MAX_NAME_SIZE * sizeof(char));
	StorageVolume *vol = VolumesFind(source_volume);
	if(vol == NULL)
	{
		_SMI_TRACE(1,("Volume cannot be found"));
		rc = M_FAILED;
		goto exit;
	}
	volSize = vol->size;

	antPool = (StoragePool *) vol->antecedentPool.element;
	strcpy(vg_name, antPool->name);
*/
	_SMI_TRACE(1,("Calling createLvmLvSnapshot"));

//	 Create the LVM Snapshot for the source volume
	
	if(rc = s->createLvmLvSnapshot((const char *)vg_name, source_volume,
						snapshot_volume, (volSize / 1024),
						snap_device))
//	if(rc = s->createLvmLvSnapshot("Pool1", "SV_Volume1", "SV_Snap1", (39845888 / 1024), snap_device ))
	{
		_SMI_TRACE(0,("Error creating LVM2 Snapshot: rc = %s", rc));
		rc = M_FAILED;
	}


exit:
	_SMI_TRACE(1,("cs_take_snapshot() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc = %d", s_rc));
	return rc;
		


}

#if 0
int cs_take_snapshot(const char *source_volume, const char *snapshot_volume)
{
	int rc=0; 
        handle_object_info_t *oi, *oi2;
	logical_volume_info_t vi;
        plugin_handle_t ph;
	task_handle_t th;
        declined_handle_array_t *dha=NULL;
	handle_array_t *ha=NULL, *ha2;
	task_effect_t te;
	value_t val;
        object_handle_t oh, oh2;
	char *region=NULL, *source_vol=NULL, *snap_vol=NULL, *snap_obj=NULL;
	
 	if(open_engine(engine_mode_t(ENGINE_READ|ENGINE_WRITE)))
		return -1;
	
	snap_vol = (char *)malloc(strlen("/dev/evms/")+strlen(snapshot_volume)+1);
	if(snap_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto CLOSE;
	}
	strcpy(snap_vol, "/dev/evms/");
	strcat(snap_vol, snapshot_volume);

	rc = evms_get_object_handle_for_name(VOLUME, snap_vol, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for volume\n", rc);
		goto FREE_SNAP_VOL;
	}
	printf("Found snapshot volume %s \n", snap_vol);
       
	rc = evms_get_info(oh, &oi);
	if(rc)
	{
		printf("evms error %d in getting object information\n", rc);
		goto FREE_SNAP_VOL;
	}

	/* vi is the logical volume, the physical entity containing this volume is a region*/
 	vi = oi->info.volume;
	rc = evms_get_info(vi.object, &oi2);
	if(rc)
	{
		printf("evms error %d in getting object info in volume\n", rc);
		goto FREE_OI;
	}
	if(oi2->type != REGION)
	{
		rc = -1;
		printf("Internal error. Expecting region\n");
		goto FREE_OI2;
	}
        
	region = (char *)malloc(strlen(oi2->info.object.name)+1);
	if(region == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto FREE_OI2;
	}
	printf("Region name is %s\n", oi2->info.object.name);
	strcpy(region, oi2->info.object.name);
	
	rc = evms_delete(oh);
	if(rc)
	{
		printf("evms error %d in deleting volume\n", rc);
		goto FREE_REGION;
	}
	
	rc = evms_get_object_handle_for_name(REGION, region, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for region\n", rc);
		goto FREE_REGION;
	}
		
       
	rc = evms_get_plugin_by_name("Snapshot", &ph);
	if(rc)
	{
		printf("evms error %d in getting plugin handle\n", rc);
		goto FREE_REGION;
	}
	
	rc = evms_create_task(ph, EVMS_Task_Create, &th);
	if(rc)
	{
		printf("evms error %d in creating task\n", rc);
		goto FREE_REGION;
	}
	ha = (handle_array_t *)malloc(sizeof(handle_array_t) + sizeof(object_handle_t));
	ha->count = 1;
	ha->handle[0] = oh;
 	rc = evms_set_selected_objects(th, ha, &dha, &te);
	if(rc)
	{
		printf("evms error %d in setting objects\n", rc);
		goto FREE_REGION;
	}
	printf("Region is set for the snapshot task\n");
	if(dha && dha->count)
	{
		rc = -1;
		printf("declined handles exist\n");
		goto FREE_REGION;
	}
	
	source_vol = (char *)malloc(strlen("/dev/evms/")+strlen(source_volume)+1);
	if(source_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto FREE_REGION;
	}
	strcpy(source_vol, "/dev/evms/");
	strcat(source_vol, source_volume);

  	val.s = source_vol;
	rc = evms_set_option_value_by_name(th, "original", &val, &te);
	if(rc)
	{
		printf("evms error %d in setting original\n", rc);
		goto FREE_SOURCE_VOL;
	}

	snap_obj = (char *)malloc(strlen("snap_obj_")+strlen(snapshot_volume)+1);
	if(snap_obj == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto FREE_SOURCE_VOL;
	}
	strcpy(snap_obj, "snap_obj_");
	strcat(snap_obj, snapshot_volume);

	val.s = snap_obj;
	rc = evms_set_option_value_by_name(th, "snapshot", &val, &te);
	if(rc)
	{
		printf("evms error %d in setting snapshot\n", rc);
		goto FREE_SNAP_OBJ;
	}
	
	val.i = 64;
	rc = evms_set_option_value_by_name(th, "chunksize", &val, &te);
	if(rc)
	{
		printf("evms error %d in setting chunksize\n", rc);
		goto FREE_SNAP_OBJ;
	}
	
	val.b = TRUE;
	rc = evms_set_option_value_by_name(th, "writeable", &val, &te);
	if(rc)
	{
		printf("evms error %d in setting writeable\n", rc);
		goto FREE_SNAP_OBJ;
	}
	
	ha2 = NULL;
	rc = evms_invoke_task(th, &ha2);
	if(rc)
	{
		printf("evms error %d in invoking task\n", rc);
		goto FREE_SNAP_OBJ;
	}
	printf("finished invoking snapshot task\n");
	if(ha2)
		evms_free(ha2);

	rc = evms_get_object_handle_for_name(EVMS_OBJECT, snap_obj, &oh2);
	if(rc)
	{
		printf("evms error %d in getting handle for obj\n", rc);
		goto FREE_SNAP_OBJ;
	}
	
	rc = evms_create_volume(oh2, (char*)snapshot_volume);
	if(rc)
	{
		printf("evms error %d in creating volume\n", rc);
		goto FREE_SNAP_OBJ;
	}
	printf("Applying snapshot to the volume\n");
	rc = evms_commit_changes();
	if(rc)
		printf("evms error %d in committing changes\n", rc);
	
	evms_destroy_task(th);
	

FREE_SNAP_OBJ:
	free(snap_obj);

FREE_SOURCE_VOL:
	free(source_vol);

FREE_REGION:	
        free(region);

FREE_OI2:
	evms_free(oi2);

FREE_OI:
	evms_free(oi);
	
FREE_SNAP_VOL:
	free(snap_vol);
	
CLOSE:
	close_engine();
	if(ha)
		free(ha);

	return rc;
}
#endif

/* Resets the Snapshot wrt Source Volume
*/
int cs_reset_snapshot(const char *snapshot_volume)
{
	int rc=M_NOT_SUPPORTED;
	_SMI_TRACE(0,("cs_reset_snapshot Not supported"));	
#if 0		
	int	 i;
	handle_array_t *ha;
        handle_object_info_t *oi;
	logical_volume_info_t vi;
	task_handle_t th;
        object_handle_t oh;
	function_info_array_t *actions;
	char *snap_vol = NULL;
	
 	if(open_engine(engine_mode_t(ENGINE_READ|ENGINE_WRITE)))
		return -1;
       
	snap_vol = (char *)malloc(strlen("/dev/evms/")+strlen(snapshot_volume)+1);
	if(snap_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto CLOSE;
	}
	strcpy(snap_vol, "/dev/evms/");
	strcat(snap_vol, snapshot_volume);
 	
	rc = evms_get_object_handle_for_name(VOLUME, snap_vol, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for volume\n", rc);
		goto FREE_SNAP_VOL;
	}
	
	rc = evms_get_info(oh, &oi);
	if(rc)
	{
		printf("evms error %d in getting object info\n", rc);
		goto FREE_SNAP_VOL;
	}
	
	vi = oi->info.volume;
	if(!vi.object)
	{
		rc = -1;
		printf("evms error %d in getting volume object info\n", rc);
		goto FREE_OI;
	}
	
	rc = evms_get_plugin_functions(vi.object, &actions);
	if(rc)
	{
		printf("evms error %d in getting plugin actions\n", rc);
		goto FREE_OI;
	}

        for(i=0; i<actions->count; i++)
	{
		if(!strcmp(actions->info[i].name, "reset"))
		{
			if(actions->info[i].flags & EVMS_FUNCTION_FLAGS_INACTIVE) 
			{
				rc = -1;
				printf("can not reset\n");
				goto FREE_ACTIONS;
			}
			rc = evms_create_task(vi.object, actions->info[i].function, &th);
			if(rc)
			{
				printf("evms error %d in getting creating task\n", rc);
				goto FREE_ACTIONS;
			}
			rc = evms_invoke_task(th, &ha);
			if(rc)
			{
				printf("evms error %d in invoking task\n", rc);
				goto FREE_ACTIONS;
			}
			rc = evms_commit_changes();
			if(rc)
			{
				printf("evms error %d in committing changes\n", rc);
				evms_destroy_task(th);
				goto FREE_ACTIONS;
			}
			evms_destroy_task(th);
			goto FREE_ACTIONS;
		}
	}
	rc = -1;
	printf("Internal error\n");

FREE_ACTIONS:	
	evms_free(actions);	

FREE_OI:
	evms_free(oi);
	
FREE_SNAP_VOL:
	free(snap_vol);
	
CLOSE:
	close_engine();
#endif
	return rc;
}



/* Rollbacks the Snapshot volume to Source volume
*/

int cs_rollback_snapshot(const char *snapshot_volume)
{
	int rc=M_NOT_SUPPORTED;
	_SMI_TRACE(0,("cs_rollback_snapshot Not supported"));		
#if 0	
	int i;
	handle_array_t *ha;
        handle_object_info_t *oi;
	logical_volume_info_t vi;
	task_handle_t th;
        object_handle_t oh;
	function_info_array_t *actions;
	char *snap_vol = NULL;
	
 	if(open_engine(engine_mode_t(ENGINE_READ|ENGINE_WRITE)))
		return -1;
       
	snap_vol = (char *)malloc(strlen("/dev/evms/")+strlen(snapshot_volume)+1);
	if(snap_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto CLOSE;
	}
	strcpy(snap_vol, "/dev/evms/");
	strcat(snap_vol, snapshot_volume);
 	
	rc = evms_get_object_handle_for_name(VOLUME, snap_vol, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for volume\n", rc);
		goto FREE_SNAP_VOL;
	}
	
	rc = evms_get_info(oh, &oi);
	if(rc)
	{
		printf("evms error %d in getting object info\n", rc);
		goto FREE_SNAP_VOL;
	}
	
	vi = oi->info.volume;
	if(!vi.object)
	{
		rc = -1;
		printf("evms error %d in getting volume object info\n", rc);
		goto FREE_OI;
	}
	
	rc = evms_get_plugin_functions(vi.object, &actions);
	if(rc)
	{
		printf("evms error %d in getting plugin actions\n", rc);
		goto FREE_OI;
	}

        for(i=0; i<actions->count; i++)
	{
		if(!strcmp(actions->info[i].name, "rollback"))      
		{
			if(actions->info[i].flags & EVMS_FUNCTION_FLAGS_INACTIVE) 
			{
				rc = -1;
				printf("can not rollback\n");
				goto FREE_ACTIONS;
			}
			rc = evms_create_task(vi.object, actions->info[i].function, &th);
			if(rc)
			{
				printf("evms error %d in getting creating task\n", rc);
				goto FREE_ACTIONS;
			}
			rc = evms_invoke_task(th, &ha);
			if(rc)
			{
				printf("evms error %d in invoking task\n", rc);
				goto FREE_ACTIONS;
			}
			rc = evms_commit_changes();
			if(rc)
			{
				printf("evms error %d in committing changes\n", rc);
				evms_destroy_task(th);
				goto FREE_ACTIONS;
			}
			evms_destroy_task(th);
			goto FREE_ACTIONS;
		}
	}
	rc = -1;
	printf("Internal error\n");

FREE_ACTIONS:	
	evms_free(actions);	

FREE_OI:
	evms_free(oi);
	
FREE_SNAP_VOL:
	free(snap_vol);
	
CLOSE:
	close_engine();
#endif	
	return rc;
}

/* Gets the Snapshot volumes associated with the source 
*/
int cs_return_associated_snapshot_volumes(const char *source_volume, char ***snapshot_volumes_list, int *no_of_volumes)
{
	int rc=0;

	deque<LvmLvInfo> lvInfoList;
	LvmLvSnapshotStateInfo snapStateInfo;
	char *vg_name;
	int snapCount = 0, j=0;
	char **snap_vols = NULL;
	
	_SMI_TRACE(0,("Entered cs_return_associated_snapshot_volumes \n"));

	/* Get Volume Group name for the source volume
	*/
	
	rc = get_VG_name(source_volume, vg_name);
	if (!vg_name)
	{
		_SMI_TRACE(1,("Unable to get VG name, rc = %d\n", rc));
		goto exit;		
	}

	
	/* Get the LV information from the input container
	*/
	if(rc = s->getLvmLvInfo(vg_name, lvInfoList))
	{
		rc = M_FAILED;
		_SMI_TRACE(1,("Unable to get Lvm Lv info, rc = %d\n", rc));
		goto exit;
	}


	for(deque<LvmLvInfo>::iterator i=lvInfoList.begin(); i!=lvInfoList.end(); ++i )
	{
		LvmLvInfo lvInfo = *i;
		
		/* Check if the origin is the source volume
		*/
		if(!strcmp(i->origin.c_str(), source_volume))
		{
			snapCount++;
			snap_vols = (char **)malloc(snapCount * sizeof(char *)); //Ram Validate this.
		
			if(snap_vols == NULL)
			{
				rc = M_UNKNOWN;
				_SMI_TRACE(1,("No more memory.\n"));
				goto FREE_SNAP_VOLS;
			}
		
			strcpy(snap_vols[j++] ,i->v.name.c_str());
		}

		
	}
		
	*no_of_volumes = snapCount;
	*snapshot_volumes_list = snap_vols;
			 

FREE_SNAP_VOLS: 
	for(j=0; j<snapCount; j++)
		if(snap_vols[j])
			free(snap_vols[j]);
	free(snap_vols);

exit:
	_SMI_TRACE(0,("Exiting cs_return_associated_snapshot_volumes \n"));

	return rc;
} 

#if 0
int cs_return_associated_snapshot_volumes(const char *source_volume, char ***snapshot_volumes_list, int *no_of_volumes)
{
	int rc=0, j, i, count=0;
        handle_object_info_t *oi;
	extended_info_array_t *info;
	logical_volume_info_t vi;
        object_handle_t oh;
	char *source_vol = NULL;
	char **snap_vols=NULL;
	
 	if(open_engine(engine_mode_t(ENGINE_READ)))
		return -1;
       
	source_vol = (char *)malloc(strlen("/dev/evms/")+strlen(source_volume)+1);
	if(source_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto CLOSE;
	}
	strcpy(source_vol, "/dev/evms/");
	strcat(source_vol, source_volume);
 	
	rc = evms_get_object_handle_for_name(VOLUME, source_vol, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for volume\n", rc);
		goto FREE_SOURCE_VOL;
	}
	
	rc = evms_get_info(oh, &oi);
	if(rc)
	{
		printf("evms error %d in getting object info\n", rc);
		goto FREE_SOURCE_VOL;
	}
	
	vi = oi->info.volume;

        rc = evms_get_extended_info(vi.object, NULL, &info);
	if(rc)
	{
		printf("evms error %d in getting extended info\n", rc);
		goto FREE_OI;
	}

	for (j=0; j < info->count; j++)
		if(!strcmp(info->info[j].name, "SnapShot") || !strcmp(info->info[j].name, "iSnapShot"))
			count++;
	
	if(!count)
		goto FREE_EI;
	
	snap_vols = (char **)calloc(count, sizeof(char*));
	if(!snap_vols)
	{
		rc = -1;
		printf("No more memory.\n");
		goto FREE_EI;
	}

	for (j=0, i=0; j < info->count; j++)
	{
		if(!strcmp(info->info[j].name, "SnapShot") || !strcmp(info->info[j].name, "iSnapShot"))
		{
			snap_vols[i] = (char *)malloc(strlen(info->info[j].value.s)-strlen("/dev/evms/")+1);
			if(!snap_vols[i])
			{
				rc = -1;
				printf("No more memory.\n");
				goto FREE_SNAP_VOLS;
			}
			strcpy(snap_vols[i], &(info->info[j].value.s[strlen("/dev/evms/")]));
			i++;
		}
	}
	
	*no_of_volumes = count;
	*snapshot_volumes_list = snap_vols;
        
	goto FREE_EI;


FREE_SNAP_VOLS:	
        for(i=0; i<count; i++)
		if(snap_vols[i])
			free(snap_vols[i]);
	free(snap_vols);

FREE_EI:
	evms_free(info);

FREE_OI:
	evms_free(oi);
	
FREE_SOURCE_VOL:
	free(source_vol);
 	
CLOSE:
	close_engine();

 	
 	return rc;	
}

#endif


int get_snap_info(const char *snapshot_volume, int code, void *return_info)
{
	int rc=0;
	deque<LvmLvInfo> lvInfoList;
	char *vg_name;
	char *source_vol = NULL;
	LvmLvSnapshotStateInfo snapStateInfo;
	double percentAlloc = 0;
	
	_SMI_TRACE(0,("get_snap_info called, snpshot volume %s", snapshot_volume));

	/* Get the Volume Group name corresponding to the snapshot volume
	*/
	vg_name = (char *)malloc (MAX_NAME_SIZE * sizeof(char));
	rc = get_VG_name(snapshot_volume, vg_name);
	if (rc || !strlen(vg_name))
	{
		_SMI_TRACE(1,("Unable to get VG name, rc = %d\n", rc));
		goto exit;		
	}

	if(code == SOURCE_VOLUME) //Get the associated source volume
	{
		
		/* Get the LV information from the input container
		*/
		if(rc = s->getLvmLvInfo(vg_name, lvInfoList))
		{
			rc = M_FAILED;
			_SMI_TRACE(1,("Unable to get Lvm Lv info, rc = %d\n", rc));
			goto exit;
		}
		/* Get the associated source volume
		*/
		for(deque<LvmLvInfo>::iterator i=lvInfoList.begin(); i!=lvInfoList.end(); ++i )
		{
			LvmLvInfo lvInfo = *i;
			source_vol = (char *)malloc(strlen(i->v.name.c_str()) + 1);
			if(!source_vol)
			{
				rc = M_UNKNOWN;
				_SMI_TRACE(1,("No more memory.\n"));
			}
			_SMI_TRACE(0,("Volume name:  %s", i->v.name.c_str()));
			if(!strcmp(i->v.name.c_str(), snapshot_volume))
			{
				_SMI_TRACE(0,("Volume is  %s", i->dm_target.c_str()));
				// Check if the volume is Original or Snapshot
				if(!strcmp(i->dm_target.c_str(), "snapshot-origin"))
				{
					strcpy(source_vol, i->v.name.c_str());
					*(char **)return_info = source_vol;
					goto exit;
				}
			 }
		}
		
	}
	else if (code == PERCENT_FULL) //get the percentage full information for the snapshot volume
	{
		if (rc = s->getLvmLvSnapshotStateInfo(vg_name, snapshot_volume, snapStateInfo))
		{
			rc = M_FAILED;
			_SMI_TRACE(1,("Unable to get Lvm Lv info, rc = %d\n", rc));
			goto exit;
		}
		_SMI_TRACE(0,("Percent allocated is %f\n", snapStateInfo.allocated));
		percentAlloc = snapStateInfo.allocated;
		_SMI_TRACE(0,("percentAlloc is %f\n", percentAlloc));
		*(int *)return_info = percentAlloc;
		goto exit;
	}

exit:
	free(source_vol);
	_SMI_TRACE(1,("get_snap_info() done, rc = %d\n", rc));
	return rc;
	
}

#if 0
int get_snap_info(const char *snapshot_volume, int code, void *return_info)
{
	int rc=0, j;
    handle_object_info_t *oi;
    object_handle_t oh;
	logical_volume_info_t vi;
	extended_info_array_t *info;
	char *source_vol = NULL;
	char *snap_vol=NULL;
	
 	if(open_engine(engine_mode_t(ENGINE_READ)))
		return -1;
       
	snap_vol = (char *)malloc(strlen("/dev/evms/")+strlen(snapshot_volume)+1);
	if(snap_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto CLOSE;
	}
	strcpy(snap_vol, "/dev/evms/");
	strcat(snap_vol, snapshot_volume);
 	
	rc = evms_get_object_handle_for_name(VOLUME, snap_vol, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for volume\n", rc);
		goto FREE_SNAP_VOL;
	}
	
	rc = evms_get_info(oh, &oi);
	if(rc)
	{
		printf("evms error %d in getting object info\n", rc);
		goto FREE_SNAP_VOL;
	}
	
	vi = oi->info.volume;

        rc = evms_get_extended_info(vi.object, NULL, &info);
	if(rc)
	{
		printf("evms error %d in getting extended info\n", rc);
		goto FREE_OI;
	}

	for (j=0; j < info->count; j++)
	{
		if(code == SOURCE_VOLUME && (!strcmp(info->info[j].name, "Original")
					||!strcmp(info->info[j].name, "iOriginal")))
		{
			source_vol = (char *)malloc(strlen(info->info[j].value.s)-strlen("/dev/evms/")+1);
			if(!source_vol)
			{
				rc = -1;
				printf("No more memory.\n");
				goto FREE_EI;
			}
			strcpy(source_vol, &(info->info[j].value.s[strlen("/dev/evms/")]));
			*(char **)return_info = source_vol;
			
			goto FREE_EI;
 		}
	 	if(code == PERCENT_FULL && !strcmp(info->info[j].name, "PercentFull"))
		{
			*(int *)return_info = info->info[j].value.ui;
			goto FREE_EI;
		}
	}
	
	goto FREE_EI;

FREE_EI:
	evms_free(info);

FREE_OI:
	evms_free(oi);
	
FREE_SNAP_VOL:
	free(snap_vol);
 	
CLOSE:
	close_engine();

 	
 	return rc;	
}
#endif
 
int cs_return_associated_source_volume(const char *snapshot_volume, char **source_volume)
{

 	return get_snap_info(snapshot_volume, SOURCE_VOLUME, source_volume);	
}



int cs_get_percent_full(const char *snapshot_volume, int *percent_full)
{

 	return get_snap_info(snapshot_volume, PERCENT_FULL, percent_full);	
}

/* Deletes the snapshot
*/
int cs_delete_snapshot(const char *snapshot_volume)
{
	int rc = 0, s_rc = 0;
	char *vg_name = NULL;

	/* Get the Volume Group name corresponding to the snapshot volume
	*/
	rc = get_VG_name(snapshot_volume, vg_name);
	if (rc)
	{
		_SMI_TRACE(1,("Unable to get VG name, rc = %d\n", rc));
		goto exit;		
	}


	/* Delete the Snapshot Volume
	*/
	if (rc = s->removeLvmLvSnapshot(vg_name, snapshot_volume))
	{
		_SMI_TRACE(0,("Error deleting LVM2 Snapshot: rc = %d", rc));
		rc = M_FAILED;	
	}

exit:	
	_SMI_TRACE(1,("cs_delete_snapshot() done, rc = %d\n", rc));
	if(s_rc = s->commit())
		_SMI_TRACE(1,("Storage Interface commit failed, rc= %d", s_rc));
	return rc;
}

#if 0
int cs_delete_snapshot(const char *snapshot_volume)
{
	int rc=0; 
        handle_object_info_t *oi, *oi2, *oi3;
	logical_volume_info_t vi;
        object_handle_t oh;
	char *region=NULL, *snap_vol=NULL, *snap_obj=NULL;
	
 	if(open_engine(engine_mode_t(ENGINE_READ|ENGINE_WRITE)))
		return -1;
	
	snap_vol = (char *)malloc(strlen("/dev/evms/")+strlen(snapshot_volume)+1);
	if(snap_vol == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto CLOSE;
	}
	strcpy(snap_vol, "/dev/evms/");
	strcat(snap_vol, snapshot_volume);

	rc = evms_get_object_handle_for_name(VOLUME, snap_vol, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for volume\n", rc);
		goto FREE_SNAP_VOL;
	}
        
	rc = evms_get_info(oh, &oi);
	if(rc)
	{
		printf("evms error %d in getting object information\n", rc);
		goto FREE_SNAP_VOL;
	}
	
 	vi = oi->info.volume;
	rc = evms_get_info(vi.object, &oi2);
	if(rc)
	{
		printf("evms error %d in getting object info in volume\n", rc);
		goto FREE_OI;
	}
	if(oi2->type != EVMS_OBJECT)
	{
		rc = -1;
		printf("Internal error. Expecting object\n");
		goto FREE_OI2;
	}
        
	snap_obj  = (char *)malloc(strlen(oi2->info.object.name)+1);
	if(snap_obj == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto FREE_SNAP_OBJ;
	}
	strcpy(snap_obj, oi2->info.object.name);

	if(!oi2->info.object.child_objects || !oi2->info.object.child_objects->count)
	{
		rc = -1;
		printf("Internal error. Expecting child containers for snap object\n");
		goto FREE_SNAP_OBJ;
	}

	rc = evms_get_info(oi2->info.object.child_objects->handle[0], &oi3);
	if(rc)
	{
		printf("evms error %d in getting object info for child objects\n", rc);
		goto FREE_SNAP_OBJ;
	}

	if(oi3->type != REGION)
	{
		rc = -1;
		printf("Internal error. Expecting region for children\n");
		goto FREE_OI3;
	}

	region  = (char *)malloc(strlen(oi3->info.region.name)+1);
	if(region == NULL)
	{
		rc = -1;
		printf("No more memory.\n");
		goto FREE_OI3;
	}
	strcpy(region, oi3->info.region.name);
 	
		
	rc = evms_delete(oh);
	if(rc)
	{
		printf("evms error %d in deleting volume %s\n", rc, snap_vol);
		goto FREE_REGION;
	}
	
	rc = evms_get_object_handle_for_name(EVMS_OBJECT, snap_obj, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for snap object\n", rc);
		goto FREE_REGION;
	}
       
	rc = evms_delete(oh);
	if(rc)
	{
		printf("evms error %d in deleting snap_object %s\n", rc, snap_obj);
		goto FREE_REGION;
	}
	
 	rc = evms_get_object_handle_for_name(REGION, region, &oh);
	if(rc)
	{
		printf("evms error %d in getting handle for region\n", rc);
		goto FREE_REGION;
	}
       
	rc = evms_delete(oh);
	if(rc)
	{
		printf("evms error %d in deleting region %s\n", rc, region);
		goto FREE_REGION;
	}
	
 	rc = evms_commit_changes();
	if(rc)
		printf("evms error %d in committing changes\n", rc);
	

FREE_REGION:	
        free(region);

FREE_OI3:	
        evms_free(oi3);
 
FREE_SNAP_OBJ:	
        free(snap_obj);
 
FREE_OI2:
	evms_free(oi2);

FREE_OI:
	evms_free(oi);
	
FREE_SNAP_VOL:
	free(snap_vol);
	
CLOSE:
	close_engine();

	return rc;
}
#endif
