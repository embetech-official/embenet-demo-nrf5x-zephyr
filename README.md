# embeNET for Zephyr on nRF5x devices

This repo contains a Zephyr module integrating the [embeNET](https://embe.net) wireless mesh networking stack.

The current version of this module supports both the
**Node** and the **Border Router** roles — on Nordic nRF52 and nRF54 devices.

## Requirements

| | |
|---|---|
| nRF Connect SDK | **v3.4.0** (Zephyr 4.4, Zephyr SDK 1.0.1) |
| Toolchain | GCC 14+ — the border router library's headers require C++23 |

The Node role builds with older toolchains too, but the Border Router does not: its release bundle
declares `INTERFACE_COMPILE_FEATURES cxx_std_23`, so a GCC that does not know C++23 fails at
configure time.

## Structure

- **Ports** (`ports/`) are built from source in this repository.
- **Libraries** (`libs/`) are vendored release bundles: `embenet_node`, `embenet_border_router`,
  `embenet_udp`, plus the `logger`, `ring_buffer`, `crc` and `embeutils` support libraries. Each
  bundle carries static libraries for every supported architecture and the module picks the right
  one from Kconfig.

## Using the module

This repository is a Zephyr module, and can be pulled into a build in any of the usual ways:

```bash
west build -b nrf52840dk/nrf52840 my_app -- -DZEPHYR_EXTRA_MODULES=/path/to/this/repo
```

or add it to your own west manifest as a project — Zephyr discovers it through `zephyr/module.yml`.

Nothing else is needed in your `CMakeLists.txt`: the module publishes the stack's headers and
archives through `zephyr_interface`, so `#include <embenet/node.h>` and linking work with no
`find_package()` or `target_link_libraries()` of your own.

It's also a manifest repo in its own right — `west.yml` pins the matching NCS release — so a full
workspace with this module already in it comes up with one command:

```bash
west init -m https://github.com/embetech-official/embenet-suite-zephyr-evaluation --mr main
cd embenet-suite-zephyr-evaluation
west update
west build -b nrf52840dk/nrf52840 embenet/samples/node
```
## embeNET device roles

In embeNET all field devices act as so called network Nodes and thus utilize the embeNET Node component (see [documentation](https://embe.net/docs/?q=doxyview/embeNET%20Node)).

Gateways are built from two components:
 - embeNET Node acting as Root (see [documentation](https://embe.net/docs/?q=doxyview/embeNET%20Node))
 - embeNET BorderRouter acting as network orchestrator and router (see [documentation](https://embe.net/docs/?q=doxyview/embeNET%20BRC))


## Configuration

Pick the roles the image should carry — they are independent:

```conf
CONFIG_EMBENET=y
CONFIG_EMBENET_NODE=y             # enables embeNET Node component
CONFIG_EMBENET_BORDER_ROUTER=y    # enables embeNET BorderRouter component
```

The Border Router additionally needs one setting the module cannot select for itself, because
Kconfig ignores `select` on choice entries:

```conf
CONFIG_GLIBCXX_LIBCPP=y           # mandatory: the library uses std::pmr, std::error_category, exceptions
```

The build fails at configure time with an explanation if it is missing.

Both roles allocate from the system heap and declare how much they need through
`CONFIG_HEAP_MEM_POOL_ADD_SIZE_EMBENET_NODE` / `..._EMBENET_BORDER_ROUTER`, which Zephyr adds to
whatever else the image asks for — so there is no `CONFIG_HEAP_MEM_POOL_SIZE` to set by hand.


## Samples

| Sample | What it is |
|---|---|
| [`samples/node`](samples/node) | Simple app that acts as field device and joins embeNET network |
| [`samples/border_router_integrated`](samples/border_router_integrated) | Simple app that acts as a border router and accepts data from field devices |

To build a sample run:

```bash
west build -b nrf52840dk/nrf52840 samples/node
```


The samples sit inside the module, so each one adds it to `ZEPHYR_EXTRA_MODULES` from its own
`CMakeLists.txt` — no extra flag, and they build from any west workspace or IDE. Only applications
outside this repository need the manifest entry or `-DZEPHYR_EXTRA_MODULES` shown above.

`samples/node` ships overlays for nRF52840 DK, nRF52833 DK and nRF54L15 DK; other boards need only
the devicetree aliases above. `samples/border_router_integrated` needs no aliases — both roles talk
over an in-memory queue, not UART.

## Repository layout

```
zephyr/module.yml          module declaration
Kconfig                    roles, transport, per-role options
cmake/embenet_arch.cmake   Kconfig CPU/ABI -> architecture triplet for the bundles
cmake/embenet_libs.cmake   resolves libs/ and publishes them to the build
libs/                      vendored release bundles
ports/common/              queues shared by both ports, and the EXPECT abort handler
ports/node/                node port
ports/border_router/       border router port
samples/                   see above
tests/port_common/         host test for ports/common, runs on native_sim
```

## Tests

```bash
west twister --testsuite-root samples --build-only          # all samples, all supported boards
west twister --testsuite-root tests -p native_sim/native/64 # host tests
```

CI runs both on every push.
