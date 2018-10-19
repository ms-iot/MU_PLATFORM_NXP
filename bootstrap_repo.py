## @file bootstrap_repo.py
# This module contains code that can locate and download the Core repo.
#
##
# Copyright (c) 2017, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
import os
import subprocess
import re

LOCAL_WORKSPACE_PATH = os.path.dirname(os.path.abspath(__file__))


# Simplified Comparison Function borrowed from StackOverflow...
# https://stackoverflow.com/questions/1714027/version-number-comparison
def version_compare(version1, version2):
    def normalize(v):
        return [int(x) for x in re.sub(r'(\.0+)*$','', v).split(".")]
    a = normalize(version1)
    b = normalize(version2)
    return (a > b) - (a < b)


def bootstrap():
  #
  # Make sure that we have Git installed and provide
  # a useful error if not.
  #
  print("## Checking prerequisites...")
  cmd = "git --version"
  # print("Command: %s" % cmd)
  try:
    c = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=LOCAL_WORKSPACE_PATH)
    c.wait()
    if c.returncode != 0:
      raise RuntimeError(c.stdout.read())
    git_version = c.stdout.read()

    # PYTHON COMPAT HACK
    if type(git_version) is bytes:
      git_version = git_version.decode()
    # PYTHON COMPAT HACK

  except:
    print("FAILED!\n")
    print("Cannot find required utilities! Please make sure all required tools are installed.\n")
    raise
  print("Done.\n")

  #
  # Check for a minimum version of Git.
  #
  min_git = "2.11.0"
  # This code is highly specific to the return value of "git version"...
  cur_git = ".".join(git_version.split(' ')[2].split(".")[:3])
  if version_compare(min_git, cur_git) > 0:
    raise RuntimeError("Please upgrade Git! Current version is %s. Minimum is %s." % (cur_git, min_git))

  #
  # Grab the core git repo.
  #
  print("## Grabbing required files...")
  cmd = "git submodule update --init --progress MU_BASECORE"
  # print("Command: %s" % cmd)
  try:
    c = subprocess.Popen(cmd, stderr=subprocess.STDOUT, cwd=LOCAL_WORKSPACE_PATH)
    c.wait()
    if c.returncode != 0:
      raise RuntimeError(c.stdout.read())
  except:
    print("FAILED!\n")
    print("Could not download MU_BASECORE! Maybe you have a network issue? Try updating/initializing git submodules manually.\n")
    raise
  print("Done.\n")


if __name__ == "__main__":
  # Finally, run the base bootstrap.
  print("### Running bare repo bootstrap...")
  bootstrap()
