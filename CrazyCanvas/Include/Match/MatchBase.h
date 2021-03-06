#pragma once
#include "LambdaEngine.h"

#include "Utilities/SHA256.h"

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Events/GameplayEvents.h"

#include "Match/MatchGameMode.h"

class Level;

struct CreateFlagDesc;
struct CreatePlayerDesc;

struct MatchDescription
{
	LambdaEngine::SHA256Hash LevelHash;
	EGameMode GameMode	= EGameMode::CTF_TEAM_FLAG;
	uint8 NumTeams		= 2;
	uint32 MaxScore		= 3;
};

static constexpr const float32 MATCH_BEGIN_COUNTDOWN_TIME = 5.0f;

class MatchBase
{
public:
	MatchBase();
	virtual ~MatchBase();

	bool Init(const MatchDescription* pDesc);

	void Tick(LambdaEngine::Timestamp deltaTime);
	void FixedTick(LambdaEngine::Timestamp deltaTime);

	void ResetMatch();

	virtual void BeginLoading()
	{
	}

	virtual void MatchStart()
	{
	}

	FORCEINLINE bool HasBegun() const { return m_HasBegun; }
	FORCEINLINE uint32 GetScore(uint32 teamIndex) const { VALIDATE((teamIndex - 1) < m_Scores.GetSize()); return m_Scores[teamIndex - 1]; }
	FORCEINLINE EGameMode GetGameMode() const { return m_MatchDesc.GameMode; }

	virtual void KillPlaneCallback(LambdaEngine::Entity killPlaneEntity, LambdaEngine::Entity otherEntity) = 0;

protected:
	bool SetScore(uint8 teamIndex, uint32 score);

	virtual bool InitInternal() = 0;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) = 0;
	virtual bool ResetMatchInternal() = 0;

	virtual void FixedTickInternal(LambdaEngine::Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	virtual bool OnWeaponFired(const WeaponFiredEvent& event) = 0;
	
protected:
	//Level
	Level* m_pLevel = nullptr;

	//Score
	LambdaEngine::TArray<uint32> m_Scores;

	//Match Description
	MatchDescription m_MatchDesc;
	bool m_GameModeHasFlag = false;

	//Match Running Variables
	bool m_HasBegun = false;
	float32 m_MatchBeginTimer = 0.0f;
};