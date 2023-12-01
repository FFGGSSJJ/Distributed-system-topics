#!/bin/bash
make node
python3 -u generator.py 1 100 | ./node $NODE logger 1234