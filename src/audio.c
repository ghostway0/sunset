#include <stddef.h>
#include <stdint.h>

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

#include "sunset/io.h"

typedef struct PCMParams {
    // a multiple of the frame size
    size_t chunk_size;
    snd_pcm_format_t format;
    size_t channel_count;
} PCMParams;

typedef struct PCMStream {
    snd_pcm_t *handle;
} PCMStream;

void pcmstream_init(PCMStream *stream_out) {}

PCMParams pcmstream_get_params(PCMStream const *stream) {}

Writer pcmstream_writer(PCMStream *stream) {}

void relay_all(
        Reader *reader, Writer *writer, size_t chunk_size, size_t max) {
    uint8_t buffer[chunk_size];

    for (size_t i = 0; max == SIZE_MAX ? true : i < max; i++) {
        ssize_t read = reader_read(reader, chunk_size, buffer);

        if (read < 0) {
            break;
        }

        writer_write(writer, buffer, read);
    }
}

typedef enum SoundFormat {
    SOUND_FORMAT_PCM,
} SoundFormat;

typedef struct Sound {
    // format
    // data
} Sound;

// 1. write something to stream a pcm from a reader

// 2. write something to get a file path of a wav file,
// store data in a `Sound`

// 3. and something to stream it too

typedef struct WavHeader {
    char riff_id[4]; // "RIFF"
    uint32_t riff_size;
    char wave_id[4]; // "WAVE"
    char fmt_id[4]; // "fmt "
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_id[4]; // "data"
    uint32_t data_size;
} __attribute__((packed)) WavHeader;

int load_wav_from_file(char const *file_path, Sound *sound_out) {
    Reader reader = reader_open_file(file_path);
    if (reader.read == NULL) {
        return -1; // Could not open file
    }

    WavHeader header;
    if (reader_read(&reader, sizeof(header), &header) != sizeof(header)) {
        reader_close(&reader);
        return -2; // Could not read WAV header
    }

    if (strncmp(header.riff_id, "RIFF", 4) != 0
            || strncmp(header.wave_id, "WAVE", 4) != 0
            || strncmp(header.fmt_id, "fmt ", 4) != 0
            || strncmp(header.data_id, "data", 4) != 0) {
        reader_close(&reader);
        return -3; // Invalid WAV file format
    }

    if (header.audio_format != 1) { // Only support PCM format
        reader_close(&reader);
        return -4; // Unsupported WAV format (not PCM)
    }

    size_t data_size = header.data_size;
    void *data = malloc(data_size);
    if (data == NULL) {
        reader_close(&reader);
        return -5; // Memory allocation failed
    }

    if (reader_read(&reader, data_size, data) != data_size) {
        free(data);
        reader_close(&reader);
        return -6; // Could not read WAV data
    }

    sound_out->format = SOUND_FORMAT_WAV;
    sound_out->data = data;
    sound_out->data_size = data_size;
    sound_out->params.channel_count = header.num_channels;
    sound_out->params.sample_rate = header.sample_rate;
    sound_out->params.format = wav_to_alsa_format(header.bits_per_sample);
    sound_out->params.chunk_size = 0; // Not relevant here

    reader_close(&reader);
    return 0;
}
