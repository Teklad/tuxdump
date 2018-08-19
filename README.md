A Linux offset dumper which, given a config file, can use the data signatures to dump offsets from memory in a human readable format for use in cheats.


A basic usage example:
```bash
tuxdump -c csgo.cfg $(pidof csgo_linux64)
Module                         Name                 Offset              
engine_client.so               dwClientState        0xdfdce8
client_panorama_client.so      dwEntityList         0x1ff26e8
client_panorama_client.so      dwForceAttack        0x2868384
client_panorama_client.so      dwGlowObject         0x285fa20
client_panorama_client.so      dwLocalPlayer        0x1fc35d0
client_panorama_client.so      dwPlayerResource     0x1fd9d30
```

