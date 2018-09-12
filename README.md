# TuxDump
A Linux offset and CSGO netvar dumper using libconfig.

## Getting Started
These instructions will give you a working copy of TuxDump to use on your system.

### Prerequisites
* A working C++ compiler (C++11 or higher)
* cmake
* libconfig
* sudo (execution)

### Building
First you will need to clone the repository.  This can be done with the following command:
```
git clone https://github.com/Teklad/tuxdump.git
```

You'll then want to cd into the created directory and create your build directory, Once completed cd into the build directory as well.:
```
cd tuxdump
mkdir build && cd build
```

From here you'll want to run cmake and build the project:
```
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

That's it, you're done with the build.

### Usage
The most basic usage example is this which will dump the offsets defined by signatures in csgo.cfg.  The target process name is also defined in this file.
```
sudo ./tuxdump
```


If you would also like a list of available netvars, this can be done with.
```
sudo ./tuxdump -d
```

For a list of available flags and options, try running tuxdump with the help flag, i.e.:
```
sudo ./tuxdump -h
```

### Licensing
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
