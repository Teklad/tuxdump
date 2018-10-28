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

You will need to cd into the created directory and create the build directory:
```
cd tuxdump
mkdir build && cd build
```

All that is left now is to compile:
```
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

### Licensing
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details.

