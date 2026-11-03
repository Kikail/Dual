//
// Created by killian on 11/3/26.
//

#ifndef DUAL_DUAL_FILE_H
#define DUAL_DUAL_FILE_H

DUAL_Result DUAL_LoadFile(const char* filename, char* buffer, size_t buffer_size);
DUAL_Result DUAL_GetFileSize(char* filename, uint64_t* out_taille_octets);

#endif //DUAL_DUAL_FILE_H
