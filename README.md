# GEGL-OpenCL
#### First official OPP (OpenCL Porting Project) of [OpenCL.org](http://opencl.org)

[GEGL](http://gegl.org/) (Generic Graphics Library) is a graph based image processing framework.

With [GEGL](http://gegl.org/) you chain together image processing operations represented by nodes into a graph. [GEGL](http://gegl.org/) provides such operations for loading and storing images, adjusting colors, filtering in different ways, transforming and compositing images.

[GEGL](http://gegl.org/) also depends on [BABL](http://www.gegl.org/babl/) to translate pixel formats. [BABL](http://www.gegl.org/babl/) allows converting between different methods of storing pixels known as pixel formats that have with different bitdepths and other data representations, color models and component permutations. A vocabulary to formulate new pixel formats from existing primitives is provided as well as the framework to add new color models and data types.

For a brief explanation on how GEGL works, read this [document](https://onedrive.live.com/redir?resid=B05FBF0EF1DAF9A4!7923&authkey=!ALQ1C8VwMFWrtwc&ithint=file%2cdocx)

</br>
####Index
1. [Installing](#installing)
2. [Development](#development)
3. [Contributing](#contributing)
4. [How to Port an Operation](#how-to-port-an-operation)
5. [How to Test Output](#how-to-test-output)
6. [How to Benchmark Performance](#how-to-benchmark-performance)


</br>
#### Installing

Read the document above for more information, or refer to the links below for the respective platforms

[On Windows](http://wiki.gimp.org/wiki/Hacking:Building/Windows)

[On Linux Platforms](http://wiki.gimp.org/wiki/Hacking:Building/Linux)

Ensure that [BABL](http://www.gegl.org/babl) is installed first before [GEGL](http://gegl.org).


</br>
#### Development

There already exists OpenCL integration into GEGL and some operations have already been ported to run on OpenCL. However, there is still a lot of operations that need porting as listed in the Google Sheet below.

[To-do List for GEGL-OpenCL](https://docs.google.com/spreadsheets/d/1gzgFiw-GVcLcENLLpPP0-LTQd_B4Zc1x4JCi95Girf8/edit?usp=drivesdk)

Likewise, we have a slack channel for discussions pertaining to development and issues here, [GEGL-OpenCL Slack](https://gegl-opencl.slack.com/)


</br>
#### Contributing

It is recommended that you fork this repository and create your branches there. After every discussion, if your kernel has the fastest speed for the vendor, you can create a pull request to have your changes merged. Please include the test results (correctness and timing), and name the pull request according to the operation you're working on and the vendor you're optimizing for, eg. box-blur_kernel_nv

Please ensure that *make clean* is executed before requesting a pull request

</br>
##### How to Port an Operation

+ Find an operation you'd like to work on under /operations/, eg. box-blur which can be found under /operations/common/
+ In box-blur.c, add the following line in the gegl_op_class_init function:
```c
static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;
  
  // insert this line below
  operation_class->opencl_support = TRUE;
  
  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:box-blur",
      "title",       _("Box Blur"),
      "categories",  "blur",
      "description", _("Blur resulting from averaging the colors of a square neighbourhood."),
      NULL);
}
```
+ Create a cl_process function (which contains the host-code implementation) with the following parameters in the same file
```c
static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result)
```
+ Add a function call in the main process function before the cpu implementation
```c
if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result))
      return TRUE;
```
+ Both kernel and kernel header file (.cl and .cl.h) should be stored in the /opencl/ folder.
+ Include both files, and any other necessary gegl-cl header in the operation's source code (eg. box-blur.c)
```c
#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"
#include "opencl/box-blur.cl.h"
```

</br>
##### How to Test Output
+ create an xml file (eg. box-blur.xml) containing the following code
```xml
<?xml version='1.0' encoding='UTF-8'?>
<gegl>
  <node operation='gegl:box-blur'>
    <params>
      <param name='radius'>25</param>
    </params>
  </node>
  <node operation='gegl:load'>
    <params>
      <param name='path'>../compositions/data/car-stack.png</param>
    </params>
  </node>
</gegl>
```
+ run the following shell command to generate an output
```sh
$ gegl box-blur.xml -o test.jpg GEGL_USE_OPENCL=yes
```
+ to check correctness of the image generated, run a command with OpenCL disabled first
```sh
$ gegl box-blur.xml -o test2.jpg GEGL_USE_OPENCL=no
```
+ and run the below to get a measure of correctness
```sh
$ gegl-imgcmp test2.jpg test.jpg
```
+ there is also a script in /tests/compositions that can be used to verify that the OpenCL operation works
```sh
$ cd tests/compositions
$ python run-compositions.py alien-map.xml
$ python run-compositions.py alien-map.xml --without-opencl
```

</br>
##### How to Benchmark Performance
+ create an xml file similar to the above in /tests/opencl
+ before running the shell script ensure that the perl module below is installed
```sh
$ cpan
cpan[1]> install "XML::Twig";
```
+ to run the benchmark script
```sh
$ cd tests/opencl
$ perl benchmark.pl box-blur.xml no 1
$ perl benchmark.pl box-blur.xml gpu 1
$ perl benchmark.pl box-blur.xml cpu 1
``` 
+ note that 'gpu'/'cpu' denotes that OpenCL is enabled, while 'no' denotes that OpenCL is disabled
+ the number at the end denotes the number of iterations to run, after which the average time for the operation is returned
