#ifndef KERO_IMAGE_H
#define KERO_IMAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "kero_math.h"
#include "tinfl.c"

#include <stdio.h>

typedef struct {
    union {
        int w;
        int width;
    };
    union {
        int h;
        int height;
    };
    uint32_t *pixels;
} kimage_t;

enum { KI_LOAD_SUCCESS, KI_LOAD_FILENOTFOUND, KI_LOAD_SIGNATUREMISMATCH, KI_LOAD_FILEREADERROR, KI_LOAD_MALLOCERROR, KI_LOAD_FORMATERROR };
#define KI_CHUNK_TYPE(a, b, c, d)  (((unsigned)(a) << 24) + ((unsigned)(b) << 16) + ((unsigned)(c) << 8) + (unsigned)(d))
#define KI_GET8(array, index) ((array)[(index)])
#define KI_GET16(array, index) (((array)[(index)] << 8) + (array)[(index) + 1])
#define KI_GET32(array, index) (((array)[(index)] << 24) + ((array)[(index) + 1] << 16) + ((array)[(index) + 2] << 8) + ((array)[(index) + 3]))

static inline uint32_t KI_GetBits(uint8_t *array, uint64_t *bit_offset, uint8_t num_bits_to_get) {
    uint32_t bits = 0; // Hold up to 20 bits
    uint64_t current_byte = (*bit_offset) / 8;
    uint8_t current_bit = *bit_offset - current_byte * 8;
    *bit_offset += num_bits_to_get;
    uint32_t loaded_bit = 0;
    while(num_bits_to_get-- > 0) {
        bits |=
            (array[current_byte] & (1 << current_bit)) ? 1 << loaded_bit : 0;
        if(++current_bit > 7) {
            current_bit = 0;
            ++current_byte;
        }
        ++loaded_bit;
    }
    return bits;
}

static inline uint8_t KI_PaethPredictor(uint8_t a, uint8_t b, uint8_t c) {
    int p, pa, pb, pc;
    p = a + b - c;
    pa = Absolute(p - a);
    pb = Absolute(p - b);
    pc = Absolute(p - c);
    if(pa <= pb && pa <= pc) {
        return a;
    }
    if(pb <= pc) {
        return b;
    }
    return c;
}

int KI_Load(kimage_t *image, const char *const filepath, uint8_t red_byte, uint8_t green_byte, uint8_t blue_byte, uint8_t opacity_byte, bool top_equals_zero) {
#ifdef KERO_IMAGE_VERBOSE
    fprintf(stderr, "Kero Image loading file: %s\n", filepath);
#endif
    
    FILE *file = fopen(filepath, "rb");
    if(!file) {
        return KI_LOAD_FILENOTFOUND;
    }
    if(fseek(file, 0, SEEK_END)) {
        fclose(file);
        return KI_LOAD_FILEREADERROR;
    }
    unsigned long filesize = ftell(file);
    if(filesize == -1L || filesize == 0) {
        fclose(file);
        return KI_LOAD_FILEREADERROR;
    }
    rewind(file);
#ifdef KERO_IMAGE_VERBOSE
    fprintf(stderr, "File size: %lu\n", filesize);
#endif
    uint8_t *data = malloc(filesize);
    if(fread(data, 1, filesize, file) != filesize) {
        fclose(file);
        free(data);
        return KI_LOAD_FILEREADERROR;
    }
    fclose(file);
    
    if(data[0] != 137 || data[1] != 80 || data[2] != 78 || data[3] != 71 || data[4] != 13 || data[5] != 10 || data[6] != 26 || data[7] != 10) {
        free(data);
        return KI_LOAD_SIGNATUREMISMATCH;
    }
#ifdef KERO_IMAGE_VERBOSE
    fprintf(stderr, "Signature correct\n");
#endif
    if(data[8] != 0 || data[9] != 0 || data[10] != 0 || data[11] != 13 || data[12] != 73 || data[13] != 72 || data[14] != 68 || data[15] != 82) {
        free(data);
        return KI_LOAD_FORMATERROR;
    }
#ifdef KERO_IMAGE_VERBOSE
    fprintf(stderr, "IHDR first chunk and length correct\n");
#endif
    
    uint32_t length;
    uint32_t chunk_type;
    uint8_t cyclic_redundancy_code[4];
    uint8_t *chunk_data = NULL;
    struct {
        unsigned int first_chunk : 1;
    } flags = {0};
    flags.first_chunk = 1;
    int width, height;
    uint8_t depth, color_type, compression, filter, interlace;
    int idat_size = 0;
    int idat_old_size;
    uint8_t *idat = malloc(idat_size);
    int window_size;
    uint16_t fcheck;
    bool fdict;
    int dictid;
    uint64_t bit_offset = 0;
    uint32_t *palette = NULL;
    int palette_size;
    
    int index = 8;
    
    for(;;) {
#ifdef KERO_IMAGE_VERBOSE
        fprintf(stderr, "\nNew Chunk | ");
#endif
        length = KI_GET32(data, index);
        index += 4;
        if(length > 2147483647) {
            free(data);
            return KI_LOAD_FORMATERROR;
        }
#ifdef KERO_IMAGE_VERBOSE
        fprintf(stderr, "Length: %6d | ", length);
#endif
        chunk_type = KI_GET32(data, index);
        index += 4;
        switch(chunk_type) {
            case KI_CHUNK_TYPE(73, 72, 68, 82):{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "IHDR | ");
#endif
                if(!flags.first_chunk) {
                    free(data);
                    return KI_LOAD_FORMATERROR;
                }
                flags.first_chunk = 0;
                width = KI_GET32(data, index);
                index += 4;
                height = KI_GET32(data, index);
                index += 4;
                depth = KI_GET8(data, index);
                index += 1;
                color_type = KI_GET8(data, index);
                index += 1;
                compression = KI_GET8(data, index);
                index += 1;
                filter = KI_GET8(data, index);
                index += 1;
                interlace = KI_GET8(data, index);
                index += 1;
                if(width == 0 || height == 0 || width > (1 << 24) || height > (1 << 24) || (color_type == 0 && !(depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16)) || (color_type == 2 && !(depth == 8 || depth == 16)) || (color_type == 3 && !(depth == 1 || depth == 2 || depth == 4 || depth == 8)) || (color_type == 4 && !(depth == 8 || depth == 16)) || (color_type == 6 && !(depth == 8 || depth == 16)) || compression != 0 || filter != 0 || (interlace != 0 && interlace != 1)) {
                    free(data);
                    return KI_LOAD_FORMATERROR;
                }
                if(color_type != 0 && color_type != 2 && color_type != 3 && color_type != 4 && color_type != 6) {
#ifdef KERO_IMAGE_VERBOSE
                    fprintf(stderr, "Color type: %d\n", color_type);
#endif
                    free(data);
                    return KI_LOAD_FORMATERROR;
                }
                index += 4;
                image->w = width;
                image->h = height;
                image->pixels = (uint32_t*)malloc(sizeof(uint32_t) * width * height);
                if(!image->pixels) {
                    free(data);
                    return KI_LOAD_MALLOCERROR;
                }
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "Image size: %d, %d\n", width, height);
#endif
            }break;
            case KI_CHUNK_TYPE(80, 76, 84, 69):{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "PLTE | ");
#endif
                palette_size = length / 3;
                if(length % 3 != 0 || palette_size > Power(2, depth)) {
#ifdef KERO_IMAGE_VERBOSE
                    fprintf(stderr, "ERROR: PLTE chunk length not divisible by 3!\n");
#endif
                    free(data);
                    return KI_LOAD_FORMATERROR;
                }
                palette = (uint32_t*)malloc(sizeof(uint32_t) * palette_size);
                for(int i = 0; i < palette_size; ++i) {
                    palette[i] = data[index] + (data[index + 1] << 8) + (data[index + 2] << 16);
                    index += 3;
                }
                index += 4;
            }break;
            case KI_CHUNK_TYPE(73, 68, 65, 84):{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "IDAT | ");
#endif
                idat_old_size = idat_size;
                idat_size += length;
                idat = realloc(idat, idat_size);
                if(idat == NULL) {
                    free(data);
                    return KI_LOAD_MALLOCERROR;
                }
                for(int i = 0; i < length; ++i) {
                    idat[idat_old_size + i] = KI_GET8(data, index + i);
                }
                index += length + 4;
            }break;
            case KI_CHUNK_TYPE(73, 69, 78, 68):{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "IEND |\n");
#endif
                free(data);
                
                uint8_t compression_method, compression_info, flg;
                
                compression_method = idat[0] & 0b1111;
                compression_info = idat[0] >> 4;
                
                if(compression_method != 8 || compression_info > 7) {
                    free(idat);
                    return KI_LOAD_FORMATERROR;
                }
                
                window_size = Power(2, compression_info + 8);
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "window size: %d\n", window_size);
#endif
                
                flg = idat[1];
                
                fcheck = (idat[0] << 8) + flg;
                if(fcheck % 31) {
                    free(idat);
#ifdef KERO_IMAGE_VERBOSE
                    fprintf(stderr, "fcheck failed\n");
#endif
                    return KI_LOAD_FORMATERROR;
                }
                
                fdict = flg & 0b10000;
                
                bit_offset = 8 * 2;
                
                if(fdict) {
                    dictid = KI_GET32(idat, 2);
                    bit_offset += 8 * 4;
                }
                size_t decompressed_length = 0;
                uint8_t *decompressed = tinfl_decompress_mem_to_heap(&idat[2], idat_size - 2, &decompressed_length, 0);
                
                free(idat);
                
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "Decompressed size: %lu\n", decompressed_length);
#endif
                
                // Process scanlines from decompressed data
                int filter;
                uint64_t current_byte = 0;
                uint32_t *new_scanline = (uint32_t*)malloc(sizeof(uint32_t) * width);
                uint8_t *new_scanline_bytes = (uint8_t*)new_scanline;
                uint32_t *last_scanline = (uint32_t*)malloc(sizeof(uint32_t) * width);
                uint8_t *last_scanline_bytes = (uint8_t*)last_scanline;
                switch(color_type) {
                    case 6:{ // RGBA
#ifdef KERO_IMAGE_VERBOSE
                        fprintf(stderr, "Color type: RGBA\n");
#endif
                        for(int y = 0; y < height; ++y) {
                            filter = decompressed[current_byte++];
                            /*#ifdef KERO_IMAGE_VERBOSE
                                                        fprintf(stderr, "Filter: %d\n", filter);
                            #endif*/
                            switch(filter) {
                                case 0:{ // None
                                    for(int x = 0; x < width * 4; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++];
                                    }
                                }break;
                                case 1:{ // Sub
                                    new_scanline_bytes[0] = decompressed[current_byte++];
                                    new_scanline_bytes[1] = decompressed[current_byte++];
                                    new_scanline_bytes[2] = decompressed[current_byte++];
                                    new_scanline_bytes[3] = decompressed[current_byte++];
                                    for(int x = 4; x < width * 4; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + new_scanline_bytes[x - 4];
                                    }
                                }break;
                                case 2:{ // Up
                                    if(y == 0) {
                                        for(int x = 0; x < width * 4; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++];
                                        }
                                    }
                                    else {
                                        for(int x = 0; x < width * 4; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + last_scanline_bytes[x];
                                        }
                                    }
                                }break;
                                case 3:{ // Average
                                    if(y == 0) {
                                        new_scanline_bytes[0] = decompressed[current_byte++];
                                        new_scanline_bytes[1] = decompressed[current_byte++];
                                        new_scanline_bytes[2] = decompressed[current_byte++];
                                        new_scanline_bytes[3] = decompressed[current_byte++];
                                        for(int x = 4; x < width * 4; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + new_scanline_bytes[x - 4] / 2;
                                        }
                                    }
                                    else {
                                        new_scanline_bytes[0] = decompressed[current_byte++] + last_scanline_bytes[0] / 2;
                                        new_scanline_bytes[1] = decompressed[current_byte++] + last_scanline_bytes[1] / 2;
                                        new_scanline_bytes[2] = decompressed[current_byte++] + last_scanline_bytes[2] / 2;
                                        new_scanline_bytes[3] = decompressed[current_byte++] + last_scanline_bytes[3] / 2;
                                        for(int x = 4; x < width * 4; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + (last_scanline_bytes[x] + new_scanline_bytes[x - 4]) / 2;
                                        }
                                    }
                                }break;
                                case 4:{ // Paeth
                                    for(int x = 0; x < width * 4; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + KI_PaethPredictor(x > 3 ? new_scanline_bytes[x - 4] : 0, y != 0 ? last_scanline_bytes[x] : 0, y != 0 && x > 3 ? last_scanline_bytes[x - 4] : 0);
                                    }
                                }break;
                                default:{
#ifdef KERO_IMAGE_VERBOSE
                                    fprintf(stderr, "Invalid filter\n");
#endif
                                    free(new_scanline);
                                    free(last_scanline);
                                    free(decompressed);
                                    return KI_LOAD_FORMATERROR;
                                }break;
                            }
                            uint8_t *pixels = (uint8_t*)image->pixels;
                            int component_index = 0;
                            int pixel_index = (top_equals_zero ? y : height - y - 1) * width * 4;
                            for(int x = 0; x < width; ++x) {
                                last_scanline[x] = new_scanline[x];
                                pixels[pixel_index + red_byte] = new_scanline_bytes[component_index++];
                                pixels[pixel_index + green_byte] = new_scanline_bytes[component_index++];
                                pixels[pixel_index + blue_byte] = new_scanline_bytes[component_index++];
                                pixels[pixel_index + opacity_byte] = new_scanline_bytes[component_index++];
                                pixel_index += 4;
                            }
                        }
                    }break;
                    
                    case 2:{ // RGB
#ifdef KERO_IMAGE_VERBOSE
                        fprintf(stderr, "Color type: RGB\n");
#endif
                        for(int y = 0; y < height; ++y) {
                            filter = decompressed[current_byte++];
                            /*#ifdef KERO_IMAGE_VERBOSE
                                                        fprintf(stderr, "Filter: %d\n", filter);
                            #endif*/
                            switch(filter) {
                                case 0:{ // None
                                    for(int x = 0; x < width * 4; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++];
                                    }
                                }break;
                                case 1:{ // Sub
                                    new_scanline_bytes[0] = decompressed[current_byte++];
                                    new_scanline_bytes[1] = decompressed[current_byte++];
                                    new_scanline_bytes[2] = decompressed[current_byte++];
                                    for(int x = 3; x < width * 3; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + new_scanline_bytes[x - 3];
                                    }
                                }break;
                                case 2:{ // Up
                                    if(y == 0) {
                                        for(int x = 0; x < width * 3; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++];
                                        }
                                    }
                                    else {
                                        for(int x = 0; x < width * 3; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + last_scanline_bytes[x];
                                        }
                                    }
                                }break;
                                case 3:{ // Average
                                    if(y == 0) {
                                        new_scanline_bytes[0] = decompressed[current_byte++];
                                        new_scanline_bytes[1] = decompressed[current_byte++];
                                        new_scanline_bytes[2] = decompressed[current_byte++];
                                        for(int x = 3; x < width * 3; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + new_scanline_bytes[x - 3] / 2;
                                        }
                                    }
                                    else {
                                        new_scanline_bytes[0] = decompressed[current_byte++] + last_scanline_bytes[0] / 2;
                                        new_scanline_bytes[1] = decompressed[current_byte++] + last_scanline_bytes[1] / 2;
                                        new_scanline_bytes[2] = decompressed[current_byte++] + last_scanline_bytes[2] / 2;
                                        for(int x = 3; x < width * 3; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + (last_scanline_bytes[x] + new_scanline_bytes[x - 3]) / 2;
                                        }
                                    }
                                }break;
                                case 4:{ // Paeth
                                    for(int x = 0; x < width * 3; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + KI_PaethPredictor(x > 2 ? new_scanline_bytes[x - 3] : 0, y != 0 ? last_scanline_bytes[x] : 0, y != 0 && x > 2 ? last_scanline_bytes[x - 3] : 0);
                                    }
                                }break;
                                default:{
#ifdef KERO_IMAGE_VERBOSE
                                    fprintf(stderr, "Invalid filter\n");
#endif
                                    free(new_scanline);
                                    free(last_scanline);
                                    free(decompressed);
                                    return KI_LOAD_FORMATERROR;
                                }break;
                            }
                            uint8_t *pixels = (uint8_t*)image->pixels;
                            int component_index = 0;
                            int pixel_index = (top_equals_zero ? y : height - y - 1) * width * 4;
                            for(int x = 0; x < width; ++x) {
                                last_scanline[x] = new_scanline[x];
                                pixels[pixel_index + red_byte] = new_scanline_bytes[component_index++];
                                pixels[pixel_index + green_byte] = new_scanline_bytes[component_index++];
                                pixels[pixel_index + blue_byte] = new_scanline_bytes[component_index++];
                                pixels[pixel_index + opacity_byte] = 255;
                                pixel_index += 4;
                            }
                        }
                    }break;
                    
                    case 3:{ // Using palette
#ifdef KERO_IMAGE_VERBOSE
                        fprintf(stderr, "Color type: Palette\n");
#endif
                        for(int y = 0; y < height; ++y) {
                            filter = decompressed[current_byte++];
                            /*#ifdef KERO_IMAGE_VERBOSE
                                                        fprintf(stderr, "Filter: %d\n", filter);
                            #endif*/
                            switch(filter) {
                                case 0:{ // None
                                    for(int x = 0; x < width; ++x) {
                                        new_scanline[x] = decompressed[current_byte++];
                                    }
                                }break;
                                case 1:{ // Sub
                                    new_scanline[0] = decompressed[current_byte++];
                                    for(int x = 1; x < width; ++x) {
                                        new_scanline[x] = decompressed[current_byte++] + new_scanline[x - 1];
                                    }
                                }break;
                                case 2:{ // Up
                                    if(y == 0) {
                                        for(int x = 0; x < width; ++x) {
                                            new_scanline[x] = decompressed[current_byte++];
                                        }
                                    }
                                    else {
                                        for(int x = 0; x < width; ++x) {
                                            new_scanline[x] = decompressed[current_byte++] + last_scanline[x];
                                        }
                                    }
                                }break;
                                case 3:{ // Average
                                    if(y == 0) {
                                        new_scanline[0] = palette[decompressed[current_byte++]];
                                        for(int x = 1; x < width; ++x) {
                                            new_scanline[x] = decompressed[current_byte] + new_scanline[x - 1] / 2;
                                            ++current_byte;
                                        }
                                    }
                                    else {
                                        new_scanline[0] = decompressed[current_byte++] + last_scanline[0] / 2;
                                        for(int x = 1; x < width; ++x) {
                                            new_scanline[x] = decompressed[current_byte++] + (last_scanline[x] + new_scanline[x - 1]) / 2;
                                        }
                                    }
                                }break;
                                case 4:{ // Paeth
                                    for(int x = 0; x < width; ++x) {
                                        new_scanline[x] = decompressed[current_byte++] + KI_PaethPredictor(x != 0 ? new_scanline[x - 1] : 0, y != 0 ? last_scanline[x] : 0, y != 0 && x != 0 ? last_scanline[x - 1] : 0);
                                    }
                                }break;
                                default:{
#ifdef KERO_IMAGE_VERBOSE
                                    fprintf(stderr, "Invalid filter\n");
#endif
                                    free(new_scanline);
                                    free(last_scanline);
                                    free(decompressed);
                                    return KI_LOAD_FORMATERROR;
                                }break;
                            }
                            uint8_t *pixels = (uint8_t*)image->pixels;
                            int pixel_index = (top_equals_zero ? y : height - y - 1) * width * 4;
                            for(int x = 0; x < width; ++x) {
                                last_scanline[x] = new_scanline[x];
                                pixels[pixel_index + red_byte] = (uint8_t)(palette[new_scanline[x]]);
                                pixels[pixel_index + green_byte] = (uint8_t)(palette[new_scanline[x]] >> 8);
                                pixels[pixel_index + blue_byte] = (uint8_t)(palette[new_scanline[x]] >> 16);
                                pixels[pixel_index + opacity_byte] = (uint8_t)(palette[new_scanline[x]] >> 24);
                                pixel_index += 4;
                            }
                        }
                    }break;
                    
                    case 4:{ // Grayscale with alpha
#ifdef KERO_IMAGE_VERBOSE
                        fprintf(stderr, "Color type: Grayscale with alpha\n");
#endif
                        for(int y = 0; y < height; ++y) {
                            filter = decompressed[current_byte++];
                            /*#ifdef KERO_IMAGE_VERBOSE
                                                        fprintf(stderr, "Filter: %d\n", filter);
                            #endif*/
                            switch(filter) {
                                case 0:{ // None
                                    for(int x = 0; x < width; ++x) {
                                        new_scanline_bytes[x * 2] = decompressed[current_byte++];
                                        new_scanline_bytes[x * 2 + 1] = decompressed[current_byte++];
                                    }
                                }break;
                                case 1:{ // Sub
                                    new_scanline_bytes[0] = decompressed[current_byte++];
                                    new_scanline_bytes[1] = decompressed[current_byte++];
                                    for(int x = 1; x < width; ++x) {
                                        new_scanline_bytes[x * 2] = decompressed[current_byte++] + new_scanline_bytes[(x - 1) * 2];
                                        new_scanline_bytes[x * 2 + 1] = decompressed[current_byte++] + new_scanline_bytes[(x - 1) * 2 + 1];
                                    }
                                }break;
                                case 2:{ // Up
                                    if(y == 0) {
                                        for(int x = 0; x < width; ++x) {
                                            new_scanline_bytes[x * 2] = decompressed[current_byte++];
                                            new_scanline_bytes[x * 2 + 1] = decompressed[current_byte++];
                                        }
                                    }
                                    else {
                                        for(int x = 0; x < width; ++x) {
                                            new_scanline_bytes[x * 2] = decompressed[current_byte++] + last_scanline_bytes[x * 2];
                                            new_scanline_bytes[x * 2 + 1] = decompressed[current_byte++] + last_scanline_bytes[x * 2 + 1];
                                        }
                                    }
                                }break;
                                case 3:{ // Average
                                    if(y == 0) {
                                        new_scanline_bytes[0] = decompressed[current_byte++];
                                        new_scanline_bytes[1] = decompressed[current_byte++];
                                        for(int x = 1; x < width; ++x) {
                                            new_scanline_bytes[x * 2] = decompressed[current_byte++] + new_scanline_bytes[(x - 1) * 2] / 2;
                                            new_scanline_bytes[x * 2 + 1] = decompressed[current_byte++] + new_scanline_bytes[(x - 1) * 2 + 1] / 2;
                                        }
                                    }
                                    else {
                                        new_scanline_bytes[0] = decompressed[current_byte++] + last_scanline_bytes[0] / 2;
                                        new_scanline_bytes[1] = decompressed[current_byte++] + last_scanline_bytes[1] / 2;
                                        for(int x = 1; x < width; ++x) {
                                            new_scanline_bytes[x * 2] = decompressed[current_byte++] + (last_scanline_bytes[x * 2] + new_scanline_bytes[(x - 1) * 2]) / 2;
                                            new_scanline_bytes[x * 2 + 1] = decompressed[current_byte++] + (last_scanline_bytes[x * 2 + 1] + new_scanline_bytes[(x - 1) * 2 + 1]) / 2;
                                        }
                                    }
                                }break;
                                case 4:{ // Paeth
                                    for(int x = 0; x < width * 2; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + KI_PaethPredictor(x > 1 ? new_scanline_bytes[x - 2] : 0, y != 0 ? last_scanline_bytes[x] : 0, y != 0 && x > 1 ? last_scanline_bytes[x - 2] : 0);
                                    }
                                }break;
                                default:{
#ifdef KERO_IMAGE_VERBOSE
                                    fprintf(stderr, "Invalid filter\n");
#endif
                                    free(new_scanline);
                                    free(last_scanline);
                                    free(decompressed);
                                    return KI_LOAD_FORMATERROR;
                                }break;
                            }
                            uint8_t *pixels = (uint8_t*)image->pixels;
                            int pixel_index = (top_equals_zero ? y : height - y - 1) * width * 4;
                            for(int x = 0; x < width; ++x) {
                                last_scanline[x] = new_scanline[x];
                                int twox = x * 2;
                                pixels[pixel_index + red_byte] = pixels[pixel_index + green_byte] = pixels[pixel_index + blue_byte] = new_scanline_bytes[twox];
                                pixels[pixel_index + opacity_byte] = new_scanline_bytes[twox + 1];
                                pixel_index += 4;
                            }
                        }
                    }break;
                    
                    case 0:{ // Grayscale
#ifdef KERO_IMAGE_VERBOSE
                        fprintf(stderr, "Color type: Grayscale\n");
#endif
                        for(int y = 0; y < height; ++y) {
                            filter = decompressed[current_byte++];
                            /*#ifdef KERO_IMAGE_VERBOSE
                                                        fprintf(stderr, "Filter: %d\n", filter);
                            #endif*/
                            switch(filter) {
                                case 0:{ // None
                                    for(int x = 0; x < width; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++];
                                    }
                                }break;
                                case 1:{ // Sub
                                    new_scanline_bytes[0] = decompressed[current_byte++];
                                    for(int x = 1; x < width; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + new_scanline_bytes[x - 1];
                                    }
                                }break;
                                case 2:{ // Up
                                    if(y == 0) {
                                        for(int x = 0; x < width; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++];
                                        }
                                    }
                                    else {
                                        for(int x = 0; x < width; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + last_scanline_bytes[x];
                                        }
                                    }
                                }break;
                                case 3:{ // Average
                                    if(y == 0) {
                                        new_scanline_bytes[0] = decompressed[current_byte++];
                                        for(int x = 1; x < width; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + new_scanline_bytes[x - 1] / 2;
                                        }
                                    }
                                    else {
                                        new_scanline_bytes[0] = decompressed[current_byte++] + last_scanline_bytes[0] / 2;
                                        for(int x = 1; x < width; ++x) {
                                            new_scanline_bytes[x] = decompressed[current_byte++] + (last_scanline_bytes[x] + new_scanline_bytes[x - 1]) / 2;
                                        }
                                    }
                                }break;
                                case 4:{ // Paeth
                                    for(int x = 0; x < width; ++x) {
                                        new_scanline_bytes[x] = decompressed[current_byte++] + KI_PaethPredictor(x != 0 ? new_scanline_bytes[x - 1] : 0, y != 0 ? last_scanline_bytes[x] : 0, y != 0 && x != 0 ? last_scanline_bytes[x - 1] : 0);
                                    }
                                }break;
                                default:{
#ifdef KERO_IMAGE_VERBOSE
                                    fprintf(stderr, "Invalid filter\n");
#endif
                                    free(new_scanline);
                                    free(last_scanline);
                                    free(decompressed);
                                    return KI_LOAD_FORMATERROR;
                                }break;
                            }
                            uint8_t *pixels = (uint8_t*)image->pixels;
                            int pixel_index = (top_equals_zero ? y : height - y - 1) * width * 4;
                            for(int x = 0; x < width; ++x) {
                                last_scanline[x] = new_scanline[x];
                                pixels[pixel_index + red_byte] = pixels[pixel_index + green_byte] = pixels[pixel_index + blue_byte] = new_scanline_bytes[x];
                                pixels[pixel_index + opacity_byte] = 255;
                                pixel_index += 4;
                            }
                        }
                    }break;
                }
                free(new_scanline);
                free(last_scanline);
                free(decompressed);
                
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "Image %s loaded successfully.\n", filepath);
#endif
                
                return KI_LOAD_SUCCESS;
            }break;
            case KI_CHUNK_TYPE('t', 'R', 'N', 'S'):{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "tRNS | ");
#endif
                int i;
                for(i = 0; i < length; ++i) {
                    palette[i] += data[index] << 24;
                    ++index;
                }
                for(; i < palette_size; ++i) {
                    palette[i] += 255 << 24;
                }
                index += 4;
            }break;
            case KI_CHUNK_TYPE('c', 'G', 'B', 'I'):{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "cGBI | ");
#endif
                index += length + 4;
            }break;
            default:{
#ifdef KERO_IMAGE_VERBOSE
                fprintf(stderr, "OTHR | ");
#endif
                index += length + 4;
            }break;
        }
    }
}

int KI_LoadSoftwareDefaults(kimage_t *image, const char *const filepath) {
    return KI_Load(image, filepath, 2, 1, 0, 3, true);
}

int KI_LoadHardwareDefaults(kimage_t *image, const char *const filepath) {
    return KI_Load(image, filepath, 0, 1, 2, 3, false);
}

void KI_Free(kimage_t *image) {
    if(image->pixels) {
        free(image->pixels);
    }
}

#endif