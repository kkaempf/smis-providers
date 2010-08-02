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
#ifndef REGISTEREDPROFILE_H_
#define REGISTEREDPROFILE_H_

	
// Exports

extern CMPIInstance*	RegisteredProfileCreateInstance(
							const char *ns,
							const char *name,
							CMPIStatus *status);

extern CMPIObjectPath*	RegisteredProfileCreateObjectPath(
							const char *ns, 
							const char *name,
							CMPIStatus *status);

extern CMPIInstance*	RegisteredProfileCreateElementConformsAssocInstance(
							const CMPIContext * ctx,
							const char *ns,
							const char *name,
							const char ** properties,
							CMPIStatus *status);

extern CMPIObjectPath*	RegisteredProfileCreateElementConformsAssocObjectPath(
							const CMPIContext * ctx,
  							const char *ns, 
							const char *name,
							CMPIStatus *status);

extern CMPIInstance*	RegisteredProfileCreateSubProfileRequiresAssocInstance(
							const char *ns,
							const char *profName,
							const char *subProfName,
							const char ** properties,
							CMPIStatus *status);

extern CMPIObjectPath*	RegisteredProfileCreateSubProfileRequiresAssocObjectPath(
  							const char *ns, 
							const char *profName,
							const char *subProfName,
							CMPIStatus *status);
							
#endif /*REGISTEREDPROFILE_H_*/
