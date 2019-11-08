# Welcome to [Project MU](https://microsoft.github.io/mu/)

## &#x1F539; Copyright
Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## IoT Core NXP BSP Repository Support Notice

This repository is no longer supported and is now read-only. The BSP and documentation for the iMX6, iMX7 and iMX8 processors for Windows 10 IoT is now published by NXP [here](https://www.nxp.com/design/software/embedded-software/windows-10-iot-core-for-i.mx-applications-processors:IMXWIN10IOT). Support for the BSP is available in NXP's [community forums](https://community.nxp.com/community/imx/content?filterID=contentstatus%5Bpublished%5D%7Ecategory%5Bwindows%5D).

The following repositories have been archived and made read-only on GitHub. If you have a fork of any of these repositories they will remain active, but no new updates will be supplied by Microsoft. Development has stopped in these repositories and all future work will be released by NXP in their BSP.

The repositories to be archived are:
https://github.com/ms-iot/imx-iotcore
https://github.com/ms-iot/u-boot
https://github.com/ms-iot/edk2
https://github.com/ms-iot/imx-edk2-platforms
https://github.com/ms-iot/mu_platform_nxp
https://github.com/ms-iot/mu_silicon_nxp

The NXP BSP only has board support for NXP EVK and Sabre devices. For other previously supported devices, contact the manufacturer for further support.

All trusted application binaries in this repository have been removed. The sources in this repository can still be built, but to generate a usable image, the trusted application firmware must be manually built and added according to the instructions on the NXP website.

## About

This document will walkthrough the layout of a "typical" project Mu repo using iMX8 as a sample platform.

[Repo Link](https://github.com/microsoft/mu)

## TL;DR

From the root: 

1. Run `git submodule update --init --recursive` to check out all the submodules.
2. Run `python {Platform}/{Device}/PlatformBuild.py --setup` to set up the build environment. Includes fetching Nuget dependencies and synchronizing submodules.
3. Run `python {Platform}/{Device}/PlatformBuild.py` to invoke the build process.

# MU Repos

## NXP Platform

This is platform repo, which is the root repository in which all the other submodules are located. This is also where code specific to the platform or devices would be located, including `PlatformBuild.py` and `PlatformBuildWorker.py`.

## [Silicon/ARM/NXP](https://github.com/ms-iot/MU_SILICON_NXP.git)

Silicon code provided from a manufacturer is kept in its own submodule so it can be maintained seperately from platform code.

## [MU_BASECORE](https://github.com/Microsoft/mu_basecore)

This is the main repo. Contains the guts of the build system as well as core UEFI code, forked from TianoCore.

## [Silicon/ARM/MU_TIANO](https://github.com/Microsoft/mu_silicon_arm_tiano)

Silicon code from TianoCore has been broken out into indivudal submodules. iMX8 is ARM, so we need this submodule. An Intel project would include [Intel/MU_TIANO](https://github.com/Microsoft/mu_silicon_intel_tiano).

## [Common/MU_TIANO](https://github.com/Microsoft/mu_tiano_plus )

Additional, optional libraries and tools forked from TianoCore.

## [Common/MU](https://github.com/Microsoft/mu_plus)

Additional, optional libraries and tools we've added to make MU great!

# Build Environment

Part of Project Mu is a major improvement to the build environment in TianoCore

**ADD LINK**

# Setup Info for iMX8

- In Windows:
    - `git clone https://windowspartners.visualstudio.com/MSCoreUEFI/MSCoreUEFI%20Team/_git/NXP_iMX_Platform`
    - Change into the directory you just cloned and run:
        - `git submodule update --init --recursive`
        - `python NXP/MCIMX8M_EVK_4GB/ PlatformBuild.py --setup`
- In WSL
    - `sudo apt-get install gcc g++ make python`
        - Python3 is not yet compatible with EDK2 in Linux
    - Download [Linaro AARCH 64 GCC 7.2.1](https://releases.linaro.org/components/toolchain/binaries/7.2-2017.11/aarch64-linux-gnu/)
        - Set GCC5_BIN to this path
            - export GCC5_BIN=/home/maknutse/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/
    - Compile BaseTools:
        - `cd MU_BASECORE`
        - `make -C BaseTools`
            - Note: sometimes I have to `sudo apt-get remove gcc` in order for the build environment to use the right version of GCC to build
    -  `rm -r Build; rm -r Conf; python NXP/MCIMX8M_EVK_4GB/PlatformBuild.py TOOL_CHAIN_TAG=GCC5 BUILDREPORTING=TRUE BUILDREPORT_TYPES="PCD"`
        - Removing Build and Conf directories ensures that you aren't accidentally keeping around configuration files and autogen build files that need to be updated.
        - Specify the `TOOL_CHAIN_TAG` either here or in `NXP/Conf/Target.template.ms`
        - Build reports are placed in the `Build` folder on a successful build and contain a complete list of every module that was built and what the PCDs were set to.
- Serial output
    - [User Guide NXP](https://www.nxp.com/docs/en/user-guide/IMX8MQUADEVKQSG.pdf)
    - [In Depth Guide NXP](https://www.nxp.com/support/developer-resources/software-development-tools/i.mx-developer-resources/evaluation-kit-for-the-i.mx-8m-applications-processor:MCIMX8M-EVK?tab=In-Depth_Tab)
    - The [drivers](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers) that actually worked for me
    - Serial configuration
        - Speed = 921600
        - Data bits = 8
        - Stop bits = 1
        - Parity = None
        - Flow Control = XON/XOFF
