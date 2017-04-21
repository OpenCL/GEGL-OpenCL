#!/usr/bin/env python
from __future__ import print_function

import os
import sys
import argparse
import subprocess

from glob import glob
from pprint import pprint

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
    self.src_dir   = os.path.realpath(os.path.join(os.path.dirname(__file__), "..", ".."))
    self.build_dir = self.src_dir

  def prep(self):
    self.fail_count = 0
    self.pass_count = 0
    self.skip_count = 0

    if sys.platform == "win32":
      exe_suffix = ".exe"
    else:
      exe_suffix = ""

    self.gegl_bin = os.path.join(self.build_dir, "bin", "gegl" + exe_suffix)
    self.img_cmp_bin = os.path.join(self.build_dir, "tools", "gegl-imgcmp" + exe_suffix)
    self.detect_opencl_bin = os.path.join(self.build_dir, "tools", "detect_opencl" + exe_suffix)

    for prog in [self.gegl_bin, self.img_cmp_bin, self.detect_opencl_bin]:
      if not os.path.isfile(prog):
        raise RuntimeError("Could not locate required file: %s" % prog)

    FNULL = open(os.devnull, 'w')

    test_env = os.environ.copy()
    test_env["GEGL_SWAP"] = "RAM"
    test_env["GEGL_PATH"] = os.path.join(self.build_dir, "operations")+os.pathsep+os.path.join(self.src_dir, "operations")
    self.opencl_available = not subprocess.call(self.detect_opencl_bin, stdout=FNULL, env=test_env)

    self.ref_dir = os.path.join(self.src_dir, "tests", "compositions", "reference")
    self.output_dir = os.path.join(self.build_dir, "tests", "compositions", "output")

    if not os.path.isdir(self.output_dir):
      os.makedirs(self.output_dir)

  def find_reference_image(self, test_name):
    images = glob(os.path.join(self.ref_dir, test_name + ".*"))
    if len(images) == 1:
      return os.path.basename(images[0])
    elif len(images) == 0:
      return test_name + ".png"
    else:
      raise RuntimeError("Ambiguous reference image for %s" % test_name)

  def run_xml_test(self, test_xml_filename, use_opencl):
    result_name_str = "%s" % os.path.basename(test_xml_filename)

    if use_opencl:
      result_name_str = "%s (OpenCL)" % os.path.basename(test_xml_filename)

    if use_opencl and not self.opencl_available:
      print(SKIP_STR, result_name_str)
      self.skip_count += 1
      return True

    test_env = os.environ.copy()

    if not use_opencl:
      test_env["GEGL_USE_OPENCL"] = "no"
    test_env["GEGL_SWAP"] = "RAM"
    test_env["GEGL_PATH"] = os.path.join(self.build_dir, "operations")+os.pathsep+os.path.join(self.src_dir, "operations")
    img_cmp_env = test_env.copy()
    img_cmp_env["GEGL_USE_OPENCL"] = "no"

    test_name = os.path.basename(test_xml_filename)
    if test_name.endswith(".xml"):
      test_name = test_name[:-4]
    else:
      print (FAIL_STR, "Bad test name: %s" % test_name)
      self.fail_count += 1
      return False

    out_image_name = self.find_reference_image(test_name)

    if use_opencl:
      out_image_path = os.path.join(self.output_dir, "opencl-" + out_image_name)
    else:
      out_image_path = os.path.join(self.output_dir, out_image_name)

    ref_image_path = os.path.join(self.ref_dir, out_image_name)

    if os.path.isfile(out_image_path):
      os.remove(out_image_path)

    xml_graph_path = os.path.realpath(test_xml_filename)

    try:
      if VERBOSE:
        print(" ".join([self.gegl_bin, xml_graph_path, "-o", out_image_path]))
      subprocess.check_call([self.gegl_bin, xml_graph_path, "-o", out_image_path], env=test_env)

      if VERBOSE:
        print(" ".join([self.img_cmp_bin, ref_image_path, out_image_path]))
      subprocess.check_call([self.img_cmp_bin, ref_image_path, out_image_path], env=img_cmp_env)
    except KeyboardInterrupt:
      raise
    except subprocess.CalledProcessError as e:
      if VERBOSE:
        print (e)
      print (FAIL_STR, result_name_str)
      self.fail_count += 1
      return False
    print (PASS_STR, result_name_str)
    self.pass_count += 1
    return True

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--without-opencl",
                      action="store_true",
                      help="disable OpenCL when running tests")
  parser.add_argument("--xml-dir",
                      help="path to the composition xml files")
  parser.add_argument("--build-dir",
                      help="path to the top build directory")
  parser.add_argument("--src-dir",
                      help="path to the top source directory")
  parser.add_argument("FILES",
                      nargs="*",
                      help="the composition xml files to run")

  args = parser.parse_args()

  context = Context()

  if args.src_dir:
    context.src_dir = os.path.realpath(args.src_dir)

  if args.build_dir:
    context.build_dir = os.path.realpath(args.build_dir)

  run_opencl_tests = not args.without_opencl

  tests = args.FILES

  if not tests:
    parser.print_help()
    sys.exit(0)

  context.prep()

  xml_dir = None
  if args.xml_dir:
    xml_dir = os.path.join(context.src_dir, "tests", "compositions")

  for testfile in tests:
    if xml_dir:
      testfile = os.path.join(xml_dir, testfile)

    context.run_xml_test(testfile, False)

    sys.stdout.flush() # Keep our ouput in sync with subprocess if redirected

    if run_opencl_tests:
      context.run_xml_test(testfile, True)

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