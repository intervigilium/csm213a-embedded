#ifndef FS_CONSTANTS_H_
#define FS_CONSTANTS_H_

#define MAX_SYNCED_BLOCKS 32
#define ECHO_TIMEOUT 30 // 30 seconds
#define MASTER_ADDR 164
#define SYNC_FS_PORT 31415
#define BLOCK_SIZE 512
#define HASH_SIZE 16
#define BLOCK_NUM 32

// slave -> master messages
#define MSG_WRITE_BLOCK 0x72 // block number, 512 byte block buffer
#define MSG_REQUEST_SYNC 0x73 // block number, MD4 for block 0 to 31

// master -> slave messages
#define MSG_UPDATE_BLOCK 0x71 // block number, 512 byte block buffer
#define MSG_WRITE_SUCCESS 0x76 // block number written

#endif