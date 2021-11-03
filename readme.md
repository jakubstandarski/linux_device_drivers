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

