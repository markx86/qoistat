# qoistat

A command line utility to get statistics about the tags in a qoi image.

## USAGE

Run `qoistat` (or `qoistatbatch`) and pass in one or more images, to get information about 
how many tags and what tags have been used to encode the image(s).

Examples:

```bash
$ qoistat /path/to/myimage.qoi       # will print statistics for myimage.qoi
$ qoistat /path/to/folder/*.qoi      # will print statistics for every single qoi image in the folder
$ qoistatbatch /path/to/folder/*.qoi # will print statistics for all qoi images in the folder
```

The statistics are formatted in the following way:

```bash
# Example: qoistat ./qoi-logo-black.qoi
Statistics for QOI file 'qoi-logo-black.qoi'
  size:       115x52                  # omitted in qoistatbatch
  channels:   RGBA                    # omitted in qoistatbatch
  colorspace: sRGB with linear alpha  # omitted in qoistatbatch
  total tags: 1587 (100.00%)
  rgb tags:   0 (0.00%)
  rgba tags:  572 (36.04%)
  index tags: 597 (37.62%)
  diff tags:  0 (0.00%)
  luma tags:  0 (0.00%)
  run tags:   418 (26.34%)
  avg. run:   11 pixels
```

# COMPILING

`qoistat` uses only the standard C library. You (should) be able to compile this on Windows as well.

```
$ gcc -o qoistat qoistat.c
```

To use `qoistatbatch` rename (or copy) `qoistat` to `qoistatbatch`
