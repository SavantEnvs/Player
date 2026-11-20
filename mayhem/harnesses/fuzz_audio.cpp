// Audio decoders reached from game Sound/Music files: AudioDecoder::Create sniffs the stream and
// picks the decoder (bundled dr_wav for RIFF/WAVE, the built-in FmMidi sequencer for MThd/SMF),
// then Open + SetFormat (resampler) + a bounded Decode loop, as AudioGeneric does on playback.
#include "fuzz_common.h"
#include "audio_decoder.h"
#include "audio_decoder_base.h"

extern "C" int LLVMFuzzerInitialize(int*, char***) {
	QuietOutput();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	if (size > (1u << 20)) return -1;
	auto stream = MemStream(data, size, "fuzz.wav");
	auto dec = AudioDecoder::Create(stream, true);
	if (!dec) return -1;
	if (!dec->Open(std::move(stream))) return -1;
	dec->SetFormat(44100, AudioDecoderBase::Format::S16, 2);
	dec->SetPitch(100);
	std::vector<uint8_t> buf(8192);
	for (int i = 0; i < 256 && !dec->IsFinished(); ++i) {
		if (dec->Decode(buf.data(), (int)buf.size()) <= 0) break;
	}
	(void)dec->GetTicks();
	dec->Rewind();
	dec->Decode(buf.data(), (int)buf.size());
	return 0;
}
