# ROSS
Robotic Oceanographic Surface Sampler "lower level" code repo

This code covers anything that is not "user-servicable", which includes all of the firmware for the Kayak, as well as some miscellaneous lower level scripts for the Raspberry Pi.

## Software

All "software" for this project (e.g. things run on ground stations) can be found in the appropriate BitBucket repo. Contact [Nick McComb](mccombn@oregonstate.edu) or [Jasmine Nahorniak](mailto:jasmine@coas.oregonstate.edu) for access/more information.

## Firmware

"Firware" as defined for this project, is anything that runs on a microcontroller (e.g. doesn't use an Operating System). This Repo contains code for the following sub-projects:

### On/Off Switch

The primary On/Off switch for the kayak.

This box also provices a thrice-redundant GPS logging mechanism, that is independent of all other systems (besides power).

### Power Distribution Board (PDB)

This primarialy handles power distribution inside of the electrioncs box. 

For more information on this PCB, visit its dedicated documentation page [here](http://nickmccomb.net/printed-circuit-boards/ross-pdb).

For more information on the firmware for the microcontroller on this PCB, visit the subdirectory [here](https://github.com/Nrpickle/ROSS/tree/master/PDB/PowerDistribution).

### Winch / EBox Auxillary Programming

For more information on this PCB, visit its dedicated documentation page [here](http://nickmccomb.net/printed-circuit-boards/ross-ebox-auxillary).

### XTend Daughterboard

For more information on this PCB, visit its dedicated documentation page [here](http://nickmccomb.net/printed-circuit-boards/xtend-daughterboard).


