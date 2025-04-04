#ifndef KERO_STD_H

#if __cplusplus__
extern "C" {
#endif
    
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
    
    typedef struct {
        union {
            uint32_t length;
            uint32_t len;
            uint32_t l;
        };
        union {
            char *data;
            char *d;
            char *str;
            char *s;
            char *text;
            char *t;
        };
    } kstring_t;
    
    bool KStd_StringInit(kstring_t *string, const char *const text) {
        string->length = strlen(text);
        string->text = malloc(string->length);
        if(!string->text) {
            return false;
        }
        strcpy(string->text, text);
        return true;
    }
    
    void KStd_StringFree(kstring_t *string) {
        if(string->text) {
            free(string->text);
        }
    }
    
    typedef enum {
        KSTD_ERROR_SUCCESS = 0, KSTD_ERROR_FILENOTFOUND, KSTD_ERROR_FILESEEK, KSTD_ERROR_FILESIZE, KSTD_ERROR_MALLOC, KSTD_ERROR_FILEREAD
    } kstd_error_t;
    int KStd_ReadFile(const char *const filepath, uint8_t **data, uint32_t *byte_count) {
        FILE *file = fopen(filepath, "rb");
        if(!file) {
            return KSTD_ERROR_FILENOTFOUND;
        }
        if(fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            return KSTD_ERROR_FILESEEK;
        }
        *byte_count = ftell(file);
        if(*byte_count <= 0) {
            fclose(file);
            return KSTD_ERROR_FILESIZE;
        }
        rewind(file);
        *data = malloc(*byte_count);
        if(!*data) {
            fclose(file);
            return KSTD_ERROR_MALLOC;
        }
        if(fread(*data, 1, *byte_count, file) < *byte_count) {
            fclose(file);
            free(*data);
            return KSTD_ERROR_FILEREAD;
        }
        fclose(file);
        return KSTD_ERROR_SUCCESS;
    }
    
#ifndef KERO_DYNAMIC_ARRAY_NO_SHORT_NAMES
#define KDA_Init KeroDynamicArray_Init
#define KDA_Free KeroDynamicArray_Free
#define KDA_Push KeroDynamicArray_Push
#define KDA_Size KeroDynamicArray_Size
#define KDA_Top KeroDynamicArray_Top
#define KDA_Shrink KeroDynamicArray_Shrink
#define KDA_Resize KeroDynamicArray_Resize
#endif
    
    void FuncNil() {}
    
#define KeroDynamicArray_Size(array) (\
    ((uint32_t*)(array))[-2] \
    )
    
#define KeroDynamicArray_Top(array) (\
    ((uint32_t*)(array))[-1] \
    )
    
#define KeroDynamicArray_Init(array, start_size) (\
    *(void **)&(array) = (uint32_t*)malloc(sizeof(*(array)) * start_size + sizeof(uint32_t) * 2) + 2, \
    KeroDynamicArray_Size(array) = start_size, \
    KeroDynamicArray_Top(array) = 0, \
    (array) \
    )
    
#define KeroDynamicArray_Free(array) (\
    free(&((uint32_t *)(array))[-2]) \
    )
    
#define KeroDynamicArray_Push(array, data) (\
    ( KeroDynamicArray_Size(array) <= KeroDynamicArray_Top(array) ? \
    KeroDynamicArray_Size(array) *= 2, *(void **)&(array) = (uint32_t*)realloc(&(*(uint32_t **)&(array))[-2], sizeof(*(array)) * KeroDynamicArray_Size(array) + sizeof(uint32_t) * 2) + 2 : 0), \
    (array) != NULL ? \
    ((array)[KeroDynamicArray_Top(array)++] = (data)), (array) : \
    NULL \
    )
    
#define KeroDynamicArray_Shrink(array) (\
    ( KeroDynamicArray_Size(array) > KeroDynamicArray_Top(array) ? \
    KeroDynamicArray_Size(array) = KeroDynamicArray_Top(array), *(void **)&(array) = (uint32_t*)realloc(&(*(uint32_t **)&(array))[-2], sizeof(*(array)) * KeroDynamicArray_Size(array) + sizeof(uint32_t) * 2) + 2 : 0), \
    (array) \
    )
    
#define KeroDynamicArray_Resize(array, size) (\
    ( KeroDynamicArray_Size(array) = (size), *(void **)&(array) = (uint32_t*)realloc(&(*(uint32_t **)&(array))[-2], sizeof(*(array)) * KeroDynamicArray_Size(array) + sizeof(uint32_t) * 2) + 2), \
    (array) \
    )
    
#if __cplusplus__
}
#endif

#define KERO_STD_H
#endif