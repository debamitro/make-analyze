# Make-analyze

A GNU make fork with added features for statically analyzing Makefile-based projects.

## How to build from source

Please follow the same instructions as GNU make, i.e., the ones in README.git

## Features

### --ctags-file=&lt;file-name&gt;

Write out a ctags file with make variable definition locations. If you open any Makefile with a ctags-enabled editor like Vi you should be able to look up the definition of any variable.
