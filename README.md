# Make-analyze

A GNU make fork with added features for statically analyzing Makefile-based projects.

## How to build from source

### From a tarball

You can find the latest tarball in this repository at releases/make-4.2.92.tar.gz.
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
