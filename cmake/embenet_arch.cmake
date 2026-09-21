# SPDX-FileCopyrightText: 2026 Embetech sp. z o.o.
# SPDX-License-Identifier: Apache-2.0

# Picks which prebuilt library in libs/ to link, from the CPU and float ABI Kconfig already knows.
#
# Each bundle can also detect this itself, but its probe reads the compiler flags from
# CMAKE_C_FLAGS - and Zephyr puts -mcpu/-mfpu on the zephyr_interface target instead, so the probe
# would see the host defaults. Setting the override variables here keeps it from running.
include_guard(GLOBAL)

function (embenet_resolve_arch out_var)
  # The libraries use FPU calling conventions; a soft-ABI build would pass arguments in the wrong
  # place, silently.
  if (CONFIG_FPU AND NOT CONFIG_FP_HARDABI)
    message(FATAL_ERROR "embeNET requires the hard-float ABI (CONFIG_FP_HARDABI).")
  endif ()

  if (CONFIG_CPU_CORTEX_M4)
    if (CONFIG_FP_HARDABI)
      set(${out_var} thumbv7em-none-eabihf PARENT_SCOPE)
    else ()
      set(${out_var} thumbv7em-none-eabi PARENT_SCOPE)
    endif ()

  elseif (CONFIG_CPU_CORTEX_M33)
    # Only the hard-float variant of thumbv8m.main ships in the bundles.
    if (NOT CONFIG_FP_HARDABI)
      message(FATAL_ERROR "embeNET on Cortex-M33 requires CONFIG_FPU=y.")
    endif ()
    set(${out_var} thumbv8m.main-none-eabihf PARENT_SCOPE)

  else ()
    message(FATAL_ERROR "embeNET supports Cortex-M4 (nRF52) and Cortex-M33 (nRF54).")
  endif ()
endfunction ()

embenet_resolve_arch(EMBENET_ARCH)
message(STATUS "embeNET: target architecture ${EMBENET_ARCH}")

# Every bundle reads <COMPONENT>_ARCHITECTURE, so this list only has to grow when a new one is added.
foreach (embenet_component EMBENET_NODE EMBENET_BORDER_ROUTER logger ring_buffer crc)
  set(${embenet_component}_ARCHITECTURE ${EMBENET_ARCH})
endforeach ()
unset(embenet_component)
