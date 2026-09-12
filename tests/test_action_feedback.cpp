#include "components/smart_display/runtime_model.h"
#include <cassert>
int main(){runtime_tiles::Tile t;t.revision="off";t.begin(100);assert(t.loading(200));assert(t.awaiting_action(200));t.observe("off");assert(!t.confirmed);t.observe("on");assert(t.confirmed);assert(t.loading(999));assert(!t.loading(1100));t.begin(2000);t.observe("on");assert(t.loading(7999));assert(!t.loading(8000));assert(!t.awaiting_action(8000));t.begin(0xFFFFFFF0);assert(t.loading(20));t.begin(100,true);assert(t.loading(1099));assert(!t.awaiting_action(1099));assert(!t.loading(1100));}
