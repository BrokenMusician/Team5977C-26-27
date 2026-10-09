# Team5977C-26-27
The official vex code for Team 5977C!

## THE GOAT 
![Logo](hq720.jpg)

## V5 Doom

The Doom port is included in [`VexV5Doom`](VexV5Doom/README.md) as a separate PROS project. It uses a different runtime from the VEXcode robot dashboard in `CLABKERDEPORTATION`, so the two programs must be built and uploaded separately. The dashboard's DOOM tab points to the port and shows its setup requirements.

From the `VexV5Doom` folder, build and upload with the PROS CLI:

```sh
prosv5 make
prosv5 upload
```

For game data, format a microSD card as FAT32 with an MBR partition table, copy `doom1.wad` onto it, then insert it into the Brain. The port does not support sound or multiplayer.
