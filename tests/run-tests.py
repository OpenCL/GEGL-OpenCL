#!/usr/bin/env python
from __future__ import print_function

import os
import sys
import argparse
import subprocess

from glob import glob
from pprint import pprint

if "SHELL" in os.environ:
  SHELL = os.environ["SHELL"]
else:
  SHELL = "sh"

if "PYTHON" in os.environ:
  PYTHON = os.environ["PYTHON"]
else:
  PYTHON = "python"

if sys.stdout.isatty() and "TERM" in os.environ:
  blue  = "\033[1;34m"
  green = "\033[1;32m"
  red   = "\033[1;31m"
  end   = "\033[0m"
  PASS_STR = green + "PASS" + end
  FAIL_STR = red + "FAIL" + end
  SKIP_STR = blue + "SKIP" + end
else:
  PASS_STR = "PASS"
  FAIL_STR = "FAIL"
  SKIP_STR = "SKIP"

VERBOSE = False
if ("V" in os.environ and os.environ["V"] != "0") or \
   ("VERBOSE" in os.environ and os.environ["VERBOSE"] != "0"):
  VERBOSE = True

class Context():
  def __init__(self):
    self.src_dir   = os.path.realpath(os.path.join(os.path.dirname(__file__), ".."))
    self.build_dir = self.src_dir

  def prep(self):
    self.fail_count = 0
    self.pass_count = 0
    self.skip_count = 0

  def run_exe_test(self, test_name):
    result_name_str = "%s" % os.path.basename(test_name)

    test_env = os.environ.copy()
    test_env["ABS_TOP_BUILDDIR"] = self.build_dir
    test_env["ABS_TOP_SRCDIR"] = self.src_dir
    test_env["GEGL_SWAP"] = "RAM"
    test_env["GEGL_PATH"] = os.path.join(self.build_dir, "operations")

    # When given a script Make will give us a path relative to the build directory, but the real
    # path should be relative to the source directory.
    if test_name.endswith(".sh"):
      test_path = os.path.realpath(os.path.join(self.src_dir, os.path.relpath(test_name, self.build_dir)))
      test_exe = [SHELL, test_path]
    elif test_name.endswith(".py"):
      test_path = os.path.realpath(os.path.join(self.src_dir, os.path.relpath(test_name, self.build_dir)))
      test_exe = [PYTHON, test_path]
    else:
      test_exe = ["./" + test_name]

    try:
      print(" ".join(test_exe))
      subprocess.check_call(test_exe, env=test_env)
    except KeyboardInterrupt:
      raise
    except subprocess.CalledProcessError as error:
      if (error.returncode == 77):
        # 77 is the magic automake skip value
        print (SKIP_STR, result_name_str)
        self.skip_count += 1
        return True
      print (FAIL_STR, result_name_str)
      self.fail_count += 1
      return False
    print (PASS_STR, result_name_str)
    self.pass_count += 1
    return True

def main():
  parser = argparse.ArgumentParser()
  # parser.add_argument("--without-opencl",
  #                     action="store_true",
  #                     help="disable OpenCL when running tests")
  parser.add_argument("--build-dir",
                      help="path to the top build directory")
  parser.add_argument("--src-dir",
                      help="path to the top source directory")
  parser.add_argument("FILES",
                      nargs="*",
                      help="the test programs to run")

  args = parser.parse_args()

  context = Context()

  if args.src_dir:
    context.src_dir = os.path.realpath(args.src_dir)

  if args.build_dir:
    context.build_dir = os.path.realpath(args.build_dir)

  tests = args.FILES

  if not tests:
    sys.exit(0)

  context.prep()

  for testfile in tests:
    context.run_exe_test(testfile)
    sys.stdout.flush() # Keep our ouput in sync with subprocess if redirected

  print ("=== Test Results ===")
  print (" tests passed:  %d" % context.pass_count)
  print (" tests skipped: %d" % context.skip_count)
  print (" tests failed:  %d" % context.fail_count)

  if context.fail_count == 0:
    print ("======  %s  ======" % PASS_STR)
    sys.exit(0)
  else:
    print ("======  %s  ======" % FAIL_STR)
    sys.exit(1)

main()