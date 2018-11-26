##
## Script to Build iMX8 UEFI firmware
##
##
## Copyright Microsoft Corporation, 2015
##

import os, sys
import stat
from optparse import OptionParser
import logging
import subprocess
import shutil
import struct
from datetime import datetime
from datetime import date
import time
from MuEnvironment.UefiBuild import UefiBuilder

#--------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.
#
class PlatformBuilder(UefiBuilder):

    def __init__(self, WorkSpace, PackagesPath, PInManager, PInHelper, args):
        super(PlatformBuilder, self).__init__(WorkSpace, PackagesPath, PInManager, PInHelper, args)

    def SetPlatformEnv(self):
        logging.debug("PlatformBuilder SetPlatformEnv")

        self.env.SetValue("CONF_TEMPLATE_DIR", "NXP", "Conf template directory hardcoded - temporary and should go away")

        self.env.SetValue("BLD_*_SHIP_MODE", "FALSE", "PROFILE VALUE")
        self.env.SetValue("ACTIVE_PLATFORM", "NXP/MCIMX8M_EVK_4GB/MCIMX8M_EVK_4GB.dsc", "Platform Hardcoded")
        self.env.SetValue("PRODUCT_NAME", "MCIMX8M_EVK_4GB", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", "AARCH64", "Platform Hardcoded")
        self.env.SetValue("TOOL_CHAIN_TAG", "VSLATESTx86", "Platform Hardcoded")
        self.env.SetValue("BLD_*_DEBUG_MSG_ENABLE", "TRUE", "flag to enable debug message")
        self.env.SetValue("BLD_RELEASE_PERF_TRACE_ENABLE", "FALSE", "perf tracing off by default")
        self.env.SetValue("BLD_DEBUG_PERF_TRACE_ENABLE", "FALSE", "perf tracing off by default")
        self.env.SetValue("BLD_*_BUILD_UNIT_TESTS", "FALSE", "Unit Test build off by default")
        self.env.SetValue("BLD_*_BUILD_APPS", "FALSE", "App Build off by default")
        self.env.SetValue("BLD_*_MS_CHANGE_ACTIVE", "TRUE", "Turn on MS CHANGE SUPPORT in Core")
        self.env.SetValue("LaunchBuildLogProgram", "Notepad", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "False", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnError", "False", "default - will fail if already set", True)
        self.env.SetValue("BLD_*_BUILDID_STRING", "1.1.1", "Computed during Build Process")

        # Without this, GCC with use non-ascii quotes
        os.environ["LC_CTYPE"] = "C"

        return 0

    def SetPlatformEnvAfterTarget(self):
        logging.debug("PlatformBuilder SetPlatformEnvAfterTarget")

        value = os.path.join(self.env.GetValue("BUILD_OUT_TEMP"), self.env.GetValue("TOOL_CHAIN_TAG"))
        self.env.SetValue("BUILD_OUTPUT_BASE", value, "Computed in HUMMINGBOARD_EDGE_IMX6Q_2GB Build")

        # Output FD file
        key = "OUTPUT_FD"
        value = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "FV", "ClientBios.fd")
        self.env.SetValue(key, value, "Computed in HUMMINGBOARD_EDGE_IMX6Q_2GB Build")

        # Output ROM directory
        key = "OUTPUT_ROM_DIR"
        value = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "ROM")
        self.env.SetValue(key, value, "Computed in HUMMINGBOARD_EDGE_IMX6Q_2GB Build")

        # Output FV file
        key = "OUTPUT_FV"
        value = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "FV", "FV_UNCOMPRESSED.Fv")
        self.env.SetValue(key, value, "Computed in HUMMINGBOARD_EDGE_IMX6Q_2GB Build")


        # Capsule
        self.env.SetValue("BUILDCAPSULE", "FALSE", "default to not build capsule")
        self.env.SetValue("TRANSBUILDCAPSULE", "FALSE", "default to not build transition capsule")

        return 0



    def PlatformPostBuild(self):
        return 0


    #------------------------------------------------------------------
    #
    # Method to do stuff pre build.
    # This is part of the build flow.
    # Currently do nothing.
    #
    #------------------------------------------------------------------
    def PlatformPreBuild(self):
        return 0

    #------------------------------------------------------------------
    #
    # Method for the platform to check if a gated build is needed
    # This is part of the build flow.
    # return:
    #  True -  Gated build is needed (default)
    #  False - Gated build is not needed for this platform
    #------------------------------------------------------------------
    def PlatformGatedBuildShouldHappen(self):
        return False

    def ComputeVersionValue(self):
        BuildType = self.env.GetValue("TARGET")
        BuildId = self.env.GetBuildValue("BUILDID")
        if(BuildId == None):
            # Get milestone
            lMilestone = self.env.GetValue("BUILD_MILESTONE")
            if(lMilestone == None):
                lMilestone = gMilestone

            # Get build type
            if(BuildType.upper().strip() == "DEBUG"):
                bt = BuildTypeIdBase + BuildTypeIdDebug
            elif(BuildType.upper().strip() == "RELEASE"):
                bt = BuildTypeIdBase + BuildTypeIdRelease
            else:
                bt = BuildTypeIdBase + BuildTypeIdOther

            # Call library to compute the version
            (ec, buildint) = self.CommonBuild.ComputeVersionInt(gProductId, lMilestone, bt)
            if(ec != 0):
                logging.debug("Failed BuildId computation")
                return ec

            #set build id value
            self.env.SetValue("BLD_*_BUILDID", str(buildint), "BuildId Computed")
        return 0
        # PALINDROME CHANGE


    def ValidateVersionValue(self):
        #
        # Validate the version value (32 bit encoded value)
        # Convert to string following the rules of the platform version string format (platform specific)
        # Print out the info.
        #

        #
        # This functions logic follows Eric Lee's defined Build ID definition
        # used on Project H, C, P.  To change product/milestone NO logic changes are needed as
        # they are set in global vars at the top of this file and  date is computed on the fly.
        #
        BuildId = self.env.GetBuildValue("BUILDID")
        if(BuildId == None):
            logging.debug("Failed to find a BuildId")
            return -1
        #Use KBL common build code to do version validation.
        (ec, ver, mil) = self.CommonBuild.ValidateVersionValue(BuildId, gProductId)
        if(ec != 0):
            logging.debug("Failed BuildId validation")
            return ec

        self.env.SetValue("BLD_*_BUILDID_STRING", ver, "Computed during Build Process")
        self.env.SetValue("BUILD_MILESTONE", str(mil), "Setting milestone based on string decode.  Might get rejected if already set which is fine.")

        if str(mil) != self.env.GetValue("BUILD_MILESTONE"):
            logging.critical("Milestone requested by build doesn't match milestone encoded in build id")
            logging.debug("Milestone from build id: %s" % str(mil))
            logging.debug("Milestone requested: %s" % self.env.GetValue("BUILD_MILESTONE"))
            return -1

        return 0

    #
    # Platform defined flash method
    #
    def PlatformFlashImage(self):
        raise Exception("Flashing not supported")
