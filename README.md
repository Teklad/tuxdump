# TuxDump
A Linux offset and CSGO netvar dumper using libconfig.

## Getting Started
The following instructions should help you get started using TuxDump.

### Prerequisites
* A working C++ compiler (C++11 or higher)
* cmake
* sudo (execution)

### Building
First you will need to clone the repository.  This can be done with the following command:
```
git clone https://github.com/Teklad/tuxdump.git
```

You will need to cd into the created directory and update the submodules:
```
cd tuxdump
git submodule update --init --recursive
```

All that is left now is to compile:
```
mkdir build && cd build
cmake ..
make
```

You should now have a working copy of TuxDump.

### Usage
Basic usage of tuxdump is as follows:
```
sudo ./tuxdump [options] [tool]
```

So if you wanted to output netvars in C++ format, you would run:
```
sudo ./tuxdump -fcpp netvars
```

The currently available tools are:
* classids
* netvars
* signatures

The currently available formats are:
* cpp
* java
* json

For an always up to date list of formats and tools, simply run:
```
sudo ./tuxdump -h
```

### Custom formatting
If you're needing some kind of formatted output that isn't already provided, there is preliminary support for this in the form of formats.cfg.  If you add a new format, please feel free to create a pull request so I can get it included into the master branch.

### Licensing
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details.

