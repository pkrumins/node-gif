#include <node.h>

#include "gif.h"
//#include "fixed_gif_stack.h"
#include "dynamic_gif_stack.h"

using namespace v8;

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;
    Gif::Initialize(target);
    //FixedGifStack::Initialize(target);
    DynamicGifStack::Initialize(target);
}

