#include "engine.h"

int main(int argc, char* argv[])
{
	Engine * engine = Engine::getInstance();

    // Allocate all necessary systems
    engine->load();

    engine->init();

    // Frame end being passed in here
    engine->update(0.f);

    // If we make to here then we should 
    // clean up allocated memory in each system
    engine->unload();

    // Then free the systems
    engine->shutdown();

    return 0;
}
