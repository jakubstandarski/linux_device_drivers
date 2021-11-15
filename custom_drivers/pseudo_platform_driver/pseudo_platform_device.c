/*****************************************************************************/
/* HEADER FILES */
/*****************************************************************************/

#include "pseudo_platform_device.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>



/*****************************************************************************/
/* MODULE INFO */
/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Standarski");
MODULE_DESCRIPTION("Pseudo platform device setup for learning purposes.");



/*****************************************************************************/
/* PRIVATE MACROS */
/*****************************************************************************/

#undef pr_fmt
#define pr_fmt(fmt) "%s: "fmt,__func__



/*****************************************************************************/
/* RELEASE FUNCTION DECLARATION */
/*****************************************************************************/

static void pseudo_platform_device_release(struct device *device);



/*****************************************************************************/
/* PRIVATE DATA */
/*****************************************************************************/

static struct pseudo_platform_device_platform_data
    pseudo_platform_device_platform_data_0 = {
        .serial_number = "ppdxyz000",
        .comms_baudrate = 115200
};

static struct platform_device pseudo_platform_device_0 = {
    .name = "ppd_0",
    .id = 0,
    .dev = {
        .platform_data = &pseudo_platform_device_platform_data_0,
        .release = pseudo_platform_device_release
    }
};



static struct pseudo_platform_device_platform_data
    pseudo_platform_device_platform_data_1 = {
        .serial_number = "ppdxyz001",
        .comms_baudrate = 115200
};

static struct platform_device pseudo_platform_device_1 = {
    .name = "ppd_1",
    .id = 1,
    .dev = {
        .platform_data = &pseudo_platform_device_platform_data_1,
        .release = pseudo_platform_device_release
    }
};



static struct platform_device *pseudo_platform_devices[
    PSEUDO_PLATFORM_DEVICE_COUNT] = {
        &pseudo_platform_device_0,
        &pseudo_platform_device_1
};



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int __init pseudo_platform_device_init(void)
{
    int return_code = 0;

    unsigned int device_index = 0;
    for (; device_index < PSEUDO_PLATFORM_DEVICE_COUNT; ++device_index) {
        return_code = platform_device_register(
            pseudo_platform_devices[device_index]);
        if (return_code == 0) {
            pr_info("Device %u registration done...\n", device_index);
        } else {
            pr_err("Device %u registration failed!\n", device_index);
        }
    }

    return return_code;
}

module_init(pseudo_platform_device_init);



static void __exit pseudo_platform_device_exit(void)
{
    unsigned device_index = 0;
    for (; device_index < PSEUDO_PLATFORM_DEVICE_COUNT; ++device_index) {
        platform_device_unregister(pseudo_platform_devices[device_index]);
    }

    pr_info("Devices unregistration done.\n");
}

module_exit(pseudo_platform_device_exit);



/*****************************************************************************/
/* RELEASE FUNCTION DEFINITION */
/*****************************************************************************/

static void pseudo_platform_device_release(struct device *device)
{
    pr_info("Nothing to be done by release function...\n");
    pr_info("Device released successfully.\n");
}

