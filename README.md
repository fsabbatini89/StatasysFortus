# StatasysFortus
This is a man-in-the-middle technique for Statasys Fortus machine

Basically you put a real cartdrige on each bay with full filament on it.
The PCB of the bay is connected to this project board input and output is connected to the original
fortus cable.
This PCB captures the "WRITE ON CARTDRIGE" commands from the main cpu to the bay pcb and replies accordingly
but that message never gets to the bay pcb so the chip is never ever written so after a machine reset, cartdrige is
full again.

Cartridge cache file of machine as to be erased so the serial number of cartrige is forggoten at startup so machine thinks
new full filament is set.

Happy printing!