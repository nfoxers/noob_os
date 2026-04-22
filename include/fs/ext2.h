#ifndef EXT2_H
#define EXT2_H

#include <dev/block_dev.h>
#include <stdint.h>

#define EXT2_SUPER_MAGIC 0xef53

struct ext2_superblock {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_siz;
  uint32_t s_log_frag;
  uint32_t s_blocks_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_mtime;
  uint32_t s_wtime;

  uint16_t s_mnt_count;
  uint16_t s_max_mnt_count;
  uint16_t s_magic;
  uint16_t s_state;
  uint16_t s_errors;
  uint16_t s_minor_rev_level;

  uint32_t s_lastcheck;
  uint32_t s_checkinterval;
  uint32_t s_creator_os;
  uint32_t s_rev_level;

  uint16_t s_def_resuid;
  uint16_t s_def_resgid;
  uint32_t s_first_ino;
  uint16_t s_inode_siz;
  uint16_t s_block_group_nr;

  uint32_t s_feature_compat;
  uint32_t s_feature_incompat;
  uint32_t s_feature_ro_compat;

  uint8_t s_uuid[16];
  char    s_volume_name[16];
  uint8_t s_last_mounted[64];

  uint32_t s_algo_bitmap;
  uint8_t  s_prealloc_blocks;
  uint8_t  s_prealloc_dir_blocks;
  uint16_t pad;
  uint8_t  s_journal_uuid[16];
  uint32_t s_journal_inum;
  uint32_t s_journal_dev;
  uint32_t s_last_orphan;

  uint32_t s_hash_seed[4];
  uint8_t  s_def_hash_version;
  uint8_t  pad3[3];

  uint32_t s_default_mount_ops;
  uint32_t s_first_meta_bg;
} __attribute__((packed));

struct ext2_blockgroup {
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table;
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs;
  uint16_t bg_pad;
  uint8_t  bg_res[12];
} __attribute__((packed));

struct ext2_inode {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size;
  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks;
  uint32_t i_flags;
  uint32_t i_osd1;
  uint32_t i_block[15];
  uint32_t i_generation;
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  uint32_t i_faddr;
  uint8_t  u_osd2[12];
} __attribute__((packed));

#define EXT2_FT_UNK    0
#define EXT2_FT_REG    1
#define EXT2_FT_DIR    2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO   5
#define EXT2_FT_SOCK   6
#define EXT2_FT_SYM    7

struct ext_direntry {
  uint32_t d_ino;
  uint16_t d_rec_len;
  uint8_t  d_name_len;
  uint8_t  d_file_type;
  char     name[];
} __attribute__((packed));

void ext2_init(struct gendisk *gd);

#endif