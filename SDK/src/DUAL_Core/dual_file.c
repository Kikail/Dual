//
// Created by killian on 11/3/26.
//
#include "../../include/DUAL_Core/dual_core.h"
#include "../../include/DUAL_Core/dual_file.h"
#include <stdio.h>
#include <string.h>

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
