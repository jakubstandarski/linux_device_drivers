# Linux Device Drivers

- [How To Compile Kernel Module](#how-to-compile-kernel-module)
    - [Cross Compilation](#cross-compilation)
    - [Host PC Compilation](#host-pc-compilation)
    - [Compilation Cleanup](#compilation-cleanup)
    - [Makefile](#makefile)
- [How To Load And Unload Kernel Module](#how-to-load-and-unload-kernel-module)
    - [Module Loading](#module-loading)
    - [Module Unloading](#module-unloading)
    - [List Loaded Modules](#list-loaded-modules)
- [Init And Exit Function Overview](#init-and-exit-function-overview)
    - [Init Function](#init-function)
    - [Exit Function](#exit-function)
    - [Error Handling During Init](#error-handling-during-init)
- [Char Drivers](#char-drivers)
    - [Major And Minor Numbers](#major-and-minor-numbers)
    - [Device Numbers Allocation](#device-numbers-allocation)
    - [Device Numbers Cleanup](#device-numbers-cleanup)
    - [Char Device Registration](#char-device-registration)
    - [Char Device Removal](#char-device-removal)
- [Important Data Structures](#important-data-structures)
    - [File Operations](#file-operations)
    - [File](#file)
    - [Inode](#inode)



## How To Compile Kernel Module


### Cross Compilation

To compile kernel module for a target platform (e.g. BeagleBone Black) you
have to specify additional flags like `ARCH` and `CROSS_COMPILE`:

```sh
$ make ARCH=<processor architecture> CROSS_COMPILE=<compiler> -C
  <path to Linux kernel directory> M=<kernel module directory> modules
```

Example:
```sh
$ make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C
  ~/workspace/beaglebone_linux/ M=${PWD} modules
```


### Host PC Compilation

To compile kernel module for a host PC you don't have to specify any additional
flags like in case of cross compilation:

```sh
$ make -C <path to built Linux kernel source of your host PC>
  M=<kernel module directory> modules
```

Example:
```sh
$ make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
```


### Compilation Cleanup

To remove all the files built during module compilation process replace
`modules` with `clean`:

Cross compilation cleanup:
```sh
$ make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C
  ~/workspace/embedded_linux/beaglebone_linux/ M=${PWD} clean
```

Host PC compilation cleanup:
```sh
$ make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
```


### Makefile

The most important line inside `Makefile` is this one:
```sh
obj-m := <module name>.o
```
It states that there is one module to be built from the object file
`<module name>.o`. The result of this built will be the `<module name>.ko`.

If your module is generated from e.g. two source files (called `file1.o` and
`file2.o`) then the it should look like this:
```sh
obj-m := <module name>.o
module-objs := file1.o file2.o
```

If you want to generate two (or more) modules, then:
```sh
obj-m := <1st module name>.o <2nd module name>.o
```



## How To Load And Unload Kernel Module


### Module Loading

To load the module you have to use `insmod` command:
```sh
$ sudo insmod <module name>.ko
```


### Module Unloading

To unload the module you have to use `rmmod` command:
```sh
$ sudo rmmod <module name>.ko
```


### List Loaded Modules

To check which modules are currently loaded in the kernel use `lsmod` command:
```sh
$ lsmod
```
`lsmod` works by reading the `/proc/modules` virtual file. More information can
be found inside `/sys/module`.



## Init And Exit Function Overview


### Init Function

The basic initialization function should look like this:
```c
static int __init my_module_init(void)
{
    /* Initialization code. */
}

module_init(my_function_init);
```

The best practice is to make that function `static` since it's not meant to be
visible outside the specific file.

The `__init` token is a hint to the kernel that the given function is used only
at initialization time so it may be removed freeing up some memory.


### Exit Function

The basic exit (cleanup) function should look like this:
```c
static void __exit my_module_exit(void)
{
    /* Exit (cleanup) code. */
}

module_exit(my_module_exit);
```

As in the case of init function, the best practice is to make that function
`static`.

The `__exit` modifier marks code of that function as being for module unload
only. If your module is built directly into the kernel or the kernel is
configured to disallow the unloading of modules, then functions prepended with
`__exit` token are simply discarded.


### Error Handling During Init

If during initialization a particular error occurs, which doesn't allow to
load a module, you must undo any registration activites performed before the
failure.
If you don't do that, the kernel will be left in an unstable state, which may
cause many unexpected problems.

Unfortunately sometimes error recovery is best handled with the `goto`
statement, which you normally should avoid because it's the worst thing in
the entire programming world. Nevertheless it can eliminate a great deal of
complicated, highly-indented logic.

On the other hand, if your initialization and exit (cleanup) functions are more
complex than dealing with a few items, then `goto` approach may be replaced
with something more sophisticated.
It means that the initialization functions calls exit (cleanup) function from
itself if any errors occur.

**Remember that exit (cleanup) function can't be prepended with `__exit`
token then.**



## Char Drivers

A character (char) device is one that can be accessed as a stream of bytes
(like a file) and the char driver is in charge of implementing this behavior.
It implements system calls like `open`, `close`, `read`, `write` and more
depending on requirements.

Char devices are acessed through names in the filesystem, called:
- special files;
- device files;
- nodes of the filesystem tree.

Special files for char devices are located in the `/dev/` directory and are
identified by a `c` in the first column of the output of `ls -l` command, e.g.
```
crw-rw-rw-  1 root  tty     4,  64 Jul 4  1998 ttyS0
```


### Major And Minor Numbers

If you issue `ls -l /dev/` command, you will see many strings like this one:
```
crw-rw-rw-  1 root  tty     4,  64 Jul 4  1998 ttyS0
```
Such a string includes two numbers (separated by a comma) before the date of
the last modification (for most of other files, this place is occupied by the
file length information).

These numbers are the `major` and `minor` device number for the particular
device (in this case `major` is `4` and `minor` is `64`).

`Major` number identifies the driver associated with the device (in the above
mentioned case ttyS0 is managed by driver 4).

`Minor` number is used by the kernel to determine exactly which device is being
referred to. The kernel itself knows almost nothing about minor numbers beyond
the fact that they refer to device implemented by your driver.

`dev_t` type (defined in `<linux/types.h>`) is used to hold device numbers
(both the major and minor parts).
To obtain the major or minor parts of a `dev_t`, use (defined in
`<linux/kdev_t.h>`):
```c
MAJOR(dev_t dev);
MINOR(dev_t dev);
```


### Device Numbers Allocation

The first thing which should be done by your driver is to obtain device
numbers. There are two functions necessary for this task (declared in
`<linux/fs.h>`).

If you know ahead of time exactly which device numbers you want, then use:
```c
int register_chrdev_region(dev_t first, unsgined int count, char *name);
```

However, probably you will not know which major numbers your device will use,
so you should use:
```c
int alloc_chrdev_region(dev_t *dev, unsigned int firstminor,
    unsigned int count, char *name);
```
- `dev` is an output-only parameter that will hold the first number in your
allocated range.

- `firstminor` should be the first minor number to use (usually 0).

- `count` is the total number of contiguous device numbers you are requesting.

- `name` is the name fo the device that should be associated with this number
range (it will appear in `/proc/devices` file and sysfs).


### Device Numbers Cleanup

If allocated device numbers are no longer required (due to some init error or
module removal) they should be freed with:
```c
void unregister_chrdev_region(dev_t first, unsgined int count);
```


### Char Device Registration

To represent char devices, the kernel uses structures of type `struct cdev`
(defined in `<linux/cdev.h>`).

To initialize `cdev` structure, you should call:
```c
void cdev_init(struct cdev *cdev, struct file_operations *fops);
```
Regardless of calling that function you have to initialize one `struct cdev`
field manually, i.e. `cdev.owner` field with `THIS_MODULE`.

Once the `cdev` structure is set up, the final step is to tell the kernel about
it:
```c
int cdev_add(struct cdev *cdev, dev_t num, unsigned int count);
```
Where:
- `dev` is the `cdev` structure.

- `num` is the first device number to which this device responds.

- `count` is the number of device numbers that should be associated with the
device (often set to `1`).


### Char Device Removal

To remove a char device from the system, simply call:
```c
void cdev_del(struct cdev *dev);
```



## Important Data Structures


### File Operations

`struct file_operations` (defined in `<linux/fs.h>`) sets up connection
between device numbers and driver's operations. It is a collection of function
pointers. Each open file is associated with its own set of functions
(`file` structure includes `f_op` field that points to a `file_operations`
structure). Those driver's operations are mostly system calls, e.g. `open`,
`read`, `write` and so on.

The very basic set of file operations for random char device may look like
this:
```c
struct file_operations my_device_fops = {
    .owner = THIS_MODULE,
    .llseek = my_device_llseek,
    .read = my_device_read,
    .write = my_device_write,
    .ioctl = my_device_ioctl,
    .open = my_device_open,
    .release = my_device_release
};
```


### File

`struct file` (defined in `<linux/fs.h>`) is the second most important data
structure used in device drivers.
It represents an open file descriptor (it is not specific to device drivers;
every open file in the system has an associated `struct file` in kernel space).
It is created by the kernel on `open` and is passed to any function that
operates on the file. After all instances of the file are closed, the kernel
releases the data structure.

It is worth mentioning, that drivers never create `file` structures, they only
access structures created elsewhere.


### Inode

`struct inode` is used by the kernel internally to represent files, but it is
not the same as `struct file` - there can be numerous `file` structures
representing multiple open descriptors on a single file, but they all point to
a single `inode` structure.

`inode` structure provides great deal of information about the file, but in
case of device driver code only a few fields are worth of interest:
- `dev_t i_rdev` - actual device number;
- `struct cdev *i_cdev` - kernel's internal structure that represents char
devices.

Due to the fact that kernel changes "from time to time", a good practice is to
use predefined macros to obtain the major and minor number from an inode:
```c
unsigned int iminor(struct inode *inode);
unsigned int imajor(struct inode *inode);
```

