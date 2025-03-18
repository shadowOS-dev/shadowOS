# shadowOS

A small educational operating system

## Building

### Basic disk

To build the basic OS just simply run:
```sh
make -j$(proc) # Use all cores available for speeeeeed.
```
*Final artifact(s): shadowOS.iso*

### Full disk

To build the full userspace of shadowOS (and the kernel) we first have to bootstrap the userspace, this is easly done using our bootstrap script. Like this:
```sh
./tools/bootstrap.sh
```
*Final artifact(s): distro-files/...*

Then to build the disk and kernel do:
```sh
make -j$(proc) # Use all cores available for speeeeeed.
```
*Final artifact(s): shadowOS.iso*


### License

The shadowOS kernel is licensed under the MIT license, see LICENSE for more information.