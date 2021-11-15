/*****************************************************************************/
/* PLATFORM PUBLIC MACROS */
/*****************************************************************************/

#define PSEUDO_PLATFORM_DEVICE_COUNT 2

#define PSEUDO_PLATFORM_DEVICE_SERIAL_NUMBER_SIZE   10



/*****************************************************************************/
/* PLATFORM PUBLIC DATA STRUCTURES */
/*****************************************************************************/

struct pseudo_platform_device_platform_data {
    const char serial_number[PSEUDO_PLATFORM_DEVICE_SERIAL_NUMBER_SIZE];
    unsigned comms_baudrate;
};

