#include <cstdlib>
#include <cassert>
#include "common.h"

using namespace v8;

Handle<Value>
ErrorException(const char *msg)
{
    HandleScope scope;
    return Exception::Error(String::New(msg));
}

Handle<Value>
VException(const char *msg) {
    HandleScope scope;
    return ThrowException(ErrorException(msg));
}

bool str_eq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

