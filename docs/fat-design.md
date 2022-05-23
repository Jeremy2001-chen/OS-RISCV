# FAT32

FAT 全称 File Allocation Table，整个磁盘被分成若干簇。FAT 用于描述每个文件使用了哪些簇。

## 超级块

FAT32 文件系统的第一个磁盘块为超级块，包含文件系统的基本信息。具体信息见下表

| 字节数 | 内容 | 名称 |
| -- | -- | -- |
| 11-12 | 每个块的大小 | `byts_per_sec` |
| 13 | 每个簇包含的块数 | `sec_per_clus` |
| 14-15 | 保留块的数量 | `rsvd_sec_cnt` |
| 16 | FAT 表的复制份数 | `fat_cnt` |
| 28 | 隐藏块数量 | `hidd_sec` |
| 32-35 | 文件系统总块数 | `tot_sec` |
| 36-39 | 每个 FAT 的块数 | `fat_sz` |
| 44-47 | 根目录的第一个簇 | `root_clus` 
| 82-89 | 字符串 `"FAT32   "` | 无 |

通过这些读出的数据，可以算出：
- 第一个数据块的块号 `first_data_sec = rsvd_sec_cnt + fat_cnt * fat_sz`
- 数据块总数 `data_sec_cnt = tot_sec - first_data_sec`
- 数据簇总数 `data_clus_cnt = data_sec_cnt / sec_per_clus`
- 每个簇字节数 `byts_per_clus = sec_per_clus * byts_per_sec`

## 文件

每个文件的数据是一个簇链表。链表头存在该文件所在的目录，链表指针存在 FAT 表中。

## FAT 表

FAT 表由一系列连续的 sector 组成。每个 FAT 项为 32 位，其中只有低 28 位有效。前两个FAT 项不能用于分配。

对于第 $i$ 个 FAT 项
- 如果 `FAT[i] == FAT_ENTRY_FREE`，则第 $i$ 个簇是可分配的
- 如果 `FAT[i] >= FAT_ENTRY_RESERVED_TO_END`，则第 $i$ 个簇是被使用的，且是当前文件的最后一个簇
- 如果 `FAT[i] == FAT_ENTRY_DEFECTIVE_CLUSTER`，则第 $i$ 个簇被保护，不能分配
- 否则，这个簇被使用，`FAT[i]` 为第 $i$ 个簇所在文件的下一个簇的编号。

## FAT 目录

fat 的目录相当于一系列长度为 32 字节的文件。根目录的簇存储在超级块中，即上文的 `root_clus`。根目录没有 `.` 和 `..`，没有文件名，没有日期等。根目录有 ATTR_VOLUME_ID 权限位。

目录的每个块包含了若干目录项，每个目录项大小为 32 字节。目录项的第一个字节表明这个项是
- 未使用，应当跳过
- 未使用，表示目录结尾
- 已使用

每个目录项，要么是短文件名目录项，要么是长文件名目录项。

### 短文件名目录项

短文件名目录项长度为 32 字节，具体包含以下信息：
| 字节数 | 内容 | 名称 |
| -- | -- | -- |
| 0-10 | 短文件名 | `name` |
| 11 | 文件特性 | `attr` |
| 12 | 保留 | `_nt_res` |
| 13 | 创建时间，百分之一秒 | `_crt_time_tenth` |
| 14-15 | 创建时间 | `_crt_time` |
| 16-17 | 创建日期 | `_crt_date` |
| 18-19 | 上次访问日期 | `_lst_acce_date` |
| 20-21 | 第一个簇的较高两字节 | `fst_clus_hi` |
| 22-23 | 上次修改时间 | `_lst_wrt_time` |
| 24-25 | 上次修改日期 | `_lst_wrt_date` |
| 26-27 | 第一个簇的较低两字节 | `fst_clus_lo` |
| 28-31 | 文件大小| `file_size` |

其中，`name[0]` 如果等于 `DIR_ENTRY_UNUSED`，则表示该项应当跳过；如果等于 `DIR_ENTRY_LAST_AND_UNUSED`，则表示该项是最后一个目录项。

### 长文件名目录项

长文件名目录项同样长度为 32 字节，具体包含以下信息：
| 字节数 | 内容 | 名称 |
| -- | -- | -- |
| 0 | | `order` |
| 1-10 | 长文件名第一部分 | `name1` |
| 11 | 文件特性 | `attr` |
| 12 | | `_type` |
| 13 || `checksum` |
| 14-25 | 长文件名第二部分 | `name2` |
| 26-27 || `_fst_clus_lo` |
| 28-31 | 长文件名第三部分 | `name3` |



## 参考链接

[FAT 介绍](https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html)