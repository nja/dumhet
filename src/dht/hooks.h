#ifndef _dht_hooks_h
#define _dht_hooks_h

#include <dht/dht.h>
#include <lcthw/darray.h>

DArray *Hooks_Create();
void Hooks_Destroy(DArray *array);

Hook *Hook_Create(HookType type, HookOp fun);
void Hook_Destroy(Hook *hook);

#endif
