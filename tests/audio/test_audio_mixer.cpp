#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "audio/audio_mixer.hpp"
#include "audio/audio_source.hpp"
#include "core/filesystem/file_system.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"
#include "resource/audio_clip.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

std::vector<u8> makeWav(const std::vector<i16> &samples, i32 sampleRate, i32 channels) {
	const u32 dataBytes = static_cast<u32>(samples.size() * sizeof(i16));
	std::vector<u8> wav;

	auto push32 = [&](u32 value) {
		for (int i = 0; i < 4; ++i) {
			wav.push_back(static_cast<u8>((value >> (i * 8)) & 0xFF));
		}
	};
	auto push16 = [&](u16 value) {
		wav.push_back(static_cast<u8>(value & 0xFF));
		wav.push_back(static_cast<u8>((value >> 8) & 0xFF));
	};
	auto pushTag = [&](const char *tag) {
		for (int i = 0; i < 4; ++i) {
			wav.push_back(static_cast<u8>(tag[i]));
		}
	};

	pushTag("RIFF");
	push32(36 + dataBytes);
	pushTag("WAVE");
	pushTag("fmt ");
	push32(16);
	push16(1);
	push16(static_cast<u16>(channels));
	push32(static_cast<u32>(sampleRate));
	push32(static_cast<u32>(sampleRate * channels * 2));
	push16(static_cast<u16>(channels * 2));
	push16(16);
	pushTag("data");
	push32(dataBytes);

	const u8 *raw = reinterpret_cast<const u8 *>(samples.data());
	wav.insert(wav.end(), raw, raw + dataBytes);
	return wav;
}

f32 energy(const std::vector<f32> &buffer) {
	f32 sum = 0.f;
	for (f32 sample : buffer) {
		sum += sample * sample;
	}
	return sum;
}

f32 energyChannel(const std::vector<f32> &buffer, u32 channel, u32 channels) {
	f32 sum = 0.f;
	const u32 frames = static_cast<u32>(buffer.size() / channels);
	for (u32 i = 0; i < frames; ++i) {
		const f32 sample = buffer[i * channels + channel];
		sum += sample * sample;
	}
	return sum;
}

constexpr f32 kCenterPan = 0.70710678f;

i32 nonzeroFrames(const std::vector<f32> &buffer, u32 channels) {
	i32 count = 0;
	const u32 frames = static_cast<u32>(buffer.size() / channels);
	for (u32 i = 0; i < frames; ++i) {
		if (std::fabs(buffer[i * channels]) > 0.05f) {
			++count;
		}
	}
	return count;
}

struct Fixture {
	std::filesystem::path dir;
	FileSystem fs;
	ResourceRegistry resources;
	World world;
	AudioMixer mixer;

	Fixture() {
		dir = std::filesystem::temp_directory_path() / "sowa_audio_mixer_test";
		std::filesystem::remove_all(dir);
		std::filesystem::create_directories(dir);
		fs.MountFolder("tmp", dir);
		resources.SetContext(ResourceContext{nullptr, &fs});
		resources.RegisterType<AudioClip>("AudioClip").Field("path", &AudioClip::path);
		mixer.Init(world, resources);
	}

	~Fixture() {
		std::filesystem::remove_all(dir);
	}

	void Write(const std::string &name, const std::vector<u8> &bytes) {
		std::ofstream out(dir / name, std::ios::binary);
		out.write(reinterpret_cast<const char *>(bytes.data()),
			static_cast<std::streamsize>(bytes.size()));
	}

	Ref<AudioClip> Load(const std::string &name, const std::vector<i16> &samples, i32 sampleRate,
		i32 channels) {
		Write(name, makeWav(samples, sampleRate, channels));
		Ref<AudioClip> clip = resources.Create<AudioClip>();
		clip->path = "tmp://" + name;
		resources.Reload(clip->Id());
		return clip;
	}

	Entity Speaker(ResourceID clip, f32 gain = 1.f, f32 pitch = 1.f, bool loop = false,
		bool autoplay = false) {
		Entity entity = world.CreateEntity();
		AudioSource source;
		source.clip = clip;
		source.gain = gain;
		source.pitch = pitch;
		source.loop = loop;
		source.autoplay = autoplay;
		entity.Add(source);
		return entity;
	}

	std::vector<f32> Mix(u32 frames, u32 rate = 44100, u32 channels = 2) {
		std::vector<f32> buffer(static_cast<size_t>(frames) * channels, 0.f);
		mixer.Mix(buffer.data(), frames, rate, channels);
		return buffer;
	}
};

} // namespace

TEST_CASE("Mixer shutdown releases playback snapshots and stops voices") {
	Fixture f;
	auto clip = f.Load("shutdown.wav", std::vector<i16>(16, 16384), 44100, 1);
	REQUIRE(clip->IsLoaded());
	const auto id = clip->Id();
	std::weak_ptr<const std::vector<f32>> samples = clip->SampleStorage();
	Entity speaker = f.Speaker(id, 1.f, 1.f, true);
	f.mixer.Play(speaker);
	REQUIRE(energy(f.Mix(4)) > 0.f);
	REQUIRE(f.resources.Remove(id));
	clip.reset();
	REQUIRE_FALSE(samples.expired());

	f.mixer.SetPaused(true);
	f.mixer.Shutdown();
	f.mixer.Shutdown();
	CHECK(samples.expired());
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	CHECK_FALSE(f.mixer.IsPaused());
	f.mixer.Sync();
	CHECK(energy(f.Mix(4)) == 0.f);

}

TEST_CASE("Mix of a playing clip has energy") {
	Fixture f;
	const std::vector<i16> samples(16, 16384);
	Ref<AudioClip> clip = f.Load("tone.wav", samples, 44100, 1);
	REQUIRE(clip->IsLoaded());

	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(8);
	CHECK(energy(buffer) > 0.f);
	CHECK(f.mixer.IsPlaying(speaker));
}

TEST_CASE("Mix is silent when nothing is playing") {
	Fixture f;
	const std::vector<i16> samples(16, 16384);
	Ref<AudioClip> clip = f.Load("quiet.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Sync();
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(8)) == doctest::Approx(0.f));
}

TEST_CASE("pause stops Mix energy and resume continues") {
	Fixture f;
	const std::vector<i16> samples(32, 16384);
	Ref<AudioClip> clip = f.Load("pause.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	CHECK(energy(f.Mix(4)) > 0.f);

	f.mixer.Pause(speaker);
	CHECK(f.mixer.IsPaused(speaker));
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));

	f.mixer.Play(speaker);
	CHECK(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(4)) > 0.f);
}

TEST_CASE("SetPaused silences the mixer without advancing or changing voices") {
	Fixture f;
	std::vector<i16> samples(8, 0);
	samples[0] = 16384;
	Ref<AudioClip> clip = f.Load("mixer_pause.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);

	f.mixer.SetPaused(true);
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));
	CHECK(f.mixer.IsPlaying(speaker));

	// The first sample is the only loud one: it is still next after unpausing.
	f.mixer.SetPaused(false);
	const std::vector<f32> resumed = f.Mix(1);
	CHECK(std::fabs(resumed[0]) > 0.05f);
}

TEST_CASE("stop resets and play starts from the beginning") {
	Fixture f;
	std::vector<i16> samples(8, 0);
	samples[0] = 16384;
	Ref<AudioClip> clip = f.Load("start.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	CHECK(energy(f.Mix(8)) > 0.f);
	CHECK_FALSE(f.mixer.IsPlaying(speaker));

	f.mixer.Stop(speaker);
	f.mixer.Play(speaker);
	const std::vector<f32> again = f.Mix(1);
	CHECK(std::fabs(again[0]) > 0.05f);
}

TEST_CASE("play while Playing does not restart") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("noop.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	(void)f.Mix(6);
	f.mixer.Play(speaker);
	CHECK(nonzeroFrames(f.Mix(4), 2) == 2);
}

TEST_CASE("loop keeps Mix energy past the clip end") {
	Fixture f;
	const std::vector<i16> samples(4, 16384);
	Ref<AudioClip> clip = f.Load("loop.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id(), 1.f, 1.f, true);
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(12);
	CHECK(nonzeroFrames(buffer, 2) == 12);
	CHECK(f.mixer.IsPlaying(speaker));
}

TEST_CASE("a clip without loop stops at the end") {
	Fixture f;
	const std::vector<i16> samples(4, 16384);
	Ref<AudioClip> clip = f.Load("once.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(8);
	CHECK(nonzeroFrames(buffer, 2) == 4);
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));
}

TEST_CASE("pitch zero does not advance or mix") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("pitch0.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id(), 1.f, 0.f);
	f.mixer.Play(speaker);
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));
	CHECK(f.mixer.IsPlaying(speaker));

	speaker.Get<AudioSource>()->pitch = 1.f;
	f.mixer.Sync();
	CHECK(energy(f.Mix(4)) > 0.f);
}

TEST_CASE("autoplay starts on first Sync and does not re-fire") {
	Fixture f;
	const std::vector<i16> samples(4, 16384);
	Ref<AudioClip> clip = f.Load("auto.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id(), 1.f, 1.f, false, true);
	f.mixer.Sync();
	CHECK(f.mixer.IsPlaying(speaker));
	(void)f.Mix(8);
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	f.mixer.Sync();
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));
}

TEST_CASE("changing clip while Playing restarts at 0") {
	Fixture f;
	std::vector<i16> first(8, 16384);
	std::vector<i16> second(8, 0);
	second[0] = 8192;
	Ref<AudioClip> clipA = f.Load("a.wav", first, 44100, 1);
	Ref<AudioClip> clipB = f.Load("b.wav", second, 44100, 1);
	Entity speaker = f.Speaker(clipA->Id());
	f.mixer.Play(speaker);
	(void)f.Mix(4);

	speaker.Get<AudioSource>()->clip = clipB->Id();
	f.mixer.Sync();
	CHECK(f.mixer.IsPlaying(speaker));
	const std::vector<f32> buffer = f.Mix(1);
	CHECK(buffer[0] == doctest::Approx((8192.f / 32768.f) * kCenterPan).epsilon(0.01));
}

TEST_CASE("a missing clip mixes silence without panic") {
	Fixture f;
	AudioSource source;
	source.clip.value = 1;
	Entity speaker = f.world.CreateEntity();
	speaker.Add(source);
	f.mixer.Play(speaker);
	CHECK(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(8)) == doctest::Approx(0.f));
}

TEST_CASE("removing AudioSource drops the voice") {
	Fixture f;
	const std::vector<i16> samples(16, 16384);
	Ref<AudioClip> clip = f.Load("drop.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	CHECK(energy(f.Mix(2)) > 0.f);
	speaker.Remove<AudioSource>();
	f.mixer.Sync();
	CHECK_FALSE(f.mixer.IsPlaying(speaker));
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));
}

TEST_CASE("mono clip is written to both stereo channels") {
	Fixture f;
	const std::vector<i16> samples(4, 16384);
	Ref<AudioClip> clip = f.Load("mono.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(1);
	REQUIRE(buffer.size() == 2);
	CHECK(buffer[0] == doctest::Approx(buffer[1]));
	CHECK(std::fabs(buffer[0]) > 0.05f);
}

TEST_CASE("gain scales Mix amplitude") {
	Fixture f;
	const std::vector<i16> samples(4, 16384);
	Ref<AudioClip> clip = f.Load("gain.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id(), 0.5f);
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(1);
	CHECK(buffer[0] == doctest::Approx(0.5f * (16384.f / 32768.f) * kCenterPan).epsilon(0.01));
}

TEST_CASE("pan -1 puts energy on the left") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("panl.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	speaker.Get<AudioSource>()->pan = -1.f;
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(4);
	CHECK(energyChannel(buffer, 0, 2) > 0.f);
	CHECK(energyChannel(buffer, 1, 2) == doctest::Approx(0.f));
}

TEST_CASE("pan +1 puts energy on the right") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("panr.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	speaker.Get<AudioSource>()->pan = 1.f;
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(4);
	CHECK(energyChannel(buffer, 1, 2) > 0.f);
	CHECK(energyChannel(buffer, 0, 2) == doctest::Approx(0.f));
}

TEST_CASE("spatial source on +x is right-heavy") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("spat.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	AudioSource *src = speaker.Get<AudioSource>();
	src->spatial = true;
	src->maxDistance = 1000.f;
	speaker.Add<Transform2D>();
	speaker.Get<Transform2D>()->position = Vector2(500.f, 0.f);
	f.world.Progress(0.f);
	f.mixer.SetListener(Vector2(0.f, 0.f));
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(4);
	CHECK(energyChannel(buffer, 1, 2) > energyChannel(buffer, 0, 2));
	CHECK(energyChannel(buffer, 0, 2) > 0.f);
}

TEST_CASE("spatial past maxDistance is silent") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("far.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	AudioSource *src = speaker.Get<AudioSource>();
	src->spatial = true;
	src->maxDistance = 100.f;
	speaker.Add<Transform2D>();
	speaker.Get<Transform2D>()->position = Vector2(500.f, 0.f);
	f.world.Progress(0.f);
	f.mixer.SetListener(Vector2(0.f, 0.f));
	f.mixer.Play(speaker);
	CHECK(energy(f.Mix(4)) == doctest::Approx(0.f));
}

TEST_CASE("spatial without Transform2D plays centred") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("noxf.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	speaker.Get<AudioSource>()->spatial = true;
	f.mixer.SetListener(Vector2(0.f, 0.f));
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(1);
	CHECK(buffer[0] == doctest::Approx(buffer[1]));
	CHECK(std::fabs(buffer[0]) > 0.05f);
}

TEST_CASE("spatial without a listener plays centred") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("nolisten.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	speaker.Get<AudioSource>()->spatial = true;
	speaker.Add<Transform2D>();
	speaker.Get<Transform2D>()->position = Vector2(500.f, 0.f);
	f.world.Progress(0.f);
	f.mixer.ClearListener();
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(1);
	CHECK(buffer[0] == doctest::Approx(buffer[1]));
	CHECK(energy(buffer) > 0.f);
}

TEST_CASE("listener rotation 180 flips spatial pan") {
	Fixture f;
	const std::vector<i16> samples(8, 16384);
	Ref<AudioClip> clip = f.Load("rot.wav", samples, 44100, 1);
	Entity speaker = f.Speaker(clip->Id());
	AudioSource *src = speaker.Get<AudioSource>();
	src->spatial = true;
	src->maxDistance = 1000.f;
	speaker.Add<Transform2D>();
	speaker.Get<Transform2D>()->position = Vector2(500.f, 0.f);
	f.world.Progress(0.f);
	f.mixer.SetListener(Vector2(0.f, 0.f), 180.f);
	f.mixer.Play(speaker);
	const std::vector<f32> buffer = f.Mix(4);
	CHECK(energyChannel(buffer, 0, 2) > energyChannel(buffer, 1, 2));
}

TEST_CASE("Audio hot reload keeps the previous sample snapshot alive until Sync") {
	Fixture f;
	auto clip = f.Load("hot.wav", std::vector<i16>(32, 16384), 44100, 1);
	Entity speaker = f.Speaker(clip->Id(), 1, 1, true);
	f.mixer.Sync();
	f.mixer.Play(speaker);
	const auto oldSamples = clip->SampleStorage();
	f.Write("hot.wav", makeWav(std::vector<i16>(32, -16384), 44100, 1));
	f.resources.Reload(clip->Id());
	CHECK(oldSamples != clip->SampleStorage());
	CHECK(f.Mix(1)[0] > 0);
	f.mixer.Sync();
	CHECK(f.Mix(1)[0] < 0);
}
