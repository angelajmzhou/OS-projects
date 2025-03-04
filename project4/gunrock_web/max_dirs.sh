#!/bin/bash

# Hard coded disk image file name
diskImageFile="test.img"

./mkfs -f "$diskImageFile" -d 200 -i 200

./ds3bits "$diskImageFile"

echo -e ">>> Creating f1 to f126"

# Loop to create directories f1 to f126
for i in $(seq 1 126); do
  dirName="f$i"
  ./ds3mkdir "$diskImageFile" 0 "$dirName"
done

./ds3bits "$diskImageFile"

echo -e ">>> Creating f127 and f128"

./ds3mkdir "$diskImageFile" 0 f127
./ds3mkdir "$diskImageFile" 0 f128

./ds3bits "$diskImageFile"

echo -e ">>> Removing f127 and f128"

./ds3rm "$diskImageFile" 0 f127

echo -e ">>> After removing f127"

./ds3bits "$diskImageFile"

./ds3rm "$diskImageFile" 0 f128

./ds3bits "$diskImageFile"
