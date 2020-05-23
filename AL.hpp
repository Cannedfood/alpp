// Copyright (c) 2018-2020 Benno Straub, licensed under the MIT license. (A copy can be found at the bottom of this file)

#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <utility>

namespace al {

enum class Format {
	Mono8    = 0x1100,
	Mono16   = 0x1101,
	Stereo8  = 0x1102,
	Stereo16 = 0x1103,

	MonoF32   = 0x10010,
	StereoF32 = 0x10011
};
Format MultiChannelFormat(Format mono, unsigned channels);
void   DecomposeFormat(Format fmt, Format* mono, unsigned* channels);

enum class SourceState {
	Initial = 0x1011,
	Playing = 0x1012,
	Paused  = 0x1013,
	Stopped = 0x1014,
};

enum class SourceType {
	Undetermined = 0x1030,
	Static       = 0x1028,
	Streaming    = 0x1029,
};

class DeviceView {
protected:
	void* mDeviceHandle = nullptr;
public:
	DeviceView(void* deviceHandle) noexcept : mDeviceHandle(deviceHandle) {}

	int geti(int param) const noexcept;
	const char* gets(int param) const noexcept;
	const char* getStringISOFT(int paramName, size_t index) const noexcept;

	operator bool() const noexcept { return mDeviceHandle != nullptr; }
};

class Device : public DeviceView {
public:
	Device(std::nullptr_t) noexcept;
	Device(const char* name) noexcept;
	~Device() noexcept;

	Device(Device&& other) noexcept;
	Device& operator=(Device&& other) noexcept;
	Device(Device const& other) noexcept            = delete;
	Device& operator=(Device const& other) noexcept = delete;

	void* release() noexcept { return std::exchange(mDeviceHandle, nullptr); }
};

class Context {
public:
	class Options {
		std::vector<int> options = { 0 };
	public:
		Device device = nullptr;
		void add(std::initializer_list<int> values) noexcept { options.insert(options.begin() + options.size() - 1, values.begin(), values.end()); }

		int const* get() const noexcept { return options.data(); }
	};

	Context(std::nullptr_t)  noexcept;
	Context(Options options) noexcept;
	~Context() noexcept;

	Context(Context const& other)            noexcept = delete;
	Context& operator=(Context const& other) noexcept = delete;
	Context(Context&& other)                 noexcept = delete;
	Context& operator=(Context&& other)      noexcept = delete;

	DeviceView device() const noexcept;

	void init(Options options) noexcept;
	void close() noexcept;

private:
	void* mContext;
};

class BufferView {
protected:
	unsigned mHandle;
public:
	explicit
	BufferView(unsigned handle = 0) noexcept;
	BufferView(std::nullptr_t) noexcept : BufferView() {}
	~BufferView() noexcept;


	void data(void const* data, size_t size, Format fmt, unsigned freq) noexcept;

	int geti(unsigned prop) const noexcept;

	int frequency() const noexcept; //<! Sample frequency in Hz
	int bits() const noexcept; //<! Bit depth
	int channels() const noexcept; //<! Number of channels in buffer

	int size() const noexcept; //<! Buffer size

	operator bool() const noexcept { return mHandle != 0; }
	explicit operator int() { return mHandle; }
	explicit operator unsigned() { return mHandle; }
};

class Buffer : public BufferView {
public:
	Buffer(std::nullptr_t = nullptr) noexcept;
	~Buffer() noexcept;

	Buffer(void const* data, size_t size, Format fmt, unsigned freq) noexcept;

	Buffer(Buffer&& buf) noexcept;
	Buffer& operator=(Buffer&& buf) noexcept;

	Buffer(Buffer const& other) noexcept            = delete;
	Buffer& operator=(Buffer const& other) noexcept = delete;

	void gen() noexcept;
	void destroy() noexcept;
};

enum FilterType {
	NullFilter = 0x0000,
	Lowpass    = 0x0001,
	Highpass   = 0x0002,
	Bandpass   = 0x0003
};

class FilterView {
protected:
	unsigned mHandle = 0;
public:
	FilterView(unsigned handle = 0) noexcept : mHandle(handle) {}

	FilterType type() const noexcept;

	void type(FilterType) noexcept;

	void lowpass_gain(float f) noexcept;
	void lowpass_gainhf(float f) noexcept;

	void highpass_gain(float f) noexcept;
	void highpass_gainlf(float f) noexcept;

	void bandpass_gain(float f) noexcept;
	void bandpass_gainlf(float f) noexcept;
	void bandpass_gainhf(float f) noexcept;

	void set(int param, int   value) noexcept;
	void set(int param, float value) noexcept;
	int   geti(int param) const noexcept;
	float getf(int param) const noexcept;

	operator bool() const noexcept { return mHandle; }
};

class Filter : public FilterView {
public:
	Filter(std::nullptr_t = nullptr) noexcept;
	~Filter() noexcept;

	Filter(Filter&& other) noexcept                 = delete;
	Filter& operator=(Filter&& other) noexcept      = delete;
	Filter(Filter const& other) noexcept            = delete;
	Filter& operator=(Filter const& other) noexcept = delete;

	void gen() noexcept;
	void destroy() noexcept;
};

enum EffectType {
	EffectNull       = 0x0000,
	Eaxreverb        = 0x8000,
	Reverb           = 0x0001,
	Chorus           = 0x0002,
	Distortion       = 0x0003,
	Echo             = 0x0004,
	Flanger          = 0x0005,
	FrequencyShifter = 0x0006,
	VocalMorpher     = 0x0007,
	PitchShifter     = 0x0008,
	RingModulator    = 0x0009,
	AutoWah          = 0x000A,
	Compressor       = 0x000B,
	Equalizer        = 0x000C
};

class EffectView {
protected:
	unsigned mHandle = 0;
public:
	EffectView(unsigned handle = 0) noexcept;

	void type(EffectType) noexcept;

	// Reverb
	void reverbDensity            (float f) noexcept;
	void reverbDiffusion          (float f) noexcept;
	void reverbGain               (float f) noexcept;
	void reverbGainhf             (float f) noexcept;
	void reverbDecayTime          (float f) noexcept;
	void reverbDecayHFRatio       (float f) noexcept;
	void reverbReflectionsGain    (float f) noexcept;
	void reverbReflectionsDelay   (float f) noexcept;
	void reverbLateReverbGain     (float f) noexcept;
	void reverbLateReverbDelay    (float f) noexcept;
	void reverbAirAbsorptionGainHF(float f) noexcept;
	void reverbRoomRolloffFactor  (float f) noexcept;
	void reverbDecayHFLimit       (bool  f) noexcept;

	// TODO: parameters
	void  set(int param, float f) noexcept;
	void  set(int param, int   i) noexcept;
	int   geti(int param)   const noexcept;
	float getf(int param)   const noexcept;

	operator bool() const noexcept { return mHandle; }
};

class Effect : public EffectView {
public:
	Effect(std::nullptr_t = nullptr) noexcept;
	~Effect() noexcept;

	Effect(Effect&& other) noexcept                 = delete;
	Effect& operator=(Effect&& other) noexcept      = delete;
	Effect(Effect const& other) noexcept            = delete;
	Effect& operator=(Effect const& other) noexcept = delete;

	void gen() noexcept;
	void destroy() noexcept;
};

class AuxiliaryEffectsSlotView {
protected:
	unsigned mHandle = 0;
public:
	AuxiliaryEffectsSlotView(unsigned handle = 0) noexcept;

	void effect(EffectView effect) noexcept;
	void gain(float f) noexcept;
	void auxiliarySendAuto(bool b) noexcept;

	operator bool() const noexcept { return mHandle; }
};

class AuxiliaryEffectsSlot : public AuxiliaryEffectsSlotView {
public:
	AuxiliaryEffectsSlot(std::nullptr_t = nullptr) noexcept;
	~AuxiliaryEffectsSlot() noexcept;

	AuxiliaryEffectsSlot(AuxiliaryEffectsSlot&& other) noexcept                 = delete;
	AuxiliaryEffectsSlot& operator=(AuxiliaryEffectsSlot&& other) noexcept      = delete;
	AuxiliaryEffectsSlot(AuxiliaryEffectsSlot const& other) noexcept            = delete;
	AuxiliaryEffectsSlot& operator=(AuxiliaryEffectsSlot const& other) noexcept = delete;

	void gen() noexcept;
	void destroy() noexcept;
};

class SourceView {
protected:
	unsigned mHandle;
public:
	SourceView(unsigned handle = 0) noexcept;
	SourceView(std::nullptr_t) noexcept : SourceView() {}

	void play() noexcept;
	void pause() noexcept;
	void stop() noexcept;
	void rewind() noexcept;

	void queueBuffers(al::BufferView const* buffers, size_t count) noexcept;
	void unqueueBuffers(al::BufferView* buffers, size_t count) noexcept;

	void           queueBuffer(al::BufferView buffer) noexcept;
	al::BufferView unqueueBuffer() noexcept;


	float     getf (unsigned prop)          const noexcept;
	int       geti (unsigned prop)          const noexcept;
	glm::vec3 get3f(unsigned prop)          const noexcept;
	void      set(unsigned prop,  float)    const noexcept;
	void      set(unsigned prop,  int)      const noexcept;
	void      set(unsigned prop, glm::vec3) const noexcept;

	float pitch()              const noexcept; //<! Pitch multiplier, always positive
	void  pitch(float)               noexcept;
	float gain()               const noexcept; //<! Gain
	void  gain(float)                noexcept;
	float max_distance()       const noexcept; //<! used with the Inverse Clamped Distance Model to set the distance where there will no longer be any attenuation of the source
	void  max_distance(float)        noexcept;
	float rolloff_factor()     const noexcept; //<! the rolloff rate for the source
	void  rolloff_factor(float)      noexcept;
	float reference_distance() const noexcept; //<! the distance under which the volume for the source would normally drop by half (before being influenced by rolloff factor or AL_MAX_DISTANCE)
	void  reference_distance(float)  noexcept;

	float min_gain()         const noexcept; //<! the minimum gain for this source
	void  min_gain(float)          noexcept;
	float max_gain()         const noexcept; //<! the maximum gain for this source
	void  max_gain(float)          noexcept;
	float cone_outer_gain()  const noexcept; //<! the gain when outside the oriented cone
	void  cone_outer_gain(float)   noexcept;
	float cone_inner_angle() const noexcept; //<! the gain when inside the oriented cone
	void  cone_inner_angle(float)  noexcept;
	float cone_outer_angle() const noexcept; //<! outer angle of the sound cone, in degrees default is 360
	void  cone_outer_angle(float)  noexcept;

	glm::vec3 position()     const noexcept; //<! X, Y, Z position
	void      position(glm::vec3)  noexcept;
	glm::vec3 velocity()     const noexcept; //<! velocity vector
	void      velocity(glm::vec3)  noexcept;
	glm::vec3 direction()    const noexcept; //<! direction vector
	void      direction(glm::vec3) noexcept;

	bool       relative() const noexcept; //<! determines if the positions are relative to the listener
	void       relative(bool)   noexcept;
	SourceType type()     const noexcept; //<! the source type – AL_UNDETERMINED, AL_STATIC, or AL_STREAMING
	void       type(SourceType) noexcept;

	bool        looping()    const noexcept; //<! turns looping on (AL_TRUE) or off (AL_FALSE)
	void        looping(bool)      noexcept;
	BufferView  buffer()     const noexcept; //<! the ID of the attached buffer
	void        buffer(BufferView) noexcept;
	SourceState state()      const noexcept; //<! the state of the source
	void        state(SourceState) noexcept;
	bool        playing()    const noexcept { return state() == SourceState::Playing; }
	bool        paused()     const noexcept { return state() == SourceState::Paused; }
	bool        stopped()    const noexcept { return state() == SourceState::Stopped; }

	unsigned buffers_queued() const noexcept; //<! the number of buffers queued on this source
	unsigned buffers_processed() const noexcept; //<! the number of buffers in the queue that have been processed

	void auxiliary_send_filter(unsigned sendIndex, AuxiliaryEffectsSlotView effectsSlot, FilterView filter = {}) noexcept;

	float  sec_offset()    const noexcept; //<! the playback position, expressed in seconds
	void   sec_offset(float)     noexcept;
	size_t sample_offset() const noexcept; //<! the playback position, expressed in samples
	void   sample_offset(size_t) noexcept;
	size_t byte_offset()   const noexcept; //<! the playback position, expressed in bytes
	void   byte_offset(size_t)   noexcept;

	operator bool() const noexcept { return mHandle != 0; }
};

class Source : public SourceView {
public:
	Source(std::nullptr_t = nullptr) noexcept;
	explicit Source(BufferView buffer) noexcept;
	~Source() noexcept;

	Source(Source&& src) noexcept;
	Source& operator=(Source&& src) noexcept;

	Source(Source const& other) noexcept            = delete;
	Source& operator=(Source const& other) noexcept = delete;

	void gen() noexcept;
	void destroy() noexcept;
};

class Listener {
public:
	static void set(unsigned param, int       value) noexcept;
	static void set(unsigned param, float     value) noexcept;
	static void set(unsigned param, glm::vec3 value) noexcept;

	static int       geti(unsigned param) noexcept;
	static float     getf(unsigned param) noexcept;
	static glm::vec3 get3f(unsigned param) noexcept;

	static float gain()        noexcept; //<! “master gain”
	static void  gain(float f) noexcept;

	static glm::vec3 position() noexcept; //<! X, Y, Z position
	static void      position(glm::vec3 v) noexcept;
	static glm::vec3 velocity() noexcept; //<! velocity vector
	static void      velocity(glm::vec3 v) noexcept;
	static void      orientation(glm::vec3 fwd, glm::vec3 up) noexcept; //<! orientation expressed as “at” and “up” vectors
};

} // namespace al

#ifdef ALPP_INLINE
#include "AL.cpp"
#endif

/*
 Copyright (c) 2018 Benno Straub

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
