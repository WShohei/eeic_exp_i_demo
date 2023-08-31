rec --buffer 192 -t raw -b 16 -c 1 -e s -r 3000 - | ./build/multiclient $1 $2 $3 | play --buffer 192  -t raw -b 16 -c 1 -e s -r 3000 -
