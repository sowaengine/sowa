#include "audio_stream.hxx"

#include "data/file_buffer.hxx"
#include "servers/file_server.hxx"

#include "AL/alext.h"
#include "sndfile.h"

struct sf_func_data {
	std::vector<unsigned char> data;
	size_t offset = 0;
};

static sf_count_t sf_func_get_file_len(void *userdata) {

	sf_func_data *func_data = reinterpret_cast<sf_func_data *>(userdata);
	return func_data->data.size();
}

static sf_count_t sf_func_read(void *ptr, sf_count_t count, void *userdata) {

	sf_func_data *func_data = reinterpret_cast<sf_func_data *>(userdata);
	if (func_data->offset + count > func_data->data.size()) {
		count = func_data->data.size() - func_data->offset;
	}

	memcpy(ptr, func_data->data.data() + func_data->offset, count);
	func_data->offset += count;
	return count;
}

static sf_count_t sf_func_seek(sf_count_t offset, int whence, void *userdata) {

	sf_func_data *func_data = reinterpret_cast<sf_func_data *>(userdata);

	if (whence == SEEK_SET) {
		func_data->offset = offset;
	} else if (whence == SEEK_CUR) {
		func_data->offset += offset;
	} else if (whence == SEEK_END) {
		func_data->offset = func_data->data.size() - offset;
	}

	return func_data->offset;
}

static sf_count_t sf_func_tell(void *userdata) {

	sf_func_data *func_data = reinterpret_cast<sf_func_data *>(userdata);

	return func_data->offset;
}

static sf_count_t sf_func_write(const void *data, sf_count_t count, void *userdata) {

	sf_func_data *func_data = reinterpret_cast<sf_func_data *>(userdata);
	return 0;
}

AudioStream::~AudioStream() {
	Delete();
}

ErrorCode AudioStream::Load(const char *path) {
	Delete();
	file_buffer data;
	if (ErrorCode err = FileServer::get().ReadFileBytes(path, data); err != OK) {
		return err;
	}

	ALenum err, format;
	ALuint buffer;
	SNDFILE *sndfile;
	SF_INFO sfinfo;
	short *membuf;
	sf_count_t num_frames;
	ALsizei num_bytes;

	std::vector<unsigned char> bytes;
	bytes.resize(data.size());
	memcpy(bytes.data(), data.data(), data.size());

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
		Utils::Error("Failed to open audio file {}", path);
		return ERR_FAILED;
	}

	if (sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT32_MAX / sizeof(short)) / sfinfo.channels) {
		Utils::Error("Bad sample count");
		sf_close(sndfile);
		return ERR_FAILED;
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
		Utils::Error("Unsupported channel count {}", sfinfo.channels);
		sf_close(sndfile);
		return ERR_FAILED;
	}

	membuf = static_cast<short *>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));
	num_frames = sf_readf_short(sndfile, membuf, sfinfo.frames);
	if (num_frames < 1) {
		free(membuf);
		sf_close(sndfile);
		Utils::Error("Failed to read samples");
		return ERR_FAILED;
	}

	num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

	buffer = 0;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);

	m_id = buffer;
	free(membuf);
	sf_close(sndfile);

	err = alGetError();
	if (err != AL_NO_ERROR) {
		Utils::Error("AL ERROR: {}", alGetString(err));
		Delete();
	}

	return OK;
}

void AudioStream::Delete() {
	if (m_id != 0) {
		alDeleteBuffers(1, &m_id);
	}

	m_id = 0;
}