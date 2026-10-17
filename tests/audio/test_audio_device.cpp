#include <doctest/doctest.h>

#include "audio/audio_device.hpp"

using namespace sowa;

TEST_CASE("MiniAudio null backend opens stereo playback") {
	Scope<AudioDevice> device = CreateAudioDevice(AudioAPI::MiniAudio);
	REQUIRE(device != nullptr);

	AudioDeviceConfig config;
	config.nullDevice = true;
	REQUIRE(device->Init(config));
	CHECK(device->IsInitialized());
	CHECK(device->SampleRate() > 0);
	CHECK(device->ChannelCount() == 2);

	device->Shutdown();
	CHECK_FALSE(device->IsInitialized());
	CHECK(device->SampleRate() == 0);
	CHECK(device->ChannelCount() == 0);
}

TEST_CASE("AudioDevice Shutdown is safe before Init and twice after") {
	Scope<AudioDevice> device = CreateAudioDevice(AudioAPI::MiniAudio);
	REQUIRE(device != nullptr);
	device->Shutdown();
	CHECK_FALSE(device->IsInitialized());

	AudioDeviceConfig config;
	config.nullDevice = true;
	REQUIRE(device->Init(config));
	device->Shutdown();
	device->Shutdown();
	CHECK_FALSE(device->IsInitialized());
}
