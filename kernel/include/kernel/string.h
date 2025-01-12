#include <kernel/types.h>

u64 StringLength(u8 *Str) {
	u64 Count = 0;
	while(Str[Count] != 0) {
		++Count;
	}
	return(Count);
}

u64 StringUpperCase(u8 *Str) {
	u64 StrIndex = 0;
	while(Str[StrIndex] != 0) {
		if(Str[StrIndex] >= 'a' && Str[StrIndex] <= 'z') {
			Str[StrIndex] -= 'a' - 'A';
		}
		
		++StrIndex;
	}

	return(StrIndex);
}