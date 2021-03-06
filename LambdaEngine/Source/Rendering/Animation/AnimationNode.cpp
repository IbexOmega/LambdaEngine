#include "Rendering/Animation/AnimationNode.h"
#include "Rendering/Animation/AnimationGraph.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	/*
	* BinaryInterpolator
	*/

	void BinaryInterpolator::Interpolate(float32 factor)
	{
		VALIDATE(In0.GetSize() == In1.GetSize());

		const uint32 size			= In0.GetSize();
		const float32 realFactor	= glm::clamp(factor, 0.0f, 1.0f);

		if (Out.GetSize() < size)
		{
			Out.Resize(size);
		}

		for (uint32 i = 0; i < size; i++)
		{
			const SQT& in0	= In0[i];
			const SQT& in1	= In1[i];
			SQT& out		= Out[i];

			out.Translation	= glm::mix(in0.Translation, in1.Translation, realFactor);
			out.Scale		= glm::mix(in0.Scale, in1.Scale, realFactor);
			out.Rotation	= glm::slerp(in0.Rotation, in1.Rotation, realFactor);
			out.Rotation	= glm::normalize(out.Rotation);

			JointIndexType jointID = INVALID_JOINT_ID;
			if (in0.JointID != INVALID_JOINT_ID && in1.JointID != INVALID_JOINT_ID) [[likely]]
			{
				VALIDATE(in0.JointID == in1.JointID);
				jointID = in0.JointID;
			}
			else
			{
				if (in0.JointID != INVALID_JOINT_ID)
				{
					VALIDATE(in0.JointID == i);
					jointID = in0.JointID;
				}
				else if (in1.JointID != INVALID_JOINT_ID)
				{
					VALIDATE(in1.JointID == i);
					jointID = in1.JointID;
				}
			}

			out.JointID = jointID;
		}
	}

	/*
	* ClipNode
	*/

	ClipNode::ClipNode(AnimationState* pParent, GUID_Lambda animationGUID, float64 playbackSpeed, bool isLooping)
		: AnimationNode(pParent)
		, m_AnimationGUID(animationGUID)
		, m_pAnimation(nullptr)
		, m_IsLooping(isLooping)
		, m_NumLoops(1)
		, m_OrignalPlaybackSpeed(playbackSpeed)
		, m_PlaybackSpeed(playbackSpeed)
		, m_RunningTime(0.0)
		, m_NormalizedTime(0.0)
		, m_LocalTimeInSeconds(0.0)
		, m_DurationInSeconds(0.0)
		, m_FrameData()
	{
		Animation* pAnimation = ResourceManager::GetAnimation(animationGUID);
		if (pAnimation)
		{
			m_DurationInSeconds = pAnimation->DurationInSeconds();
			m_pAnimation = pAnimation;
		}

		if (m_IsLooping)
		{
			m_NumLoops = INFINITE_LOOPS;
		}
	}

	void ClipNode::Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds)
	{
		// Get localtime for the animation-clip
		m_RunningTime += deltaTimeInSeconds;
		float64 localTime = m_RunningTime * fabs(m_PlaybackSpeed);
		if (m_IsLooping)
		{
			if (m_NumLoops != INFINITE_LOOPS)
			{
				const float64 totalDuration = m_NumLoops * m_DurationInSeconds;
				localTime = glm::clamp(localTime, 0.0, totalDuration);
			}

			localTime = fmod(localTime, m_DurationInSeconds);

			// If we are equal to the duration or if localtime got flipped back to the beginning the loop finished
			if (localTime >= m_DurationInSeconds || localTime < m_LocalTimeInSeconds)
			{
				m_LoopFinished = true;
			}
		}
		else
		{
			localTime = glm::clamp(localTime, 0.0, m_DurationInSeconds);
			if (localTime >= m_DurationInSeconds)
			{
				m_LoopFinished = true;
			}
		}

		// Reset loop
		if (IsLoopFinished())
		{
			OnLoopFinish();
		}

		m_LocalTimeInSeconds	= localTime;
		m_NormalizedTime		= m_LocalTimeInSeconds / m_DurationInSeconds;
		if (m_PlaybackSpeed < 0.0)
		{
			m_NormalizedTime = 1.0 - m_NormalizedTime;
		}

		// Make sure we have enough matrices
		Animation& animation = *m_pAnimation;
		const uint32 numJoints = skeleton.Joints.GetSize();
		if (m_FrameData.GetSize() < numJoints)
		{
			m_FrameData.Resize(numJoints, SQT(glm::vec3(0.0f), glm::vec3(1.0f), glm::identity<glm::quat>()));
		}

		const float64 timestamp	= m_NormalizedTime * animation.DurationInTicks;
		for (Animation::Channel& channel : animation.Channels)
		{
			// Retrive the bone ID
			auto it = skeleton.JointMap.find(channel.Name);
			if (it != skeleton.JointMap.end())
			{
				// Sample SQT for this animation
				glm::vec3 position	= SamplePosition(channel, timestamp);
				glm::quat rotation	= SampleRotation(channel, timestamp);
				glm::vec3 scale		= SampleScale(channel, timestamp);

				const JointIndexType jointID = it->second;
				m_FrameData[jointID] = SQT(position, scale, rotation, jointID);
			}
		}

		// Handle triggers
		if (m_Triggers.GetSize() > 0)
		{
			for (ClipTrigger& trigger : m_Triggers)
			{
				if (!trigger.IsTriggered)
				{
					constexpr float64 EPSILON = 0.025;
					if (trigger.TriggerAt >= (m_NormalizedTime - EPSILON) && trigger.TriggerAt <= (m_NormalizedTime + EPSILON))
					{
						AnimationGraph& graph = *m_pParent->GetOwner();
						trigger.Func(*this, graph);
						trigger.IsTriggered = true;
						break;
					}
				}
			}
		}
	}

	glm::vec3 ClipNode::SamplePosition(Animation::Channel& channel, float64 time)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numPositions = m_IsLooping ? channel.Positions.GetSize() - 1 : channel.Positions.GetSize();

		Animation::Channel::KeyFrame pos0 = channel.Positions[0];
		Animation::Channel::KeyFrame pos1 = channel.Positions[0];
		if (numPositions > 1)
		{
			for (uint32 i = 0; i < (numPositions - 1); i++)
			{
				if (time <= channel.Positions[i + 1].Time)
				{
					pos0 = channel.Positions[i];
					pos1 = channel.Positions[i + 1];
					break;
				}
			}
		}

		const float64 factor	= (pos1.Time != pos0.Time) ? (time - pos0.Time) / (pos1.Time - pos0.Time) : 0.0f;
		glm::vec3 position		= glm::mix(pos0.Value, pos1.Value, glm::vec3(float32(factor)));
		return position;
	}

	glm::vec3 ClipNode::SampleScale(Animation::Channel& channel, float64 time)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numScales = m_IsLooping ? channel.Scales.GetSize() - 1 : channel.Scales.GetSize();

		Animation::Channel::KeyFrame scale0 = channel.Scales[0];
		Animation::Channel::KeyFrame scale1 = channel.Scales[0];
		if (numScales > 1)
		{
			for (uint32 i = 0; i < (numScales - 1); i++)
			{
				if (time <= channel.Scales[i + 1].Time)
				{
					scale0 = channel.Scales[i];
					scale1 = channel.Scales[i + 1];
					break;
				}
			}
		}

		const float64 factor = (scale1.Time != scale0.Time) ? (time - scale0.Time) / (scale1.Time - scale0.Time) : 0.0f;
		glm::vec3 scale = glm::mix(scale0.Value, scale1.Value, glm::vec3(float32(factor)));
		return scale;
	}

	glm::quat ClipNode::SampleRotation(Animation::Channel& channel, float64 time)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numRotations = m_IsLooping ? channel.Rotations.GetSize() - 1 : channel.Rotations.GetSize();

		Animation::Channel::RotationKeyFrame rot0 = channel.Rotations[0];
		Animation::Channel::RotationKeyFrame rot1 = channel.Rotations[0];
		if (numRotations > 1)
		{
			for (uint32 i = 0; i < (numRotations - 1); i++)
			{
				if (time <= channel.Rotations[i + 1].Time)
				{
					rot0 = channel.Rotations[i];
					rot1 = channel.Rotations[i + 1];
					break;
				}
			}
		}

		const float64 factor = (rot1.Time != rot0.Time) ? (time - rot0.Time) / (rot1.Time - rot0.Time) : 0.0;
		glm::quat rotation	= glm::slerp(rot0.Value, rot1.Value, float32(factor));
		rotation			= glm::normalize(rotation);
		return rotation;
	}

	void ClipNode::OnLoopFinish()
	{
		ResetTriggers();
		m_LoopFinished = false;
	}

	/*
	* BlendNode
	*/

	BlendNode::BlendNode(AnimationState* pParent, AnimationNode* pIn0, AnimationNode* pIn1, const BlendInfo& blendInfo)
		: AnimationNode(pParent)
		, m_pIn0(pIn0)
		, m_pIn1(pIn1)
		, m_BlendInfo(blendInfo)
	{
		VALIDATE(m_pIn0 != nullptr);
		VALIDATE(m_pIn1 != nullptr);
	}

	void BlendNode::Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds)
	{
		// Tick both
		if (m_BlendInfo.ShouldSync)
		{
			const float64 duration0 = m_pIn0->GetDurationInSeconds();
			m_pIn1->MatchDuration(duration0);
		}

		m_pIn0->Tick(skeleton, deltaTimeInSeconds);
		m_pIn1->Tick(skeleton, deltaTimeInSeconds);

		// We need to store a copy since we might change the content, and we cant change the clip data
		// What if a clip is used in mulitple states?
		m_In0 = m_pIn0->GetResult();
		m_In1 = m_pIn1->GetResult();

		VALIDATE(m_In0.GetSize() == skeleton.Joints.GetSize());

		// Make sure we do not use unwanted joints
		if (m_BlendInfo.ClipLimit1 != "")
		{
			auto joint = skeleton.JointMap.find(m_BlendInfo.ClipLimit1);
			if (joint != skeleton.JointMap.end())
			{
				JointIndexType clipLimit = joint->second;
				for (uint32 i = 0; i < m_In1.GetSize(); i++)
				{
					JointIndexType myID = static_cast<JointIndexType>(i);
					if (!FindLimit(skeleton, myID, clipLimit))
					{
						m_In1[i] = m_In0[i];
					}
				}
			}
		}

		BinaryInterpolator interpolator(m_In0, m_In1, m_FrameData);
		interpolator.Interpolate(m_BlendInfo.WeightIn1);
	}
	
	bool BlendNode::FindLimit(const Skeleton& skeleton, JointIndexType parentID, JointIndexType clipLimit)
	{
		if (parentID == clipLimit)
		{
			return true;
		}
		else if (parentID == INVALID_JOINT_ID)
		{
			return false;
		}
		else
		{
			const Joint& joint = skeleton.Joints[parentID];
			return FindLimit(skeleton, joint.ParentBoneIndex, clipLimit);
		}
	}
}