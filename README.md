# Make-analyze

A GNU make fork with added features for statically analyzing Makefile-based projects. This is based on GNU make 4.2.92 and has all of the functionality provided by it. This is why the executable's name has not been changed.

## How to build from source

### From a tarball

You can find the latest tarball in this repository at releases/make-analyze-4.2.92.tar.gz. Or you can download it from http://debamitro.github.io/make-analyze/make-analyze-4.2.92.tar.gz.
Once you unpack it you should be able to follow the usual steps of

```bash
./configure
make
make install
```

### From this repository

Please follow the same instructions as GNU make, i.e., the ones in README.git. Note that paths to all the executables from the GNU gettext package should be in your PATH environment variable. Otherwise your build will fail even though the main executable will get created.

## Features

### --ctags-file=&lt;file-name&gt;

Write out a ctags file with make variable definition locations. If you open any Makefile with a ctags-enabled editor like Vi you should be able to look up the definition of any variable.

### --goaltree-file=&lt;file-name&gt;

Write out a json file with the current goals and their dependencies, recursively. This is work-in-progress. A PyQt5-based viewer is supplied at pyqt-viewer/visualize-goaltree.py which can show the data from this file.

### --goaltree-browser

Starting an interactve console-based browser to inspect the current goals and their dependencies.

### --goaltree-html-dir &lt;directory-name&gt;

Generate an HTML-based viewer for the current goals and their directories, inside a directory named directory-name.<current-process-id>. This is work in progress and uses https://github.com/debamitro/minimalist-tree-js for displaying the tree interactively.

### --detect-multiple-definition

Print out details about multiply-defined global variables. This is helpful for tracking down problems in large makefiles with a lot of variables.