#ifndef BUTTER_KERN_INC_FUNCTIONS_H
#define BUTTER_KERN_INC_FUNCTIONS_H

void MemCopy(void *_Destination, void *_Source, UINTN Size) {
	CHAR8 *Destination = (CHAR8*)_Destination;
	CHAR8 *Source = (CHAR8*)_Source;

	for(UINTN ByteIndex = 0; ByteIndex < Size; ++ByteIndex) {
		Destination[ByteIndex] = Source[ByteIndex];
	}
}

void MemSet(void *_Mem, UINT32 Value, UINTN Size) {
	CHAR8 *Mem = (CHAR8*)_Mem;
	for(UINTN ByteIndex = 0; ByteIndex < Size; ++ByteIndex) {
		Mem[ByteIndex] = Value;
	}
}

#endif // BUTTER_KERN_INC_FUNCTIONS_H
