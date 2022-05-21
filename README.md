# Minichat
This is a tiny chatting software based on fuse file system.
#### Environment Setting

Before using this chatting software, you need to configurate the environment refer to [libfuse]([libfuse/libfuse: The reference implementation of the Linux FUSE (Filesystem in Userspace) interface (github.com)](https://github.com/libfuse/libfuse))

#### Feature

* Instruction Supported

  ```
  mkdir rmdir ls cd echo touch cat
  ```

#### Using Tutorial

Compile

```
gcc -Wall minichat.c `pkg-config fuse3 --cflags --libs` -o minichat
```

Mount and Access

```
./minichat minichatroom
cd minichatroom
```

Adding new members Harry and Bob to the minichatroom

```
mkdir Harry
mkdir Bob
```

Chatting freely

```
echo "Hi, I'm Harry. I like fishing." >> Harry/Bob 
//Send message from Harry to Bob
echo "Hi, I'm Bob, I like fishing too." >> Bob/harry
//Send message from Bob to harry
```

Check the chatting records

```
cat Harry/Bob 
|--This is minichat room, talk freely!--|
[Harry]
Hi, I'm Harry. I like fishing.
[Bob]
Hi, I'm Bob, I like fishing too.

cat Bob/Harry
|--This is minichat room, talk freely!--|
[Harry]
Hi, I'm Harry. I like fishing.
[Bob]
Hi, I'm Bob, I like fishing too.
```

