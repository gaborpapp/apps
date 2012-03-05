#include "Effect.h"

using namespace ci;
using namespace std;

void Effect::setup( const std::string name )
{
	mApp->console() << name << " setup" << endl;
	mParams = params::PInterfaceGl( name, Vec2i(200, 300));
    mParams.addPersistentSizeAndPosition();
}

