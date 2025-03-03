#!/bin/bash

# Define the source file
source_file="tests/6kwords.txt"

# Define the target files
small_file="small.txt"
mid_file="mid.txt"
big_file="big.txt"

# Create small.txt with the content of 6kwords.txt
cp "$source_file" "$small_file"

# Create mid.txt with the content of 6kwords.txt duplicated
cat "$source_file" "$source_file" > "$mid_file"

# Create big.txt with the content of 6kwords.txt tripled
cat "$source_file" "$source_file" "$source_file" > "$big_file"

touch empty.txt

./mkfs -f test.img -d 200 -i 200

./ds3bits test.img

echo -e "Creating file called dummy.txt in root"
./ds3touch test.img 0 dummy.txt
./ds3bits test.img
./ds3ls test.img /

echo "Creating folders........."
# Loop to create directories f1 to f26
for i in $(seq 1 128); do
  dirName="f$i"
  ./ds3mkdir test.img 0 "$dirName"
done
echo -e "done creating folders, remove..."

./ds3rm test.img 0 f5
./ds3rm test.img 0 f34
./ds3rm test.img 0 f110
./ds3rm test.img 0 f77


echo -e "Copying empty.txt into dummy.txt"
./ds3cp test.img empty.txt 1
./ds3bits test.img
./ds3cat test.img 1

# inode bitmap is fine, data bitmap does nto match...

echo -e "Copying mid.txt into dummy.txt"
./ds3cp test.img mid.txt 1
./ds3bits test.img
./ds3cat test.img 1

echo -e "Copying small.txt into dummy.txt"
./ds3cp test.img small.txt 1
./ds3bits test.img
./ds3cat test.img 1

echo -e "Copying big.txt into dummy.txt"
./ds3cp test.img big.txt 1
./ds3bits test.img
./ds3cat test.img 1

# Remove the created files
rm "$small_file" "$mid_file" "$big_file" empty.txt