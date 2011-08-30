/*
 * AD7816 digital temperature sensor driver supporting AD7816/7/8
 *
 * Copyright 2010 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/list.h>
#include <linux/spi/spi.h>
#include <linux/module.h>

#include "../iio.h"
#include "../sysfs.h"

/*
 * AD7816 config masks
 */
#define AD7816_FULL			0x1
#define AD7816_PD			0x2
#define AD7816_CS_MASK			0x7
#define AD7816_CS_MAX			0x4

/*
 * AD7816 temperature masks
 */
#define AD7816_VALUE_OFFSET		6
#define AD7816_BOUND_VALUE_BASE		0x8
#define AD7816_BOUND_VALUE_MIN		-95
#define AD7816_BOUND_VALUE_MAX		152
#define AD7816_TEMP_FLOAT_OFFSET	2
#define AD7816_TEMP_FLOAT_MASK		0x3


/*
 * struct ad7816_chip_info - chip specifc information
 */

struct ad7816_chip_info {
	struct spi_device *spi_dev;
	u16 rdwr_pin;
	u16 convert_pin;
	u16 busy_pin;
	u8  oti_data[AD7816_CS_MAX+1];
	u8  channel_id;	/* 0 always be temperature */
	u8  mode;
};

/*
 * ad7816 data access by SPI
 */
static int ad7816_spi_read(struct ad7816_chip_info *chip, u16 *data)
{
	struct spi_device *spi_dev = chip->spi_dev;
	int ret = 0;

	gpio_set_value(chip->rdwr_pin, 1);
	gpio_set_value(chip->rdwr_pin, 0);
	ret = spi_write(spi_dev, &chip->channel_id, sizeof(chip->channel_id));
	if (ret < 0) {
		dev_err(&spi_dev->dev, "SPI channel setting error\n");
		return ret;
	}
	gpio_set_value(chip->rdwr_pin, 1);


	if (chip->mode == AD7816_PD) { /* operating mode 2 */
		gpio_set_value(chip->convert_pin, 1);
		gpio_set_value(chip->convert_pin, 0);
	} else { /* operating mode 1 */
		gpio_set_value(chip->convert_pin, 0);
		gpio_set_value(chip->convert_pin, 1);
	}

	while (gpio_get_value(chip->busy_pin))
		cpu_relax();

	gpio_set_value(chip->rdwr_pin, 0);
	gpio_set_value(chip->rdwr_pin, 1);
	ret = spi_read(spi_dev, (u8 *)data, sizeof(*data));
	if (ret < 0) {
		dev_err(&spi_dev->dev, "SPI data read error\n");
		return ret;
	}

	*data = be16_to_cpu(*data);

	return ret;
}

static int ad7816_spi_write(struct ad7816_chip_info *chip, u8 data)
{
	struct spi_device *spi_dev = chip->spi_dev;
	int ret = 0;

	gpio_set_value(chip->rdwr_pin, 1);
	gpio_set_value(chip->rdwr_pin, 0);
	ret = spi_write(spi_dev, &data, sizeof(data));
	if (ret < 0)
		dev_err(&spi_dev->dev, "SPI oti data write error\n");

	return ret;
}

static ssize_t ad7816_show_mode(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);

	if (chip->mode)
		return sprintf(buf, "power-save\n");
	else
		return sprintf(buf, "full\n");
}

static ssize_t ad7816_store_mode(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);

	if (strcmp(buf, "full")) {
		gpio_set_value(chip->rdwr_pin, 1);
		chip->mode = AD7816_FULL;
	} else {
		gpio_set_value(chip->rdwr_pin, 0);
		chip->mode = AD7816_PD;
	}

	return len;
}

static IIO_DEVICE_ATTR(mode, S_IRUGO | S_IWUSR,
		ad7816_show_mode,
		ad7816_store_mode,
		0);

static ssize_t ad7816_show_available_modes(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "full\npower-save\n");
}

static IIO_DEVICE_ATTR(available_modes, S_IRUGO, ad7816_show_available_modes, NULL, 0);

static ssize_t ad7816_show_channel(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);

	return sprintf(buf, "%d\n", chip->channel_id);
}

static ssize_t ad7816_store_channel(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);
	unsigned long data;
	int ret;

	ret = strict_strtoul(buf, 10, &data);
	if (ret)
		return -EINVAL;

	if (data > AD7816_CS_MAX && data != AD7816_CS_MASK) {
		dev_err(&chip->spi_dev->dev, "Invalid channel id %lu for %s.\n",
			data, dev_info->name);
		return -EINVAL;
	} else if (strcmp(dev_info->name, "ad7818") == 0 && data > 1) {
		dev_err(&chip->spi_dev->dev,
			"Invalid channel id %lu for ad7818.\n", data);
		return -EINVAL;
	} else if (strcmp(dev_info->name, "ad7816") == 0 && data > 0) {
		dev_err(&chip->spi_dev->dev,
			"Invalid channel id %lu for ad7816.\n", data);
		return -EINVAL;
	}

	chip->channel_id = data;

	return len;
}

static IIO_DEVICE_ATTR(channel, S_IRUGO | S_IWUSR,
		ad7816_show_channel,
		ad7816_store_channel,
		0);


static ssize_t ad7816_show_value(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);
	u16 data;
	s8 value;
	int ret;

	ret = ad7816_spi_read(chip, &data);
	if (ret)
		return -EIO;

	data >>= AD7816_VALUE_OFFSET;

	if (chip->channel_id == 0) {
		value = (s8)((data >> AD7816_TEMP_FLOAT_OFFSET) - 103);
		data &= AD7816_TEMP_FLOAT_MASK;
		if (value < 0)
			data = (1 << AD7816_TEMP_FLOAT_OFFSET) - data;
		return sprintf(buf, "%d.%.2d\n", value, data * 25);
	} else
		return sprintf(buf, "%u\n", data);
}

static IIO_DEVICE_ATTR(value, S_IRUGO, ad7816_show_value, NULL, 0);

static struct attribute *ad7816_attributes[] = {
	&iio_dev_attr_available_modes.dev_attr.attr,
	&iio_dev_attr_mode.dev_attr.attr,
	&iio_dev_attr_channel.dev_attr.attr,
	&iio_dev_attr_value.dev_attr.attr,
	NULL,
};

static const struct attribute_group ad7816_attribute_group = {
	.attrs = ad7816_attributes,
};

/*
 * temperature bound events
 */

#define IIO_EVENT_CODE_AD7816_OTI IIO_UNMOD_EVENT_CODE(IIO_TEMP,	\
						       0,		\
						       IIO_EV_TYPE_THRESH, \
						       IIO_EV_DIR_FALLING)

static irqreturn_t ad7816_event_handler(int irq, void *private)
{
	iio_push_event(private, 0,
		       IIO_EVENT_CODE_AD7816_OTI,
		       iio_get_time_ns());
	return IRQ_HANDLED;
}

static ssize_t ad7816_show_oti(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);
	int value;

	if (chip->channel_id > AD7816_CS_MAX) {
		dev_err(dev, "Invalid oti channel id %d.\n", chip->channel_id);
		return -EINVAL;
	} else if (chip->channel_id == 0) {
		value = AD7816_BOUND_VALUE_MIN +
			(chip->oti_data[chip->channel_id] -
			AD7816_BOUND_VALUE_BASE);
		return sprintf(buf, "%d\n", value);
	} else
		return sprintf(buf, "%u\n", chip->oti_data[chip->channel_id]);
}

static inline ssize_t ad7816_set_oti(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *dev_info = dev_get_drvdata(dev);
	struct ad7816_chip_info *chip = iio_priv(dev_info);
	long value;
	u8 data;
	int ret;

	ret = strict_strtol(buf, 10, &value);

	if (chip->channel_id > AD7816_CS_MAX) {
		dev_err(dev, "Invalid oti channel id %d.\n", chip->channel_id);
		return -EINVAL;
	} else if (chip->channel_id == 0) {
		if (ret || value < AD7816_BOUND_VALUE_MIN ||
			value > AD7816_BOUND_VALUE_MAX)
			return -EINVAL;

		data = (u8)(value - AD7816_BOUND_VALUE_MIN +
			AD7816_BOUND_VALUE_BASE);
	} else {
		if (ret || value < AD7816_BOUND_VALUE_BASE || value > 255)
			return -EINVAL;

		data = (u8)value;
	}

	ret = ad7816_spi_write(chip, data);
	if (ret)
		return -EIO;

	chip->oti_data[chip->channel_id] = data;

	return len;
}

static IIO_DEVICE_ATTR(oti, S_IRUGO | S_IWUSR,
		       ad7816_show_oti, ad7816_set_oti, 0);

static struct attribute *ad7816_event_attributes[] = {
	&iio_dev_attr_oti.dev_attr.attr,
	NULL,
};

static struct attribute_group ad7816_event_attribute_group = {
	.attrs = ad7816_event_attributes,
	.name = "events",
};

static const struct iio_info ad7816_info = {
	.attrs = &ad7816_attribute_group,
	.num_interrupt_lines = 1,
	.event_attrs = &ad7816_event_attribute_group,
	.driver_module = THIS_MODULE,
};

/*
 * device probe and remove
 */

static int __devinit ad7816_probe(struct spi_device *spi_dev)
{
	struct ad7816_chip_info *chip;
	struct iio_dev *indio_dev;
	unsigned short *pins = spi_dev->dev.platform_data;
	int ret = 0;
	int i;

	if (!pins) {
		dev_err(&spi_dev->dev, "No necessary GPIO platform data.\n");
		return -EINVAL;
	}

	indio_dev = iio_allocate_device(sizeof(*chip));
	if (indio_dev == NULL) {
		ret = -ENOMEM;
		goto error_ret;
	}
	chip = iio_priv(indio_dev);
	/* this is only used for device removal purposes */
	dev_set_drvdata(&spi_dev->dev, indio_dev);

	chip->spi_dev = spi_dev;
	for (i = 0; i <= AD7816_CS_MAX; i++)
		chip->oti_data[i] = 203;
	chip->rdwr_pin = pins[0];
	chip->convert_pin = pins[1];
	chip->busy_pin = pins[2];

	ret = gpio_request(chip->rdwr_pin, spi_get_device_id(spi_dev)->name);
	if (ret) {
		dev_err(&spi_dev->dev, "Fail to request rdwr gpio PIN %d.\n",
			chip->rdwr_pin);
		goto error_free_device;
	}
	gpio_direction_input(chip->rdwr_pin);
	ret = gpio_request(chip->convert_pin, spi_get_device_id(spi_dev)->name);
	if (ret) {
		dev_err(&spi_dev->dev, "Fail to request convert gpio PIN %d.\n",
			chip->convert_pin);
		goto error_free_gpio_rdwr;
	}
	gpio_direction_input(chip->convert_pin);
	ret = gpio_request(chip->busy_pin, spi_get_device_id(spi_dev)->name);
	if (ret) {
		dev_err(&spi_dev->dev, "Fail to request busy gpio PIN %d.\n",
			chip->busy_pin);
		goto error_free_gpio_convert;
	}
	gpio_direction_input(chip->busy_pin);

	indio_dev->name = spi_get_device_id(spi_dev)->name;
	indio_dev->dev.parent = &spi_dev->dev;
	indio_dev->info = &ad7816_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_device_register(indio_dev);
	if (ret)
		goto error_free_gpio;

	if (spi_dev->irq) {
		/* Only low trigger is supported in ad7816/7/8 */
		ret = request_threaded_irq(spi_dev->irq,
					   NULL,
					   &ad7816_event_handler,
					   IRQF_TRIGGER_LOW,
					   indio_dev->name,
					   indio_dev);
		if (ret)
			goto error_unreg_dev;
	}

	dev_info(&spi_dev->dev, "%s temperature sensor and ADC registered.\n",
			 indio_dev->name);

	return 0;

error_unreg_dev:
	iio_device_unregister(indio_dev);
error_free_gpio:
	gpio_free(chip->busy_pin);
error_free_gpio_convert:
	gpio_free(chip->convert_pin);
error_free_gpio_rdwr:
	gpio_free(chip->rdwr_pin);
error_free_device:
	iio_free_device(indio_dev);
error_ret:
	return ret;
}

static int __devexit ad7816_remove(struct spi_device *spi_dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(&spi_dev->dev);
	struct ad7816_chip_info *chip = iio_priv(indio_dev);

	dev_set_drvdata(&spi_dev->dev, NULL);
	if (spi_dev->irq)
		free_irq(spi_dev->irq, indio_dev);
	gpio_free(chip->busy_pin);
	gpio_free(chip->convert_pin);
	gpio_free(chip->rdwr_pin);
	iio_device_unregister(indio_dev);
	iio_free_device(indio_dev);

	return 0;
}

static const struct spi_device_id ad7816_id[] = {
	{ "ad7816", 0 },
	{ "ad7817", 0 },
	{ "ad7818", 0 },
	{}
};

MODULE_DEVICE_TABLE(spi, ad7816_id);

static struct spi_driver ad7816_driver = {
	.driver = {
		.name = "ad7816",
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = ad7816_probe,
	.remove = __devexit_p(ad7816_remove),
	.id_table = ad7816_id,
};

static __init int ad7816_init(void)
{
	return spi_register_driver(&ad7816_driver);
}

static __exit void ad7816_exit(void)
{
	spi_unregister_driver(&ad7816_driver);
}

MODULE_AUTHOR("Sonic Zhang <sonic.zhang@analog.com>");
MODULE_DESCRIPTION("Analog Devices AD7816/7/8 digital"
			" temperature sensor driver");
MODULE_LICENSE("GPL v2");

module_init(ad7816_init);
module_exit(ad7816_exit);
