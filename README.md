# WOL - **W**ake **O**n **L**an
> With this smart C program you can send wol magic packets in the network<br />
> *Version 0.01.07*

***

**Author**: Timo Furrer <tuxtimo@gmail.com><br />
**License:** GPL (See `LICENSE`)

## What ?
With this smart C program you can send wol magic packets to one or more MAC addresses in the network.

## How to use
    Usage: ./wol.c [-r remoteaddr] [-f filename1, ...|mac1, ...]

You can either pass some MAC addresses or you can pass with the option f some filenames contains a MAC address on each line to wake up.
With the option r you can specify the remote ip address. The default value is 255.255.255.255 for a broadcast call.

**Some examples:**

    $ ./wol 00:0B:CD:39:2D:E9 0E:FD:FA:33:5D:A6
    $ ./wol -r 192.168.1.36 00:0B:CD:39:2D:E9 0E:FD:FA:33:5D:A6
    $ ./wol -f macaddresses
    $ ./wol -r 192.168.1.36 -f macaddresses
    $ ./wol -f macaddresses macaddresses2

## MAC Address file syntax
The mac address syntax is very simple. It expects one mac address per line.
Lines starting with `#` are comment lines and will be ignored.

Valid Syntax:

    # Mac address of host 192.168.1.2
    00:0B:CD:39:2D:E9

    # Brothers mac address
    00:05:FE:AB:3D:99

    # Servers mac address
    00:32:D5:35:EF:63

## How to compile
To compile the wol source you just need the `gcc`.
Compile it with the following command:

    $ gcc -Wall -o wol src/wol.c

If you want to be able to execute this script from command line without to be in the right directory you can copy the compiled binary file to you `/usr/bin/` directory.
For this command you need *root-privileges*:

    # cp wol /usr/bin

Now if this worked fine and your `PATH` environment variable contains `/usr/bin/` you can execute this program from everywhere with:

    $ wol 00:0B:CD:39:2D:E9
