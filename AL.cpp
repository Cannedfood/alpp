// Copyright (c) 2018 Benno Straub, licensed under the MIT license. (A copy can be found at the bottom of this file)

#include "AL.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <stdexcept>
#include <cassert>

#define mContext ((ALCcontext*&) mContextHandle)

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

// =============================================================
// == Context =============================================
// =============================================================

Context::Context() {
	ALCdevice* device = alcOpenDevice(NULL); ALC_CHECK_ERROR(device);
	mContext = alcCreateContext(device, NULL); ALC_CHECK_ERROR(device);
	alcMakeContextCurrent(mContext); ALC_CHECK_ERROR(device);
	alGetError(); // Clear errors
}
Context::~Context() noexcept {
	auto device = alcGetContextsDevice(mContext); ALC_CHECK_ERROR(device);
	alcMakeContextCurrent(NULL); ALC_CHECK_ERROR(device);
	alcDestroyContext(mContext); ALC_CHECK_ERROR(device);
	alcCloseDevice(device);
}

// =============================================================
// == BufferView =============================================
// =============================================================

BufferView::BufferView(unsigned handle) noexcept : mHandle(handle) {}
BufferView::~BufferView() noexcept {}

void BufferView::data(void const* data, size_t size, Format fmt, unsigned freq) noexcept {
	alBufferData(mHandle, (ALenum)fmt, data, size, freq); AL_CHECK_ERROR();
}

int BufferView::geti(unsigned param) const noexcept {
	int result;
	alGetBufferi(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}

int BufferView::frequency() const noexcept { return geti(AL_FREQUENCY); }
int BufferView::bits() const noexcept { return geti(AL_BITS); }
int BufferView::channels() const noexcept { return geti(AL_CHANNELS); }

int BufferView::size() const noexcept { return geti(AL_SIZE); }

// =============================================================
// == Buffer =============================================
// =============================================================

Buffer::Buffer() noexcept :
	BufferView()
{}
Buffer::Buffer(void const* data, size_t size, Format fmt, unsigned freq) noexcept :
	Buffer()
{
	gen();
	this->data(data, size, fmt, freq);
}
Buffer::~Buffer() noexcept { destroy(); }

Buffer::Buffer(Buffer&& src) noexcept : BufferView(std::exchange(src.mHandle, 0)){}
Buffer& Buffer::operator=(Buffer&& src) noexcept {
	destroy();
	mHandle = std::exchange(src.mHandle, 0);
	return *this;
}

void Buffer::gen() noexcept {
	destroy();
	alGenBuffers(1, &mHandle); AL_CHECK_ERROR();
}
void Buffer::destroy() noexcept {
	if(mHandle) {
		alDeleteBuffers(1, &mHandle); AL_CHECK_ERROR();
	}
}

// =============================================================
// == SourceView =============================================
// =============================================================

SourceView::SourceView(unsigned handle) noexcept : mHandle(handle) {}

void SourceView::play() {
	alSourcePlay(mHandle); AL_CHECK_ERROR();
}
void SourceView::pause() {
	alSourcePause(mHandle); AL_CHECK_ERROR();
}
void SourceView::stop() {
	alSourceStop(mHandle); AL_CHECK_ERROR();
}
void SourceView::rewind() {
	alSourceRewind(mHandle); AL_CHECK_ERROR();
}

void SourceView::queueBuffers(BufferView const* buffers, size_t count) {
	static_assert(sizeof(BufferView) == sizeof(unsigned));
	alSourceQueueBuffers(
		mHandle,
		count,
		reinterpret_cast<unsigned const*>(buffers)
	); AL_CHECK_ERROR();
}
void SourceView::unqueueBuffers(al::BufferView* buffers, size_t count) {
	static_assert(sizeof(BufferView) == sizeof(unsigned));
	alSourceUnqueueBuffers(
		mHandle,
		count,
		reinterpret_cast<unsigned*>(buffers)
	); AL_CHECK_ERROR();
}


float     SourceView::getf(unsigned param) const noexcept {
	float result;
	alGetSourcef(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}
int       SourceView::geti(unsigned param) const noexcept {
	int result;
	alGetSourcei(mHandle, param, &result); AL_CHECK_ERROR();
	return result;
}
glm::vec3 SourceView::get3f(unsigned param) const noexcept {
	glm::vec3 result;
	alGetSourcefv(mHandle, param, &result[0]); AL_CHECK_ERROR();
	return result;
}
void SourceView::set(unsigned param, float value) const noexcept {
	alSourcef(mHandle, param, value); AL_CHECK_ERROR();
}
void SourceView::set(unsigned param, int value) const noexcept {
	alSourcei(mHandle, param, value); AL_CHECK_ERROR();
}
void SourceView::set(unsigned param, glm::vec3 value) const noexcept {
	alSource3f(mHandle, param, value.x, value.y, value.z); AL_CHECK_ERROR();
}


float SourceView::pitch() const noexcept { return getf(AL_PITCH); }
void  SourceView::pitch(float value) noexcept { set(AL_PITCH, value); }
float SourceView::gain() const noexcept { return getf(AL_GAIN); }
void  SourceView::gain(float value) noexcept { set(AL_GAIN, value); }
float SourceView::max_distance() const noexcept { return getf(AL_MAX_DISTANCE); }
void  SourceView::max_distance(float value) noexcept { set(AL_MAX_DISTANCE, value); }
float SourceView::rolloff_factor() const noexcept { return getf(AL_ROLLOFF_FACTOR); }
void  SourceView::rolloff_factor(float value) noexcept { set(AL_ROLLOFF_FACTOR, value); }
float SourceView::reference_distance() const noexcept { return getf(AL_REFERENCE_DISTANCE); }
void  SourceView::reference_distance(float value) noexcept { set(AL_REFERENCE_DISTANCE, value); }

float SourceView::min_gain() const noexcept { return getf(AL_MIN_GAIN); }
void  SourceView::min_gain(float value) noexcept { set(AL_MIN_GAIN, value); }
float SourceView::max_gain() const noexcept { return getf(AL_MAX_GAIN); }
void  SourceView::max_gain(float value) noexcept { set(AL_MAX_GAIN, value); }
float SourceView::cone_outer_gain() const noexcept { return getf(AL_CONE_OUTER_GAIN); }
void  SourceView::cone_outer_gain(float value) noexcept { set(AL_CONE_OUTER_GAIN, value); }
float SourceView::cone_inner_angle() const noexcept { return getf(AL_CONE_INNER_ANGLE); }
void  SourceView::cone_inner_angle(float value) noexcept { set(AL_CONE_INNER_ANGLE, value); }
float SourceView::cone_outer_angle() const noexcept { return getf(AL_CONE_OUTER_ANGLE); }
void  SourceView::cone_outer_angle(float value) noexcept { set(AL_CONE_OUTER_ANGLE, value); }

glm::vec3 SourceView::position() const noexcept { return get3f(AL_POSITION); }
void      SourceView::position(glm::vec3 value) noexcept { set(AL_POSITION, value); }
glm::vec3 SourceView::velocity() const noexcept { return get3f(AL_VELOCITY); }
void      SourceView::velocity(glm::vec3 value) noexcept { set(AL_VELOCITY, value); }
glm::vec3 SourceView::direction() const noexcept { return get3f(AL_DIRECTION); }
void      SourceView::direction(glm::vec3 value) noexcept { set(AL_DIRECTION, value); }

bool       SourceView::relative() const noexcept { return geti(AL_SOURCE_RELATIVE); }
void       SourceView::relative(bool value) noexcept { set(AL_SOURCE_RELATIVE, value ? AL_TRUE : AL_FALSE); }
SourceType SourceView::type() const noexcept { return (SourceType)geti(AL_SOURCE_TYPE); }
void       SourceView::type(SourceType value) noexcept { set(AL_SOURCE_TYPE, (int)value); }

bool        SourceView::looping() const noexcept { return geti(AL_LOOPING); }
void        SourceView::looping(bool value) noexcept { set(AL_LOOPING, value ? AL_TRUE : AL_FALSE); }
BufferView  SourceView::buffer() const noexcept { return BufferView(geti(AL_BUFFER)); }
void        SourceView::buffer(BufferView value) noexcept { set(AL_BUFFER, (int)value); }
SourceState SourceView::state() const noexcept { return (SourceState)geti(AL_SOURCE_STATE); }
void        SourceView::state(SourceState value) noexcept { set(AL_SOURCE_STATE, (int)value); }

unsigned SourceView::buffers_queued() const noexcept { return geti(AL_BUFFERS_QUEUED); }
unsigned SourceView::buffers_processed() const noexcept { return geti(AL_BUFFERS_PROCESSED); }

float  SourceView::sec_offset() const noexcept { return getf(AL_SEC_OFFSET); }
void   SourceView::sec_offset(float value) noexcept { set(AL_SEC_OFFSET, value); }
size_t SourceView::sample_offset() const noexcept { return geti(AL_SAMPLE_OFFSET); }
void   SourceView::sample_offset(size_t value) noexcept { set(AL_SAMPLE_OFFSET, (int)value); }
size_t SourceView::byte_offset() const noexcept { return geti(AL_BYTE_OFFSET); }
void   SourceView::byte_offset(size_t value) noexcept { set(AL_BYTE_OFFSET, (int)value); }

// =============================================================
// == Source =============================================
// =============================================================

Source::Source() noexcept :
	SourceView()
{}

Source::Source(BufferView buffer) noexcept :
	Source()
{
	gen();
	this->buffer(buffer);
}

Source::~Source() noexcept {
	destroy();
}

Source::Source(Source&& src) noexcept : SourceView(std::exchange(src.mHandle, 0)) {}
Source& Source::operator=(Source&& src) noexcept {
	destroy();
	mHandle = std::exchange(src.mHandle, 0);
	return *this;
}

void Source::gen() noexcept {
	destroy();
	alGenSources(1, &mHandle); AL_CHECK_ERROR();
}
void Source::destroy() noexcept {
	if(mHandle) {
		alDeleteSources(1, &mHandle); AL_CHECK_ERROR();
	}
}

// =============================================================
// == Listener =============================================
// =============================================================

void Listener::set(unsigned param, int       value) noexcept {
	alListeneri(param, value); AL_CHECK_ERROR();
}
void Listener::set(unsigned param, float     value) noexcept {
	alListenerf(param, value); AL_CHECK_ERROR();
}
void Listener::set(unsigned param, glm::vec3 value) noexcept {
	alListener3f(param, value.x, value.y, value.z); AL_CHECK_ERROR();
}

int Listener::geti(unsigned param) noexcept {
	int result;
	alGetListeneri(param, &result); AL_CHECK_ERROR();
	return result;
}
float Listener::getf(unsigned param) noexcept {
	float result;
	alGetListenerf(param, &result); AL_CHECK_ERROR();
	return result;
}
glm::vec3 Listener::get3f(unsigned param) noexcept {
	glm::vec3 result;
	alGetListenerfv(param, &result[0]); AL_CHECK_ERROR();
	return result;
}

float Listener::gain() noexcept { return Listener::getf(AL_GAIN); }
void  Listener::gain(float f) noexcept { Listener::set(AL_GAIN, f); }

glm::vec3 Listener::position() noexcept { return Listener::get3f(AL_POSITION); }
void Listener::position(glm::vec3 v) noexcept { Listener::set(AL_POSITION, v); }
glm::vec3 Listener::velocity() noexcept { return Listener::get3f(AL_VELOCITY); }
void Listener::velocity(glm::vec3 v) noexcept { Listener::set(AL_VELOCITY, v); }
void Listener::orientation(glm::vec3 fwd, glm::vec3 up) noexcept {
	glm::vec3 fwdup[] = {fwd, up};
	alListenerfv(AL_ORIENTATION, &fwdup[0][0]); AL_CHECK_ERROR();
}

} // namespace al

/*
 Copyright (c) 2018 Benno Straub

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
