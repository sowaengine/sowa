#include "audio_stream.hpp"

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "sndfile.h"

#include <vector>

#include "debug.hpp"

struct sf_func_data {
    std::vector<unsigned char> data;
    size_t offset = 0;
};

static sf_count_t sf_func_get_file_len(void *userdata) {
	sf_func_data* func_data = reinterpret_cast<sf_func_data*>(userdata);
    return func_data->data.size();
}

static sf_count_t sf_func_read(void *ptr, sf_count_t count, void *userdata) {
    sf_func_data* func_data = reinterpret_cast<sf_func_data*>(userdata);

    memcpy(ptr, func_data->data.data() + func_data->offset, count);
    func_data->offset += count;
    return count;
}

static sf_count_t sf_func_seek(sf_count_t offset, int whence, void *userdata) {
    sf_func_data* func_data = reinterpret_cast<sf_func_data*>(userdata);

    // whence = 1 -> end to begin
    // whence = 0 -> begin to end

    if(whence == 1) {
        func_data->offset = func_data->data.size() - offset;
    } else {
        func_data->offset = offset;
    }
    return func_data->offset;
}

static sf_count_t sf_func_tell(void *userdata) {
    sf_func_data* func_data = reinterpret_cast<sf_func_data*>(userdata);

    return func_data->offset;
}

static sf_count_t sf_func_write(const void* data, sf_count_t count, void* userdata) {
    sf_func_data* func_data = reinterpret_cast<sf_func_data*>(userdata);

    return 0;
}

namespace sowa {
AudioStream::AudioStream() {
}
AudioStream::~AudioStream() {
	Delete();
}

void AudioStream::Load(unsigned char *data, size_t length) {
	Delete();

	ALenum err, format;
	ALuint buffer;
	SNDFILE *sndfile;
	SF_INFO sfinfo;
	short *membuf;
	sf_count_t num_frames;
	ALsizei num_bytes;

	std::vector<unsigned char> bytes;
	bytes.resize(length);
	memcpy(bytes.data(), data, length);

	SF_VIRTUAL_IO io;
	io.get_filelen = sf_func_get_file_len;
	io.read = sf_func_read;
    io.seek = sf_func_seek;
    io.tell = sf_func_tell;
    io.write = sf_func_write;

    sf_func_data func_data;
    func_data.data = bytes;
    func_data.offset = 0;
	
    sndfile = sf_open_virtual(&io, SFM_READ, &sfinfo, &func_data);

	if (!sndfile) {
		Debug::Error("Failed to open audio file");
		return;
	}

	if (sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT32_MAX / sizeof(short)) / sfinfo.channels) {
		Debug::Error("Bad sample count");
		sf_close(sndfile);
		return;
	}

	format = AL_NONE;
	if (sfinfo.channels == 1) {
		format = AL_FORMAT_MONO16;
	} else if (sfinfo.channels == 2) {
		format = AL_FORMAT_STEREO16;
	} else if (sfinfo.channels == 3) {
		if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT) {
			format = AL_FORMAT_BFORMAT2D_16;
		}
	} else if (sfinfo.channels == 4) {
		if (sf_command(sndfile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT) {
			format = AL_FORMAT_BFORMAT3D_16;
		}
	}
	if (!format) {
		Debug::Error("Unsupported channel count {}", sfinfo.channels);
		sf_close(sndfile);
		return;
	}

	membuf = static_cast<short *>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));
	num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
	if (num_frames < 1) {
		free(membuf);
		sf_close(sndfile);
		Debug::Error("Failed to read samples");
		return;
	}

	num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

	buffer = 0;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

	m_bufferId = buffer;
	free(membuf);
	sf_close(sndfile);

	err = alGetError();
	if (err != AL_NO_ERROR) {
		Debug::Error("AL ERROR: {}", alGetString(err));
		Delete();
	}

	alGenSources(1, &m_sourceId);
	alSourcef(m_sourceId, AL_PITCH, pitch);
	alSourcef(m_sourceId, AL_GAIN, gain);
	alSource3f(m_sourceId, AL_POSITION, position[0], position[1], position[2]);
	alSource3f(m_sourceId, AL_VELOCITY, velocity[0], velocity[1], velocity[2]);
	alSourcei(m_sourceId, AL_LOOPING, loop);
	alSourcei(m_sourceId, AL_BUFFER, m_bufferId);
}

void AudioStream::Play() {
	alSourcePlay(m_sourceId);
}
void AudioStream::Delete() {
	if (m_bufferId != 0) {
		alDeleteBuffers(1, &m_bufferId);
	}
	if (m_sourceId != 0) {
		alDeleteSources(1, &m_sourceId);
	}

	m_bufferId = 0;
	m_sourceId = 0;
}
} // namespace sowa
