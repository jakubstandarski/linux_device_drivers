# PSEUDO PLATFORM DEVICE DRIVER (BASED ON DEVICE TREE)



## Why Do We Use Device Tree

`Device tree` is a flexible and convenient way of adding new devices (either
pseudo or hardware) to the existing specification of a given target (e.g.
hardware specification of the BeagleBone Black board).

Programmer writes the `.dts` file, which is then converted to the
binary format `.dtb` (or `.dtbo` in case of overlays).
Eventually `.dtb` (and its overlays if needed) is loaded during boot time by
the bootloader.



## How To Update Existing Device Tree


### Approach #1 - modification of the main dts (not recommended)

In this approach, programmer creates additional `.dtsi` files, which can
define new modules/hardware or edit existing one. Then newly created `.dtsi`
file is included in the main `.dts` file and recompiled by invoking the
following command from the Linux source directory:
```sh
$ make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- am335x-boneblack.dtb
```


### Approach #2 - overlays (recommended)

Overlays are used to modify the existing `.dtb` and are loaded during boot time
by the bootloader just after main `.dtb` is loaded.

Overlays are simply `.dts` files with slightly different syntax, which is
described in Linux documentation. They are recognizable by their extension,
which is often `.dtbo` (after compilation of course).

To compile overlay, programmers has to use `dtc` (device tree compiler).
As an example, imagine we want to compile `am335x-boneblack-overlay.dts` to
create `am335x-boneblack-overlay.dtbo` file:
```sh
$ dtc -@ -I dts am335x-boneblack-overlay.dts -O dtb -o
  am335x-boneblack-overlay.dtbo
```

Next, the overlay has to be loaded together with the main `.dtb` file during
boot. Take a look at [uEnv.txt](./uEnv.txt) and `README.fdt-overlays` inside
`u-boot` source directory how to perform such a procedure.

