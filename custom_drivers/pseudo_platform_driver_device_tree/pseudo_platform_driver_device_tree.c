/*****************************************************************************/
/* HEADER FILES */
/*****************************************************************************/

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>



/*****************************************************************************/
/* MODULE INFO */
/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Standarski");
MODULE_DESCRIPTION("Pseudo platform driver for learning purposes.");



/*****************************************************************************/
/* PRIVATE MACROS */
/*****************************************************************************/

#undef pr_fmt
#define pr_fmt(fmt) "%s: "fmt,__func__

#define DEVICE_COUNT    2

#define DEVICE_DATA_BUFFER_SIZE 512



/*****************************************************************************/
/* PRIVATE DATA STRUCTURES DECLARATIONS */
/*****************************************************************************/

struct platform_specific_data {
    const char *serial_number;
    unsigned comms_baudrate;
};

struct device_data {
    struct platform_specific_data platform_specific_data;
    char buffer[DEVICE_DATA_BUFFER_SIZE];
    dev_t device_number;
    struct cdev cdev;
    struct device *device;
};

struct device_config {
    int x_axis_calibration;
    int y_axis_calibration;
};



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static int __init pseudo_platform_driver_init(void);

static void __exit pseudo_platform_driver_exit(void);



/*****************************************************************************/
/* PLATFORM DRIVER FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static int pseudo_platform_driver_probe(
    struct platform_device *platform_device);

static int pseudo_platform_driver_remove(
    struct platform_device *platform_device);



/*****************************************************************************/
/* FILE OPERATIONS FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static int pseudo_platform_device_open(struct inode *inode, struct file *file);

static int pseudo_platform_device_release(struct inode *inode,
    struct file *file);

static ssize_t pseudo_platform_device_read(struct file *file,
    char __user *data_destination, size_t byte_to_read_count,
    loff_t *file_position);

static ssize_t pseudo_platform_device_write(struct file *file,
    const char __user *data_source, size_t byte_to_write_count,
    loff_t *file_position);

static loff_t pseudo_platform_device_llseek(struct file *file,
    loff_t file_position_offset, int whence);

static struct file_operations file_operations = {
    .owner = THIS_MODULE,
    .open = pseudo_platform_device_open,
    .release = pseudo_platform_device_release,
    .read = pseudo_platform_device_read,
    .write = pseudo_platform_device_write,
    .llseek = pseudo_platform_device_llseek
};



/*****************************************************************************/
/* HELPER FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static struct platform_specific_data get_platform_specific_data_from_dt(
    const struct device *const device);



/*****************************************************************************/
/* PRIVATE DATA DEFINITIONS */
/*****************************************************************************/

static unsigned active_device_count;

static dev_t device_number_base;

static struct class *device_class;

static const struct device_config ppd0_device_config = {
    .x_axis_calibration = 50,
    .y_axis_calibration = 100
};

static const struct device_config ppd1_device_config = {
    .x_axis_calibration = 10,
    .y_axis_calibration = 20
};

static const struct of_device_id device_tree_match_table[] = {
    [0] = {
        .compatible = "ppd0",
        .data = (void *)&ppd0_device_config
    },
    [1] = {
        .compatible = "ppd1",
        .data = (void *)&ppd1_device_config
    },
    { }
};

static struct platform_driver platform_driver = {
    .probe = pseudo_platform_driver_probe,
    .remove = pseudo_platform_driver_remove,
    .driver = {
        .name = "pseudo_platform_device_driver",
        .of_match_table = of_match_ptr(device_tree_match_table)
    }
};



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int __init pseudo_platform_driver_init(void)
{
    int return_code = 0;

    return_code = alloc_chrdev_region(&device_number_base, 0, DEVICE_COUNT,
        "pseudo_platform_devices");
    if (return_code == 0) {
        pr_info("Device number allocation done...\n");

        device_class = class_create(THIS_MODULE,
            "pseudo_platform_device_class");
        if (!IS_ERR(device_class)) {
            pr_info("Device class creation done...\n");

            return_code = platform_driver_register(&platform_driver);
            if (return_code == 0) {
                pr_info("Platform driver registration done...\n");
            } else {
                pr_err("Platform driver registration failed!\n");

                class_destroy(device_class);
                unregister_chrdev_region(device_number_base, DEVICE_COUNT);
            }
        } else {
            pr_err("Device class creation failed!\n");

            unregister_chrdev_region(device_number_base, DEVICE_COUNT);
            return_code = PTR_ERR(device_class);
        }
    } else {
        pr_err("Device number allocation failed!\n");
    }

    if (return_code == 0) {
        pr_info("Driver loaded successfully!\n");
    } else {
        pr_err("Driver loading failed!\n");
    }

    return return_code;
}

module_init(pseudo_platform_driver_init);



static void __exit pseudo_platform_driver_exit(void)
{
    platform_driver_unregister(&platform_driver);

    class_destroy(device_class);

    unregister_chrdev_region(device_number_base, DEVICE_COUNT);

    pr_info("Driver unloaded successfully.\n");
}

module_exit(pseudo_platform_driver_exit);



/*****************************************************************************/
/* PLATFORM DRIVER FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int pseudo_platform_driver_probe(
    struct platform_device *platform_device)
{
    int return_code = 0;
    struct device_data *device_data = NULL;
    struct platform_specific_data platform_specific_data = {0};

    struct device *device = &platform_device->dev;
    dev_info(device, "Device detection has been started...\n");

    device_data = devm_kzalloc(device, sizeof(struct device_data), GFP_KERNEL);
    if (device_data != NULL) {
        dev_info(device, "Memory allocation for device data done...\n");

        platform_specific_data = get_platform_specific_data_from_dt(device);

        memcpy(&device_data->platform_specific_data,
            &platform_specific_data, sizeof(struct platform_specific_data));

        dev_set_drvdata(device, device_data);

        device_data->device_number = device_number_base + active_device_count;

        cdev_init(&device_data->cdev, &file_operations);
        device_data->cdev.owner = THIS_MODULE;

        return_code = cdev_add(&device_data->cdev,
            device_data->device_number, 1);
        if (return_code == 0) {
            dev_info(device, "Adding character device done...\n");

            device_data->device = device_create(device_class, device,
                device_data->device_number, NULL,
                "pseudo_platform_device_%d", active_device_count);
            if (!IS_ERR(device_data->device)) {
                dev_info(device, "Device creation done...\n");

                ++active_device_count;
            } else {
                dev_err(device, "Device creation failed!\n");

                cdev_del(&device_data->cdev);
                return_code = PTR_ERR(device_data->device);
            }
        } else {
            dev_err(device, "Adding character device failed!\n");
        }
    } else {
        dev_err(device, "Memory allocation for device data failed!\n");

        return_code = -ENOMEM;
    }

    if (return_code == 0) {
        dev_info(device, "Device detected successfully.\n");
    } else {
        dev_err(device, "Device detection failed!\n");
    }

    return return_code;
}



static int pseudo_platform_driver_remove(
    struct platform_device *platform_device)
{
    int return_code = 0;
    struct device_data *device_data = NULL;
    struct device *device = &platform_device->dev;

    device_data = dev_get_drvdata(device);

    device_destroy(device_class, device_data->device_number);

    cdev_del(&device_data->cdev);

    --active_device_count;

    dev_info(device, "Device removed successfully.\n");

    return return_code;
}



/*****************************************************************************/
/* FILE OPERATIONS FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int pseudo_platform_device_open(struct inode *inode, struct file *file)
{
    /* TODO */
    return 0;
}

static int pseudo_platform_device_release(struct inode *inode,
    struct file *file)
{
    /* TODO */
    return 0;
}

static ssize_t pseudo_platform_device_read(struct file *file,
    char __user *data_destination, size_t byte_to_read_count,
    loff_t *file_position)
{
    /* TODO */
    return 0;
}

static ssize_t pseudo_platform_device_write(struct file *file,
    const char __user *data_source, size_t byte_to_write_count,
    loff_t *file_position)
{
    /* TODO */
    return 0;
}

static loff_t pseudo_platform_device_llseek(struct file *file,
    loff_t file_position_offset, int whence)
{
    /* TODO */
    return 0;
}



/*****************************************************************************/
/* HELPER FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static struct platform_specific_data get_platform_specific_data_from_dt(
    const struct device *const device)
{
    int check_code = 0;
    struct platform_specific_data platform_specific_data = {0};
    struct device_node *device_node = NULL;

    device_node = device->of_node;
    if (device_node != NULL) {
        dev_info(device, "Device node information available...\n");

        check_code = of_property_read_string(device_node,
            "organization-name,serial-number",
            &platform_specific_data.serial_number);
        if (check_code == 0) {
            dev_info(device, "Serial number found: %s\n",
                platform_specific_data.serial_number);
        } else {
            dev_err(device, "Lack of serial number!\n");
        }

        check_code = of_property_read_u32(device_node,
            "organization-name,comms-baudrate",
            &platform_specific_data.comms_baudrate);
        if (check_code == 0) {
            dev_info(device, "Comms baudrate found: %u\n",
                platform_specific_data.comms_baudrate);
        } else {
            dev_err(device, "Lack of comms baudrate info!\n");
        }
    } else {
        dev_err(device, "Lack of device node information...\n");
    }

    return platform_specific_data;
}

