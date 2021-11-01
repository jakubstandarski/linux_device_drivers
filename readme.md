# Linux Device Drivers



## How To Compile Kernel Module

```sh
$ make ARCH=<processor architecture> CROSS_COMPILE=<compiler> -C
  <path to Linux kernel directory> M=<kernel module directory> modules
```

Example:
```sh
$ make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C
  ~/workspace/embedded_linux/beaglebone_linux/ M=${PWD} modules
```

`NOTE`: To delete compiled kernel module (i.e. all the files created during
compilation process) replace `modules` with `clean`, e.g.
```sh
$ make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- -C
  ~/workspace/embedded_linux/beaglebone_linux/ M=${PWD} clean
```
