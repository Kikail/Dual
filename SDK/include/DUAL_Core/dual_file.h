//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_FILE_H
#define DUAL_DUAL_FILE_H
#include <stdio.h>

#define DUAL_FILE_SIZE_MINIMUM 128

typedef struct {
    char name[DUAL_FILE_SIZE_MINIMUM];
    size_t size;
    char* data;
} DUAL_VFS_File;

typedef struct {
    unsigned int file_count;
    DUAL_VFS_File* files;
} DUAL_VFS_Archive;

struct DUAL_File {
    char name[DUAL_FILE_SIZE_MINIMUM];
    char* block;
    size_t block_size;
};
typedef struct DUAL_File DUAL_File;

long int DUAL_File_Size(FILE* fp);
bool DUAL_File_Exist(const char* filename);
DUAL_File* DUAL_File_Load(const char* filename);
void DUAL_File_Pack(const char* filepath, DUAL_File** files, unsigned int file_count);
DUAL_VFS_Archive* DUAL_VFS_Init(const char* filepath);
DUAL_VFS_File* DUAL_VFS_GetFile(DUAL_VFS_Archive* archive, const char* filename);
void DUAL_VFS_Free(DUAL_VFS_Archive* archive);

DUAL_Result DUAL_LoadFile(const char* filename, char* buffer, size_t buffer_size);
DUAL_Result DUAL_GetFileSize(char* filename, uint64_t* out_taille_octets);

#endif //DUAL_DUAL_FILE_H
