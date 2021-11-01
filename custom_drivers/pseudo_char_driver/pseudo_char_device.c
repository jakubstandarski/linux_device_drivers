/*****************************************************************************/
/* HEADER FILES */
/*****************************************************************************/

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/module.h>



/*****************************************************************************/
/* PRIVATE MACROS */
/*****************************************************************************/

#define DEVICE_COUNT        4
#define DEVICE_BUFFER_SIZE  512


#undef pr_fmt
#define pr_fmt(fmt) "%s: "fmt, __func__



/*****************************************************************************/
/* PRIVATE ENUMS */
/*****************************************************************************/

typedef enum permission_type {
    PERMISSION_TYPE_READ,
    PERMISSION_TYPE_WRITE,
    PERMISSION_TYPE_READ_WRITE
}permission_type_t;



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static int __init pseudo_char_device_init(void);

static void __exit pseudo_char_device_exit(void);



/*****************************************************************************/
/* MODULE FILE OPERATIONS FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static loff_t pseudo_char_device_llseek(struct file *file,
    loff_t file_position_offset, int whence);

static ssize_t pseudo_char_device_read(struct file *file,
    char __user *dest_buffer, size_t byte_count, loff_t *file_position);

static ssize_t pseudo_char_device_write(struct file *file,
    const char __user *src_buffer, size_t byte_count, loff_t *file_position);

static int pseudo_char_device_open(struct inode *inode, struct file *file);

static int pseudo_char_device_release(struct inode *inode, struct file *file);



/*****************************************************************************/
/* HELPER FUNCTIONS DECLARATIONS */
/*****************************************************************************/

static int check_permission(const permission_type_t device_permission,
    const int access_mode);



/*****************************************************************************/
/* PRIVATE VARIABLES */
/*****************************************************************************/

static struct file_operations file_operations = {
    .owner = THIS_MODULE,
    .llseek = pseudo_char_device_llseek,
    .read = pseudo_char_device_read,
    .write = pseudo_char_device_write,
    .open = pseudo_char_device_open,
    .release = pseudo_char_device_release
};




/*****************************************************************************/
/* PRIVATE STRUCTURES */
/*****************************************************************************/

typedef struct device_data {
    size_t buffer_size;
    char buffer[DEVICE_BUFFER_SIZE];
    const char *serial_number;
    permission_type_t permission_type;
    struct cdev cdev;
}device_data_t;



typedef struct driver_data {
    size_t device_count;
    dev_t device_number;
    struct class *device_class;
    struct device *device_info;
    device_data_t device_data[DEVICE_COUNT];
}driver_data_t;

static driver_data_t driver_data = {
    .device_count = DEVICE_COUNT,
    .device_data = {
        [0] = {
            .buffer_size = DEVICE_BUFFER_SIZE,
            .serial_number = "pcd1",
            .permission_type = PERMISSION_TYPE_READ
        },
        [1] = {
            .buffer_size = DEVICE_BUFFER_SIZE,
            .serial_number = "pcd2",
            .permission_type = PERMISSION_TYPE_WRITE
        },
        [2] = {
            .buffer_size = DEVICE_BUFFER_SIZE,
            .serial_number = "pcd3",
            .permission_type = PERMISSION_TYPE_READ_WRITE
        },
        [3] = {
            .buffer_size = DEVICE_BUFFER_SIZE,
            .serial_number = "pcd4",
            .permission_type = PERMISSION_TYPE_READ_WRITE
        }
    }
};



/*****************************************************************************/
/* MODULE INIT & EXIT FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int __init pseudo_char_device_init(void)
{
    int return_code = 0;
    unsigned device_index = 0;
    bool cdev_registration_failure = false;
    bool device_creation_failure = false;

    /* Dynamically allocate a device number. */
    return_code = alloc_chrdev_region(&driver_data.device_number, 0,
        driver_data.device_count, "pseudo_char_devices");
    if (return_code == 0) {
        pr_info("Device number allocation done...\n");

        /* Create a device class under /sys/class/. */
        driver_data.device_class = class_create(THIS_MODULE,
            "pseudo_char_device_class");
        if (!IS_ERR(driver_data.device_class)) {
            pr_info("Device class creation done...\n");

            for (; device_index < driver_data.device_count; ++device_index) {
                pr_info("Device %d:%d initialization...\n",
                    MAJOR(driver_data.device_number + device_index),
                    MINOR(driver_data.device_number + device_index));

                /* Initialize cdev structure with the file operations. */
                cdev_init(&driver_data.device_data[device_index].cdev,
                    &file_operations);

                driver_data.device_data[device_index].cdev.owner = THIS_MODULE;
                /* Register cdev structure with VFS. */
                return_code = cdev_add(
                    &driver_data.device_data[device_index].cdev,
                    driver_data.device_number + device_index, 1);
                if (return_code == 0) {
                    pr_info("Adding device to the system done...\n");

                    /* Populate the sysfs with device information. */
                    driver_data.device_info = device_create(
                        driver_data.device_class, NULL,
                        driver_data.device_number + device_index, NULL,
                        "pseudo_char_device_%u", device_index);
                    if (!IS_ERR(driver_data.device_info)) {
                        pr_info("Creating device under sysfs done...\n");
                    } else {
                        pr_err("Creating device under sysfs failed!\n");
                        device_creation_failure = true;
                        return_code = PTR_ERR(driver_data.device_info);
                        break;
                    }
                } else {
                    pr_err("Adding device to the system failed!\n");
                    cdev_registration_failure = true;
                    break;
                }
            }
        } else {
            pr_err("Device class creation failed!\n");
            unregister_chrdev_region(driver_data.device_number,
                driver_data.device_count);
            return_code = PTR_ERR(driver_data.device_class);
        }
    } else {
        pr_err("Device number allocation failed!\n");
    }

    if (cdev_registration_failure || device_creation_failure) {
        for (; device_index >= 0; --device_index) {
            device_destroy(driver_data.device_class,
                driver_data.device_number + device_index);
            cdev_del(&driver_data.device_data[device_index].cdev);
        }
        class_destroy(driver_data.device_class);
        unregister_chrdev_region(driver_data.device_number,
            driver_data.device_count);
    }

    if (return_code == 0) {
        pr_info("Module initialization done successfully.\n");
    } else {
        pr_err("Module initialization failed!\n");
    }

    return return_code;
}



static void __exit pseudo_char_device_exit(void)
{
    unsigned device_index = 0;
    for (; device_index < driver_data.device_count; ++device_index) {
        /* Remove device information from the sysfs. */
        device_destroy(driver_data.device_class, driver_data.device_number +
            device_index);

        /* Remove a device (cdev structure) from Virtual File System (VFS). */
        cdev_del(&driver_data.device_data[device_index].cdev);
    }

    /* Remove class from /sys/class/. */
    class_destroy(driver_data.device_class);

    /* Deallocate assigned device number. */
    unregister_chrdev_region(driver_data.device_number,
        driver_data.device_count);

    pr_info("Module unloaded...\n");
}



/*****************************************************************************/
/* MODULE FILE OPERATIONS FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static loff_t pseudo_char_device_llseek(struct file *file,
    loff_t file_position_offset, int whence)
{
    loff_t return_code = 0;
    device_data_t *device_data = (device_data_t *)file->private_data;

    pr_info("Llseek operation requested...\n");
    pr_info("Current file position: %lld\n", file->f_pos);

    switch (whence) {
        case SEEK_SET: {
            if ((file_position_offset < device_data->buffer_size) &&
                (file_position_offset >= 0)) {
                    file->f_pos = file_position_offset;
                    return_code = file->f_pos;
            } else {
                return_code = -EINVAL;
            }
        }
        break;

        case SEEK_CUR: {
            const loff_t new_file_position = file->f_pos +
                file_position_offset;
            if ((new_file_position < device_data->buffer_size) &&
                (new_file_position >= 0)) {
                    file->f_pos = new_file_position;
                    return_code = file->f_pos;
            } else {
                return_code = -EINVAL;
            }
        }
        break;

        case SEEK_END: {
            const loff_t new_file_position = DEVICE_BUFFER_SIZE +
                file_position_offset;
            if ((new_file_position < device_data->buffer_size) &&
                (new_file_position >= 0)) {
                    file->f_pos = new_file_position;
                    return_code = file->f_pos;
            } else {
                return_code = -EINVAL;
            }
        }
        break;

        default: {
            return_code = -EINVAL;
        }
    }

    if (return_code == -EINVAL) {
        pr_err("Llseek operation failed!\n");
    } else {
        pr_info("New file position: %lld\n", file->f_pos);
    }

    return return_code;
}



static ssize_t pseudo_char_device_read(struct file *file,
    char __user *dest_buffer, size_t byte_count, loff_t *file_position)
{
    ssize_t return_code = 0;
    unsigned uncopied_byte_count = 0;
    device_data_t *device_data = (device_data_t *)file->private_data;

    pr_info("Read operation requested for %zu bytes...\n", byte_count);
    pr_info("Current file position: %lld\n", *file_position);

    if ((*file_position + byte_count) > device_data->buffer_size) {
        byte_count = device_data->buffer_size - *file_position;
    }

    uncopied_byte_count = copy_to_user(dest_buffer,
        &device_data->buffer[*file_position], byte_count);
    if (uncopied_byte_count > 0) {
        return_code = -EFAULT;
    } else {
        return_code = byte_count;
    }

    *file_position += byte_count;
    pr_info("New file position: %lld\n", *file_position);

    pr_info("Successfully read byte count: %zu\n", byte_count);
    return return_code;
}



static ssize_t pseudo_char_device_write(struct file *file,
    const char __user *src_buffer, size_t byte_count, loff_t *file_position)
{
    ssize_t return_code = 0;
    unsigned uncopied_byte_count = 0;
    device_data_t *device_data = (device_data_t *)file->private_data;

    pr_info("Write operation requested for %zu bytes...\n", byte_count);
    pr_info("Current file position: %lld\n", *file_position);

    if ((*file_position + byte_count) > device_data->buffer_size) {
        byte_count = device_data->buffer_size - *file_position;
    }

    if (byte_count == 0) {
        pr_err("No available memory for write operation...\n");
        pr_err("Write operation failed!\n");
        return_code = -ENOMEM;
    } else {
        uncopied_byte_count = copy_from_user(
            &device_data->buffer[*file_position], src_buffer, byte_count);
        if (uncopied_byte_count > 0) {
            pr_err("Unable to copy %u bytes...\n", uncopied_byte_count);
            return_code = -EFAULT;
        } else if (uncopied_byte_count == 0) {
            return_code = byte_count;
        }

        *file_position += byte_count;
        pr_info("New file position: %lld\n", *file_position);
        pr_info("Successfully written byte count: %zu\n", byte_count);
    }

    return return_code;
}



static int pseudo_char_device_open(struct inode *inode, struct file *file)
{
    int return_code = 0;
    device_data_t *device_data = NULL;

    pr_info("Open operation requested on the device %d:%d...\n",
        MAJOR(inode->i_rdev), MINOR(inode->i_rdev));

    /* Get device's private data structure. */
    device_data = container_of(inode->i_cdev, device_data_t, cdev);

    /* Store address of device's private data structure for other file
       operations like llseek, read, write, etc. */
    file->private_data = device_data;

    return_code = check_permission(device_data->permission_type, file->f_mode);
    if (return_code == 0) {
        pr_info("Open operation done successfully.\n");
    } else {
        pr_err("Invalid permission type...\n");
        pr_err("Open operation failed!\n");
    }

    return return_code;
}



static int pseudo_char_device_release(struct inode *inode, struct file *file)
{
    pr_info("Release operation requested, but nothing to be done...\n");

    return 0;
}



/*****************************************************************************/
/* HELPER FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int check_permission(const permission_type_t device_permission,
    const int access_mode)
{
    int return_code = 0;

    switch (device_permission) {
        case PERMISSION_TYPE_READ: {
            if ((access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE)) {
                return_code = 0;
            } else {
                return_code = -EPERM;
            }
        }
        break;

        case PERMISSION_TYPE_WRITE: {
            if ((access_mode & FMODE_WRITE) && !(access_mode & FMODE_READ)) {
                return_code = 0;
            } else {
                return_code = -EPERM;
            }
        }
        break;

        case PERMISSION_TYPE_READ_WRITE: {
            return_code = 0;
        }
        break;

        default: {
            return_code = -EPERM;
        }
        break;
    }

    return return_code;
}



/*****************************************************************************/
/* MODULE REGISTRATION */
/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Standarski");
MODULE_DESCRIPTION("Pseudo character device driver for learning purposes.");

module_init(pseudo_char_device_init);
module_exit(pseudo_char_device_exit);

