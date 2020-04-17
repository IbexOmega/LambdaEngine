#include "Engine/EngineLoop.h"

#include "Memory/Memory.h"

namespace LambdaEngine
{
    extern Game* CreateGame();
}

#ifdef LAMBDA_PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
#else
int main(int, const char*[])
#endif 
{
	using namespace LambdaEngine;

	SET_DEBUG_FLAGS();

#ifdef LAMBDA_PLATFORM_WINDOWS
	if (!EngineLoop::PreInit(hInstance))
#else
	if (!EngineLoop::PreInit())
#endif
	{
		return -1;
	}

	if (!EngineLoop::Init())
	{
		return -1;
	}

	Game* pGame = CreateGame();	
	EngineLoop::Run();

    SAFEDELETE(pGame);

    if (!EngineLoop::Release())
    {
        return -1;
    }
    
	if (!EngineLoop::PostRelease())
	{
		return -1;
	}

	return 0;
}
