#include "xs.h"

void Snapshot_prototype_constructor(xsMachine* the)
{
}

void Snapshot_prototype_destructor(xsMachine* the)
{
}

void Snapshot_prototype_dump(xsMachine* the)
{
  xsResult = xsString("hello from dump!");
}
