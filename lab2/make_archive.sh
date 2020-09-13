#!/bin/bash
mkdir -p ./for_bot/pa6
cp ./pa6/*.h ./for_bot/pa6/
cp ./pa6/*.c ./for_bot/pa6/
cd for_bot
tar -cvzf pa6.tar.gz pa6
rm -rf pa6
