#!/bin/sh
on_ctrl_c() {
        echo "Ignoring Ctrl-C"
}
# Call on_ctrl_c() when the interrupt signal is received.
# The interrupt signal is sent when you press Ctrl-C.

trap  on_ctrl_c  INT

if [ "$#" -ne 1 ]; then
        echo "usage: ./myscript.sh <arg>"
        exit 1
fi

#make the named pipe
mkfifo mypipe-$$

#do the netcat job
cat mypipe-$$ | nc -l $1 | /home/jae/cs3157-pub/bin/mdb-lookup-cs3157 > mypipe-$$

#delete FIFO pipe
rm -f mypipe-$$