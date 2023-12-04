#!bin/bash
make node
python3 -u gentx.py 0.5 | ./node $NODE_ID $NODE_PORT $NODE_CONFIG_PATH