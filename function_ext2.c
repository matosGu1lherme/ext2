#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ext2_fs.h"

#define BASE_OFFSET 1024                   /* locates beginning of the super block (first group) */
#define FD_DEVICE "myext2image.img"              /* the floppy disk device */
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*block_size)

unsigned int ROOT_DIRECTORY = 2;
unsigned int CURRENT_DIRECTORY = 2;

static unsigned int block_size = 0;        /* block size (to be calculated) */

static void read_dir(int, const struct ext2_inode*, const struct ext2_group_desc*);
static void read_inode(int, int, const struct ext2_group_desc*, struct ext2_inode*);


struct ext2_super_block Read_SuperBlock(int print){
	struct ext2_super_block super;
	int fd;

	/* open floppy device */

	if ((fd = open(FD_DEVICE, O_RDONLY)) < 0) {
		perror(FD_DEVICE);
		exit(1);  /* error while opening the floppy device */
	}

	/* read super-block */

	lseek(fd, BASE_OFFSET, SEEK_SET); 
	read(fd, &super, sizeof(super));
	close(fd);

	if (super.s_magic != EXT2_SUPER_MAGIC) {
		fprintf(stderr, "Not a Ext2 filesystem\n");
		exit(1);
	}
		
	block_size = 1024 << super.s_log_block_size;

	if(print == 1){
		printf("Reading super-block from device " FD_DEVICE ":\n"
			"Inodes count            : %u\n"
			"Blocks count            : %u\n"
			"Reserved blocks count   : %u\n"
			"Free blocks count       : %u\n"
			"Free inodes count       : %u\n"
			"First data block        : %u\n"
			"Block size              : %u\n"
			"Blocks per group        : %u\n"
			"Inodes per group        : %u\n"
			"Creator OS              : %u\n"
			"First non-reserved inode: %u\n"
			"Size of inode structure : %hu\n"
			,
			super.s_inodes_count,  
			super.s_blocks_count,
			super.s_r_blocks_count,     /* reserved blocks count */
			super.s_free_blocks_count,
			super.s_free_inodes_count,
			super.s_first_data_block,
			block_size,
			super.s_blocks_per_group,
			super.s_inodes_per_group,
			super.s_creator_os,
			super.s_first_ino,          /* first non-reserved inode */
			super.s_inode_size);
	}
	
	return super;
}

struct ext2_group_desc Read_GroupDesc(int print){
	struct ext2_super_block super;
	struct ext2_group_desc group;
	int fd;

	/* open floppy device */

	if ((fd = open(FD_DEVICE, O_RDONLY)) < 0) {
		perror(FD_DEVICE);
		exit(1);  /* error while opening the floppy device */
	}

	/* read super-block */

	lseek(fd, BASE_OFFSET, SEEK_SET); 
	read(fd, &super, sizeof(super));

	if (super.s_magic != EXT2_SUPER_MAGIC) {
		fprintf(stderr, "Not a Ext2 filesystem\n");
		exit(1);
	}
		
	block_size = 1024 << super.s_log_block_size;

	/* read group descriptor */

	lseek(fd, BASE_OFFSET + block_size, SEEK_SET);
	read(fd, &group, sizeof(group));
	close(fd);

	if(print == 1){
		printf("Reading first group-descriptor from device " FD_DEVICE ":\n"
			"Blocks bitmap block: %u\n"
			"Inodes bitmap block: %u\n"
			"Inodes table block : %u\n"
			"Free blocks count  : %u\n"
			"Free inodes count  : %u\n"
			"Directories count  : %u\n"
			,
			group.bg_block_bitmap,
			group.bg_inode_bitmap,
			group.bg_inode_table,
			group.bg_free_blocks_count,
			group.bg_free_inodes_count,
			group.bg_used_dirs_count);    /* directories count */
	}
	return group;
}

void Read_RootInode(){
	struct ext2_super_block super;
	struct ext2_group_desc group;
	struct ext2_inode inode;
	int fd;
	int i;

	/* open floppy device */

	if ((fd = open(FD_DEVICE, O_RDONLY)) < 0) {
		perror(FD_DEVICE);
		exit(1);  /* error while opening the floppy device */
	}

	/* read super-block */

	lseek(fd, BASE_OFFSET, SEEK_SET); 
	read(fd, &super, sizeof(super));

	if (super.s_magic != EXT2_SUPER_MAGIC) {
		fprintf(stderr, "Not a Ext2 filesystem\n");
		exit(1);
	}
		
	block_size = 1024 << super.s_log_block_size;

	/* read group descriptor */

	lseek(fd, BASE_OFFSET + block_size, SEEK_SET);
	read(fd, &group, sizeof(group));

	/* read root inode */

	read_inode(fd, 2, &group, &inode);

	printf("Reading root inode\n"
	       "File mode: %hu\n"
	       "Owner UID: %hu\n"
	       "Size     : %u bytes\n"
	       "Blocks   : %u\n"
	       ,
	       inode.i_mode,
	       inode.i_uid,
	       inode.i_size,
	       inode.i_blocks);

	for(i=0; i<EXT2_N_BLOCKS; i++)
		if (i < EXT2_NDIR_BLOCKS)         /* direct blocks */
			printf("Block %2u : %u\n", i, inode.i_block[i]);
		else if (i == EXT2_IND_BLOCK)     /* single indirect block */
			printf("Single   : %u\n", inode.i_block[i]);
		else if (i == EXT2_DIND_BLOCK)    /* double indirect block */
			printf("Double   : %u\n", inode.i_block[i]);
		else if (i == EXT2_TIND_BLOCK)    /* triple indirect block */
			printf("Triple   : %u\n", inode.i_block[i]);

	close(fd);
}

void List_ETX(){
	struct ext2_super_block super;
	struct ext2_group_desc group;
	struct ext2_inode inode;
	int fd;

	/* open floppy device */

	if ((fd = open(FD_DEVICE, O_RDONLY)) < 0) {
		perror(FD_DEVICE);
		exit(1);  /* error while opening the floppy device */
	}

	/* read super-block */

	lseek(fd, BASE_OFFSET, SEEK_SET); 
	read(fd, &super, sizeof(super));

	if (super.s_magic != EXT2_SUPER_MAGIC) {
		fprintf(stderr, "Not a Ext2 filesystem\n");
		exit(1);
	}
		
	block_size = 1024 << super.s_log_block_size;

	/* read group descriptor */

	lseek(fd, BASE_OFFSET + block_size, SEEK_SET);
	read(fd, &group, sizeof(group));

	/* show entries in the root directory */

	read_inode(fd, 2, &group, &inode);   /* read inode 2 (root directory) */
	read_dir(fd, &inode, &group);

	close(fd);
}


//Funções Auxiliares 4.c

static 
void read_inode(int fd, int inode_no, const struct ext2_group_desc *group, struct ext2_inode *inode)
{
	lseek(fd, BLOCK_OFFSET(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), 
	      SEEK_SET);
	read(fd, inode, sizeof(struct ext2_inode));
} /* read_inode() */


static void read_dir(int fd, const struct ext2_inode *inode, const struct ext2_group_desc *group)
{
	void *block;

	if (S_ISDIR(inode->i_mode)) {
		struct ext2_dir_entry_2 *entry;
		unsigned int size = 0;

		if ((block = malloc(block_size)) == NULL) { /* allocate memory for the data block */
			fprintf(stderr, "Memory error\n");
			close(fd);
			exit(1);
		}

		lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
		read(fd, block, block_size);                /* read block from disk*/

		entry = (struct ext2_dir_entry_2 *) block;  /* first entry in the directory */
                /* Notice that the list may be terminated with a NULL
                   entry (entry->inode == NULL)*/
		while((size < inode->i_size) && entry->inode) {
			char file_name[EXT2_NAME_LEN+1];
			memcpy(file_name, entry->name, entry->name_len);
			file_name[entry->name_len] = 0;     /* append null character to the file name */
			printf("%10u %s\n", entry->inode, file_name);
			entry = (void*) entry + entry->rec_len;
			size += entry->rec_len;
		}

		free(block);
	}
} /* read_dir() */

static unsigned int Read_Inode_Number(int fd, const struct ext2_inode *inode, const struct ext2_group_desc *group, char* nome_arquivo)
{
	void *block;

	if (S_ISDIR(inode->i_mode)) {
		struct ext2_dir_entry_2 *entry;
		unsigned int size = 0;

		if ((block = malloc(block_size)) == NULL) { /* allocate memory for the data block */
			fprintf(stderr, "Memory error\n");
			close(fd);
			exit(1);
		}

		lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
		read(fd, block, block_size);                /* read block from disk*/

		entry = (struct ext2_dir_entry_2 *) block;  /* first entry in the directory */
                /* Notice that the list may be terminated with a NULL
                   entry (entry->inode == NULL)*/
		while((size < inode->i_size) && entry->inode) {
			char file_name[EXT2_NAME_LEN+1];
			memcpy(file_name, entry->name, entry->name_len);
			file_name[entry->name_len] = 0;     /* append null character to the file name */

			if(strcmp(nome_arquivo, entry->name) == 0){
				return entry->inode;
			}

			entry = (void*) entry + entry->rec_len;
			size += entry->rec_len;
		}

		free(block);
	}
} /* retorna numero inode */

void read_arq(int fd, struct ext2_inode *inode){
	int size = inode->i_size;
	char *bloco = malloc(block_size);

	lseek(fd, BLOCK_OFFSET(inode->i_block[0]),SEEK_SET);
	read(fd, bloco, block_size);

	for (int i = 0; i < size; i++)
	{
		printf("%c", bloco[i]);
	}
	
	
}


int main(){
struct ext2_super_block super;
	struct ext2_group_desc group;
	struct ext2_inode inode;
	int fd;

	/* open floppy device */

	if ((fd = open(FD_DEVICE, O_RDONLY)) < 0) {
		perror(FD_DEVICE);
		exit(1);  /* error while opening the floppy device */
	}

	/* read super-block */

	lseek(fd, BASE_OFFSET, SEEK_SET); 
	read(fd, &super, sizeof(super));

	if (super.s_magic != EXT2_SUPER_MAGIC) {
		fprintf(stderr, "Not a Ext2 filesystem\n");
		exit(1);
	}
		
	block_size = 1024 << super.s_log_block_size;

	/* read group descriptor */

	lseek(fd, BASE_OFFSET + block_size, SEEK_SET);
	read(fd, &group, sizeof(group));

	/* show entries in the root directory */

	read_inode(fd, 2, &group, &inode);   /* read inode 2 (root directory) */
	unsigned int i_number = Read_Inode_Number(fd, &inode, &group, "hello.txt");
	read_inode(fd, i_number, &group, &inode);
	read_arq(fd, &inode);


    return 0;
}