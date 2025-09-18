/*
 * mp-axi-vctcxo: AXI VCTCXO control
 *
 * Copyright (c) 2023 lone boy <lone_boy@microphase.com>
 * Copyright (c) 2025 Mateusz Nalewajski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/gpio/consumer.h>
#include <asm/io.h>

#define DAC_MEM_ADDR 0x43C00000

/*
*   DAC MODE 0 自动设置  1 手动设置
*   DAC value 1 写 dac的值
*   DAC dyn_value  mode 1 用户设置的值 mode 0 外部参考校准的值
*   DAC ref sel DAC MODE 0 0:10M 1:PPS 2:GPS
*   DAC_LOCKED PLL 锁定状态
*/

struct axi_vctcxo_data {
	struct device *dev;
	void __iomem *control_virtualaddr;
};

enum axi_vctcxo_iio_dev_attr {
	DAC_MODE,
	DAC_VALUE,
	DAC_READ_VALUE,
	DAC_REF_SEL,
	DAC_LOCKED,
};

static ssize_t axi_vctcxo_show(struct device *dev, struct device_attribute *attr,
			       char *buf)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	struct axi_vctcxo_data *axi_vctcxo = iio_priv(indio_dev);
	int ret = 0;

	switch ((u32)this_attr->address) {
	case DAC_MODE:
		ret = sprintf(buf, "%u\n",
			      ioread32(axi_vctcxo->control_virtualaddr));
		break;
	case DAC_VALUE:
		ret = sprintf(buf, "%u\n",
			      ioread32(axi_vctcxo->control_virtualaddr + 4));
		break;
	case DAC_READ_VALUE:
		ret = sprintf(buf, "%u\n",
			      ioread32(axi_vctcxo->control_virtualaddr + 8));
		break;
	case DAC_REF_SEL:
		ret = sprintf(buf, "%u\n",
			      ioread32(axi_vctcxo->control_virtualaddr + 12));
		break;
	case DAC_LOCKED:
		ret = sprintf(buf, "%u\n",
			      ioread32(axi_vctcxo->control_virtualaddr + 16));
		break;
	default:
		ret = EINVAL;
		break;
	}
	return ret;
}

static ssize_t axi_vctcxo_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	struct axi_vctcxo_data *axi_vctcxo = iio_priv(indio_dev);
	int ret = 0;
	u32 val;

	switch ((u32)this_attr->address) {
	case DAC_MODE:
		ret = kstrtou32(buf, 10, &val);
		iowrite32(val, axi_vctcxo->control_virtualaddr);
		break;
	case DAC_VALUE:
		ret = kstrtou32(buf, 10, &val);
		iowrite32(val, axi_vctcxo->control_virtualaddr + 4);
		break;
	case DAC_REF_SEL:
		ret = kstrtou32(buf, 10, &val);
		iowrite32(val, axi_vctcxo->control_virtualaddr + 12);
		break;
	default:
		ret = EINVAL;
		break;
	}
	return ret ? ret : len;
}

static IIO_DEVICE_ATTR(in_voltage_dac_mode, S_IRUGO | S_IWUSR, axi_vctcxo_show,
		       axi_vctcxo_store, DAC_MODE);

static IIO_DEVICE_ATTR(in_voltage_dac_value, S_IRUGO | S_IWUSR, axi_vctcxo_show,
		       axi_vctcxo_store, DAC_VALUE);

static IIO_DEVICE_ATTR(in_voltage_dac_read_value, S_IRUGO | S_IWUSR,
		       axi_vctcxo_show, axi_vctcxo_store, DAC_READ_VALUE);

static IIO_DEVICE_ATTR(in_voltage_dac_ref_sel, S_IRUGO | S_IWUSR, axi_vctcxo_show,
		       axi_vctcxo_store, DAC_REF_SEL);

static IIO_DEVICE_ATTR(in_voltage_dac_locked, S_IRUGO | S_IWUSR, axi_vctcxo_show,
		       axi_vctcxo_store, DAC_LOCKED);

static struct attribute *axi_vctcxo_attributes[] = {
	&iio_dev_attr_in_voltage_dac_mode.dev_attr.attr,
	&iio_dev_attr_in_voltage_dac_value.dev_attr.attr,
	&iio_dev_attr_in_voltage_dac_read_value.dev_attr.attr,
	&iio_dev_attr_in_voltage_dac_ref_sel.dev_attr.attr,
	&iio_dev_attr_in_voltage_dac_locked.dev_attr.attr,
	NULL
};

static const struct attribute_group axi_vctcxo_attribute_group = {
	.attrs = axi_vctcxo_attributes,
};

static int axi_vctcxo_read_raw(struct iio_dev *indio_dev,
			       const struct iio_chan_spec *chan, int *val,
			       int *val2, long mask)
{
	return 0;
}

static int axi_vctcxo_write_raw(struct iio_dev *indio_dev,
			        struct iio_chan_spec const *chan, int val, int val2,
			        long mask)
{
	return 0;
}

static const struct iio_info axi_vctcxo_iio_info = {
	.read_raw = &axi_vctcxo_read_raw,
	.write_raw = &axi_vctcxo_write_raw,
	.attrs = &axi_vctcxo_attribute_group,
};

static const struct of_device_id of_axi_vctcxo_match[] = {
	{ .compatible = "microphase,axi-vctcxo" },
	{},
};

MODULE_DEVICE_TABLE(of, of_axi_vctcxo_match);

static const struct iio_chan_spec axi_vctcxo_channles[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 0,
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_RAW),
	},
};

static int axi_vctcxo_probe(struct platform_device *pdev)
{
	struct axi_vctcxo_data *axi_vctcxo;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*axi_vctcxo));
	if (!indio_dev)
		return -ENOMEM;

	axi_vctcxo = iio_priv(indio_dev);
	axi_vctcxo->dev = &pdev->dev;

	axi_vctcxo->control_virtualaddr = ioremap(DAC_MEM_ADDR, 0x10000);
	if (!axi_vctcxo->control_virtualaddr) {
		dev_err(&pdev->dev, "unable to IOMAP axi-vctcx registers\n");
		return -ENOMEM;
	}

	dev_info(&pdev->dev, "IOMAP axi_vctcxo registers phyaddr %x virtaddr %x",
		 DAC_MEM_ADDR, (uint32_t)axi_vctcxo->control_virtualaddr);

	iowrite32(1, axi_vctcxo->control_virtualaddr);
	iowrite32(23000, axi_vctcxo->control_virtualaddr + 4);

	indio_dev->name = "axi-vctcxo";
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &axi_vctcxo_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	indio_dev->channels = axi_vctcxo_channles;
	indio_dev->num_channels = ARRAY_SIZE(axi_vctcxo_channles);
	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Couldn't register the device\n");
	}

	platform_set_drvdata(pdev, indio_dev);

	return ret;
}

static int axi_vctcxo_removed(struct platform_device *pdev)
{
	struct axi_vctcxo_data *axi_vctcxo;
	struct iio_dev *indio_dev;

	indio_dev = platform_get_drvdata(pdev);
	axi_vctcxo = iio_priv(indio_dev);

	if (axi_vctcxo->control_virtualaddr)
		iounmap(axi_vctcxo->control_virtualaddr);

	iio_device_unregister(indio_dev);
	return 0;
}

static struct platform_driver lpf1600_driver = {
	.probe		= axi_vctcxo_probe,
	.remove		= axi_vctcxo_removed,
	.driver		= {
	.name		= KBUILD_MODNAME,
	.of_match_table	= of_axi_vctcxo_match,
	},
};

module_platform_driver(lpf1600_driver);

MODULE_AUTHOR("loneboy <995586238@qq.com>");
MODULE_DESCRIPTION("mp_axi_vctcxo driver");
MODULE_LICENSE("GPL V2");
