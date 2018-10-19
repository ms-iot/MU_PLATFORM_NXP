# Welcome to [Project MU](https://microsoft.github.io/mu/)

## &#x1F539; Copyright
Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## About

This document will walkthrough the layout of a "typical" project Mu repo using iMX8 as a sample platform.

[Repo Link](https://github.com/microsoft/mu)

## TL;DR

From the root: 

1. Run `git submodule update --init --recursive` to check out all the submodules.
2. Run `python {Platform}/{Device}/`[`PlatformBuild.py`](#platformbuildpy) `--setup` to set up the build environment. Includes fetching Nuget dependencies and synchronizing submodules.
3. Run `python {Platform}/{Device}/`[`PlatformBuild.py`](#platformbuildpy) to invoke the build process.

# MU Repos

## [NXP Platform](https://windowspartners.visualstudio.com/MSCoreUEFI/MSCoreUEFI%20Team/_git/NXP_iMX_Platform)

The platform repo is the root repository in which all the other submodules are located. This is also where code specific to the platform or devices would be located, including [`PlatformBuild.py`](#platformbuildpy) and [`PlatformBuildWorker.py`](#platformbuildworkerpy).

## [Silicon/NXP](https://windowspartners.visualstudio.com/MSCoreUEFI/MSCoreUEFI%20Team/_git/NXP_iMX_Silicon)

Silicon code provided from a manufacturer is kept in its own submodule so it can be maintained seperately from platform code.

## [MU_BASECORE](https://github.com/Microsoft/mu_basecore)

This is the main repo. Contains the guts of the build system as well as core UEFI code, forked from TianoCore.

## [Silicon/MU_TIANO_ARM](https://github.com/Microsoft/mu_silicon_arm_tiano)

Silicon code from TianoCore has been broken out into indivudal submodules. iMX8 is ARM, so we need this submodule. An Intel project would include [MU_TIANO_INTEL](https://github.com/Microsoft/mu_silicon_intel_tiano).

## [Common/MU_TIANO_PLUS](https://github.com/Microsoft/mu_tiano_plus )

Additional, optional libraries and tools forked from TianoCore.

## [Common/MU](https://github.com/Microsoft/mu_plus)

Additional, optional libraries and tools we've added to make MU great!

## [Common/MU_OEMSAMPLE](https://windowspartners.visualstudio.com/MSCoreUEFI/_git/mu_oemsample)

Starting point for setting up a UEFI Front Page.

# Build Environment

Part of Project Mu is a major improvement to the build environment in TianoCore

**LINK**

# Setup Info for iMX8

- In Windows:
    - `git clone https://windowspartners.visualstudio.com/MSCoreUEFI/MSCoreUEFI%20Team/_git/NXP_iMX_Platform`
    - Change into the directory you just cloned and run:
        - `git submodule update --init --recursive`
        - `python NXP/MCIMX8M_EVK_4GB/`[`PlatformBuild.py`](#platformbuildpy) `--setup`
- In WSL
    - `sudo apt-get install python2`
        - Python3 is not yet compatible with EDK2 in Linux
    - Download [Linaro AARCH 64 GCC 7.2.1](https://releases.linaro.org/components/toolchain/binaries/7.2-2017.11/aarch64-linux-gnu/)
        - Set GCC5_BIN to this path
            - export GCC5_BIN=/home/maknutse/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin/
    - Compile BaseTools:
        - `sudo apt-get install gcc`
        - `cd MU_BASECORE`
        - `make -C BaseTools`
        - `sudo apt-get remove gcc`
            - If I didn't uninstall GCC, I was having a hard time getting the build environment to use the right version of GCC
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
