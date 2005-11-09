/*
 * RapidIO Tsi500 switch support
 *
 * Copyright 2005 MontaVista Software, Inc.
 * Matt Porter <mporter@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/rio.h>
#include <linux/rio_drv.h>
#include <linux/rio_ids.h>
#include "../rio.h"

static int
tsi500_route_add_entry(struct rio_mport *mport, u16 destid, u8 hopcount, u16 table, u16 route_destid, u8 route_port)
{
	int i;
	u32 offset = 0x10000 + 0xa00 + ((route_destid / 2)&~0x3);
	u32 result;

	if (table == 0xff) {
		rio_mport_read_config_32(mport, destid, hopcount, offset, &result);
		result &= ~(0xf << (4*(route_destid & 0x7)));
		for (i=0;i<4;i++)
			rio_mport_write_config_32(mport, destid, hopcount, offset + (0x20000*i), result | (route_port << (4*(route_destid & 0x7))));
	}
	else {
		rio_mport_read_config_32(mport, destid, hopcount, offset + (0x20000*table), &result);
		result &= ~(0xf << (4*(route_destid & 0x7)));
		rio_mport_write_config_32(mport, destid, hopcount, offset + (0x20000*table), result | (route_port << (4*(route_destid & 0x7))));
	}

	return 0;
}

static int
tsi500_route_get_entry(struct rio_mport *mport, u16 destid, u8 hopcount, u16 table, u16 route_destid, u8 *route_port)
{
	int ret = 0;
	u32 offset = 0x10000 + 0xa00 + ((route_destid / 2)&~0x3);
	u32 result;

	if (table == 0xff)
		rio_mport_read_config_32(mport, destid, hopcount, offset, &result);
	else
		rio_mport_read_config_32(mport, destid, hopcount, offset + (0x20000*table), &result);

	result &= 0xf << (4*(route_destid & 0x7));
	*route_port = result >> (4*(route_destid & 0x7));
	if (*route_port > 3)
		ret = -1;

	return ret;
}

DECLARE_RIO_ROUTE_OPS(RIO_VID_TUNDRA, RIO_DID_TSI500, tsi500_route_add_entry, tsi500_route_get_entry);
