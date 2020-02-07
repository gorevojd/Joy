#include "joy_platform.h"

#define PLATFORM_USE_STD_MUTEX 1

PLATFORM_READ_FILE(PlatformReadFile) {
    Platform_Read_File_Result res = {};
    
	FILE* fp = fopen(filePath, "rb");
    
	if (fp) {
		fseek(fp, 0, 2);
		u64 fileSize = ftell(fp);
		fseek(fp, 0, 0);
        
		res.dataSize = fileSize;
		res.data = (u8*)calloc(fileSize + 1, 1);
        
		fread(res.data, 1, fileSize, fp);
        
		fclose(fp);
	}
    
	return(res);
}

PLATFORM_WRITE_FILE(PlatformWriteFile) {
	FILE* file = (FILE*)fopen(filePath, "wb");
    
    b32 res = 0;
    
	if (file) {
		size_t elementsWritten = fwrite(data, 1, size, file);
        
        res = (elementsWritten == size);
        
		fclose(file);
	}
    
    return(res);
}

PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory) {
	if (fileReadResult->data) {
		free(fileReadResult->data);
	}
	fileReadResult->data = 0;
}
