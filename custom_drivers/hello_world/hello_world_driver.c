/*****************************************************************************/
/* HEADER FILES */
/*****************************************************************************/

#include <linux/module.h>



/*****************************************************************************/
/* MODULE INFO */
/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Standarski");
MODULE_DESCRIPTION("Custom driver only for learning purposes.");
MODULE_INFO(board, "BeagleBone Black REV C");



/*****************************************************************************/
/* PRIVATE FUNCTIONS PROTOTYPES */
/*****************************************************************************/

static int __init hello_world_driver_init(void);

static void __exit hello_world_driver_exit(void);



/*****************************************************************************/
/* PRIVATE FUNCTIONS DEFINITIONS */
/*****************************************************************************/

static int __init hello_world_driver_init(void)
{
    pr_info("Hello World!\n");
    return 0;
}



static void __exit hello_world_driver_exit(void)
{
    pr_info("Goodbye World!\n");
}



/*****************************************************************************/
/* MODULE REGISTRATION */
/*****************************************************************************/

module_init(hello_world_driver_init);
module_exit(hello_world_driver_exit);

