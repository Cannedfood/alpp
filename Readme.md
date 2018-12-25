# alpp
A C++ abstraction of OpenAL

## Compilation
Just compile AL.cpp and link against OpenAL.
You can enable error checking by defining `AL_ERROR_CHECKING`

## Usage

### Static buffer:
```C++
#include <alpp/AL.h>

#include <thread>
#include <chrono>

using namespace std::chrono::literals;

void main() {
	// Load data (Implement this yourself)
	auto audioData = myLoadWavMono16("MyWav.wav", &frequency);

	al::Context context; // The OpenAL context.

	al::Buffer buffer { // Load data into buffer
		audioData.bytes(), audioData.num_bytes(),
		al::Format::Mono16, audioData.frequency
	};

	al::Source source { buffer }; // Create source, set it's buffer to buffer

	source.play(); // Play audio

	// Wait until it finished playing
	while(source.playing()) std::this_thread::sleep(100ms);
}
```

### Streaming buffer: TODO (use Source::enqueueBuffers and Source::buffers_processed)
