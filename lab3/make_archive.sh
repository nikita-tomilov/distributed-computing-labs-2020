#!/bin/bash
mkdir -p ./for_bot/pa7
cp ./pa7/*.h ./for_bot/pa7/
cp ./pa7/*.c ./for_bot/pa7/
cd for_bot
tar -cvzf pa7.tar.gz pa7
rm -rf pa7
