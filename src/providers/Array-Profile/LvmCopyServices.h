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

#ifndef LVMCOPYSERVICES_H_
#define LVMCOPYSERVICES_H_

#ifdef __cplusplus
extern "C" {
#endif	
	

int cs_return_associated_snapshot_volumes(const char *source_volume, char ***snapshot_volumes_list, int *no_of_volumes);

int cs_return_associated_source_volume(const char *snapshot_volume, char **source_volume);

int cs_take_snapshot(const char *source_volume, const char *snapshot_volume);

int cs_reset_snapshot(const char *snapshot_volume);

int cs_rollback_snapshot(const char *snapshot_volume);

int cs_get_percent_full(const char *snapshot_volume, int *percent_full);

int cs_delete_snapshot(const char *snapshot_volume);


#ifdef __cplusplus
}
#endif

#endif /*LVMCOPYSERVICES_H_*/
