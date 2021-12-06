/*****************************************************************************/
/* HEADER FILES */
/*****************************************************************************/

#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>



/*****************************************************************************/
/* MODULE INFO */
/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Standarski");
MODULE_DESCRIPTION("External LEDs driver for BeagleBone Black Rev. C");



/*****************************************************************************/
/* PRIVATE MACROS */
/*****************************************************************************/

#undef pr_fmt
#define pr_fmt(fmt) "%s: "fmt,__func__

#define LABEL_SIZE_MAX  20



/*****************************************************************************/
/* PLATFORM DRIVER FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static int led_ext_probe(struct platform_device *platform_device);

static int led_ext_remove(struct platform_device *platform_device);



/*****************************************************************************/
/* DEVICE ATTRIBUTES FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static ssize_t state_show(struct device *device,
    struct device_attribute *device_attribute, char *output_buffer);

static ssize_t state_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count);

static ssize_t label_show(struct device *dev, struct device_attribute *attr,
    char *buf);



/*****************************************************************************/
/* PRIVATE STRUCTURES DEFINITIONS*/
/*****************************************************************************/

struct led_ext_private_data {
    char label[LABEL_SIZE_MAX];
    struct gpio_desc *gpio_desc;
};



/*****************************************************************************/
/* PRIVATE VARIABLES DECLARATIONS */
/*****************************************************************************/

static unsigned led_ext_device_count;

static struct class *led_ext_class;

static struct device **led_ext_devices;

static struct of_device_id led_ext_device_match[] = {
    {.compatible = "jstand,led_ext"},
    { }
};

static struct platform_driver led_ext_platform_driver = {
    .probe = led_ext_probe,
    .remove = led_ext_remove,
    .driver = {
        .name = "led_ext",
        .of_match_table = of_match_ptr(led_ext_device_match)
    }
};

static DEVICE_ATTR_RW(state);
static DEVICE_ATTR_RO(label);

static struct attribute *led_ext_attributes[] = {
    &dev_attr_state.attr,
    &dev_attr_label.attr,
    NULL
};

static struct attribute_group led_ext_attributes_group = {
    .attrs = led_ext_attributes
};

static const struct attribute_group *led_ext_attributes_groups[] = {
    &led_ext_attributes_group,
    NULL
};



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int __init led_ext_init(void)
{
    int return_code = 0;

    led_ext_class = class_create(THIS_MODULE, "led_ext");
    if (!IS_ERR(led_ext_class)) {
        pr_info("Class creation done...\n");

        return_code = platform_driver_register(&led_ext_platform_driver);
        if (return_code == 0) {
            pr_info("Driver registration done...\n");
        } else {
            pr_err("Driver registration failed...\n");

            class_destroy(led_ext_class);
        }
    } else {
        pr_err("Class creation failed...\n");
        return_code = PTR_ERR(led_ext_class);
    }

    if (return_code == 0) {
        pr_info("Module loaded successfully!\n");
    } else {
        pr_err("Unable to load module!\n");
    }

    return return_code;
}
module_init(led_ext_init);



static void __exit led_ext_exit(void)
{
    platform_driver_unregister(&led_ext_platform_driver);

    class_destroy(led_ext_class);

    pr_info("Module removed successfully!\n");
}
module_exit(led_ext_exit);



/*****************************************************************************/
/* PLATFORM DRIVER FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int led_ext_probe(struct platform_device *platform_device)
{
    int return_code = 0;
    unsigned device_index = 0;
    struct device_node *parent_device_node = platform_device->dev.of_node;
    struct device_node *child_device_node = NULL;
    struct led_ext_private_data *led_ext_private_data = NULL;
    const char *label = NULL;

    led_ext_device_count = of_get_available_child_count(parent_device_node);
    if (led_ext_device_count > 0) {
        dev_info(&platform_device->dev, "Total external LEDs found: %u\n",
            led_ext_device_count);

        led_ext_devices = devm_kzalloc(&platform_device->dev,
            sizeof(struct device *) * led_ext_device_count, GFP_KERNEL);
        if (led_ext_devices != NULL) {
            dev_info(&platform_device->dev, "Memory allocation for "
                "external LEDs devices done...\n");

            for_each_available_child_of_node(parent_device_node,
                child_device_node) {

                if (return_code != 0) {
                    break;
                }

                led_ext_private_data = devm_kzalloc(&platform_device->dev,
                    sizeof(struct led_ext_private_data), GFP_KERNEL);
                if (led_ext_private_data != NULL) {
                    dev_err(&platform_device->dev, "Memory allocation for "
                        "external LED private data done...\n");

                    return_code = of_property_read_string(child_device_node,
                        "label", &label);
                    if (return_code == 0) {
                        dev_info(&platform_device->dev, "External LED label: "
                            "%s\n", label);
                        strcpy(led_ext_private_data->label, label);
                    } else {
                        dev_warn(&platform_device->dev, "Missing label "
                            "information...\n");
                    }

                    led_ext_private_data->gpio_desc =
                        devm_fwnode_get_gpiod_from_child(&platform_device->dev,
                            "state", &child_device_node->fwnode, GPIOD_OUT_LOW,
                            led_ext_private_data->label);
                    if (!IS_ERR(led_ext_private_data->gpio_desc)) {
                        dev_info(&platform_device->dev, "GPIO assignment "
                            "done...\n");

                        led_ext_devices[device_index] =
                            device_create_with_groups(led_ext_class,
                                &platform_device->dev, 0, led_ext_private_data,
                                led_ext_attributes_groups,
                                led_ext_private_data->label);
                        if (!IS_ERR(led_ext_devices[device_index])) {
                            dev_info(&platform_device->dev, "Device creation "
                             "done...\n");
                            ++device_index;
                        } else {
                            dev_err(&platform_device->dev, "Device creation "
                                "failed...\n");
                            return_code = PTR_ERR(
                                led_ext_devices[device_index]);
                        }
                    } else {
                        dev_err(&platform_device->dev, "Missing GPIO for the"
                            "requested function and/or index...\n");
                        return_code = PTR_ERR(led_ext_private_data->gpio_desc);
                    }
                } else {
                    dev_err(&platform_device->dev, "Memory allocation for"
                        "external LEDs private data failed...\n");
                    return_code = -ENOMEM;
                }
            }
        } else {
            dev_err(&platform_device->dev, "Memory allocation for"
                "external LEDs devices failed...\n");
            return_code = -ENOMEM;
        }
    } else {
        dev_err(&platform_device->dev, "No external LEDs found...\n");
    }

    if (return_code == 0) {
        dev_info(&platform_device->dev, "Probe function finished "
            "successfully!\n");
    } else {
        dev_err(&platform_device->dev, "Probe function failed!\n");
    }

    return return_code;
}



static int led_ext_remove(struct platform_device *platform_device)
{
    int return_code = 0;
    unsigned device_index = 0;
    
    for (; device_index < led_ext_device_count; ++device_index) {
        device_unregister(led_ext_devices[device_index]);
    }

    dev_info(&platform_device->dev, "Remove function finished "
        "successfully!\n");

    return return_code;
}



/*****************************************************************************/
/* DEVICE ATTRIBUTES FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static ssize_t state_show(struct device *device,
    struct device_attribute *device_attribute, char *output_buffer)
{
    ssize_t return_code = 0;
    int gpio_value = 0;
    char *led_state = NULL;
    struct led_ext_private_data *led_ext_private_data = NULL;

    led_ext_private_data = dev_get_drvdata(device);
    if (led_ext_private_data != NULL) {
        gpio_value = gpiod_get_value(led_ext_private_data->gpio_desc);
        if (gpio_value == 0) {
            led_state = "OFF";
        } else if (gpio_value == 1) {
            led_state = "ON";
        } else {
            led_state = "UNKNOWN";
        }
    } else {
        led_state = "UNKNOWN";
    }

    return_code = sprintf(output_buffer, "%s\n", led_state);

    return return_code;
}



static ssize_t state_store(struct device *device,
    struct device_attribute *device_attribute, const char *input_buffer,
    size_t char_count)
{
    ssize_t return_code = 0;
    struct led_ext_private_data *led_ext_private_data = NULL;

    led_ext_private_data = dev_get_drvdata(device);
    if (led_ext_private_data != NULL) {
        if (sysfs_streq(input_buffer, "ON")) {
            gpiod_set_value(led_ext_private_data->gpio_desc, 1);
            return_code = char_count;
        } else if (sysfs_streq(input_buffer, "OFF")) {
            gpiod_set_value(led_ext_private_data->gpio_desc, 0);
            return_code = char_count;
        } else {
            return_code = -EINVAL;
        }
    } else {
        return_code = -ENOENT;
    }

    return return_code;
}



static ssize_t label_show(struct device *device,
    struct device_attribute *device_attribute, char *output_buffer)
{
    ssize_t return_code = 0;
    struct led_ext_private_data *led_ext_private_data = NULL;

    led_ext_private_data = dev_get_drvdata(device);
    if (led_ext_private_data != NULL) {
        return_code = sprintf(output_buffer, "%s\n",
            led_ext_private_data->label);
    } else {
        return_code = -ENOENT;
    }

    return return_code;
}

