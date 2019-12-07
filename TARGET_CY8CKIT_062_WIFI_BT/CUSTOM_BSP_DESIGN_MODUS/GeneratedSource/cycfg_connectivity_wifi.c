/*******************************************************************************
* File Name: cycfg_connectivity_wifi.c
*
* Description:
* Connectivity Wi-Fi configuration
* This file was automatically generated and should not be modified.
* Device Configurator: 2.0.0.1483
* Device Support Library (../../../psoc6pdl): 1.3.1.1499
*
********************************************************************************
* Copyright 2017-2019 Cypress Semiconductor Corporation
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
********************************************************************************/

#include "cycfg_connectivity_wifi.h"

#define CYCFG_ARP_OL_ENABLED (1u)

static arp_ol_t arp_ol_0_ctxt;
static const arp_ol_cfg_t arp_ol_cfg_0 = 
{
	.awake_enable_mask = CY_ARP_OL_FEATURE_AWAKE_ENABLE_MASK_0,
	.sleep_enable_mask = CY_ARP_OL_FEATURE_SLEEP_ENABLE_MASK_0,
	.peerage = CY_ARP_OL_PEER_AGE_0,
};
static const cy_pf_ol_cfg_t cy_pf_ol_cfg_0[] = 
{
	[0u] = {.feature = CY_PF_OL_FEAT_LAST},
};
static const ol_desc_t ol_list_0[] = 
{
	[0u] = {"ARP", &arp_ol_cfg_0, &arp_ol_fns, &arp_ol_0_ctxt},
	[1u] = {NULL, NULL, NULL, NULL},
};

const ol_desc_t *cycfg_get_default_ol_list(void)
{
	return &ol_list_0[0];
}

