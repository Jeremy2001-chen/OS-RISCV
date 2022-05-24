# FAT 文件系统的实现

FAT 文件系统的实现，分为了磁盘层，数据层，FAT 层，文件层，应用层。

## 磁盘层

使用设备驱动，如 `sdRead`、`sdWrite`，对某个设备块进行读写到块缓存中。块缓存记录了设备号和块号。当缓存满了以后，就进行写回。

## 数据层

数据层包含以下函数：

> `struct buf* bread(uint dev, uint blockno);`

表示从表示读某设备某块的缓存。如该块原本没有缓存，则先从设备上将该块读到缓存中。

文件系统 `fs` 通过 `fs.read()` 函数读到某块缓存。

一般情况下（没有 mount 磁盘镜像的情况下），有 `fs.read == bread`。

## FAT 簇层

FAT 簇层的函数主要是解析 FAT 簇结构。包括以下函数

> `uint rw_clus(FileSystem *fs, uint32 cluster, int write, int user, u64 data, uint off, uint n)`

> `void free_clus(FileSystem *fs, uint32 cluster)`

> `uint32 alloc_clus(FileSystem *fs, uint8 dev)`

> `int write_fat(FileSystem *fs, uint32 cluster, uint32 content)`

> `uint32 read_fat(FileSystem *fs, uint32 cluster)`

## 文件层

文件层

> `eread(struct dirent* entry, int user_dst, u64 dst, uint off, uint n)`

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
