#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/reflection/field_type.hpp"
#include "core/serialize/archive.hpp"
#include "resource/audio_clip.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

// Generated WAV fixtures allow exact sample checks; Vorbis output is lossy.
std::vector<u8> makeWav(const std::vector<i16> &samples, i32 sampleRate, i32 channels) {
	const u32 dataBytes = static_cast<u32>(samples.size() * sizeof(i16));
	std::vector<u8> wav;

	auto push32 = [&](u32 value) {
		for (int i = 0; i < 4; ++i)
			wav.push_back(static_cast<u8>((value >> (i * 8)) & 0xFF));
	};
	auto push16 = [&](u16 value) {
		wav.push_back(static_cast<u8>(value & 0xFF));
		wav.push_back(static_cast<u8>((value >> 8) & 0xFF));
	};
	auto pushTag = [&](const char *tag) {
		for (int i = 0; i < 4; ++i)
			wav.push_back(static_cast<u8>(tag[i]));
	};

	pushTag("RIFF");
	push32(36 + dataBytes);
	pushTag("WAVE");
	pushTag("fmt ");
	push32(16);
	push16(1); // PCM
	push16(static_cast<u16>(channels));
	push32(static_cast<u32>(sampleRate));
	push32(static_cast<u32>(sampleRate * channels * 2)); // byte rate
	push16(static_cast<u16>(channels * 2));				 // block align
	push16(16);											 // bits per sample
	pushTag("data");
	push32(dataBytes);

	const u8 *raw = reinterpret_cast<const u8 *>(samples.data());
	wav.insert(wav.end(), raw, raw + dataBytes);
	return wav;
}

struct Fixture {
	std::filesystem::path dir;
	FileSystem fs;
	ResourceRegistry registry;

	Fixture() {
		dir = std::filesystem::temp_directory_path() / "sowa_audio_clip_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);

		fs.MountFolder("res", SOWA_EXAMPLE_DIR);
		fs.MountFolder("tmp", dir);
		registry.SetContext(ResourceContext{nullptr, &fs});
		registry.RegisterType<AudioClip>("AudioClip").Field("path", &AudioClip::path);
	}

	~Fixture() { std::filesystem::remove_all(dir); }

	void Write(const std::string &name, const std::vector<u8> &bytes) {
		std::ofstream out(dir / name, std::ios::binary);
		out.write(reinterpret_cast<const char *>(bytes.data()),
				  static_cast<std::streamsize>(bytes.size()));
	}

	Ref<AudioClip> Load(const std::string &path) {
		Ref<AudioClip> clip = registry.Create<AudioClip>();
		clip->path = path;
		registry.Reload(clip->Id());
		return clip;
	}
};

} // namespace

TEST_CASE("A synthesized WAV decodes to exactly the samples it was built from") {
	Fixture fixture;

	// Full-scale positive, full-scale negative, silence, half scale.
	const std::vector<i16> source{32767, -32768, 0, 16384};
	fixture.Write("exact.wav", makeWav(source, 44100, 1));

	Ref<AudioClip> clip = fixture.Load("tmp://exact.wav");
	REQUIRE(clip->IsLoaded());
	CHECK(clip->SampleRate() == 44100);
	CHECK(clip->ChannelCount() == 1);
	CHECK(clip->FrameCount() == 4);
	REQUIRE(clip->Samples().size() == 4);

	// This is what pins the i16 -> f32 scaling: a wrong constant (32767 vs
	// 32768) or a missed division shows up here and nowhere else.
	CHECK(clip->Samples()[0] == doctest::Approx(32767.f / 32768.f).epsilon(0.001));
	CHECK(clip->Samples()[1] == doctest::Approx(-1.f).epsilon(0.001));
	CHECK(clip->Samples()[2] == doctest::Approx(0.f));
	CHECK(clip->Samples()[3] == doctest::Approx(0.5f).epsilon(0.001));
}

TEST_CASE("Stereo samples are interleaved, left first") {
	Fixture fixture;

	// Left channel silent, right channel full scale, for two frames.
	const std::vector<i16> source{0, 32767, 0, 32767};
	fixture.Write("stereo.wav", makeWav(source, 22050, 2));

	Ref<AudioClip> clip = fixture.Load("tmp://stereo.wav");
	REQUIRE(clip->IsLoaded());
	CHECK(clip->ChannelCount() == 2);
	CHECK(clip->SampleRate() == 22050);
	// Two frames, four samples - the distinction FrameCount exists to make.
	CHECK(clip->FrameCount() == 2);
	CHECK(clip->Samples().size() == 4);

	CHECK(clip->Samples()[0] == doctest::Approx(0.f));
	CHECK(clip->Samples()[1] == doctest::Approx(1.f).epsilon(0.001));
	CHECK(clip->Samples()[2] == doctest::Approx(0.f));
	CHECK(clip->Samples()[3] == doctest::Approx(1.f).epsilon(0.001));
}

TEST_CASE("Duration is frames over sample rate, not samples over sample rate") {
	Fixture fixture;

	// One second of stereo at 8 kHz: 8000 frames, 16000 samples.
	const std::vector<i16> source(16000, 0);
	fixture.Write("second.wav", makeWav(source, 8000, 2));

	Ref<AudioClip> clip = fixture.Load("tmp://second.wav");
	REQUIRE(clip->IsLoaded());
	CHECK(clip->FrameCount() == 8000);
	CHECK(clip->Duration() == doctest::Approx(1.f));
}

TEST_CASE("A real WAV file decodes with the properties its header declares") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://file_example_WAV_1MG.wav");

	REQUIRE(clip->IsLoaded());
	CHECK(clip->ChannelCount() == 2);
	CHECK(clip->SampleRate() == 44100);
	CHECK(clip->FrameCount() == 262094);
	CHECK(clip->Duration() == doctest::Approx(5.943f).epsilon(0.01));
}

TEST_CASE("A real Ogg Vorbis file decodes with the same properties") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://file_example_OGG_1MG.ogg");

	REQUIRE(clip->IsLoaded());
	CHECK(clip->ChannelCount() == 2);
	CHECK(clip->SampleRate() == 44100);
	CHECK(clip->Duration() == doctest::Approx(5.943f).epsilon(0.02));

	// Check peak amplitude to confirm lossy decoding produced audio.
	const std::vector<f32> &samples = clip->Samples();
	REQUIRE(!samples.empty());
	f32 peak = 0.f;
	for (const f32 sample : samples)
		peak = std::fabs(sample) > peak ? std::fabs(sample) : peak;

	CHECK(peak > 0.01f);  // decoded audio, not silence
	CHECK(peak < 1.5f);   // and in range, allowing a little intersample overshoot
}

// Compare equivalent WAV/Vorbis fixtures for channel, interleaving, and sample-rate
// consistency.
TEST_CASE("The WAV and OGG of the same audio agree") {
	Fixture fixture;
	Ref<AudioClip> wav = fixture.Load("res://file_example_WAV_1MG.wav");
	Ref<AudioClip> ogg = fixture.Load("res://file_example_OGG_1MG.ogg");
	REQUIRE(wav->IsLoaded());
	REQUIRE(ogg->IsLoaded());

	CHECK(ogg->ChannelCount() == wav->ChannelCount());
	CHECK(ogg->SampleRate() == wav->SampleRate());
	// Vorbis pads and trims at the edges, so frame counts agree closely rather
	// than exactly.
	CHECK(std::abs(static_cast<i64>(ogg->FrameCount()) -
				   static_cast<i64>(wav->FrameCount())) < 4096);

	// Compared as energy per channel rather than sample by sample: lossy
	// compression changes individual samples, but it preserves loudness, and a
	// channel swap or a scaling error would move these apart immediately.
	const size_t frames = std::min(wav->FrameCount(), ogg->FrameCount());
	const i32 channels = wav->ChannelCount();
	for (i32 channel = 0; channel < channels; ++channel) {
		f64 wavEnergy = 0.0;
		f64 oggEnergy = 0.0;
		for (size_t frame = 0; frame < frames; ++frame) {
			const f32 w = wav->Samples()[frame * channels + channel];
			const f32 o = ogg->Samples()[frame * channels + channel];
			wavEnergy += static_cast<f64>(w) * w;
			oggEnergy += static_cast<f64>(o) * o;
		}
		const f64 wavRms = std::sqrt(wavEnergy / static_cast<f64>(frames));
		const f64 oggRms = std::sqrt(oggEnergy / static_cast<f64>(frames));

		CHECK(wavRms > 0.001);
		CHECK(oggRms == doctest::Approx(wavRms).epsilon(0.15));
	}
}

TEST_CASE("Extension matching ignores case") {
	Fixture fixture;
	fixture.Write("shouty.WAV", makeWav(std::vector<i16>{0, 1000, -1000, 0}, 44100, 1));

	Ref<AudioClip> clip = fixture.Load("tmp://shouty.WAV");
	CHECK(clip->IsLoaded());
	CHECK(clip->FrameCount() == 4);
}

TEST_CASE("An unsupported extension is refused rather than guessed at") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://icon.png");

	CHECK_FALSE(clip->IsLoaded());
	CHECK(clip->FrameCount() == 0);
	CHECK(clip->Duration() == doctest::Approx(0.f));
}

TEST_CASE("A file that lies about its extension fails to decode") {
	Fixture fixture;

	// PNG bytes under a .wav name: the extension picks the decoder, but the
	// decoder still has to accept the bytes.
	const auto png = fixture.fs.Load("res://icon.png");
	REQUIRE(png);
	std::vector<u8> bytes(png.value->Size());
	std::memcpy(bytes.data(), png.value->Data(), png.value->Size());
	fixture.Write("liar.wav", bytes);

	Ref<AudioClip> clip = fixture.Load("tmp://liar.wav");
	CHECK_FALSE(clip->IsLoaded());
}

// A valid header declaring zero frames: dr_wav accepts it and sets the sample
// rate and channel count, then reports no frames read. Without a guard that
// resets everything, the clip would report 44100 Hz mono while holding nothing
// - and IsLoaded() alone would never notice, since samples are empty either way.
TEST_CASE("A valid header with no audio leaves nothing half-set") {
	Fixture fixture;
	fixture.Write("silent.wav", makeWav({}, 44100, 1));

	Ref<AudioClip> clip = fixture.Load("tmp://silent.wav");
	CHECK_FALSE(clip->IsLoaded());
	CHECK(clip->SampleRate() == 0);
	CHECK(clip->ChannelCount() == 0);
	CHECK(clip->FrameCount() == 0);
	CHECK(clip->Duration() == doctest::Approx(0.f));
}

TEST_CASE("A missing file leaves the clip empty") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://nonexistent.wav");

	CHECK_FALSE(clip->IsLoaded());
	CHECK(clip->Samples().empty());
	CHECK(clip->SampleRate() == 0);
}

TEST_CASE("Reload drops the previous decode") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://file_example_WAV_1MG.wav");
	REQUIRE(clip->IsLoaded());

	clip->path = "res://nonexistent.wav";
	fixture.registry.Reload(clip->Id());

	CHECK_FALSE(clip->IsLoaded());
	CHECK(clip->FrameCount() == 0);
	CHECK(clip->SampleRate() == 0);
}

TEST_CASE("Only path is described, and the samples never reach the document") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://file_example_WAV_1MG.wav");

	REQUIRE(clip->Fields().size() == 1);
	CHECK(clip->Fields()[0].name == "path");
	CHECK(clip->Fields()[0].type == FieldType::String);

	Archive saving = Archive::Save();
	clip->Serialize(saving);
	const std::string yaml = saving.ToString();
	CHECK(yaml.find("file_example_WAV_1MG.wav") != std::string::npos);
	CHECK(yaml.find("samples") == std::string::npos);
	CHECK(yaml.find("sampleRate") == std::string::npos);
}

TEST_CASE("A clip round-trips through an archive") {
	Fixture fixture;
	Ref<AudioClip> clip = fixture.Load("res://file_example_OGG_1MG.ogg");
	const ResourceID id = clip->Id();

	Archive saving = Archive::Save();
	clip->Serialize(saving);

	fixture.registry.Remove(id);
	Ref<Resource> restored = fixture.registry.Create("AudioClip", id);
	REQUIRE(restored != nullptr);
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);

	Ref<AudioClip> typed = fixture.registry.Get<AudioClip>(id);
	REQUIRE(typed != nullptr);
	CHECK(typed->path == "res://file_example_OGG_1MG.ogg");
	// Deserialization sets fields; Reload is what acts on them.
	CHECK_FALSE(typed->IsLoaded());

	fixture.registry.Reload(id);
	CHECK(typed->IsLoaded());
}
