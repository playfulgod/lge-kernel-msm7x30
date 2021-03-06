/* arch/arm/mach-msm/lge_ats.c
 *
 * Copyright (C) 2008 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/msm_rpcrouter.h>
#include <linux/lge_alohag_at.h>
#include <mach/board_lge.h>
#include "lge_ats.h"

/* Ats server definitions. */

#define ATS_APPS_APISPROG		0x30000006
#define ATS_APPS_APISVERS		0

#define ONCRPC_LGE_ATCMD_ATS_PROC 3
#define ONCRPC_LGE_ATCMD_ATS_ETA_PROC 6
#define ONCRPC_LGE_GET_FLEX_MCC_PROC 7 //LGE_UPDATE_S [irene.park@lge.com] 2010.06.04 - Get Flex MCC/MNC value
#define ONCRPC_LGE_GET_FLEX_MNC_PROC 8 //LGE_UPDATE_S [irene.park@lge.com] 2010.06.04 - Get Flex MCC/MNC value
#define ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC 9 ///LGE_UPDATE_S [irene.park@lge.com] 2010.06.04 - Get Flex Operator ?value
#define ONCRPC_LGE_GET_FLEX_COUNTRY_CODE_PROC 10 //LGE_UPDATE_S [irene.park@lge.com] 2010.06.04 - Get Flex Operator ?value

struct ats_data {
	struct atcmd_dev *atdev;
	int (*handle_atcmd) (struct msm_rpc_server *server,
						 struct rpc_request_hdr *req, unsigned len,
						 void (*update_atcmd_state)(char *cmd, int state) );
	int (*handle_atcmd_eta) (struct msm_rpc_server *server,
							 struct rpc_request_hdr *req, unsigned len);
	void (*update_atcmd_state) (char *cmd, int state);
//LGE_CHAGE[irene.park@lge.com] 2010-06- 04 - to get flex value from ARM9 
	int (*handle_flex) (struct msm_rpc_server *server,
							 struct rpc_request_hdr *req, unsigned len);

};

struct ats_data lge_ats_data;

static void lge_ats_update_atcmd_state(char *cmd, int state)
{
#if defined (CONFIG_LGE_SUPPORT_AT_CMD)
	struct ats_data *data = &lge_ats_data;

	if(!data->atdev)
		data->atdev = atcmd_get_dev();
	if(data->atdev)
		update_atcmd_state(data->atdev, cmd, state);
#endif
}

static int handle_ats_rpc_call(struct msm_rpc_server *server,
							   struct rpc_request_hdr *req, unsigned len)
{
	struct ats_data *data = &lge_ats_data;

	switch (req->procedure)
	{
		case ONCRPC_LGE_ATCMD_ATS_ETA_PROC:
			printk(KERN_INFO"%s: ONCRPC_LGE_ATCMD_ATS_ETA_PROC\n", __func__);
			if(data->handle_atcmd_eta)
				return data->handle_atcmd_eta(server, req, len);
			break;
		case ONCRPC_LGE_ATCMD_ATS_PROC:
			printk(KERN_INFO"%s: ONCRPC_LGE_ATCMD_ATS_PROC\n", __func__);
			if(data->handle_atcmd)
				return data->handle_atcmd(server, req, len, data->update_atcmd_state);
			break;
	    //LGE_CHAGE[irene.park@lge.com] 2010-06- 04 - to get flex value from ARM9 			
		case ONCRPC_LGE_GET_FLEX_MCC_PROC:
		case ONCRPC_LGE_GET_FLEX_MNC_PROC:
		case ONCRPC_LGE_GET_FLEX_OPERATOR_CODE_PROC:
		case ONCRPC_LGE_GET_FLEX_COUNTRY_CODE_PROC:
			printk(KERN_INFO"%s:ONCRPC_LGE_GET_FLEX_ %d\n", __func__,req->procedure);
			if(data->handle_flex)
				return data->handle_flex(server, req, len);
			break;
		default:
			return -ENODEV;
	}

	return 0;
}

#ifdef CONFIG_LGE_SUPPORT_AT_CMD
static struct atcmd_platform_data ats_atcmd_pdata = {
	.name = "lge_atcmd",
};

static struct platform_device ats_atcmd_device = {
	.name = "lge_atcmd",
	.id = -1,
	.dev    = {
		.platform_data = &ats_atcmd_pdata
	},
}; 
#endif

#ifdef CONFIG_LGE_ATS_INPUT_DEVICE
static struct platform_device ats_input_device = {
	.name = "ats_input",
};
#endif

static struct msm_rpc_server ats_rpc_server = {
	.prog = ATS_APPS_APISPROG,
	.vers = ATS_APPS_APISVERS,
	.rpc_call = handle_ats_rpc_call,
};

#ifdef CONFIG_LGE_DOMESTIC
extern int event_log_start(void);
extern int event_log_end(void);

extern unsigned int ats_mtc_log_mask;

ssize_t log_show_onoff(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_ERR "%s\n", __func__);
	return 0;
}

ssize_t log_store_onoff(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int onoff;
	sscanf(buf, "%d", &onoff);

	ats_mtc_log_mask = onoff;

	printk(KERN_ERR "%s : (%u)\n", __func__, ats_mtc_log_mask);
	if (onoff) {
		event_log_start();
	} else {
		event_log_end();
	}

	return 0;
}

DEVICE_ATTR(log_onoff, 0666, log_show_onoff, log_store_onoff);

static int logstart_probe(struct platform_device *pdev)
{
	if( pdev->id == 0 )
		return 0;

	printk(KERN_ERR "%s\n", __func__);
	return 0;
}

static struct platform_driver this_driver = {
	.probe	= logstart_probe,
	.driver = {
		.name   = "key_logging",
	},
};
#endif

static int __init lge_ats_init(void)
{
	int err;

	if((err = msm_rpc_create_server(&ats_rpc_server)) != 0) {
		printk(KERN_ERR"%s: Error during creating rpc server for ats atcmd\n", __func__);
		return err;
	}

	lge_ats_data.handle_atcmd = lge_ats_handle_atcmd;
	lge_ats_data.handle_atcmd_eta = lge_ats_handle_atcmd_eta;
	lge_ats_data.update_atcmd_state = lge_ats_update_atcmd_state;
	lge_ats_data.handle_flex = lge_ats_handle_flex; //LGE_CHAGE[irene.park@lge.com] 2010-06- 04 - to get flex value from ARM9 

#ifdef CONFIG_LGE_DOMESTIC
	err = platform_driver_register(&this_driver);
	if (err)
		return err;

#ifdef CONFIG_LGE_SUPPORT_AT_CMD
	err = platform_device_register(&ats_atcmd_device);
	if (err) {
		printk(KERN_ERR "%s not able to register the device\n",
			 __func__);
		goto fail_driver;
	}
#endif

#ifdef CONFIG_LGE_ATS_INPUT_DEVICE
	platform_device_register(&ats_input_device);
#endif

	err = device_create_file(&ats_atcmd_device.dev, &dev_attr_log_onoff);
	return err;

fail_driver:
	platform_driver_unregister(&this_driver);
#else
#ifdef CONFIG_LGE_SUPPORT_AT_CMD
	platform_device_register(&ats_atcmd_device);
#endif

#ifdef CONFIG_LGE_ATS_INPUT_DEVICE
	platform_device_register(&ats_input_device);
#endif

#endif
	return err;
}

module_init(lge_ats_init);

