#pragma once

#include "PortAudio.h"
#include "Audio/API/ISoundEffect3D.h"
#include <WavLib.h>

namespace LambdaEngine
{
	class IAudioDevice;

	class SoundEffect3DLambda : public ISoundEffect3D
	{
	public:
		SoundEffect3DLambda(const IAudioDevice* pAudioDevice);
		~SoundEffect3DLambda();

		virtual bool Init(const SoundEffect3DDesc& desc) override final;
		virtual void PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch) override final;

		FORCEINLINE const byte* GetWaveform()	{ return m_pWaveForm; }
		FORCEINLINE uint32 GetSampleCount()		{ return m_Header.SampleCount; }
		FORCEINLINE uint32 GetSampleRate()		{ return m_Header.SampleRate; }
		FORCEINLINE uint32 GetChannelCount()	{ return m_Header.ChannelCount; }
		
	private:		
		byte* m_pWaveForm;
		WaveFile m_Header;
	};
}