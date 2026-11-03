//
// Created by killian on 11/3/26.
//
#include "../../include/DUAL_Core/dual_core.h"
#include "../../include/DUAL_Core/dual_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long int DUAL_File_Size(FILE* fp) {
    if (!fp) return -1;
    if (fseek(fp, 0L, SEEK_END) != 0) return -1;
    long int res = ftell(fp);
    rewind(fp);
    return res;
}

bool DUAL_File_Exist(const char* filename) {
    FILE* file = fopen(filename, "rb");
    bool ret = (file != NULL);
    if (file) fclose(file);
    return ret;
}

DUAL_File* DUAL_File_Load(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return NULL;

    long int size = DUAL_File_Size(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    DUAL_File* file = malloc(sizeof(DUAL_File));
    if (!file) {
        fclose(fp);
        return NULL;
    }

    file->block_size = (size_t)size;
    if (file->block_size > 0) {
        file->block = malloc(file->block_size);
        if (!file->block) {
            free(file);
            fclose(fp);
            return NULL;
        }
        if (fread(file->block, 1, file->block_size, fp) != file->block_size) {
            free(file->block);
            free(file);
            fclose(fp);
            return NULL;
        }
    } else {
        file->block = NULL;
    }

    fclose(fp);
    strncpy(file->name, filename, sizeof(file->name) - 1);
    file->name[sizeof(file->name) - 1] = '\0';
    return file;
}

void DUAL_File_Free(DUAL_File* file) {
    if (!file) return;
    free(file->block);
    free(file);
}

void DUAL_File_Pack(const char* filepath, DUAL_File** files, unsigned int file_count) {
    FILE* fp = fopen(filepath, "wb");
    if (!fp) return;

    fwrite(&file_count, sizeof(unsigned int), 1, fp);

    for (unsigned int i = 0; i < file_count; i++) {
        DUAL_File* file = files[i];

        char name[DUAL_FILE_SIZE_MINIMUM] = {0};
        strncpy(name, file->name, sizeof(name) - 1);
        fwrite(name, 1, DUAL_FILE_SIZE_MINIMUM, fp);

        fwrite(&file->block_size, sizeof(size_t), 1, fp);

        if (file->block_size > 0 && file->block) {
            fwrite(file->block, 1, file->block_size, fp);
        }
    }
    fclose(fp);
}

DUAL_VFS_Archive* DUAL_VFS_Init(const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) return NULL;

    unsigned int count = 0;
    if (fread(&count, sizeof(unsigned int), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    DUAL_VFS_Archive* archive = malloc(sizeof(DUAL_VFS_Archive));
    if (!archive) {
        fclose(fp);
        return NULL;
    }

    archive->file_count = count;
    archive->files = malloc(count * sizeof(DUAL_VFS_File));
    if (!archive->files) {
        free(archive);
        fclose(fp);
        return NULL;
    }

    for (unsigned int i = 0; i < count; i++) {
        char fname[DUAL_FILE_SIZE_MINIMUM] = {0};
        if (fread(fname, 1, DUAL_FILE_SIZE_MINIMUM, fp) != DUAL_FILE_SIZE_MINIMUM) break;
        fname[63] = '\0';
        strncpy(archive->files[i].name, fname, sizeof(archive->files[i].name) - 1);

        size_t size = 0;
        if (fread(&size, sizeof(size_t), 1, fp) != 1) break;
        archive->files[i].size = size;

        if (size > 0) {
            archive->files[i].data = malloc(size);
            if (!archive->files[i].data) {
                archive->files[i].size = 0;
                continue;
            }
            if (fread(archive->files[i].data, 1, size, fp) != size) {
                free(archive->files[i].data);
                archive->files[i].data = NULL;
                archive->files[i].size = 0;
            }
        } else {
            archive->files[i].data = NULL;
        }
    }

    fclose(fp);
    return archive;
}

DUAL_VFS_File* DUAL_VFS_GetFile(DUAL_VFS_Archive* archive, const char* filename) {
    if (!archive) return NULL;
    for (unsigned int i = 0; i < archive->file_count; i++) {
        if (strcmp(archive->files[i].name, filename) == 0) {
            return &archive->files[i];
        }
    }
    return NULL;
}

void DUAL_VFS_Free(DUAL_VFS_Archive* archive) {
    if (!archive) return;
    for (unsigned int i = 0; i < archive->file_count; i++) {
        free(archive->files[i].data);
    }
    free(archive->files);
    free(archive);
}

DUAL_Result DUAL_LoadFile(const char* filename, char* buffer, size_t buffer_size) {
    if (!buffer || !filename || buffer_size == 0) return DUAL_ERROR_INVALID_ARG;
    memset(buffer, 0, buffer_size);
    FILE* fileptr = fopen(filename, "rb");
    if (!fileptr) return DUAL_ERROR_NOT_FOUND;
    size_t bytes_read = fread(buffer, 1, buffer_size - 1, fileptr);
    buffer[bytes_read] = '\0';
    fclose(fileptr);
    return DUAL_OK;
}

DUAL_Result DUAL_GetFileSize(char* filename, uint64_t* out_taille_octets) {
    if (!filename || !out_taille_octets) return DUAL_ERROR_INVALID_ARG;
    FILE* handle = fopen(filename, "rb");
    if (!handle) return DUAL_ERROR_NOT_FOUND;
    long position_actuelle = ftell(handle);
    if (position_actuelle < 0) return DUAL_ERROR_UNKNOWN;
    if (fseek(handle, 0, SEEK_END) != 0) return DUAL_ERROR_UNKNOWN;
    long taille = ftell(handle);
    if (taille < 0) return DUAL_ERROR_UNKNOWN;
    fseek(handle, position_actuelle, SEEK_SET);
    *out_taille_octets = (uint64_t)taille;
    fclose(handle);
    return DUAL_OK;
}
