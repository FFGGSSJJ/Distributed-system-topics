#!/bin/bash
PORT=1234
# RA=(0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0)
# RA=(1.0)
# make all

# for i in ${RA[@]}
# do
    # LOGGER_PID=$!
(python3 -u generator.py 1 100 | ./node A 127.0.0.1 $PORT) &
(python3 -u generator.py 1 100 | ./node B 127.0.0.1 $PORT) &
(python3 -u generator.py 2 100 | ./node C 127.0.0.1 $PORT) &
wait
# done