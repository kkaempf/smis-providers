
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
#ifndef STORAGEREPLICATIONELEMENTCAPABILITIES_H_
#define STORAGEREPLICATIONELEMENTCAPABILITIES_H_ 

//Properties
#define ManagedElementProperty 				"ManagedElement"
#define CapabilitiesProperty 				"Capabilities"

void  OMC_CreateSRECObjectPaths(
		int syncTypes,
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		CMPIStatus *rc);

void OMC_CreateSRECInstances(
		int  syncTypes, 
		const CMPIBroker *broker,
		const CMPIContext* ctx,
		const CMPIResult* rslt,
		const CMPIObjectPath *op,
		const char** properties,
		CMPIStatus *rc); 


#endif  /*STORAGEREPLICATIONELEMENTCAPABILITIES_H_*/


