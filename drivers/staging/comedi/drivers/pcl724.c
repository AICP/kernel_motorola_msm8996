/*
    comedi/drivers/pcl724.c

    Michal Dobes <dobes@tesnet.cz>

    hardware driver for Advantech cards:
     card:   PCL-724, PCL-722, PCL-731
     driver: pcl724,  pcl722,  pcl731
    and ADLink cards:
     card:   ACL-7122, ACL-7124, PET-48DIO
     driver: acl7122,  acl7124,  pet48dio

    Options for PCL-724, PCL-731, ACL-7124 and PET-48DIO:
     [0] - IO Base

    Options for PCL-722 and ACL-7122:
     [0] - IO Base
     [1] - IRQ (0=disable IRQ) IRQ isn't supported at this time!
     [2] -number of DIO:
	      0, 144: 144 DIO configuration
	      1,  96:  96 DIO configuration
*/
/*
Driver: pcl724
Description: Advantech PCL-724, PCL-722, PCL-731 ADLink ACL-7122, ACL-7124,
  PET-48DIO
Author: Michal Dobes <dobes@tesnet.cz>
Devices: [Advantech] PCL-724 (pcl724), PCL-722 (pcl722), PCL-731 (pcl731),
  [ADLink] ACL-7122 (acl7122), ACL-7124 (acl7124), PET-48DIO (pet48dio)
Status: untested

This is driver for digital I/O boards PCL-722/724/731 with 144/24/48 DIO
and for digital I/O boards ACL-7122/7124/PET-48DIO with 144/24/48 DIO.
It need 8255.o for operations and only immediate mode is supported.
See the source for configuration details.
*/
/*
 * check_driver overrides:
 *   struct comedi_insn
 */

#include "../comedidev.h"

#include <linux/ioport.h>
#include <linux/delay.h>

#include "8255.h"

#define PCL722_SIZE    32
#define PCL722_96_SIZE 16
#define PCL724_SIZE     4
#define PCL731_SIZE     8
#define PET48_SIZE      2

#define SIZE_8255	4

struct pcl724_board {

	const char *name;	/*  board name */
	int dio;		/*  num of DIO */
	int numofports;		/*  num of 8255 subdevices */
	unsigned int io_range;	/*  len of IO space */
	char can_have96;
	char is_pet48;
};

static const struct pcl724_board boardtypes[] = {
	{
		.name		= "pcl724",
		.dio		= 24,
		.numofports	= 1,
		.io_range	= PCL724_SIZE,
	}, {
		.name		= "pcl722",
		.dio		= 144,
		.numofports	= 6,
		.io_range	= PCL722_SIZE,
		.can_have96	= 1,
	}, {
		.name		= "pcl731",
		.dio		= 48,
		.numofports	= 2,
		.io_range	= PCL731_SIZE,
	}, {
		.name		= "acl7122",
		.dio		= 144,
		.numofports	= 6,
		.io_range	= PCL722_SIZE,
		.can_have96	= 1,
	}, {
		.name		= "acl7124",
		.dio		= 24,
		.numofports	= 1,
		.io_range	= PCL724_SIZE,
	}, {
		.name		= "pet48dio",
		.dio		= 48,
		.numofports	= 2,
		.io_range	= PET48_SIZE,
		.is_pet48	= 1,
	},
};

static int subdev_8255_cb(int dir, int port, int data, unsigned long arg)
{
	unsigned long iobase = arg;

	if (dir) {
		outb(data, iobase + port);
		return 0;
	} else {
		return inb(iobase + port);
	}
}

static int subdev_8255mapped_cb(int dir, int port, int data,
				unsigned long iobase)
{
	int movport = SIZE_8255 * (iobase >> 12);

	iobase &= 0x0fff;

	if (dir) {
		outb(port + movport, iobase);
		outb(data, iobase + 1);
		return 0;
	} else {
		outb(port + movport, iobase);
		return inb(iobase + 1);
	}
}

static int pcl724_attach(struct comedi_device *dev, struct comedi_devconfig *it)
{
	const struct pcl724_board *board = comedi_board(dev);
	struct comedi_subdevice *s;
	unsigned int iorange;
	int ret, i, n_subdevices;

	iorange = board->io_range;
	if ((board->can_have96) &&
	    ((it->options[1] == 1) || (it->options[1] == 96)))
		iorange = PCL722_96_SIZE; /* PCL-724 in 96 DIO configuration */
	ret = comedi_request_region(dev, it->options[0], iorange);
	if (ret)
		return ret;

	n_subdevices = board->numofports;
	if ((board->can_have96) && ((it->options[1] == 1)
					 || (it->options[1] == 96)))
		n_subdevices = 4;	/*  PCL-724 in 96 DIO configuration */

	ret = comedi_alloc_subdevices(dev, n_subdevices);
	if (ret)
		return ret;

	for (i = 0; i < dev->n_subdevices; i++) {
		s = &dev->subdevices[i];
		if (board->is_pet48) {
			subdev_8255_init(dev, s, subdev_8255mapped_cb,
					 (unsigned long)(dev->iobase +
							 i * 0x1000));
		} else
			subdev_8255_init(dev, s, subdev_8255_cb,
					 (unsigned long)(dev->iobase +
							 SIZE_8255 * i));
	}

	return 0;
}

static void pcl724_detach(struct comedi_device *dev)
{
	int i;

	for (i = 0; i < dev->n_subdevices; i++)
		comedi_spriv_free(dev, i);
	comedi_legacy_detach(dev);
}

static struct comedi_driver pcl724_driver = {
	.driver_name	= "pcl724",
	.module		= THIS_MODULE,
	.attach		= pcl724_attach,
	.detach		= pcl724_detach,
	.board_name	= &boardtypes[0].name,
	.num_names	= ARRAY_SIZE(boardtypes),
	.offset		= sizeof(struct pcl724_board),
};
module_comedi_driver(pcl724_driver);

MODULE_AUTHOR("Comedi http://www.comedi.org");
MODULE_DESCRIPTION("Comedi low-level driver");
MODULE_LICENSE("GPL");
