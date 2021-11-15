/*****************************************************************************/
/* HEADER FILES */
/*****************************************************************************/

#include "pseudo_platform_device.h"

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
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

#define DEVICE_DATA_BUFFER_SIZE 512



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
/* PRIVATE DATA */
/*****************************************************************************/

static unsigned active_device_count;

static dev_t device_number_base;

static struct class *device_class;

static const struct platform_device_id pseudo_platform_device_id[
    PSEUDO_PLATFORM_DEVICE_COUNT] = {
        [0] = {.name = "ppd_0"},
        [1] = {.name = "ppd_1"}
};

static struct platform_driver platform_driver = {
    .probe = pseudo_platform_driver_probe,
    .remove = pseudo_platform_driver_remove,
    .driver = {
        .name = "pseudo_platform_device"
    },
    .id_table = pseudo_platform_device_id
};

struct device_data {
    struct pseudo_platform_device_platform_data platform_data;
    char buffer[DEVICE_DATA_BUFFER_SIZE];
    dev_t device_number;
    struct cdev cdev;
    struct device *device;
};



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int __init pseudo_platform_driver_init(void)
{
    int return_code = 0;

    return_code = alloc_chrdev_region(&device_number_base, 0,
        PSEUDO_PLATFORM_DEVICE_COUNT, "pseudo_platform_devices");
    if (return_code == 0) {
        pr_info("Device number allocation done...\n");

        device_class = class_create(THIS_MODULE,
            "device_class");
        if (!IS_ERR(device_class)) {
            pr_info("Device class creation done...\n");

            return_code = platform_driver_register(&platform_driver);
            if (return_code == 0) {
                pr_info("Platform driver registration done...\n");
            } else {
                pr_err("Platform driver registration failed!\n");

                class_destroy(device_class);
                unregister_chrdev_region(device_number_base,
                    PSEUDO_PLATFORM_DEVICE_COUNT);
            }
        } else {
            pr_err("Device class creation failed!\n");

            return_code = PTR_ERR(device_class);
            unregister_chrdev_region(device_number_base,
                PSEUDO_PLATFORM_DEVICE_COUNT);
        }
    } else {
        pr_err("Device number allocation failed!\n");
    }

    if (return_code == 0) {
        pr_info("Driver loading done!\n");
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

    unregister_chrdev_region(device_number_base, PSEUDO_PLATFORM_DEVICE_COUNT);

    pr_info("Driver unloading done.\n");
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
    struct pseudo_platform_device_platform_data *platform_data = NULL;

    platform_data =
        (struct pseudo_platform_device_platform_data *)dev_get_platdata(
            &platform_device->dev);
    if (platform_data != NULL) {
        pr_info("Platform data obtained successfully...\n");

        device_data = devm_kzalloc(&platform_device->dev,
            sizeof(struct device_data), GFP_KERNEL);
        if (device_data != NULL) {
            pr_info("Memory allocation for device data done...\n");

            dev_set_drvdata(&platform_device->dev, device_data);

            memcpy(&device_data->platform_data, platform_data,
                sizeof(struct pseudo_platform_device_platform_data));

            device_data->device_number = device_number_base +
                platform_device->id;

            cdev_init(&device_data->cdev, &file_operations);
            device_data->cdev.owner = THIS_MODULE;

            return_code = cdev_add(&device_data->cdev,
                device_data->device_number, 1);
            if (return_code == 0) {
                pr_info("Adding character device done...\n");

                device_data->device = device_create(device_class, NULL,
                    device_data->device_number, NULL,
                    "pseudo_platform_device_%d", platform_device->id);
                if (!IS_ERR(device_data->device)) {
                    pr_info("Device created successfully...\n");

                    ++active_device_count;
                } else {
                    pr_err("Device creation failed!\n");

                    cdev_del(&device_data->cdev);
                    return_code = PTR_ERR(device_data->device);
                }
            } else {
                pr_err("Adding character device failed!\n");
            }
        } else {
            pr_err("Memory allocation for device data failed!\n");

            return_code = -ENOMEM;
        }
    } else {
        pr_err("Platform data not available!\n");

        return_code = -EINVAL;
    }

    if (return_code == 0) {
        pr_info("Device detection done.\n");
    } else {
        pr_err("Device detection failed!\n");
    }

    return return_code;
}



static int pseudo_platform_driver_remove(
    struct platform_device *platform_device)
{
    int return_code = 0;
    struct device_data *device_data = NULL;

    device_data = dev_get_drvdata(&platform_device->dev);

    device_destroy(device_class, device_data->device_number);

    cdev_del(&device_data->cdev);

    --active_device_count;

    pr_info("Device removed successfully.\n");

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

