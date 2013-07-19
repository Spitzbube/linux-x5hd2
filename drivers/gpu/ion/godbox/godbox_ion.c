/*
 * drivers/gpu/ion/godbox/godbox_ion.c
 *
 * Copyright (C) Linaro 2012
 * Author: <benjamin.gaignard at linaro.org> for ST-Ericsson.
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

#include <linux/err.h>
#include <linux/ion.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "../ion_priv.h"


struct ion_device *godbox_ion_device;
int num_heaps;
struct ion_heap **godbox_ion_heaps;

int godbox_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i, previous_heaps_count = 0;

	/* test if it is the first time we try to create ions heaps */
	if (num_heaps == 0) {
		num_heaps = pdata->nr;

		godbox_ion_heaps =
		    kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);
		memset(godbox_ion_heaps, 0,
		       sizeof(struct ion_heap *) * pdata->nr);

		godbox_ion_device = ion_device_create(NULL);
		if (IS_ERR_OR_NULL(godbox_ion_device)) {
			kfree(godbox_ion_heaps);
			num_heaps = 0;
			return PTR_ERR(godbox_ion_device);
		}
	} else {
		struct ion_heap **new_godbox_ion_heaps;

		previous_heaps_count = num_heaps;
		num_heaps += pdata->nr;

		/* allocate a bigger array of ion_heap */
		new_godbox_ion_heaps =
		    kzalloc(sizeof(struct ion_heap *) * num_heaps, GFP_KERNEL);
		memset(new_godbox_ion_heaps, 0,
		       sizeof(struct ion_heap *) * num_heaps);

		/* copy old heap array info into the new one */
		for (i = 0; i < previous_heaps_count; i++)
			new_godbox_ion_heaps[i] = godbox_ion_heaps[i];

		/* free old heap array and swap it with the new one */
		kfree(godbox_ion_heaps);
		godbox_ion_heaps = new_godbox_ion_heaps;
	}

	/* create the heaps as specified in the board file */
	for (i = previous_heaps_count; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data =
		    &pdata->heaps[i - previous_heaps_count];

		/* heap_data->priv = &pdev->dev; */

		godbox_ion_heaps[i] = ion_heap_create(heap_data);

		if (IS_ERR_OR_NULL(godbox_ion_heaps[i])) {
			err = PTR_ERR(godbox_ion_heaps[i]);
			godbox_ion_heaps[i] = NULL;
			goto err;
		}
		ion_device_add_heap(godbox_ion_device, godbox_ion_heaps[i]);
	}

	platform_set_drvdata(pdev, godbox_ion_device);

	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (godbox_ion_heaps[i])
			ion_heap_destroy(godbox_ion_heaps[i]);
	}
	kfree(godbox_ion_heaps);
	return err;
}

int godbox_ion_remove(struct platform_device *pdev)
{
	struct ion_device *idev = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(idev);
	for (i = 0; i < num_heaps; i++)
		ion_heap_destroy(godbox_ion_heaps[i]);
	kfree(godbox_ion_heaps);
	return 0;
}

static struct platform_driver godbox_ion_driver = {
	.probe = godbox_ion_probe,
	.remove = godbox_ion_remove,
	.driver = {
		   .name = "hisi-ion",
	}
};

static int __init godbox_ion_init(void)
{
	godbox_ion_device = NULL;
	num_heaps = 0;
	godbox_ion_heaps = NULL;

	return platform_driver_register(&godbox_ion_driver);
}

static void __exit godbox_ion_exit(void)
{
	if (godbox_ion_device)
		ion_device_destroy(godbox_ion_device);

	platform_driver_unregister(&godbox_ion_driver);
}

module_init(godbox_ion_init);
module_exit(godbox_ion_exit);
