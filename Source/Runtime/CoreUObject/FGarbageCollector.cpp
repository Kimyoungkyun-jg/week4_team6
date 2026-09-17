#include "FGarbageCollector.h"

#include "FReferenceCollector.h"
#include "FUObjectArray.h"

#include <algorithm>

void FGarbageCollector::AddRoot(UObject* Object)
{
	if (Object == nullptr) return;

	if (std::find(RootObjects.begin(), RootObjects.end(), Object) == RootObjects.end())
		RootObjects.push_back(Object);
}

void FGarbageCollector::RemoveRoot(UObject* Object)
{
	std::erase(RootObjects, Object);
}

void FGarbageCollector::CollectGarbage()
{
	FReferenceCollector Collector;

	for (UObject* Root : RootObjects)
		Collector.AddReferencedObject(Root);

	// 마킹 단계
	Collector.ProcessReferences();

	// 스위프 단계
	FUObjectArray& ObjectArray = FUObjectArray::Get();

	uint32 Index = 0;

	while (Index < ObjectArray.GetNumObjects()) {
		UObject* Object = ObjectArray.GetObjectByIndex(Index);
		if (Collector.bIsReferenced(Object))
			++Index;
		else
			ObjectArray.DestroyObject(Object);
	}
}
