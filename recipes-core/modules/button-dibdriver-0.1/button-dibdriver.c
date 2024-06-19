#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

static int button_irq;
static int button_value = 0;
static dev_t dev_num;
static struct cdev my_cdev;
static struct class *button_class;
static struct gpio_desc *button_gpio;

static irqreturn_t button_irq_handler(int irq, void *dev_id) {
    button_value = gpiod_get_value(button_gpio);
    printk(KERN_INFO "Button interrupt: value = %d\n", button_value);
    return IRQ_HANDLED;
}

static int dev_open(struct inode *inode, struct file *file) {
    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    int ret;
    char buffer[2];
    buffer[0] = button_value + '0';
    buffer[1] = '\n';

    ret = copy_to_user(buf, buffer, 2);
    if (ret)
        return -EFAULT;

    return 2;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
};

static int gpio_button_probe(struct platform_device *pdev) {
    int result;
    struct device *dev = &pdev->dev;

	struct device_node *child = of_get_child_by_name(dev->of_node, "button0");
    if (!child) {
        dev_err(dev, "No child node named 'button0'\n");
        return -ENODEV;
    }
    button_gpio = devm_gpiod_get_from_of_node(dev, child, "gpios", 0, GPIOD_IN, "button0");
    if (IS_ERR(button_gpio)) {
        dev_err(dev, "Failed to get GPIO for button\n");
        return PTR_ERR(button_gpio);
    }

    button_irq = gpiod_to_irq(button_gpio);
    if (button_irq < 0) {
        dev_err(dev, "Failed to get IRQ number for button\n");
        return button_irq;
    }

    result = devm_request_irq(dev, button_irq, button_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button_gpio_handler", NULL);
    if (result) {
        dev_err(dev, "Failed to request IRQ\n");
        return result;
    }

    alloc_chrdev_region(&dev_num, 0, 1, "gpio_button");
    cdev_init(&my_cdev, &fops);
    result = cdev_add(&my_cdev, dev_num, 1);
    if (result) {
        dev_err(dev, "Failed to add cdev\n");
        return result;
    }

    button_class = class_create(THIS_MODULE, "gpio_button_class");
    if (IS_ERR(button_class)) {
        cdev_del(&my_cdev);
        return PTR_ERR(button_class);
    }

    if (IS_ERR(device_create(button_class, NULL, dev_num, NULL, "gpio_button"))) {
        class_destroy(button_class);
        cdev_del(&my_cdev);
        return -EINVAL;
    }

    dev_info(dev, "GPIO Button Driver Initialized\n");
    return 0;
}

static int gpio_button_remove(struct platform_device *pdev) {
    device_destroy(button_class, dev_num);
    class_unregister(button_class);
    class_destroy(button_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    return 0;
}

static const struct of_device_id gpio_button_dt_ids[] = {
    { .compatible = "packt,gpio-descriptor-sample", },
    { }
};

MODULE_DEVICE_TABLE(of, gpio_button_dt_ids);

static struct platform_driver gpio_button_driver = {
    .probe  = gpio_button_probe,
    .remove = gpio_button_remove,
    .driver = {
        .name = "gpio_button",
        .of_match_table = of_match_ptr(gpio_button_dt_ids),
    },
};

module_platform_driver(gpio_button_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lampaBiurkowa");
MODULE_DESCRIPTION("GPIO Button");
