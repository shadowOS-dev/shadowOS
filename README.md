# shadowOS

An educational operating system.

## Building

### Building the Basic Disk

To build the basic version of shadowOS, simply run the following command:
```sh
make -j$(nproc)  # Use all available CPU cores for faster compilation.
```
*Output artifact(s): `shadowOS.iso`*

### Building the Full Disk

To build the full userspace and kernel of shadowOS, we first need to bootstrap the userspace. This can be done easily using the provided bootstrap script:
```sh
./tools/bootstrap.sh
```
*Output artifact(s): `distro-files/...`*

Once the userspace is bootstrapped, you can build the complete disk and kernel by running:
```sh
make -j$(nproc)  # Use all available CPU cores for faster compilation.
```
*Output artifact(s): `shadowOS.iso`*

## License

The shadowOS kernel is licensed under the MIT License. See the `LICENSE` file for more information.
