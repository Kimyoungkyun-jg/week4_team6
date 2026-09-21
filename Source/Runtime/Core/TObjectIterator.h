#pragma once

#include <cassert>

#include "Runtime/Core/IntTypes.h"
#include "Runtime/CoreUObject/FUObjectArray.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/UClass.h"

class FObjectIterator
{
private:
    uint32  CurrentIndex;
    UClass* TargetClass;


    void AdvanceToNextValidObject() {
        while (true)
        {
            FUObjectArray& ObjectArray = FUObjectArray::Get();
            if (CurrentIndex >= ObjectArray.GetMaxIndex())
                return;
            UObject* Object = ObjectArray.GetObjectByIndex(CurrentIndex);
            if (Object && Object->IsA(TargetClass))
                return;
            ++CurrentIndex;
        }
    }

public:

    explicit FObjectIterator(UClass* Class = UObject::StaticClass()) : CurrentIndex(0), TargetClass(Class)
    {
        assert(UClass::AreTypeBitsetsResolved() && "UClass::ResolveTypeBitsets() not call");
        AdvanceToNextValidObject();
    }

    FObjectIterator& operator++()
    {
        ++CurrentIndex;
        AdvanceToNextValidObject();
        return *this;
    }

    UObject* operator*() const
    {
        return FUObjectArray::Get().GetObjectByIndex(CurrentIndex);
    }

    bool operator==(const FObjectIterator& ref) const
    {
        return (this->CurrentIndex == ref.CurrentIndex && 
            this->TargetClass == ref.TargetClass);
    }

    bool operator!=(const FObjectIterator& ref) const
    {
        return (!(*this == ref));
    }

    UObject* operator->() const
    {
        return FUObjectArray::Get().GetObjectByIndex(CurrentIndex);
    }

    explicit operator bool() const
    {
        return (CurrentIndex < FUObjectArray::Get().GetMaxIndex());
    }
};


template<typename TObject>
class TObjectIterator
{
public:
    TObjectIterator() : Inner(TObject::StaticClass()) {}

    explicit operator bool() const 
    { 
        return static_cast<bool>(Inner); 
    }

    TObject* operator*() const 
    {
        return static_cast<TObject*>(*Inner); 
    }

    TObject* operator->() const 
    {
        return static_cast<TObject*>(*Inner); 
    }

    TObjectIterator& operator++() 
    { 
        ++Inner; return *this; 
    }

    bool operator==(const TObjectIterator& o) const 
    { 
        return Inner == o.Inner; 
    }

    bool operator!=(const TObjectIterator& o) const 
    { 
        return Inner != o.Inner; 
    }

private:
    FObjectIterator Inner;
};