# FAT 文件系统的实现

FAT 文件系统的实现，分为了磁盘层，数据层，簇层，文件层，应用层。

## 磁盘层

使用设备驱动，如 `sdRead`、`sdWrite`，对某个设备块进行读写到块缓存中。块缓存记录了设备号和块号。当缓存满了以后，就进行写回。

## 数据层

数据层包含以下函数：

> `struct buf* bread(uint dev, uint blockno);`

表示从表示读某设备某块的缓存。如该块原本没有缓存，则先从设备上将该块读到缓存中。

文件系统 `fs` 通过 `fs.read()` 函数读到某块缓存。

一般情况下（没有 mount 磁盘镜像的情况下），有 `fs.read == bread`。

## 簇层

簇层的函数主要是操作 FAT 簇。包括以下函数：

> `uint rw_clus(FileSystem *fs, uint32 cluster, int write, int user, u64 data, uint off, uint n)`

读写某个簇的 `off` 偏移往后长度 `n` 的内容。如果 `write` 为真，则为写簇，否则为读簇。读写的内存地址是 `data`。如果 `user` 为 true 则为读写用户态，否则是读写内核态。该函数首先根据从超级块中读到的信息计算出第 `cluster` 个簇的块号。对每个需要操作的块，首先用 `fs->read` 读到块缓存。然后再根据具体情况对块缓存进行读写。

> `void free_clus(FileSystem *fs, uint32 cluster)`

将 FAT 表中簇 `cluster` 对应的 FAT 项写入 `content`。直接调用 `write_fat` 函数即可。

> `uint32 alloc_clus(FileSystem *fs, uint8 dev)`

分配设备 `dev` 上新的簇，并将该簇内容清空。

读 FAT 表，找到空闲项并将其设为末尾簇。

> `int write_fat(FileSystem *fs, uint32 cluster, uint32 content)`

将 `content` 写入 FAT 表中簇 `cluster` 对应的 FAT 项。

> `uint32 read_fat(FileSystem *fs, uint32 cluster)`

读 FAT 表中簇 `cluster` 对应的 FAT 项。

## 文件层

> `int reloc_clus(FileSystem *fs, struct dirent* entry, uint off, int alloc)`

将 `entry->cur_clus` 设为文件 `off` 偏移所在的簇。如果 `alloc` 为真，则当原文件不够大时可以分配新簇。

> `eread(struct dirent* entry, int user, u64 dst, uint off, uint n)`

将文件 `entry` 的 `off` 偏移往后长度为 `n` 的内容读到 `dst` 中。如果 `user` 为真，则为用户地址，否则为内核地址。

调用 `reloc_clus` 和 `rw_clus` 函数即可。

> `ewrite(struct dirent* entry, int user_src, u64 src, uint off, uint n)`

> `struct dirent* eget(struct dirent* parent, char* name)`

> `void emake(struct dirent* dp, struct dirent* ep, uint off)`

> `struct dirent* ealloc(struct dirent* dp, char* name, int attr)`

> `void eupdate(struct dirent* entry)`

> `void eremove(struct dirent* entry)`

> `void etrunc(struct dirent* entry)`

> `void eput(struct dirent* entry)`

> `void estat(struct dirent* de, struct stat* st)`

> `int enext(struct dirent* dp, struct dirent* ep, uint off, int* count)`

## 应用层

> `struct dirent* dirlookup(struct dirent* dp, char* filename, uint* poff)`

> `struct dirent* lookup_path(char* path, int parent, char* name)`

> `struct dirent* ename(char* path)`

> `struct dirent* enameparent(char* path, char* name)`

> `struct file* filealloc(void)`

> `void fileclose(struct file* f)`

> `int filestat(struct file* f, u64 addr)`

> `int fileread(struct file* f, u64 addr, int n)`

> `int filewrite(struct file* f, u64 addr, int n)`

> `int dirnext(struct file* f, u64 addr)`
