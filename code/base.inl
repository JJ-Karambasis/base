#ifndef BASE_INL_H
#define BASE_INL_H

#ifdef __cplusplus

#include <initializer_list>

function inline v2 operator-(v2 V) {
    return V2_Negate(V);
}

function inline v2 operator+(v2 A, v2 B) {
	return V2_Add_V2(A, B);
}

function inline v2 operator+(v2 A, f32 B) {
	return V2_Add_S(A, B);
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

function inline v2i operator+(v2i A, v2i B) {
	return V2i(A.x+B.x, A.y+B.y);
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

function inline v3 operator-(v3 V) {
	return V3_Negate(V);
}

function inline v3 operator+(v3 A, v3 B) {
	v3 Result = V3_Add_V3(A, B);
	return Result;
}

function inline v3& operator+=(v3& A, v3 B) {
	A = V3_Add_V3(A, B);
	return A;
}

function inline v3 operator-(v3 A, v3 B) {
    v3 Result = V3_Sub_V3(A, B);
    return Result;
}

function inline v3& operator-=(v3& A, v3 B) {
    A = V3_Sub_V3(A, B);
    return A;
}

function inline v3 operator*(v3 A, f32 B) {
	v3 Result = V3_Mul_S(A, B);
	return Result;
}

function inline v3 operator*(v3 A, v3 B) {
	v3 Result = V3_Mul_V3(A, B);
	return Result;
}

function inline v3& operator*=(v3& A, f32 B) {
	A = V3_Mul_S(A, B);
	return A;
}

function inline v3& operator*=(v3& A, v3 B) {
	A = V3_Mul_V3(A, B);
	return A;
}

function inline v3 operator/(v3 A, f32 B) {
    v3 Result = V3_Div_S(A, B);
    return Result;
}

function inline v4 operator*(v4 A, f32 B) {
	v4 Result = V4_Mul_S(A, B);
	return Result;
}

function inline v4 operator+(v4 A, v4 B) {
    return V4(A.x + B.x, A.y + B.y, A.z + B.z, A.w + B.w);
}

function inline v4 operator-(v4 A, v4 B) {
    return V4(A.x - B.x, A.y - B.y, A.z - B.z, A.w - B.w);
}

function inline v4 V4(v3 XYZ, f32 W) {
    return V4_From_V3(XYZ, W);
}

function inline quat operator*(quat A, quat B) {
	quat Result = Quat_Mul_Quat(A, B);
	return Result;
}

function inline v3 operator*(v3 A, const m3& B) {
	v3 Result = V3_Mul_M3(A, &B);
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

function inline m4 operator*(const m4& A, const m4& B) {
    m4 Result = M4_Mul_M4(&A, &B);
    return Result;
}

function inline v3 operator*(v4 A, const m4_affine& B) {
    v3 Result = V4_Mul_M4_Affine(A, &B);
    return Result;
}

function inline v3 operator*(v3 A, const m4_affine& B) {
    return V4_From_V3(A, 1.0f) * B;
}

function inline m4_affine operator*(const m4_affine& A, const m4_affine& B) {
    m4_affine Result = M4_Affine_Mul_M4_Affine(&A, &B);
    return Result;
}

function inline b32 operator==(string A, string B) {
    return String_Equals(A, B);
}
function inline b32 operator!=(string A, string B) {
    return !String_Equals(A, B);
}

template <typename type>
struct array;

template <typename type>
struct static_array;

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
    
    inline span(const array<type>& Array);
    inline span(const static_array<type>& Array);
	inline const type& operator[](size_t Index) const {
		Assert(Index < Count);
		return Ptr[Index];
	}
};

template <typename type>
struct static_array {
    type* Ptr = NULL;
    size_t Count = 0;
    
    static_array() = default;
    inline static_array(type* Ptr, size_t Count) : Ptr(Ptr), Count(Count) {}
    inline static_array(const array<type>& Array);

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
struct array {
    enum class array_type {
        VIRTUAL,
        ALLOCATOR
    };
    array_type Type = array_type::VIRTUAL;
    union {
        memory_reserve MemoryReserve;
        allocator* Allocator;
    };
	type* Ptr            = NULL;
	size_t Count         = 0;
	size_t Capacity 	 = 0;
    
	array() = default;
    inline array(allocator* Allocator);
    inline array(allocator* Allocator, size_t Count);
	
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
inline span<type>::span(const array<type>& Array) : Ptr(Array.Ptr), Count(Array.Count) { }

template <typename type>
function inline b32 Span_Find(span<type> Span, const type& Value) {
    for(size_t i = 0; i < Span.Count; i++) {
        if(Span[i] == Value) {
            return true;
        }
    }
    return false;
}

template <typename type>
inline static_array<type>::static_array(const array<type>& Array) : Ptr(Array.Ptr), Count(Array.Count) { }

template <typename type>
inline span<type>::span(const static_array<type>& Array) : Ptr(Array.Ptr), Count(Array.Count) { }

template <typename type>
function inline void Array_Init(static_array<type>* Array, type* Ptr, size_t Count) {
    Array->Ptr = Ptr;
    Array->Count = Count;
}

template <typename type>
function inline b32 Array_Find(static_array<type> Array, const type& Value) {
    for(size_t i = 0; i < Array.Count; i++) {
        if(Array[i] == Value) {
            return true;
        }
    }
    return false;
}

template <typename type>
function inline void Array_Init(array<type>* Array, allocator* Allocator = Default_Allocator_Get()) {
	*Array = array<type>();
    Array->Type = array<type>::array_type::ALLOCATOR;
    Array->Allocator = Allocator;
}

template <typename type>
function inline void Array_Release(array<type>* Array) {
    if(Array->Type == array<type>::array_type::VIRTUAL) {
        Delete_Memory_Reserve(&Array->MemoryReserve);
    } else {
        Allocator_Free_Memory(Array->Allocator, Array->Ptr);
    }
    Memory_Clear(Array, sizeof(array<type>));
}

template <typename type>
function inline void Virtual_Array_Init(array<type>* Array, size_t ReserveSize=GB(1)) {
	*Array = array<type>();
    Array->Type = array<type>::array_type::VIRTUAL;
    Array->MemoryReserve = Make_Memory_Reserve(ReserveSize);
    Array->Ptr = (type*)Array->MemoryReserve.BaseAddress;
}

template<typename type>
function inline void Array_Reserve(array<type>* Array, size_t NewCapacity) {
    //If the new capacity is less than the minimum, set it to the minimum
    if(NewCapacity < 32) {
        NewCapacity = 32;
    }
    
    
    if(Array->Type == array<type>::array_type::VIRTUAL) {
        Recommit_New_Size(&Array->MemoryReserve, NewCapacity*sizeof(type));
        NewCapacity = Array->MemoryReserve.CommitSize/sizeof(type);
    } else {
        type* NewPtr = Allocator_Allocate_Array(Array->Allocator, NewCapacity, type);
        if(Array->Ptr) {
            size_t CopyCapacity = Min(NewCapacity, Array->Capacity);
            Memory_Copy(NewPtr, Array->Ptr, CopyCapacity*sizeof(type));
            Allocator_Free_Memory(Array->Allocator, Array->Ptr);
        }
        
        Array->Ptr = NewPtr;
    }
    
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
function inline void Array_Init(array<type>* Array, size_t Count, allocator* Allocator = Default_Allocator_Get()) {
    Array_Init(Array, Allocator);
    Array_Resize(Array, Count);
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
function inline type* Array_Get(array<type>* Array, size_t Index) {
    if(Index >= Array->Count) return NULL;
    return Array->Ptr + Index;
}

template <typename type>
function inline void Array_Clear(array<type>* Array) {
	Array->Count = 0;
}

template<typename type>
function inline array<type> Array_Copy(allocator* Allocator, const array<type>& Src) {
	array<type> Array(Allocator, Src.Count);
    Memory_Copy(Array.Ptr, Src.Ptr, Src.Count*sizeof(type));
    return Array;
}

template<typename type>
function inline void Array_Add_Range(array<type>* Array, const type* Ptr, size_t Count) {
    if (Array->Count+Count >= Array->Capacity) {
        Array_Reserve(Array, Max(Array->Capacity*2, Array->Count+Count));
    }
    Memory_Copy(Array->Ptr + Array->Count, Ptr, Count * sizeof(type));
    Array->Count += Count;
}

template<typename type>
function inline void Array_Add_Range(array<type>* Array, const array<type>* ArrayRange) {
    Array_Add_Range(Array, ArrayRange->Ptr, ArrayRange->Count);
}

template <typename type>
function inline array<type> Array_Combine(allocator* Allocator, span<array<type>> Arrays) {
    size_t TotalCount = 0;
    for(size_t i = 0; i < Arrays.Count; i++) {
        array<type> Array = Arrays[i];
        TotalCount += Array.Count;
    }
    
    array<type> Result;
    Array_Init(&Result, TotalCount, Allocator);
    
    size_t Offset = 0;
    for(size_t i = 0; i < Arrays.Count; i++) {
        Memory_Copy(Result.Ptr+Offset, Arrays[i].Ptr, Arrays[i].Count*sizeof(type));
        Offset += Arrays[i].Count;
    }
    
    return Result;
}

template <typename type>
function inline array<type> Array_Splice(array<type>* Array, size_t StartIdx, size_t EndIdx) {
    if(StartIdx == EndIdx) return {};
    
    EndIdx = Min(EndIdx, Array->Count);
    Assert(EndIdx > StartIdx);
    
    type* Ptr = Array->Ptr+StartIdx;
    array<type> Result = { };
    Result.Ptr = Ptr;
    Result.Count = EndIdx-StartIdx;
    return Result;
}

template <typename type>
function inline void Array_Insert(array<type>* Array, const array<type>* InsertArray, size_t Cursor) {
    Assert(Cursor <= Array->Count);
    Assert(Array->Type == array<type>::array_type::ALLOCATOR);
    if(Cursor == 0) {
        array<type> OldArray = *Array;
        *Array = Array_Combine<type>(OldArray.Allocator, {*InsertArray, *Array});
        Array_Release(&OldArray);
    } else if(Cursor == Array->Count) {
        Array_Add_Range(Array, InsertArray);
    } else {
        array<type> OldArray = *Array;
        *Array = Array_Combine<type>(OldArray.Allocator, {
            Array_Splice(Array, 0, Cursor), 
            *InsertArray, 
            Array_Splice(Array, Cursor, Array->Count)
        });
        Array_Release(&OldArray);
    }
}

template <typename type>
function inline void Array_Remove(array<type>* Array, size_t Cursor, size_t Size) {
    Assert(Array->Type == array<type>::array_type::ALLOCATOR);
    if(!Array_Is_Empty(Array)) {
        Assert(Cursor < Array->Count);
        Assert((Cursor+Size) <= Array->Count);
        
        array<type> OldArray = *Array;
        if(Cursor == 0) {
            *Array = Array_Copy<type>(OldArray.Allocator, Array_Splice(Array, Cursor+Size, Array->Count));
        } else if(Cursor+Size == Array->Count) {
            *Array = Array_Copy<type>(OldArray.Allocator, Array_Splice(Array, 0, Cursor));
        } else {
            *Array = Array_Combine<type>(OldArray.Allocator, {
                Array_Splice(Array, 0, Cursor),
                Array_Splice(Array, Cursor+Size, Array->Count)
            });
        }
        Array_Release(&OldArray);
    }
}

template <typename type>
function inline b32 Array_Is_Empty(array<type>* Array) {
    return !Array->Count || !Array->Ptr;
}

template <typename type>
function inline type& Array_Pop(array<type>* Array) {
    Assert(!Array_Is_Empty(Array));
    Array->Count--;
    return Array->Ptr[Array->Count];
}

template <typename type> 
function inline type& Array_Last(array<type>* Array) {
    return Array->Ptr[Array->Count-1];
}

template <typename type>
function inline void Array_Sort(array<type>* Array, sort_callback_func* SortCallbackFunc, void* SortUserData) {
    Array_Sort(Array->Ptr, Array->Count, sizeof(type), SortCallbackFunc, SortUserData);
}

template <typename type>
inline array<type>::array(allocator* Allocator) {
    Array_Init(this, Allocator);
}

template <typename type>
inline array<type>::array(allocator* Allocator, size_t Count) {
    Array_Init(this, Count, Allocator);
}

template <typename type>
struct pool_handle {
	pool_id ID;
};

template <typename type>
function inline pool_handle<type> Null_Handle() {
    return {};
}

template <typename type>
function inline b32 Handle_Is_Null(pool_handle<type> Handle) {
    return Pool_ID_Null(Handle.ID);
}

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

// Parallel items for an existing pool, addressed by that pool's handles.
// Storage commits up to the source pool's high-water mark. A recycled source slot starts cleared.
template<typename type, typename source_type>
struct linked_pool_t {
	pool_t<source_type>* Source;
	memory_reserve Reserve;
	u32 Capacity;

	struct slot {
		u32 Generation;
		type Item;
	};
	slot* Slots;
};

template<typename type, typename source_type>
function inline void Linked_Pool_Init(linked_pool_t<type, source_type>* Pool, pool_t<source_type>* Source) {
	Memory_Clear(Pool, sizeof(*Pool));
	Pool->Source = Source;
	Pool->Reserve = Make_Memory_Reserve(GB(1));
	Pool->Slots = (decltype(Pool->Slots))Pool->Reserve.BaseAddress;
}

template<typename type, typename source_type>
function inline void Linked_Pool_Delete(linked_pool_t<type, source_type>* Pool) {
	if (Pool && Pool->Reserve.BaseAddress) {
		Delete_Memory_Reserve(&Pool->Reserve);
		Memory_Clear(Pool, sizeof(*Pool));
	}
}

template<typename type, typename source_type>
function inline void Linked_Pool_Ensure(linked_pool_t<type, source_type>* Pool, u32 Count) {
	if (Count <= Pool->Capacity) return;

	size_t SlotSize = sizeof(*Pool->Slots);
	if (!Commit_New_Size(&Pool->Reserve, (size_t)Count * SlotSize)) {
		Debug_Log("Failed to commit more memory for the linked pool");
		return;
	}

	Pool->Capacity = (u32)(Pool->Reserve.CommitSize / SlotSize);
}

template<typename type, typename source_type>
function inline type* Linked_Pool_Get(linked_pool_t<type, source_type>* Pool, pool_handle<source_type> Handle) {
	Assert(Pool->Source);
	if (Pool_ID_Null(Handle.ID)) return NULL;
	if (Handle.ID.Index >= Pool->Source->MaxUsed) return NULL;
	if (!Pool_Is_Allocated(Pool->Source, Handle.ID)) return NULL;

	Linked_Pool_Ensure(Pool, Pool->Source->MaxUsed);
	if (Handle.ID.Index >= Pool->Capacity) return NULL;

	auto* Slot = &Pool->Slots[Handle.ID.Index];
	if (Slot->Generation != Handle.ID.Generation) {
		Memory_Clear(&Slot->Item, sizeof(type));
		Slot->Generation = Handle.ID.Generation;
	}
	return &Slot->Item;
}

function inline string String_Combine(allocator* Allocator, span<string> Strings) {
    string Result = String_Combine(Allocator, Strings.Ptr, Strings.Count);
    return Result;
}

function inline string String_Directory_Combine(allocator* Allocator, span<string> Strings) {
    string Result = String_Directory_Combine(Allocator, Strings.Ptr, Strings.Count);
    return Result;
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

template <typename key, typename value>
struct kvp {
	key Key;
	value Value;
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
function inline value* Hashmap_Add(hashmap_t<key, value, hasher, comparer>* Hashmap, const key& Key, const value& Value) {
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
    
    value* Result = Hashmap->Values + Hashmap->Count;
    Hashmap->Count++;
    
    return Result;
}

template<typename key, typename value, typename hasher, typename comparer>
function inline void Hashmap_Init(hashmap_t<key, value, hasher, comparer>* Hashmap, allocator* Allocator, std::initializer_list<kvp<key, value>> KVPs) {
	Hashmap_Init(Hashmap, Allocator);
	for(auto KVP : KVPs) {
		Hashmap_Add(Hashmap, KVP.Key, KVP.Value);
	}
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
	value* Value = Hashmap_Find(this, Key);
	Assert(Value);
	return *Value;
}

template <typename type>
struct queue {
    allocator* Allocator = NULL;
    type*  Ptr      = NULL;
    size_t Head     = 0;
    size_t Count    = 0;
    size_t Capacity = 0;
    
    queue() = default;
    inline queue(allocator* Allocator);
};

template <typename type>
function inline void Queue_Init(queue<type>* Queue, allocator* Allocator = Default_Allocator_Get()) {
    *Queue = queue<type>();
    Queue->Allocator = Allocator;
}

template <typename type>
function inline void Queue_Release(queue<type>* Queue) {
    if(Queue->Ptr) {
        Allocator_Free_Memory(Queue->Allocator, Queue->Ptr);
    }
    Memory_Clear(Queue, sizeof(queue<type>));
}

template <typename type>
function inline void Queue_Reserve(queue<type>* Queue, size_t NewCapacity) {
    if(NewCapacity < 32) {
        NewCapacity = 32;
    }
    
    if(NewCapacity <= Queue->Capacity) {
        return;
    }
    
    type* NewPtr = Allocator_Allocate_Array(Queue->Allocator, NewCapacity, type);
    if(Queue->Ptr) {
        //Unwrap the ring buffer so the elements start at index 0 in the new buffer
        for(size_t i = 0; i < Queue->Count; i++) {
            NewPtr[i] = Queue->Ptr[(Queue->Head + i) % Queue->Capacity];
        }
        Allocator_Free_Memory(Queue->Allocator, Queue->Ptr);
    }
    
    Queue->Ptr = NewPtr;
    Queue->Head = 0;
    Queue->Capacity = NewCapacity;
}

template <typename type>
function inline void Queue_Push(queue<type>* Queue, const type& Entry) {
    if(Queue->Count == Queue->Capacity) {
        size_t NewCapacity = Queue->Capacity ? Queue->Capacity*2 : 32;
        Queue_Reserve(Queue, NewCapacity);
    }
    
    size_t Tail = (Queue->Head + Queue->Count) % Queue->Capacity;
    Queue->Ptr[Tail] = Entry;
    Queue->Count++;
}

template <typename type>
function inline b32 Queue_Is_Empty(queue<type>* Queue) {
    return Queue->Count == 0;
}

template <typename type>
function inline b32 Queue_Pop(queue<type>* Queue, type* OutEntry) {
    if(Queue_Is_Empty(Queue)) {
        return false;
    }
    
    Memory_Copy(OutEntry, Queue->Ptr + Queue->Head, sizeof(type));
    Queue->Head = (Queue->Head + 1) % Queue->Capacity;
    Queue->Count--;
    return true;
}

template <typename type>
function inline b32 Queue_Peek(queue<type>* Queue, type* OutEntry) {
    if(Queue_Is_Empty(Queue)) {
        return false;
    }
    
    Memory_Copy(OutEntry, Queue->Ptr + Queue->Head, sizeof(type));
    return true;
}

template <typename type>
function inline void Queue_Clear(queue<type>* Queue) {
    Queue->Head = 0;
    Queue->Count = 0;
}

template <typename type>
inline queue<type>::queue(allocator* Allocator) {
    Queue_Init(this, Allocator);
}

template <typename type, size_t capacity=1024>
struct spsc_queue {
    type       Entries[capacity];
    atomic_u32 NextEntryToRead;
    atomic_u32 NextEntryToWrite;
};

template <typename type, size_t capacity>
function inline void SPSC_Push(spsc_queue<type, capacity>* Queue, const type& Entry) {
    u32 NextEntryToWrite = Atomic_Load_U32(&Queue->NextEntryToWrite);
    u32 NewNextEntryToWrite = (NextEntryToWrite + 1) % capacity;
    Assert(NewNextEntryToWrite != Atomic_Load_U32(&Queue->NextEntryToRead));
    Queue->Entries[NextEntryToWrite] = Entry;
    Atomic_Store_U32(&Queue->NextEntryToWrite, NewNextEntryToWrite);
}

template <typename type, size_t capacity>
function inline b32 SPSC_Pop(spsc_queue<type, capacity>* Queue, type* OutEntry) {
    u32 NextEntryToRead = Atomic_Load_U32(&Queue->NextEntryToRead);
    b32 Result = NextEntryToRead != Atomic_Load_U32(&Queue->NextEntryToWrite);
    if(Result) {
        u32 NewNextEntryToRead = (NextEntryToRead+1) % capacity;
        Memory_Copy(OutEntry, Queue->Entries + NextEntryToRead, sizeof(type));
        Atomic_Store_U32(&Queue->NextEntryToRead, NewNextEntryToRead);
    }
    
    return Result;
}

template <typename type, size_t capacity>
function inline void SPSC_Flush(spsc_queue<type, capacity>* Queue) {
    type Entry;
    while(SPSC_Pop(Queue, &Entry)) {}
}

template <typename type>
function inline span<type> BStream_Reader_Array(bstream_reader* Reader, size_t Count) {
    return span<type>((type*)BStream_Reader_Size(Reader, Count * sizeof(type)), Count);
}

#endif

#endif