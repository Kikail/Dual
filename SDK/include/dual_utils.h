//
// Created by killian on 7/9/26.
//

#ifndef DUAL_DUAL_UTILS_H
#define DUAL_DUAL_UTILS_H

#define DEBUG

// La fonction qui fait le travail (propre et facile à lire)
static inline void dual_log_result_internal(int result, int line, const char* file, const char* func) {
#ifdef DEBUG
    switch (result) {
        case DUAL_OK:
            break;
        case DUAL_ERROR_INVALID_ARG:
            DUAL_Log(DUAL_LOG_ERROR, "Invalid argument [%d , %s, %s]", line, file, func);
            break;
        case DUAL_ERROR_NOT_FOUND:
            DUAL_Log(DUAL_LOG_ERROR, "Not found [%d , %s, %s]", line, file, func);
            break;
        case DUAL_ERROR_UNKNOWN:
            DUAL_Log(DUAL_LOG_ERROR, "Unknown error [%d , %s, %s]", line, file, func);
            break;
        case DUAL_ERROR_OUT_OF_MEMORY:
            DUAL_Log(DUAL_LOG_ERROR, "Out of memory [%d , %s, %s]", line, file, func);
            break;
        case DUAL_ERROR_INIT_FAILED:
            DUAL_Log(DUAL_LOG_ERROR, "Initialization failed [%d , %s, %s]", line, file, func);
            break;
        default:
            DUAL_Log(DUAL_LOG_ERROR, "Unknown error [%d , %s, %s]", line, file, func);
            break;
    }
#else
    (void)result; (void)line; (void)file; (void)func; // Évite les warnings en mode Release
#endif
}

// La macro finale que tu appelles dans ton code
#define DEBUG_DUAL_RESULT(result) dual_log_result_internal(result, __LINE__, __FILE__, __FUNCTION__)


#endif //DUAL_DUAL_UTILS_H
