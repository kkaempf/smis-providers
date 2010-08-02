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
 |	 OMC SMI-S Volume Management provider include file
 |
 |---------------------------------------------------------------------------
 |
 | $Id:
 |
 +-------------------------------------------------------------------------*/
#ifndef STORAGESETTING_H_
#define STORAGESETTING_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SETTING_ARRAY_SIZE_DEFAULT	64

typedef struct _StorageSetting
{
	char			*name;
	char			*instanceID;

	CMPIBoolean		noSinglePointOfFailure;
	CMPIUint16		packageRedundancyMin;
	CMPIUint16		packageRedundancyMax;
	CMPIUint16		packageRedundancyGoal;
	CMPIUint16		dataRedundancyMin;
	CMPIUint16		dataRedundancyMax;
	CMPIUint16		dataRedundancyGoal;
	CMPIUint16		extentStripeLength;
	CMPIUint16		extentStripeLengthMin;
	CMPIUint16		extentStripeLengthMax;
	CMPIUint16		parityLayout;
	CMPIUint64		userDataStripeDepth;
	CMPIUint64		userDataStripeDepthMin;
	CMPIUint64		userDataStripeDepthMax;

	struct _StorageCapability	*capability;
	struct _StorageVolume		*volume;
} StorageSetting;

//
// Exports
//

// Setting management
extern StorageSetting*	SettingAlloc(const char *settingName);
extern void				SettingFree(StorageSetting *setting);
extern void				SettingInitFromCapability(StorageSetting *setting, const CMPIUint16 settingType);

extern CMPIObjectPath*	SettingCreateObjectPath(
								StorageSetting *setting, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	SettingCreateInstance(
								StorageSetting *setting, 
								const char *ns, 
								CMPIStatus *status);

extern CMPIInstance*	SettingCreateGFCAssocInstance(
								StorageSetting *setting,
								const char *ns, 
								const char ** properties,
								CMPIStatus *status);

extern CMPIObjectPath*	SettingCreateGFCAssocObjectPath(
								StorageSetting *setting,
  								const char *ns, 
								CMPIStatus *status);

extern CMPIBoolean		SettingIsJBOD(StorageSetting *setting);
extern CMPIBoolean		SettingIsRAID0(StorageSetting *setting);
extern CMPIBoolean		SettingIsRAID1(StorageSetting *setting);
extern CMPIBoolean		SettingIsRAID10(StorageSetting *setting);
extern CMPIBoolean		SettingIsRAID5(StorageSetting *setting);


// Settings array manipulation
extern CMPICount		SettingsSize();
extern StorageSetting*	SettingsGet(const CMPICount index);
extern StorageSetting*	SettingsFind(const char *instanceID);
extern void				SettingsAdd(StorageSetting *setting);
extern void				SettingsRemove(StorageSetting *setting);
extern void				SettingsFree();


#ifdef __cplusplus
}
#endif

#endif /*STORAGESETTING_H_*/
