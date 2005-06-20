/*
 *  i2c-sysfs.h - i2c chip driver sysfs defines
 *
 *  Copyright (C) 2005 Yani Ioannou <yani.ioannou@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _LINUX_I2C_SYSFS_H
#define _LINUX_I2C_SYSFS_H

struct sensor_device_attribute{
	struct device_attribute dev_attr;
	int index;
};
#define to_sensor_dev_attr(_dev_attr) \
	container_of(_dev_attr, struct sensor_device_attribute, dev_attr)

#define SENSOR_DEVICE_ATTR(_name,_mode,_show,_store,_index)	\
struct sensor_device_attribute sensor_dev_attr_##_name = {	\
	.dev_attr =	__ATTR(_name,_mode,_show,_store),	\
	.index =	_index,					\
}

#endif /* _LINUX_I2C_SYSFS_H */
