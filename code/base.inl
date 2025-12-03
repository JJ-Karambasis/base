#ifndef BASE_INL_H
#define BASE_INL_H

#ifdef __cplusplus

#include <initializer_list>

function inline v2 operator+(v2 A, v2 B) {
	return V2_Add_V2(A, B);
}

function inline v2 operator-(v2 A, v2 B) {
	return V2_Sub_V2(A, B);
}

function inline v2 operator*(v2 A, f32 B) {
	return V2_Mul_S(A, B);
}

function inline v2 operator/(v2 A, f32 B) {
	return V2_Div_S(A, B);
}

function inline v2& operator+=(v2& A, v2 B) {
	A = V2_Add_V2(A, B);
	return A;
}

function inline v2& operator*=(v2& A, f32 B) {
	A = V2_Mul_S(A, B);
	return A;
}

function inline v2i operator+(v2i A, s32 B) {
	return V2i(A.x+B, A.y+B);
}

function inline v2i operator-(v2i A, s32 B) {
	return V2i(A.x-B, A.y-B);
}

function inline v2i operator/(v2i A, s32 B) {
	return V2i(A.x/B, A.y/B);
}

function inline bool operator!=(v2i A, v2i B) {
	return A.x != B.x || A.y != B.y;
}

function inline v3 operator+(v3 A, v3 B) {
	v3 Result = V3_Add_V3(A, B);
	return Result;
}

function inline v3& operator+=(v3& A, v3 B) {
	A = V3_Add_V3(A, B);
	return A;
}

function inline v3 operator*(v3 A, f32 B) {
	v3 Result = V3_Mul_S(A, B);
	return Result;
}

function inline v3& operator*=(v3& A, f32 B) {
	A = V3_Mul_S(A, B);
	return A;
}

function inline quat operator*(quat A, quat B) {
	quat Result = Quat_Mul_Quat(A, B);
	return Result;
}

function inline m4 operator*(const m4_affine& A, const m4& B) {
	m4 Result = M4_Affine_Mul_M4(&A, &B);
	return Result;
}

function inline v4 operator*(v4 A, const m4& B) {
    v4 Result = V4_Mul_M4(A, &B);
    return Result;
}

template <typename type>
struct span {
	const type* Ptr = NULL;
	size_t 		Count = 0;

	span() = default;
	inline span(std::initializer_list<type> List) {
		Ptr = List.begin();
		Count = List.size();
	}

	inline span(const type* _Ptr, size_t _Count) : Ptr(_Ptr), Count(_Count) {}

	inline const type& operator[](size_t Index) const {
		Assert(Index < Count);
		return Ptr[Index];
	}
};

template <typename type>
struct array {
	allocator* Allocator = NULL;
	type* Ptr 			 = NULL;
	size_t Count 		 = 0;
	size_t Capacity 	 = 0;

	array() = default;
	inline array(allocator* _Allocator) : Allocator(_Allocator) { }
	
	array(allocator* Allocator, size_t Count);
	array(allocator* _Allocator, type* _Ptr, size_t _Count);

	inline type& operator[](size_t Index) {
		Assert(Index < Count);
		return Ptr[Index];
	}

    inline const type& operator[](size_t Index) const {
		Assert(Index < Count);
		return Ptr[Index];
	}
};

template <typename type>
function inline void Array_Init(array<type>* Array, allocator* Allocator = Default_Allocator_Get()) {
	*Array = array<type>(Allocator);
}

template<typename type>
function inline void Array_Reserve(array<type>* Array, size_t NewCapacity) {
	type* NewPtr = Allocator_Allocate_Array(Array->Allocator, NewCapacity, type);

	if(Array->Ptr) {
		size_t CopyCapacity = Min(NewCapacity, Array->Capacity);
		Memory_Copy(NewPtr, Array->Ptr, CopyCapacity*sizeof(type));
		Allocator_Free_Memory(Array->Allocator, Array->Ptr);
	}

	Array->Ptr = NewPtr;
	Array->Capacity = NewCapacity;
}

template<typename type>
function inline void Array_Resize(array<type>* Array, size_t NewSize) {
	if (NewSize > Array->Capacity) {
		Array_Reserve(Array, NewSize);
	}
	Array->Count = NewSize;
}

template <typename type>
function inline void Array_Add(array<type>* Array, const type& Entry) {
	if(Array->Count == Array->Capacity) {
		size_t NewCapacity = Array->Capacity ? Array->Capacity*2 : 32;
		Array_Reserve(Array, NewCapacity);
	}

	Array->Ptr[Array->Count++] = Entry;
}

template <typename type>
function inline void Array_Clear(array<type>* Array) {
	Array->Count = 0;
}

template<typename type>
function inline void Array_Copy(array<type>* Dst, const array<type>* Src) {
	size_t CopyCapacity = Min(Dst->Count, Src->Count);
	Memory_Copy(Dst->Ptr, Src->Ptr, CopyCapacity * sizeof(type));
}

template<typename type>
inline array<type>::array(allocator* _Allocator, type* _Ptr, size_t _Count) : Allocator(_Allocator) {
	Array_Resize(this, _Count);
	Memory_Copy(Ptr, _Ptr, Count * sizeof(type));
}

template<typename type>
inline array<type>::array(allocator* _Allocator, size_t _Count) : Allocator(_Allocator) {
	Array_Resize(this, _Count);
}

template <typename type>
struct pool_handle {
	pool_id ID;
};

template<typename type>
struct pool_t : public pool {
	
};

template<typename type>
function inline void Pool_Init(pool_t<type>* Pool) {
	Pool_Init(Pool, sizeof(type));
}

template<typename type>
function inline pool_handle<type> Pool_Allocate(pool_t<type>* Pool) {
	pool_handle<type> Result = {
		.ID = Pool_Allocate((pool*)Pool)
	};
	return Result;
}

template<typename type>
function inline void Pool_Free(pool_t<type>* Pool, pool_handle<type> Handle) {
	Pool_Free((pool*)Pool, Handle.ID);
}

template<typename type>
function inline type* Pool_Get(pool_t<type>* Pool, pool_handle<type> Handle) {
	return (type*)Pool_Get((pool*)Pool, Handle.ID);
}

template <typename type>
struct hasher {
    u32 Hash(const type& Value) const;
};

template <typename type>
struct comparer {
    b32 Equal(const type& A, const type& B) const;
};

inline void Hash_Combine(u32& seed) { }

template <typename type, typename... rest, typename hasher=hasher<type>>
inline void Hash_Combine(u32& Seed, const type& Value, rest... Rest) {
    Seed ^= hasher{}.Hash(Value) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
    Hash_Combine(Seed, Rest...); 
}

template <typename type, typename... rest, typename hasher=hasher<type>>
inline u32 Hash_Combine(const type& Value, rest... Rest) {
    u32 Result = hasher{}.Hash(Value);
    Hash_Combine(Result, Rest...);
    return Result;
}

template <>
struct hasher<u32> {
    inline u32 Hash(u32 Key) {
        return U32_Hash_U32(Key);
    }
};

template <>
struct comparer<u32> {
    inline b32 Equal(u32 A, u32 B) {
        return A == B;
    }
};

template <>
struct hasher<u64> {
    inline u32 Hash(u64 Key) {
		return U32_Hash_U64(Key);
    }
};

template <>
struct comparer<u64> {
    inline b32 Equal(u64 A, u64 B) {
        return A == B;
    }
};

template <>
struct hasher<s32> {
    inline u32 Hash(s32 Key) {
        return U32_Hash_U32((u32)Key);
    }
};

template <>
struct comparer<s32> {
    inline b32 Equal(s32 A, s32 B) {
        return A == B;
    }
};

template <>
struct hasher<s64> {
    inline u32 Hash(s64 Key) {
		return U32_Hash_U64((u64)Key);
    }
};

template <>
struct comparer<s64> {
    inline b32 Equal(s64 A, s64 B) {
        return A == B;
    }
};

template <>
struct hasher<string> {
    inline u32 Hash(const string& Str) {
		return U32_Hash_String(Str);
    }
};

template <>
struct comparer<string> {
    inline b32 Equal(const string& A, const string& B) {
        return String_Equals(A, B);
    }
};

template <>
struct hasher<void*> {
    inline u32 Hash(const void* Ptr) {
		return U32_Hash_Ptr(Ptr);
    }
};

template <>
struct comparer<void*> {
    inline bool Equal(const void* A, const void* B) {
        return A == B;
    }
};

template <typename key, typename value, typename hasher = hasher<key>, typename comparer = comparer<key>>
struct hashmap_t {
    static const u32 INVALID = HASH_INVALID_SLOT;

    allocator* Allocator = nullptr;
    hash_slot* Slots = nullptr;
    key*       Keys = nullptr;
    value*     Values = nullptr;
    u32*       ItemSlots = nullptr;
    u32        SlotCapacity = 0;
    u32        ItemCapacity = 0;
    u32        Count = 0;

	hashmap_t() = default;
    inline hashmap_t(allocator* _Allocator) : Allocator(_Allocator) {}

	value& operator[](const key& Key);
};

function inline u32 Expand_Slots(allocator* Allocator, hash_slot** Slots, u32 SlotCapacity, u32* ItemSlots) {
    u32 OldCapacity = SlotCapacity;
    SlotCapacity = SlotCapacity ? SlotCapacity*2 : 128;
    u32 SlotMask = SlotCapacity-1;
    hash_slot* NewSlots = (hash_slot*)Allocator_Allocate_Array(Allocator, SlotCapacity, hash_slot);
    
    for(u32 SlotIndex = 0; SlotIndex < SlotCapacity; SlotIndex++) {
        NewSlots[SlotIndex].ItemIndex = HASH_INVALID_SLOT;
    }
    
    for(u32 i = 0; i < OldCapacity; i++)
    {
        if((*Slots)[i].ItemIndex != HASH_INVALID_SLOT)
        {
            u32 Hash = (*Slots)[i].Hash;
            u32 BaseSlot = (Hash & SlotMask); 
            u32 Slot = BaseSlot;
            while(NewSlots[Slot].ItemIndex != HASH_INVALID_SLOT)
                Slot = (Slot + 1) & SlotMask;
            NewSlots[Slot].Hash = Hash;
            u32 ItemIndex = (*Slots)[i].ItemIndex;
            NewSlots[Slot].ItemIndex = ItemIndex;
            ItemSlots[ItemIndex] = Slot;
            NewSlots[BaseSlot].BaseCount++;
        }
    }

    if(*Slots) Allocator_Free_Memory(Allocator, *Slots);
    *Slots = NewSlots;
    return SlotCapacity;
}


template <typename key, typename value>
function inline u32 Expand_Items(allocator* Allocator, key** Keys, value** Values, u32** ItemSlots, u32 ItemCapacity) {
    u32 OldItemCapacity = ItemCapacity;
    ItemCapacity = ItemCapacity == 0 ? 64 : ItemCapacity*2;

    size_t ItemSize = sizeof(key)+sizeof(u32);
    if(Values) ItemSize += sizeof(value);

    key* NewKeys = (key*)Allocator_Allocate_Memory(Allocator, ItemSize*ItemCapacity);
    value* NewValues = Values ? (value*)(NewKeys+ItemCapacity) : nullptr;
    u32* NewItemSlots = Values ? (u32*)(NewValues+ItemCapacity) : (u32*)(NewKeys+ItemCapacity);

    for(u32 ItemIndex = 0; ItemIndex < ItemCapacity; ItemIndex++)
        NewItemSlots[ItemIndex] = HASH_INVALID_SLOT;

    if(*Keys) {
		Memory_Copy(NewKeys, *Keys, OldItemCapacity * sizeof(key));
        if(NewValues) Memory_Copy(NewValues, *Values, OldItemCapacity*sizeof(value));
        Memory_Copy(NewItemSlots, *ItemSlots, OldItemCapacity*sizeof(u32));
        Allocator_Free_Memory(Allocator, *Keys); 
    }

    *Keys = NewKeys;
    if(Values) *Values = NewValues;
    *ItemSlots = NewItemSlots;
    return ItemCapacity;
}

template <typename key, typename comparer>
function inline u32 Find_Slot(hash_slot* Slots, u32 SlotCapacity, key* Keys, const key& Key, u32 Hash) {
    if(SlotCapacity == 0 || !Slots) return HASH_INVALID_SLOT;

    u32 SlotMask = SlotCapacity-1;
    u32 BaseSlot = (Hash & SlotMask);
    u32 BaseCount = Slots[BaseSlot].BaseCount;
    u32 Slot = BaseSlot;
    
    while (BaseCount > 0) {
        if (Slots[Slot].ItemIndex != HASH_INVALID_SLOT) {
            u32 SlotHash = Slots[Slot].Hash;
            u32 SlotBase = (SlotHash & SlotMask);
            if (SlotBase == BaseSlot) {
                Assert(BaseCount > 0);
                BaseCount--;
                            
                if (SlotHash == Hash) { 
                    comparer Comparer = {};
                    if(Comparer.Equal(Key, Keys[Slots[Slot].ItemIndex]))
                        return Slot;
                }
            }
        }
        
        Slot = (Slot + 1) & SlotMask;
    }
    
    return HASH_INVALID_SLOT;
}

function inline u32 Find_Free_Slot(hash_slot* Slots, u32 SlotMask, u32 BaseSlot) {
    u32 BaseCount = Slots[BaseSlot].BaseCount;
    u32 Slot = BaseSlot;
    u32 FirstFree = Slot;
    while (BaseCount) 
    {
        if (Slots[Slot].ItemIndex == HASH_INVALID_SLOT && Slots[FirstFree].ItemIndex != HASH_INVALID_SLOT) FirstFree = Slot;
        u32 SlotHash = Slots[Slot].Hash;
        u32 SlotBase = (SlotHash & SlotMask);
        if (SlotBase == BaseSlot) 
            --BaseCount;
        Slot = (Slot + 1) & SlotMask;
    }
    
    Slot = FirstFree;
    while (Slots[Slot].ItemIndex != HASH_INVALID_SLOT) 
        Slot = (Slot + 1) & SlotMask;

    return Slot;
}

template<typename key, typename value, typename hasher, typename comparer>
function inline void Hashmap_Init(hashmap_t<key, value, hasher, comparer>* Hashmap, allocator* Allocator) {
	*Hashmap = hashmap_t<key, value, hasher, comparer>(Allocator);
}

template<typename key, typename value, typename hasher, typename comparer>
function inline void Hashmap_Add(hashmap_t<key, value, hasher, comparer>* Hashmap, const key& Key, const value& Value) {
	u32 Hash = hasher{}.Hash(Key);
    Assert((Find_Slot<key, comparer>(Hashmap->Slots, Hashmap->SlotCapacity, Hashmap->Keys, Key, Hash) == -1));
    
    if(Hashmap->Count >= (Hashmap->SlotCapacity - (Hashmap->SlotCapacity/3)))
        Hashmap->SlotCapacity = Expand_Slots(Hashmap->Allocator, &Hashmap->Slots, Hashmap->SlotCapacity, Hashmap->ItemSlots);

    u32 SlotMask = Hashmap->SlotCapacity-1;
    u32 BaseSlot = (Hash & SlotMask);
    u32 Slot = Find_Free_Slot(Hashmap->Slots, SlotMask, BaseSlot);
    
    if (Hashmap->Count >= Hashmap->ItemCapacity)
        Hashmap->ItemCapacity = Expand_Items(Hashmap->Allocator, &Hashmap->Keys, &Hashmap->Values, &Hashmap->ItemSlots, Hashmap->ItemCapacity);
    
    Assert(Hashmap->Count < Hashmap->ItemCapacity);
    Assert(Hashmap->Slots[Slot].ItemIndex == HASH_INVALID_SLOT && (Hash & SlotMask) == BaseSlot);
    
    Hashmap->Slots[Slot].Hash = Hash;
    Hashmap->Slots[Slot].ItemIndex = Hashmap->Count;
    Hashmap->Slots[BaseSlot].BaseCount++;
    
    Hashmap->ItemSlots[Hashmap->Count] = Slot;
    Hashmap->Keys[Hashmap->Count] = Key;
    Hashmap->Values[Hashmap->Count] = Value;

    Hashmap->Count++;
}

template<typename key, typename value, typename hasher, typename comparer>
function inline value* Hashmap_Find(hashmap_t<key, value, hasher, comparer>* Hashmap, const key& Key) {
	u32 Hash = hasher{}.Hash(Key);
    u32 Slot = Find_Slot<key, comparer>(Hashmap->Slots, Hashmap->SlotCapacity, Hashmap->Keys, Key, Hash);
    if(Slot == HASH_INVALID_SLOT) return nullptr;
    return &Hashmap->Values[Hashmap->Slots[Slot].ItemIndex]; 
}

template<typename key, typename value, typename hasher, typename comparer>
function inline void Hashmap_Remove(hashmap_t<key, value, hasher, comparer>* Hashmap, const key& Key) {
	u32 Hash = hasher{}.Hash(Key);
    u32 Slot = Find_Slot<key, comparer>(Hashmap->Slots, Hashmap->SlotCapacity, Hashmap->Keys, Key, Hash);
    
    if(Slot == HASH_INVALID_SLOT) return;

    u32 SlotMask = Hashmap->SlotCapacity-1;
    u32 BaseSlot = (Hash & SlotMask);
    u32 Index = Hashmap->Slots[Slot].ItemIndex;
    u32 LastIndex = Hashmap->Count-1;

    Hashmap->Slots[BaseSlot].BaseCount--;
    Hashmap->Slots[Slot].ItemIndex = HASH_INVALID_SLOT;

    if(Index != LastIndex) {
        Hashmap->Keys[Index] = Hashmap->Keys[LastIndex];
        Hashmap->Values[Index] = Hashmap->Values[LastIndex];
        Hashmap->ItemSlots[Index] = Hashmap->ItemSlots[LastIndex];
        Hashmap->Slots[Hashmap->ItemSlots[LastIndex]].ItemIndex = Index;
    }

    Hashmap->Count--;
}

template<typename key, typename value, typename hasher, typename comparer>
function inline void Hashmap_Clear(hashmap_t<key, value, hasher, comparer>* Hashmap) {
	Memory_Clear(Hashmap->Slots, sizeof(hash_slot)*Hashmap->SlotCapacity);
	for(u32 SlotIndex = 0; SlotIndex < Hashmap->SlotCapacity; SlotIndex++) {
		Hashmap->Slots[SlotIndex].ItemIndex = -1;
    }

    for(u32 ItemIndex = 0; ItemIndex < Hashmap->ItemCapacity; ItemIndex++) 
        Hashmap->ItemSlots[ItemIndex] = HASH_INVALID_SLOT;
    Hashmap->Count = 0;
}

template<typename key, typename value, typename hasher, typename comparer>
inline value& hashmap_t<key, value, hasher, comparer>::operator[](const key& Key) {
	value* Value = Hashmap_Get(this, Key);
	Assert(Value);
	return *Value;
}

#endif

#endif