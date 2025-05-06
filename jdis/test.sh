#!/bin/bash

EXEC="./jdis"
FILES=(test_files/toto*.txt)

CMD="$EXEC ${FILES[*]}"
echo "Lancement : $CMD"

$CMD
