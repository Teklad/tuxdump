A Linux offset dumper which, given a config file, can use the data signatures to dump offsets from memory in a human readable format for use in cheats.


A basic usage example:
```bash
sudo tuxdump $(pidof csgo_linux64)
Module                         Name                 Offset              
engine_client.so               dwClientState        0xdfdce8
client_panorama_client.so      dwEntityList         0x1ff26e8
client_panorama_client.so      dwForceAttack        0x2868384
client_panorama_client.so      dwGlowObject         0x285fa20
client_panorama_client.so      dwLocalPlayer        0x1fc35d0
client_panorama_client.so      dwPlayerResource     0x1fd9d30
```


You can also dump netvars to a file with the **--dump-netvars** command line option, with an optional C++ format via **--dump-netvars=cpp** for convenient updated netvars without the need to manually transfer each individual netvar you need over:


```bash
sudo tuxdump --dump-netvars=cpp $(pidof csgo_linux64)
```


### Building the project
In order to build the project you'll need to use the following commands from the project directory:
```bash
mkdir build && cd build
cmake ..
make
```

### Running the project
If you run the project without any parameters you'll get the following output which lists the currently available flags:
```bash
TuxDump - The Linux offset dumper
Usage: tuxdump [options] <pid>
    --config=, -c <file>      Alternative configuration file to use
    --dump-netvars=, -d [style] Dumps netvars to a file.  Available styles: raw, cpp
    --help, -h                Show this message
```

