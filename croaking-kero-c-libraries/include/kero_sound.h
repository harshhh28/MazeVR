/*
int thread_error = pthread_create(&sound_data.thread_id, NULL, &KSoundInit, (void*)&sound_data);
    if(thread_error != 0) {
        fprintf(stderr, "Failed to create sound thread: %d\n", thread_error);
        return -1;
    }
    
    // Block until sound thread is ready
    while(!sound_data.flags.thread_initialized);
*/

#ifndef KERO_SOUND_H

#ifdef __cplusplus
extern "C"{
#endif
    
#include "kero_math.h"
#include <alsa/asoundlib.h>
#include <pthread.h>
    
    typedef struct {
        uint32_t* frame;
        uint32_t frame_count;
        uint32_t frame_current;
        uint32_t loop_start_frame;
        struct {
            unsigned int active : 1;
            unsigned int looping : 1;
        } flags;
    } ksound_t;
    
#ifndef KERO_SOUND_MAX_CHANNELS
#define KERO_SOUND_MAX_CHANNELS 32
#endif
    
    typedef struct {
        snd_pcm_t *pcm_handle;
        snd_pcm_hw_params_t *pcm_hwparams;
        snd_pcm_stream_t pcm_stream;
        char *pcm_name;
        int rate;
        int rate_exact;
        int rate_dir;
        int periods;
        snd_pcm_uframes_t period_size;
        int64_t buffer_size;
        pthread_t thread_id;
        struct {
            unsigned int thread_initialized : 1;
            unsigned int kill_thread : 1;
        } flags;
        ksound_t sfx[KERO_SOUND_MAX_CHANNELS];
    } ksound_data_t;
    
    void *KSoundInit(void *sound_data_v) {
        ksound_data_t *sound_data = (ksound_data_t*)sound_data_v;
        
        for(int i = 0; i < KERO_SOUND_MAX_CHANNELS; ++i) {
            sound_data->sfx[i].frame = NULL;
            sound_data->sfx[i].frame_count = sound_data->sfx[i].frame_current = sound_data->sfx[i].flags.active = sound_data->sfx[i].flags.looping = 0;
        }
        
        sound_data->flags.kill_thread = false;
        
        sound_data->flags.thread_initialized = true;
        
        sound_data->pcm_handle = NULL;
        sound_data->pcm_hwparams = NULL;
        sound_data->pcm_name = NULL;
        sound_data->pcm_stream = SND_PCM_STREAM_PLAYBACK;
        sound_data->rate = sound_data->rate_exact = 44100;
        sound_data->rate_dir = sound_data->buffer_size = sound_data->thread_id = 0;
        sound_data->periods = 16;
        sound_data->period_size = 8192;
        
        sound_data->pcm_name = strdup("default");
        snd_pcm_hw_params_alloca(&sound_data->pcm_hwparams);
        
        if(snd_pcm_open(&sound_data->pcm_handle, sound_data->pcm_name, sound_data->pcm_stream, 0) < 0) {
            fprintf(stderr, "Error opening PCM device %s\n", sound_data->pcm_name);
            return NULL;
        }
        if(snd_pcm_hw_params_any(sound_data->pcm_handle, sound_data->pcm_hwparams) < 0) {
            fprintf(stderr, "Cannot configure this pcm device.\n");
            return NULL;
        }
        
        sound_data->buffer_size = sound_data->rate / 60;
        
        if(snd_pcm_hw_params_set_access(sound_data->pcm_handle, sound_data->pcm_hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
            fprintf(stderr, "Error setting sound access.\n");
            return NULL;
        }
        if(snd_pcm_hw_params_set_format(sound_data->pcm_handle, sound_data->pcm_hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
            fprintf(stderr, "Error setting sound format.\n");
            return NULL;
        }
        sound_data->rate_exact = sound_data->rate;
        if(snd_pcm_hw_params_set_rate_near(sound_data->pcm_handle, sound_data->pcm_hwparams, &sound_data->rate_exact, 0) < 0) {
            fprintf(stderr, "Error setting sound sound_data->rate.\n");
            return NULL;
        }
        if(sound_data->rate != sound_data->rate_exact) {
            fprintf(stderr, "Warning: Sound sound_data->rate %dHz is not supported by the sound card.\nUsing %dHz instead.\n", sound_data->rate, sound_data->rate_exact);
            sound_data->rate_dir = sound_data->rate_exact > sound_data->rate ? 1 : -1;
        }
        if(snd_pcm_hw_params_set_channels(sound_data->pcm_handle, sound_data->pcm_hwparams, 2) < 0) {
            fprintf(stderr, "Error setting sound channels.\n");
            return NULL;
        }
        if(snd_pcm_hw_params_set_periods(sound_data->pcm_handle, sound_data->pcm_hwparams, sound_data->periods, 0) < 0) {
            fprintf(stderr, "Error setting sound sound_data->periods.\n");
            return NULL;
        }
        if(snd_pcm_hw_params_set_buffer_size(sound_data->pcm_handle, sound_data->pcm_hwparams, sound_data->buffer_size) < 0) {
            fprintf(stderr, "Error setting sound buffer size.\n");
            return NULL;
        }
        if(snd_pcm_hw_params(sound_data->pcm_handle, sound_data->pcm_hwparams) < 0) {
            fprintf(stderr, "Error setting sound HW params.\n");
            return NULL;
        }
        
        int frame_written_count;
        snd_pcm_uframes_t frames_per_packet = 1000;
        int buffer_underruns = 0;
        int last_buffer_underruns = 0;
        int loops_since_buffer_underrun = 0;
        
        while(!sound_data->flags.kill_thread) {
            last_buffer_underruns = buffer_underruns;
            if(buffer_underruns != 0) {
                buffer_underruns = 0;
                frames_per_packet += 20;
            }
            int32_t sound_packet[frames_per_packet];
            for(int i = 0; i < frames_per_packet; ++i) {
                int sound_count = 0;
                ksound_t* sounds[KERO_SOUND_MAX_CHANNELS];
                for(int i = 0; i < KERO_SOUND_MAX_CHANNELS; ++i) {
                    if(sound_data->sfx[i].flags.active) {
                        if(++sound_data->sfx[i].frame_current >= sound_data->sfx[i].frame_count) {
                            if(sound_data->sfx[i].flags.looping) {
                                sound_data->sfx[i].frame_current = sound_data->sfx[i].loop_start_frame;
                            }
                            else {
                                sound_data->sfx[i].flags.active = false;
                                continue;
                            }
                        }
                        sounds[sound_count++] = &sound_data->sfx[i];
                    }
                }
                
                int32_t l;
                int32_t r;
                
                float la, lb = 0, ra, rb = 0, gain;
                
                for(int j = 0; j < sound_count; ++j) {
                    la = (float)(int16_t)sounds[j]->frame[sounds[j]->frame_current] / 32768.f;
                    ra = (float)(int16_t)(sounds[j]->frame[sounds[j]->frame_current] >> 16) / 32768.f;
                    
                    gain = (float)(Max(2, 5-j)) / 5.f;
                    
                    lb += la * gain;
                    rb += ra * gain;
                }
                
                l = Sign(lb) * Min(1.f, Absolute(lb)) * 32767.f;
                r = Sign(rb) * Min(1.f , Absolute(rb)) * 32767.f;
                
                sound_packet[i] = l + (r << 16);
            }
            
            if((frame_written_count = snd_pcm_writei(sound_data->pcm_handle, &sound_packet, frames_per_packet)) < 0) {
                snd_pcm_prepare(sound_data->pcm_handle);
                fprintf(stderr, "Sound buffer underrun!\n");
                ++buffer_underruns;
            }
            
            if(buffer_underruns == last_buffer_underruns) {
                ++loops_since_buffer_underrun;
                if(loops_since_buffer_underrun > 60 && frames_per_packet > 300) {
                    frames_per_packet -= 20;
                    loops_since_buffer_underrun = 0;
                }
            }
        }
        
        return NULL;
    }
    
    bool KSoundLoad(ksound_t *sound, const char *const filename) {
        memset(sound, 0, sizeof(ksound_t));
        FILE* infile = fopen(filename, "rb");
        if(!infile) {
            fprintf(stderr, "Failed to open \"%s\"\n", filename);
            return false;
        }
        uint32_t chunk_id;
        if(fread(&chunk_id, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(chunk_id != 0x46464952) {
            fprintf(stderr, "Chunk ID incorrect: %u, should be: %u\n", chunk_id, 0x46464952);
            fclose(infile);
            return false;
        }
        uint32_t chunk_size;
        if(fread(&chunk_size, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        uint32_t file_format;
        if(fread(&file_format, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(file_format != 0x45564157) {
            fprintf(stderr, "File format incorrect: %u, should be: %u\n", file_format, 0x45564157);
            fclose(infile);
            return false;
        }
        uint32_t format;
        if(fread(&format, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(format != 0x20746d66) {
            fprintf(stderr, "Format incorrect: %u, should be: %u\n", format, 0x20746d66);
            fclose(infile);
            return false;
        }
        uint32_t sub_chunk_1_size;
        if(fread(&sub_chunk_1_size, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(sub_chunk_1_size != 16) { // PCM
            fprintf(stderr, "Sub chunk 1 size incorrect: %u, should be: %u\n", sub_chunk_1_size, 16);
            fclose(infile);
            return false;
        }
        uint16_t audio_format;
        if(fread(&audio_format, sizeof(uint16_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(audio_format != 1) { // PCM - Linear quantization
            fprintf(stderr, "Audio format incorrect: %u, should be: %u\nMeans the audio is compressed when it shouldn't be!\n", audio_format, 16);
            fclose(infile);
            return false;
        }
        uint16_t channels;
        if(fread(&channels, sizeof(uint16_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(channels != 2) { // Not stereo
            fprintf(stderr, "Number of channels incorrect. Is %u, should be stereo (2).\n", channels);
            fclose(infile);
            return false;
        }
        uint32_t sample_rate;
        if(fread(&sample_rate, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(sample_rate != 44100) { // PCM
            fprintf(stderr, "Sample rate incorrect: %u, should be: 44100\n", sample_rate);
            fclose(infile);
            return false;
        }
        uint32_t byte_rate;
        if(fread(&byte_rate, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        /*if(byte_rate != 44100) { // PCM
            fprintf(stderr, "Byte rate incorrect: %u, should be: 44100\n", sample_rate);
            fclose(infile);
            return false;
        }*/
        uint16_t block_align;
        if(fread(&block_align, sizeof(uint16_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        uint16_t bits_per_sample;
        if(fread(&bits_per_sample, sizeof(uint16_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(bits_per_sample != 16) { // Not stereo
            fprintf(stderr, "Bits per sample incorrect: %u, should be 16.\n", channels);
            fclose(infile);
            return false;
        }
        uint32_t sub_chunk_2_id;
        if(fread(&sub_chunk_2_id, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        if(sub_chunk_2_id != 0x61746164) {
            fprintf(stderr, "Sub chunk 2 ID incorrect: %u, should be: %u\n", sub_chunk_2_id, 0x61746164);
            fclose(infile);
            return false;
        }
        uint32_t sub_chunk_2_size;
        if(fread(&sub_chunk_2_size, sizeof(uint32_t), 1, infile) != 1) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            return false;
        }
        sound->frame_count = sub_chunk_2_size / 4; // Fix this later when I support variable bit rates, etc.
        sound->frame = (uint32_t*)malloc(sizeof(uint32_t)*sound->frame_count);
        size_t read_length = fread(sound->frame, sizeof(uint32_t), sound->frame_count, infile);
        if(ferror(infile) != 0 || read_length != sound->frame_count) {
            fprintf(stderr, "fread error.\n");
            fclose(infile);
            free(sound->frame);
            return false;
        }
        fclose(infile);
        sound->flags.active = true;
        return true;
    }
    
    void KSound_Play(ksound_data_t *data, ksound_t *sound) {
        int selected_sound = 0;
        int non_looping_sound = -1;
        for(; selected_sound < KERO_SOUND_MAX_CHANNELS; ++selected_sound) {
            if(!data->sfx[selected_sound].flags.active) {
                break;
            }
            if(!data->sfx[selected_sound].flags.looping) {
                non_looping_sound = selected_sound;
            }
        }
        if(selected_sound == KERO_SOUND_MAX_CHANNELS) {
            selected_sound = non_looping_sound;
        }
        if(selected_sound != -1) {
            data->sfx[selected_sound] = *sound;
        }
    }
    
#ifdef __cplusplus
}
#endif

#define KERO_SOUND_H
#endif