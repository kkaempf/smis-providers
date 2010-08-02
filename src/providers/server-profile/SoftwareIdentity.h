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
 |	 OMC SMI-S Server profile provider include file
 |
 |---------------------------------------------------------------------------
 |
 | $Id:
 |
 +-------------------------------------------------------------------------*/
#ifndef SOFTWAREIDENTITY_H_
#define SOFTWAREIDENTITY_H_

	
// Exports

extern CMPIInstance*	SoftwareIdentityCreateInstance(
							const char *ns,
							const char *name,
							CMPIStatus *status);

extern CMPIObjectPath*	SoftwareIdentityCreateObjectPath(
							const char *ns, 
							const char *name,
							CMPIStatus *status);

extern CMPIInstance*	SoftwareIdentityCreateElementAssocInstance(
							const char *ns,
							const char *siname,
							const char *name,
							const char ** properties,
							CMPIStatus *status);

extern CMPIObjectPath*	SoftwareIdentityCreateElementAssocObjectPath(
  							const char *ns,
							const char *siname,
							const char *name,
							CMPIStatus *status);

							
#endif /*SOFTWARE_IDENTITY_H_*/
