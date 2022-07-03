// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2019-2022 Stephan Gerhold <stephan@gerhold.net> */

#include <debug.h>
#include <fastboot.h>
#include <lk2nd/device.h>
#include <string.h>

#include <libfdt.h>

#include "device.h"

struct lk2nd_device lk2nd_dev;

static int find_device_node(const void *dtb)
{
	int lk2nd_node, node, ret;

	lk2nd_node = fdt_path_offset(dtb, "/lk2nd");
	if (lk2nd_node < 0)
		return lk2nd_node;

	if (lk2nd_dev.compatible) {
		/* Check if root node is already compatible */
		ret = fdt_node_check_compatible(dtb, lk2nd_node, lk2nd_dev.compatible);
		if (ret == 0)
			return lk2nd_node;
		if (ret == 1) /* root node has "compatible" but is incompatible */
			return -FDT_ERR_NOTFOUND;

		/* Search in subnodes instead */
		fdt_for_each_subnode(node, dtb, lk2nd_node)
			if (fdt_node_check_compatible(dtb, node, lk2nd_dev.compatible) == 0)
				return node;
		return node;
	}

	/* If root node already has a "compatible" it always matches */
	if (fdt_getprop(dtb, lk2nd_node, "compatible", NULL))
		return lk2nd_node;

	/* Fallback to dynamic matching */
	return lk2nd_device2nd_match_device_node(dtb, lk2nd_node);
}

static void parse_dtb(const void *dtb)
{
	int node, len;
	const char *val;

	node = find_device_node(dtb);
	if (node < 0) {
		dprintf(CRITICAL, "Failed to find matching lk2nd device node: %d\n", node);
		return;
	}

	val = fdt_getprop(dtb, node, "compatible", &len);
	if (val && len > 0)
		lk2nd_dev.compatible = strndup(val, len);
	else
		dprintf(CRITICAL, "Failed to read 'compatible': %d\n", len);

	val = fdt_getprop(dtb, node, "model", &len);
	if (val && len > 0)
		lk2nd_dev.model = strndup(val, len);
	else
		dprintf(CRITICAL, "Failed to read 'model': %d\n", len);

	dprintf(INFO, "Detected device: %s (compatible: %s)\n",
		lk2nd_dev.model, lk2nd_dev.compatible);
}

void lk2nd_device_init(void)
{
	const void *dtb;

	dprintf(INFO, "lk2nd_device_init()\n");

	dtb = lk2nd_device2nd_init();
	if (!dtb) {
		dprintf(CRITICAL, "No device DTB provided\n");
		return;
	}

	parse_dtb(dtb);
}

static void lk2nd_device_register(void)
{
	if (lk2nd_dev.compatible)
		fastboot_publish("lk2nd:compatible", lk2nd_dev.compatible);
	if (lk2nd_dev.model)
		fastboot_publish("lk2nd:model", lk2nd_dev.model);
	if (lk2nd_dev.panel.name)
		fastboot_publish("lk2nd:panel", lk2nd_dev.panel.name);
}
FASTBOOT_INIT(lk2nd_device_register);
