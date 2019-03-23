// Copyright (c) 2018 Benno Straub, licensed under the MIT license. (A copy can be found at the bottom of this file)

#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>

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

class Context {
public:
	Context(std::nullptr_t) noexcept;

	Context() noexcept;
	~Context() noexcept;

	void init() noexcept;
	void close() noexcept;

private:
	void* mContextHandle;
};

class BufferView {
protected:
	unsigned mHandle;
public:
	explicit
	BufferView(unsigned handle = 0) noexcept;
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
	Buffer() noexcept;
	~Buffer() noexcept;

	Buffer(void const* data, size_t size, Format fmt, unsigned freq) noexcept;

	Buffer(Buffer&& buf) noexcept;
	Buffer& operator=(Buffer&& buf) noexcept;

	void gen() noexcept;
	void destroy() noexcept;
};

class SourceView {
protected:
	unsigned mHandle;
public:
	SourceView(unsigned handle = 0) noexcept;

	void play();
	void pause();
	void stop();
	void rewind();
	void queueBuffers(al::BufferView const* buffers, size_t count);
	void unqueueBuffers(al::BufferView* buffers, size_t count);


	float     getf(unsigned prop) const noexcept;
	int       geti(unsigned prop) const noexcept;
	glm::vec3 get3f(unsigned prop) const noexcept;
	void      set(unsigned prop,  float) const noexcept;
	void      set(unsigned prop,  int) const noexcept;
	void      set(unsigned prop, glm::vec3) const noexcept;

	float pitch() const noexcept; //<! Pitch multiplier, always positive
	void  pitch(float) noexcept;
	float gain() const noexcept; //<! Gain
	void  gain(float) noexcept;
	float max_distance() const noexcept; //<! used with the Inverse Clamped Distance Model to set the distance where there will no longer be any attenuation of the source
	void  max_distance(float) noexcept;
	float rolloff_factor() const noexcept; //<! the rolloff rate for the source
	void  rolloff_factor(float) noexcept;
	float reference_distance() const noexcept; //<! the distance under which the volume for the source would normally drop by half (before being influenced by rolloff factor or AL_MAX_DISTANCE)
	void  reference_distance(float) noexcept;

	float min_gain() const noexcept; //<! the minimum gain for this source
	void  min_gain(float) noexcept;
	float max_gain() const noexcept; //<! the maximum gain for this source
	void  max_gain(float) noexcept;
	float cone_outer_gain() const noexcept; //<! the gain when outside the oriented cone
	void  cone_outer_gain(float) noexcept;
	float cone_inner_angle() const noexcept; //<! the gain when inside the oriented cone
	void  cone_inner_angle(float) noexcept;
	float cone_outer_angle() const noexcept; //<! outer angle of the sound cone, in degrees default is 360
	void  cone_outer_angle(float) noexcept;

	glm::vec3 position() const noexcept; //<! X, Y, Z position
	void      position(glm::vec3) noexcept;
	glm::vec3 velocity() const noexcept; //<! velocity vector
	void      velocity(glm::vec3) noexcept;
	glm::vec3 direction() const noexcept; //<! direction vector
	void      direction(glm::vec3) noexcept;

	bool       relative() const noexcept; //<! determines if the positions are relative to the listener
	void       relative(bool) noexcept;
	SourceType type() const noexcept; //<! the source type – AL_UNDETERMINED, AL_STATIC, or AL_STREAMING
	void       type(SourceType) noexcept;

	bool        looping() const noexcept; //<! turns looping on (AL_TRUE) or off (AL_FALSE)
	void        looping(bool) noexcept;
	BufferView  buffer() const noexcept; //<! the ID of the attached buffer
	void        buffer(BufferView) noexcept;
	SourceState state() const noexcept; //<! the state of the source
	void        state(SourceState) noexcept;
	bool        playing() const noexcept { return state() == SourceState::Playing; }
	bool        paused()  const noexcept { return state() == SourceState::Paused; }
	bool        stopped() const noexcept { return state() == SourceState::Stopped; }

	unsigned buffers_queued() const noexcept; //<! the number of buffers queued on this source
	unsigned buffers_processed() const noexcept; //<! the number of buffers in the queue that have been processed

	float  sec_offset() const noexcept; //<! the playback position, expressed in seconds
	void   sec_offset(float) noexcept;
	size_t sample_offset() const noexcept; //<! the playback position, expressed in samples
	void   sample_offset(size_t) noexcept;
	size_t byte_offset() const noexcept; //<! the playback position, expressed in bytes
	void   byte_offset(size_t) noexcept;

	operator bool() const noexcept { return mHandle != 0; }
};

class Source : public SourceView {
public:
	Source() noexcept;
	explicit Source(BufferView buffer) noexcept;
	~Source() noexcept;

	Source(Source&& src) noexcept;
	Source& operator=(Source&& src) noexcept;

	void gen() noexcept;
	void destroy() noexcept;
};

namespace Listener {
	void set(unsigned param, int       value) noexcept;
	void set(unsigned param, float     value) noexcept;
	void set(unsigned param, glm::vec3 value) noexcept;

	int       geti(unsigned param) noexcept;
	float     getf(unsigned param) noexcept;
	glm::vec3 get3f(unsigned param) noexcept;

	float gain() noexcept; //<! “master gain”
	void  gain(float f) noexcept;

	glm::vec3 position() noexcept; //<! X, Y, Z position
	void position(glm::vec3 v) noexcept;
	glm::vec3 velocity() noexcept; //<! velocity vector
	void velocity(glm::vec3 v) noexcept;
	void orientation(glm::vec3 fwd, glm::vec3 up) noexcept; //<! orientation expressed as “at” and “up” vectors
}

} // namespace al

/*
 Copyright (c) 2018 Benno Straub

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
