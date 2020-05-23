// Copyright (c) 2018-2020 Benno Straub, licensed under the MIT license. (A copy can be found at the bottom of this file)

#define AL_ALEXT_PROTOTYPES

#include "AL.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>

#include <utility>
#include <stdexcept>
#include <cassert>
#include <string>

#ifndef ALPP_DECL
	#ifdef ALPP_INLINE
		#define ALPP_DECL inline
	#else
		#define ALPP_DECL
	#endif
#endif

#ifndef NDEBUG
	#define AL_ERROR_CHECKING
#endif

#ifdef AL_ERROR_CHECKING
#define AL_HANDLE_CHECK(X) assert(X)
#define AL_CHECK_ERROR() alCheckError(__FILE__, __LINE__)
#define ALC_CHECK_ERROR(device) alcCheckError(device, __FILE__, __LINE__)
static
void alcCheckError(ALCdevice* device, const char* file, int line) {
	int err = alcGetError(device);
	if(err != ALC_NO_ERROR) {
		#define ERRCASE(X, MSG) case X: throw std::runtime_error(#X ": " MSG " at " + std::string(file) + ":" + std::to_string(line))
		switch (err) {
			ERRCASE(ALC_NO_ERROR, "there is not currently an error");
			ERRCASE(ALC_INVALID_DEVICE, "a bad device was passed to an OpenAL function");
			ERRCASE(ALC_INVALID_CONTEXT, "a bad context was passed to an OpenAL function");
			ERRCASE(ALC_INVALID_ENUM, "an unknown enum value was passed to an OpenAL function");
			ERRCASE(ALC_INVALID_VALUE, "an invalid value was passed to an OpenAL function");
			ERRCASE(ALC_OUT_OF_MEMORY, "the requested operation resulted in OpenAL running out of memory");
		}
		#undef ERRCASE
	}
}
static
void alCheckError(const char* file, int line) {
	int err = alGetError();
	if(err != AL_NO_ERROR) {
		#define ERRCASE(X, MSG) case X: throw std::runtime_error(#X ": " MSG " at " + std::string(file) + ":" + std::to_string(line))
		switch (err) {
			ERRCASE(AL_NO_ERROR, "there is not currently an error");
			ERRCASE(AL_INVALID_NAME, "a bad name (ID) was passed to an OpenAL function");
			ERRCASE(AL_INVALID_ENUM, "an invalid enum value was passed to an OpenAL function");
			ERRCASE(AL_INVALID_VALUE, "an invalid value was passed to an OpenAL function");
			ERRCASE(AL_INVALID_OPERATION, "the requested operation is not valid");
			ERRCASE(AL_OUT_OF_MEMORY, "allocation failed");
		}
		#undef ERRCASE
	}
}
#else
#define AL_HANDLE_CHECK(X)
#define AL_CHECK_ERROR()
#define ALC_CHECK_ERROR(device)
#endif

namespace al {

ALPP_DECL int DeviceView::geti(int param) const noexcept {
	int result;
	alcGetIntegerv((ALCdevice*) mDeviceHandle, param, 1, &result);
	return result;
}
ALPP_DECL const char* DeviceView::gets(int param) const noexcept { return alcGetString((ALCdevice*) mDeviceHandle, param); }
ALPP_DECL const char* DeviceView::getStringISOFT(int paramName, size_t index) const noexcept { return alcGetStringiSOFT((ALCdevice*) mDeviceHandle, paramName, index); }

// =============================================================
// == Device =============================================
// =============================================================


ALPP_DECL Device::Device(std::nullptr_t) noexcept :
	DeviceView(nullptr)
{}
ALPP_DECL Device::Device(const char* name) noexcept :
	DeviceView(nullptr)
{
	mDeviceHandle = alcOpenDevice(name);
}
ALPP_DECL Device::Device(Device&& other) noexcept :
	DeviceView(other.release())
{}
ALPP_DECL Device& Device::operator=(Device&& other) noexcept {
	if(mDeviceHandle) {
		alcCloseDevice((ALCdevice*)mDeviceHandle);
	}
	mDeviceHandle = other.release();
	return *this;
}
ALPP_DECL Device::~Device() noexcept {
	if(mDeviceHandle) {
		alcCloseDevice((ALCdevice*)mDeviceHandle);
	}
}

// =============================================================
// == Context =============================================
// =============================================================

ALPP_DECL Context::Context(std::nullptr_t) noexcept : mContext(nullptr) {}

ALPP_DECL Context::Context(Options options) noexcept :
	Context(nullptr)
{
	init(std::move(options));
}
ALPP_DECL Context::~Context() noexcept {
	close();
}

ALPP_DECL void Context::init(Options options) noexcept {
	close();

	ALCdevice* device = (ALCdevice*)options.device.release();
	if(!device) {
		device = alcOpenDevice(NULL); ALC_CHECK_ERROR(device);
	}
	mContext = alcCreateContext(device, options.get()); ALC_CHECK_ERROR(device);
	alcMakeContextCurrent((ALCcontext*)mContext); ALC_CHECK_ERROR(device);
	alGetError(); // Clear errors
}
ALPP_DECL void Context::close() noexcept {
	if(mContext == nullptr)
		return;

	auto device = alcGetContextsDevice((ALCcontext*)mContext); ALC_CHECK_ERROR(device);
	alcMakeContextCurrent(NULL); ALC_CHECK_ERROR(device);
	alcDestroyContext((ALCcontext*)mContext); ALC_CHECK_ERROR(device);
	alcCloseDevice(device);
}
ALPP_DECL DeviceView Context::device() const noexcept {
	return { alcGetContextsDevice((ALCcontext*)mContext) };
}


// =============================================================
// == Format =============================================
// =============================================================

ALPP_DECL Format MultiChannelFormat(Format mono, unsigned channels) {
	if(channels == 1) return mono;

	if(channels == 2) {
		switch (mono) {
			case Format::Mono8: return Format::Stereo8;
			case Format::Mono16: return Format::Stereo16;
			case Format::MonoF32: return Format::StereoF32;
			default: throw std::runtime_error("Argument mono is not a mono format!");
		}
	}

	throw std::runtime_error("Unsupported number of channels: " + std::to_string(channels));
}

ALPP_DECL void DecomposeFormat(Format fmt, Format* mono, unsigned* channels) {
	switch(fmt) {
	case Format::Mono8:     if(mono) *mono = Format::Mono8;   if(channels) *channels = 1; break;
	case Format::Mono16:    if(mono) *mono = Format::Mono16;  if(channels) *channels = 1; break;
	case Format::MonoF32:   if(mono) *mono = Format::MonoF32; if(channels) *channels = 1; break;
	case Format::Stereo8:   if(mono) *mono = Format::Mono8;   if(channels) *channels = 2; break;
	case Format::Stereo16:  if(mono) *mono = Format::Mono16;  if(channels) *channels = 2; break;
	case Format::StereoF32: if(mono) *mono = Format::MonoF32; if(channels) *channels = 2; break;
	default: throw std::runtime_error("Unknown format");
	}
}

// =============================================================
// == BufferView =============================================
// =============================================================

ALPP_DECL BufferView::BufferView(unsigned handle) noexcept : mHandle(handle) {}
ALPP_DECL BufferView::~BufferView() noexcept {}

ALPP_DECL void BufferView::data(void const* data, size_t size, Format fmt, unsigned freq) noexcept {
	alBufferData(mHandle, (ALenum)fmt, data, size, freq); AL_CHECK_ERROR();
}

ALPP_DECL int BufferView::geti(unsigned param) const noexcept {
	int result;
	alGetBufferi(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}

ALPP_DECL int BufferView::frequency() const noexcept { return geti(AL_FREQUENCY); }
ALPP_DECL int BufferView::bits()      const noexcept { return geti(AL_BITS); }
ALPP_DECL int BufferView::channels()  const noexcept { return geti(AL_CHANNELS); }

ALPP_DECL int BufferView::size() const noexcept { return geti(AL_SIZE); }

// =============================================================
// == Buffer =============================================
// =============================================================

ALPP_DECL Buffer::Buffer(std::nullptr_t) noexcept :
	BufferView()
{}
ALPP_DECL Buffer::Buffer(void const* data, size_t size, Format fmt, unsigned freq) noexcept :
	Buffer()
{
	gen();
	this->data(data, size, fmt, freq);
}
ALPP_DECL Buffer::~Buffer() noexcept { destroy(); }

ALPP_DECL Buffer::Buffer(Buffer&& src) noexcept : BufferView(std::exchange(src.mHandle, 0)){}
ALPP_DECL Buffer& Buffer::operator=(Buffer&& src) noexcept {
	destroy();
	mHandle = std::exchange(src.mHandle, 0);
	return *this;
}

ALPP_DECL void Buffer::gen() noexcept {
	destroy();
	alGenBuffers(1, &mHandle); AL_CHECK_ERROR();
}
ALPP_DECL void Buffer::destroy() noexcept {
	if(mHandle) {
		alDeleteBuffers(1, &mHandle); AL_CHECK_ERROR();
		mHandle = 0;
	}
}

// =============================================================
// == SourceView =============================================
// =============================================================

ALPP_DECL SourceView::SourceView(unsigned handle) noexcept : mHandle(handle) {}

ALPP_DECL void SourceView::play()   noexcept { alSourcePlay(mHandle);   AL_CHECK_ERROR(); }
ALPP_DECL void SourceView::pause()  noexcept { alSourcePause(mHandle);  AL_CHECK_ERROR(); }
ALPP_DECL void SourceView::stop()   noexcept { alSourceStop(mHandle);   AL_CHECK_ERROR(); }
ALPP_DECL void SourceView::rewind() noexcept { alSourceRewind(mHandle); AL_CHECK_ERROR(); }

ALPP_DECL void SourceView::queueBuffers(BufferView const* buffers, size_t count) noexcept {
	static_assert(sizeof(BufferView) == sizeof(unsigned));
	alSourceQueueBuffers(
		mHandle,
		count,
		reinterpret_cast<unsigned const*>(buffers)
	); AL_CHECK_ERROR();
}
ALPP_DECL void SourceView::unqueueBuffers(al::BufferView* buffers, size_t count) noexcept {
	static_assert(sizeof(BufferView) == sizeof(unsigned));
	alSourceUnqueueBuffers(
		mHandle,
		count,
		reinterpret_cast<unsigned*>(buffers)
	); AL_CHECK_ERROR();
}
ALPP_DECL void SourceView::queueBuffer(al::BufferView buffer) noexcept {
	queueBuffers(&buffer, 1);
}
ALPP_DECL al::BufferView SourceView::unqueueBuffer() noexcept {
	al::BufferView result;
	unqueueBuffers(&result, 1);
	return result;
}

ALPP_DECL float     SourceView::getf(unsigned param) const noexcept {
	float result;
	alGetSourcef(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL int       SourceView::geti(unsigned param) const noexcept {
	int result;
	alGetSourcei(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL glm::vec3 SourceView::get3f(unsigned param) const noexcept {
	glm::vec3 result;
	alGetSourcefv(mHandle, param, &result[0]); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL void SourceView::set(unsigned param, float     value) const noexcept { alSourcef(mHandle, param, value); AL_CHECK_ERROR(); }
ALPP_DECL void SourceView::set(unsigned param, int       value) const noexcept { alSourcei(mHandle, param, value); AL_CHECK_ERROR(); }
ALPP_DECL void SourceView::set(unsigned param, glm::vec3 value) const noexcept { alSource3f(mHandle, param, value.x, value.y, value.z); AL_CHECK_ERROR(); }

ALPP_DECL float SourceView::pitch()                   const noexcept { return getf(AL_PITCH); }
ALPP_DECL void  SourceView::pitch(float value)              noexcept { set(AL_PITCH, value); }
ALPP_DECL float SourceView::gain()                    const noexcept { return getf(AL_GAIN); }
ALPP_DECL void  SourceView::gain(float value)               noexcept { set(AL_GAIN, value); }
ALPP_DECL float SourceView::max_distance()            const noexcept { return getf(AL_MAX_DISTANCE); }
ALPP_DECL void  SourceView::max_distance(float value)       noexcept { set(AL_MAX_DISTANCE, value); }
ALPP_DECL float SourceView::rolloff_factor()          const noexcept { return getf(AL_ROLLOFF_FACTOR); }
ALPP_DECL void  SourceView::rolloff_factor(float value)     noexcept { set(AL_ROLLOFF_FACTOR, value); }
ALPP_DECL float SourceView::reference_distance()      const noexcept { return getf(AL_REFERENCE_DISTANCE); }
ALPP_DECL void  SourceView::reference_distance(float value) noexcept { set(AL_REFERENCE_DISTANCE, value); }

ALPP_DECL float SourceView::min_gain()              const noexcept { return getf(AL_MIN_GAIN); }
ALPP_DECL void  SourceView::min_gain(float value)         noexcept { set(AL_MIN_GAIN, value); }
ALPP_DECL float SourceView::max_gain()              const noexcept { return getf(AL_MAX_GAIN); }
ALPP_DECL void  SourceView::max_gain(float value)         noexcept { set(AL_MAX_GAIN, value); }
ALPP_DECL float SourceView::cone_outer_gain()       const noexcept { return getf(AL_CONE_OUTER_GAIN); }
ALPP_DECL void  SourceView::cone_outer_gain(float value)  noexcept { set(AL_CONE_OUTER_GAIN, value); }
ALPP_DECL float SourceView::cone_inner_angle()      const noexcept { return getf(AL_CONE_INNER_ANGLE); }
ALPP_DECL void  SourceView::cone_inner_angle(float value) noexcept { set(AL_CONE_INNER_ANGLE, value); }
ALPP_DECL float SourceView::cone_outer_angle()      const noexcept { return getf(AL_CONE_OUTER_ANGLE); }
ALPP_DECL void  SourceView::cone_outer_angle(float value) noexcept { set(AL_CONE_OUTER_ANGLE, value); }

ALPP_DECL glm::vec3 SourceView::position ()          const noexcept { return get3f(AL_POSITION); }
ALPP_DECL void      SourceView::position(glm::vec3 value)  noexcept { set(AL_POSITION, value); }
ALPP_DECL glm::vec3 SourceView::velocity()           const noexcept { return get3f(AL_VELOCITY); }
ALPP_DECL void      SourceView::velocity(glm::vec3 value)  noexcept { set(AL_VELOCITY, value); }
ALPP_DECL glm::vec3 SourceView::direction()          const noexcept { return get3f(AL_DIRECTION); }
ALPP_DECL void      SourceView::direction(glm::vec3 value) noexcept { set(AL_DIRECTION, value); }

ALPP_DECL bool       SourceView::relative()       const noexcept { return geti(AL_SOURCE_RELATIVE); }
ALPP_DECL void       SourceView::relative(bool value)   noexcept { set(AL_SOURCE_RELATIVE, value ? AL_TRUE : AL_FALSE); }
ALPP_DECL SourceType SourceView::type()           const noexcept { return (SourceType)geti(AL_SOURCE_TYPE); }
ALPP_DECL void       SourceView::type(SourceType value) noexcept { set(AL_SOURCE_TYPE, (int)value); }

ALPP_DECL bool        SourceView::looping()          const noexcept { return geti(AL_LOOPING); }
ALPP_DECL void        SourceView::looping(bool value)      noexcept { set(AL_LOOPING, value ? AL_TRUE : AL_FALSE); }
ALPP_DECL BufferView  SourceView::buffer()           const noexcept { return BufferView(geti(AL_BUFFER)); }
ALPP_DECL void        SourceView::buffer(BufferView value) noexcept { set(AL_BUFFER, (int)value); }
ALPP_DECL SourceState SourceView::state()            const noexcept { return (SourceState)geti(AL_SOURCE_STATE); }
ALPP_DECL void        SourceView::state(SourceState value) noexcept { set(AL_SOURCE_STATE, (int)value); }

ALPP_DECL unsigned SourceView::buffers_queued()    const noexcept { return geti(AL_BUFFERS_QUEUED); }
ALPP_DECL unsigned SourceView::buffers_processed() const noexcept { return geti(AL_BUFFERS_PROCESSED); }

ALPP_DECL void SourceView::auxiliary_send_filter(unsigned sendIndex, AuxiliaryEffectsSlotView effectsSlot, FilterView filter) noexcept {
	alSource3i(mHandle, AL_AUXILIARY_SEND_FILTER, effectsSlot, sendIndex, filter); AL_CHECK_ERROR();
}

ALPP_DECL float  SourceView::sec_offset()          const noexcept { return getf(AL_SEC_OFFSET); }
ALPP_DECL void   SourceView::sec_offset(float value)     noexcept { set(AL_SEC_OFFSET, value); }
ALPP_DECL size_t SourceView::sample_offset()       const noexcept { return geti(AL_SAMPLE_OFFSET); }
ALPP_DECL void   SourceView::sample_offset(size_t value) noexcept { set(AL_SAMPLE_OFFSET, (int)value); }
ALPP_DECL size_t SourceView::byte_offset()         const noexcept { return geti(AL_BYTE_OFFSET); }
ALPP_DECL void   SourceView::byte_offset(size_t value)   noexcept { set(AL_BYTE_OFFSET, (int)value); }

// =============================================================
// == Source =============================================
// =============================================================

ALPP_DECL Source::Source(std::nullptr_t) noexcept :
	SourceView()
{}

ALPP_DECL Source::Source(BufferView buffer) noexcept :
	Source()
{
	gen();
	this->buffer(buffer);
}

ALPP_DECL Source::~Source() noexcept {
	destroy();
}

ALPP_DECL Source::Source(Source&& src) noexcept : SourceView(std::exchange(src.mHandle, 0)) {}
ALPP_DECL Source& Source::operator=(Source&& src) noexcept {
	destroy();
	mHandle = std::exchange(src.mHandle, 0);
	return *this;
}

ALPP_DECL void Source::gen() noexcept {
	destroy();
	alGenSources(1, &mHandle); AL_CHECK_ERROR();
}
ALPP_DECL void Source::destroy() noexcept {
	if(mHandle) {
		alDeleteSources(1, &mHandle); AL_CHECK_ERROR();
		mHandle = 0;
	}
}

// =============================================================
// == Listener =============================================
// =============================================================

ALPP_DECL void Listener::set(unsigned param, int       value) noexcept { alListeneri(param, value); AL_CHECK_ERROR(); }
ALPP_DECL void Listener::set(unsigned param, float     value) noexcept { alListenerf(param, value); AL_CHECK_ERROR(); }
ALPP_DECL void Listener::set(unsigned param, glm::vec3 value) noexcept { alListener3f(param, value.x, value.y, value.z); AL_CHECK_ERROR(); }

ALPP_DECL int Listener::geti(unsigned param) noexcept {
	int result;
	alGetListeneri(param, &result); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL float Listener::getf(unsigned param) noexcept {
	float result;
	alGetListenerf(param, &result); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL glm::vec3 Listener::get3f(unsigned param) noexcept {
	glm::vec3 result;
	alGetListenerfv(param, &result[0]); AL_CHECK_ERROR();
	return result;
}

ALPP_DECL float Listener::gain()        noexcept { return Listener::getf(AL_GAIN); }
ALPP_DECL void  Listener::gain(float f) noexcept { Listener::set(AL_GAIN, f); }

ALPP_DECL glm::vec3 Listener::position() noexcept { return Listener::get3f(AL_POSITION); }
ALPP_DECL void Listener::position(glm::vec3 v) noexcept { Listener::set(AL_POSITION, v); }
ALPP_DECL glm::vec3 Listener::velocity() noexcept { return Listener::get3f(AL_VELOCITY); }
ALPP_DECL void Listener::velocity(glm::vec3 v) noexcept { Listener::set(AL_VELOCITY, v); }
ALPP_DECL void Listener::orientation(glm::vec3 fwd, glm::vec3 up) noexcept {
	glm::vec3 fwdup[] = {fwd, up};
	alListenerfv(AL_ORIENTATION, &fwdup[0][0]); AL_CHECK_ERROR();
}

// =============================================================
// == FilterView =============================================
// =============================================================


ALPP_DECL FilterType FilterView::type() const noexcept {
	ALint value;
	alGetFilteri(mHandle, AL_FILTER_TYPE, &value); AL_CHECK_ERROR();
	return (FilterType)value;
}
ALPP_DECL void FilterView::type(FilterType type) noexcept { set(AL_FILTER_TYPE, type); }
ALPP_DECL void FilterView::lowpass_gain(float f) noexcept { set(AL_LOWPASS_GAIN, f); }
ALPP_DECL void FilterView::lowpass_gainhf(float f) noexcept { set(AL_LOWPASS_GAINHF, f); }
ALPP_DECL void FilterView::highpass_gain(float f) noexcept { set(AL_HIGHPASS_GAIN, f); }
ALPP_DECL void FilterView::highpass_gainlf(float f) noexcept { set(AL_HIGHPASS_GAINLF, f); }
ALPP_DECL void FilterView::bandpass_gain(float f) noexcept { set(AL_BANDPASS_GAIN, f); }
ALPP_DECL void FilterView::bandpass_gainlf(float f) noexcept { set(AL_BANDPASS_GAINLF, f); }
ALPP_DECL void FilterView::bandpass_gainhf(float f) noexcept { set(AL_BANDPASS_GAINHF, f); }
ALPP_DECL void FilterView::set(int param, int   value) noexcept { alFilteri(mHandle, param, value); AL_CHECK_ERROR(); }
ALPP_DECL void FilterView::set(int param, float value) noexcept { alFilterf(mHandle, param, value); AL_CHECK_ERROR(); }
ALPP_DECL int   FilterView::geti(int param) const noexcept {
	int result;
	alGetFilteri(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL float FilterView::getf(int param) const noexcept {
	float result;
	alGetFilterf(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}

// =============================================================
// == Filter =============================================
// =============================================================


ALPP_DECL Filter::Filter(std::nullptr_t) noexcept : FilterView() {}
ALPP_DECL Filter::~Filter() noexcept { destroy(); }
ALPP_DECL void Filter::gen() noexcept {
	destroy();
	alGenFilters(1, &mHandle); AL_CHECK_ERROR();
}
ALPP_DECL void Filter::destroy() noexcept {
	if(!mHandle) return;
	alDeleteFilters(1, &mHandle); AL_CHECK_ERROR();
	mHandle = 0;
}

// =============================================================
// == EffectView =============================================
// =============================================================

ALPP_DECL EffectView::EffectView(unsigned handle) noexcept :
	mHandle(handle)
{}
ALPP_DECL void EffectView::type(EffectType effectType) noexcept {
	alEffecti(mHandle, AL_EFFECT_TYPE, effectType); AL_CHECK_ERROR();
}
ALPP_DECL void  EffectView::set(int param, float f) noexcept { alEffectf(mHandle, param, f); AL_CHECK_ERROR(); }
ALPP_DECL void  EffectView::set(int param, int   i) noexcept { alEffecti(mHandle, param, i); AL_CHECK_ERROR(); }
ALPP_DECL int   EffectView::geti(int param)   const noexcept {
	int result;
	alGetFilteri(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}
ALPP_DECL float EffectView::getf(int param)   const noexcept {
	float result;
	alGetFilterf(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}

// Reverb

ALPP_DECL void EffectView::reverbDensity(float f)             noexcept { set(AL_REVERB_DENSITY, f); }
ALPP_DECL void EffectView::reverbDiffusion(float f)           noexcept { set(AL_REVERB_DIFFUSION, f); }
ALPP_DECL void EffectView::reverbGain(float f)                noexcept { set(AL_REVERB_GAIN, f); }
ALPP_DECL void EffectView::reverbGainhf(float f)              noexcept { set(AL_REVERB_GAINHF, f); }
ALPP_DECL void EffectView::reverbDecayTime(float f)           noexcept { set(AL_REVERB_DECAY_TIME, f); }
ALPP_DECL void EffectView::reverbDecayHFRatio(float f)        noexcept { set(AL_REVERB_DECAY_HFRATIO, f); }
ALPP_DECL void EffectView::reverbReflectionsGain(float f)     noexcept { set(AL_REVERB_REFLECTIONS_GAIN, f); }
ALPP_DECL void EffectView::reverbReflectionsDelay(float f)    noexcept { set(AL_REVERB_REFLECTIONS_DELAY, f); }
ALPP_DECL void EffectView::reverbLateReverbGain(float f)      noexcept { set(AL_REVERB_LATE_REVERB_GAIN, f); }
ALPP_DECL void EffectView::reverbLateReverbDelay(float f)     noexcept { set(AL_REVERB_LATE_REVERB_DELAY, f); }
ALPP_DECL void EffectView::reverbAirAbsorptionGainHF(float f) noexcept { set(AL_REVERB_AIR_ABSORPTION_GAINHF, f); }
ALPP_DECL void EffectView::reverbRoomRolloffFactor(float f)   noexcept { set(AL_REVERB_ROOM_ROLLOFF_FACTOR, f); }
ALPP_DECL void EffectView::reverbDecayHFLimit(bool f)         noexcept { set(AL_REVERB_DECAY_HFLIMIT, f?AL_TRUE:AL_FALSE); }

// =============================================================
// == Effect =============================================
// =============================================================

ALPP_DECL Effect::Effect(std::nullptr_t) noexcept : EffectView(0) {}
ALPP_DECL Effect::~Effect() noexcept { destroy(); }
ALPP_DECL void Effect::gen() noexcept {
	destroy();
	alGenEffects(1, &mHandle); AL_CHECK_ERROR();
}
ALPP_DECL void Effect::destroy() noexcept {
	if(mHandle == 0) return;

	alDeleteEffects(1, &mHandle); AL_CHECK_ERROR();
	mHandle = 0;
}

// =============================================================
// == AuxiliaryEffectsSlotView =================================
// =============================================================

ALPP_DECL AuxiliaryEffectsSlotView::AuxiliaryEffectsSlotView(unsigned handle) noexcept :
	mHandle(handle)
{}
ALPP_DECL void AuxiliaryEffectsSlotView::effect(EffectView effect) noexcept {
	alAuxiliaryEffectSloti(mHandle, AL_EFFECTSLOT_EFFECT, effect); AL_CHECK_ERROR();
}
ALPP_DECL void AuxiliaryEffectsSlotView::gain(float f) noexcept {
	alAuxiliaryEffectSlotf(mHandle, AL_EFFECTSLOT_GAIN, f);
}
ALPP_DECL void AuxiliaryEffectsSlotView::auxiliarySendAuto(bool b) noexcept {
	alAuxiliaryEffectSloti(mHandle, AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, b?AL_TRUE:AL_FALSE);
}

// =============================================================
// == AuxiliaryEffectsSlot =================================
// =============================================================

ALPP_DECL AuxiliaryEffectsSlot::AuxiliaryEffectsSlot(std::nullptr_t) noexcept : AuxiliaryEffectsSlotView(0) {}
ALPP_DECL AuxiliaryEffectsSlot::~AuxiliaryEffectsSlot() noexcept { destroy(); }
ALPP_DECL void AuxiliaryEffectsSlot::gen() noexcept {
	destroy();
	alGenAuxiliaryEffectSlots(1, &mHandle); AL_CHECK_ERROR();
}
ALPP_DECL void AuxiliaryEffectsSlot::destroy() noexcept {
	if(mHandle == 0) return;

	alDeleteAuxiliaryEffectSlots(1, &mHandle); AL_CHECK_ERROR();
	mHandle = 0;
}

} // namespace al

/*
 Copyright (c) 2018 Benno Straub

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
