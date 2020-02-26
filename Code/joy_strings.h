#ifndef JOY_STRINGS_H
#define JOY_STRINGS_H

#include "joy_types.h"
#include "joy_defines.h"
#include <stdarg.h>

inline b32 StringsAreEqual(char* A, char* B) {
	b32 Result = false;
    
	while (*A && *B) {
        
		if (*A != *B) {
			Result = false;
			break;
		}
        
		A++;
		B++;
	}
    
	if (*A == 0 && *B == 0) {
		Result = true;
	}
    
	return(Result);
}

inline void CopyStrings(char* Dst, char* Src) {
	if (Src) {
		while (*Src) {
			*Dst++ = *Src++;
		}
	}
	*Dst = 0;
}

inline void CopyStrings(char* Dst, int DstSize, char* Src){
    if(Src){
        int SpaceInDstAvailable = DstSize - 1;
        while(*Src && SpaceInDstAvailable){
            *Dst++ = *Src++;
            SpaceInDstAvailable--;
        }
    }
    *Dst = 0;
}

inline void ConcatStringsUnsafe(char* Dst, char* Src1, char* Src2) {
	int Index = 0;
    
	char* To = Dst;
	char* At = Src1;
	while (*At != 0) {
		*To++ = *At++;
	}
    
	At = Src2;
	while (*At) {
		*To++ = *At++;
	}
    
	*To = 0;
}

inline void ConcatStringsUnsafe(char* Dst, int Count, ...){
    va_list ap;
    
    char* To = Dst;
	
    va_start(ap, Count);
    for(int i = 0; i < Count; i++){
        char* At = va_arg(ap, char*);
        
        while(*At){
            *To++ = *At++;
        }
    }
    va_end(ap);
    
    *To = 0;
}

inline int StringLength(char* Text) {
	int Res = 0;
    
	char* At = Text;
	while (*At) {
		Res++;
        
		At++;
	}
    
	return(Res);
}


inline void ChangeAllChars(char* String, char From, char To){
    char* At = String;
    
    while(*At){
        if(*At == From){
            *At = To;
        }
        
        At++;
    }
}

inline u32 StringHashFNV(char* Name) {
    u32 Result = 2166136261;
    
    char* At = Name;
    while (*At) {
        
        Result *= 16777619;
        Result ^= *At;
        
        At++;
    }
    
    return(Result);
}

inline b32 StringIsDecimalInteger(char* String) {
    b32 Result = 1;
    
    int FirstCheckIndex = 0;
    if (String[0] == '-') {
        FirstCheckIndex = 1;
    }
    
    char* At = String + FirstCheckIndex;
    while (*At)
    {
        if (*At >= '0' &&
            *At <= '9')
        {
            
        }
        else {
            return(0);
        }
        
        *At++;
    }
    
    return(Result);
}

inline int StringToInteger(char* String) {
    int Result = 0;
    
    char* At = String;
    
    int Len = StringLength(String);
    
    int NumberIsNegative = 1;
    int FirstNumberIndex = 0;
    if (String[0] == '-') {
        FirstNumberIndex = 1;
        NumberIsNegative = -1;
    }
    
    int CurrentMultiplier = 1;
    for (int CharIndex = Len - 1;
         CharIndex >= FirstNumberIndex;
         CharIndex--)
    {
        Result += (String[CharIndex] - '0') * CurrentMultiplier;
        CurrentMultiplier *= 10;
    }
    
    Result *= NumberIsNegative;
    
    return(Result);
}

inline float StringToFloat(char* String) {
    float Result = 0.0f;
    
    //NOTE(dima): Detecting if negative and whole part start index
    float IsNegative = 1.0f;
    int WholeStart = 0;
    if (String[0] == '-') {
        IsNegative = -1.0f;
        WholeStart = 1;
    }
    
    char* At = String + WholeStart;
    b32 DotExist = 0;
    char* DotAt = 0;
    //NOTE(dima): Detecting whole part end
    int WholeEndIndex = WholeStart;
    while (*At) {
        if (*At == '.') {
            DotExist = 1;
            DotAt = At;
            break;
        }
        At++;
        WholeEndIndex++;
    }
    
    //NOTE(dima): Converting whole part
    float CurrentMultiplier = 1.0f;
    for (int Index = WholeEndIndex - 1;
         Index >= WholeStart;
         Index--)
    {
        Result += (float)(String[Index] - '0') * CurrentMultiplier;
        CurrentMultiplier *= 10.0f;
    }
    
    //NOTE(dima): Converting fractional part if exist
    if (DotExist) {
        int FractionalPartLen = 0;
        At = DotAt;
        ++At;
        while (*At) {
            FractionalPartLen++;
            
            At++;
        }
        
        if (FractionalPartLen) {
            char* FractionalBegin = DotAt + 1;
            char* FractionalEnd = At;
            
            char* FractionalAt = FractionalBegin;
            CurrentMultiplier = 0.1f;
            while (FractionalAt != FractionalEnd) {
                float CurrentDigit = (float)(*FractionalAt - '0');
                
                Result += CurrentDigit * CurrentMultiplier;
                CurrentMultiplier /= 10.0f;
                FractionalAt++;
            }
        }
    }
    
    Result *= IsNegative;
    
    return(Result);
}

inline void IntegerToString(int Value, char* String) {
    int DigitIndex = 0;
    
    do {
        String[DigitIndex++] = '0' + (Value % 10);
        
        Value /= 10;
    } while (Value);
    
    //NOTE(dima): Reversing string
    int ScanBeginIndex = 0;
    int ScanEndIndex = DigitIndex - 1;
    while (ScanBeginIndex < ScanEndIndex) {
        char Temp = String[ScanBeginIndex];
        String[ScanBeginIndex] = String[ScanEndIndex];
        String[ScanEndIndex] = Temp;
        
        ScanBeginIndex++;
        ScanEndIndex--;
    }
    
    //NOTE(dima): Null terminating the string
    String[DigitIndex] = 0;
}

/*
NOTE(Dima): Tmp must be the same size as Dst
This function returns the advance for the caret
*/
inline int InsertStringToString(char* Dst, 
                                int DstMaxSize, 
                                char* Tmp, 
                                int TmpMaxSize,
                                int CaretPos, char* Src)
{
    int DstLen = StringLength(Dst);
    int SrcLen = StringLength(Src);
    
    char* FirstPart = Dst;
    int FirstPartLen = CaretPos;
    
    char* SecondPart = Dst + CaretPos;
    int SecondPartLen = DstLen - CaretPos;
    
    // NOTE(Dima): If we can insert
    if(DstLen + SrcLen < (DstMaxSize - 1)){
        
        ASSERT(TmpMaxSize > SecondPartLen);
        CopyStrings(Tmp, SecondPart);
        
        // NOTE(Dima): Copy from source after first part
        CopyStrings(Dst + FirstPartLen, Src);
        
        // NOTE(Dima): Copy Second part
        CopyStrings(Dst + FirstPartLen + SrcLen, Tmp);
    }
    
    return(SrcLen);
}

// NOTE(Dima): Function returns advance for the caret
inline int ErasePrevCharFromString(char* Dst, int CaretPos){
    int Result = 0;
    
    int DstLen = StringLength(Dst);
    if(CaretPos != 0){
        int i = 0;
        for(i = 0; i < DstLen - CaretPos; i++){
            Dst[CaretPos + i - 1] = Dst[CaretPos + i];
            
        }
        Dst[CaretPos + i - 1] = 0;
        
        Result = -1;
    }
    
    return(Result);
}

inline int EraseNextCharFromString(char* Dst, int CaretPos){
    int Result = 0;
    
    int DstLen = StringLength(Dst);
    if(CaretPos != DstLen){
        int i = 0;
        for(i = 0; i < DstLen - CaretPos; i++){
            Dst[CaretPos + i] = Dst[CaretPos + i + 1];
        }
        Dst[CaretPos + i] = 0;
    }
    
    return(Result);
}

inline b32 IsLetter(char C){
    if((C >= 'a' && C <= 'z') || 
       (C >= 'A' && C <= 'Z'))
    {
        return(1);
    }
    return(0);
}

inline b32 IsUpper(char C){
    b32 Result = 0;
    if(C >= 'A' && C <= 'Z'){
        Result = 1;
    }
    
    return(Result);
}

inline b32 IsWhitespace(char C){
    b32 Result = 0;
    
    if(C == ' ' ||
       C == '\n' ||
       C == '\r')
    {
        Result = 1;
    }
    
    return(Result);
}

inline void StringToUpper(char* To, char* From){
    char* AtFrom = From;
    char* AtTo = To;
    
    if(AtFrom && AtTo){
        while(*AtFrom){
            
            if(IsLetter(*AtFrom)){
                if(IsUpper(*AtFrom)){
                    *AtTo = *AtFrom;
                }
                else{
                    *AtTo = *AtFrom - 'a' + 'A';
                }
            }
            else{
                *AtTo = *AtFrom;
            }
            
            AtFrom++;
            AtTo++;
        }
        
        *AtTo = 0;
    }
}

// NOTE(Dima): Returns the found index
inline int FindSkipPrev(char* Dst, int CaretPos){
    int Result = 0;
    
    int At = CaretPos;
    
    if(At){
        b32 CurIsLetter = IsLetter(Dst[At]);
        b32 PrevIsLetter = IsLetter(Dst[At - 1]);
        
        if(!CurIsLetter){
            --At;
            if(!PrevIsLetter){
                while((At >= 0) && !IsLetter(Dst[At])){
                    --At;
                }
            }
            
            while((At >= 0) && IsLetter(Dst[At])){
                --At;
            }
        }
        
        if(CurIsLetter){
            if(!PrevIsLetter){
                --At;
                while((At >= 0) && !IsLetter(Dst[At])){
                    --At;
                }
            }
            
            while((At >= 0) && IsLetter(Dst[At])){
                --At;
            }
        }
        
        Result = At + 1;
    }
    
    return(Result);
}

// NOTE(Dima): Returns the index of the found char
inline int FindSkipNext(char* Dst, int CaretPos){
    int Result = 0;
    
    int DstLen = StringLength(Dst);
    
    int At = CaretPos;
    
    while(At < DstLen){
        if(IsLetter(Dst[At])){
            break;
        }
        
        At++;
    }
    
    Result = At;
    
    if(At < DstLen){
        while(At < DstLen){
            if(!IsLetter(Dst[At])){
                break;
            }
            At++;
        }
        
        Result = At;
    }
    
    return(Result);
}

inline int EraseToNonLetterPrev(char* Dst, int CaretP){
    int FoundIndex = FindSkipPrev(Dst, CaretP);
    
    int Diff = CaretP - FoundIndex;
    
    if(Diff){
        char *AtSrc = Dst + CaretP;
        char *AtDst = Dst + CaretP - Diff;
        while(*AtSrc){
            *AtDst++ = *AtSrc++;
        }
        *AtDst = 0;
    }
    
    return(-(Diff));
}

inline int EraseToNonLetterNext(char* Dst, int CaretP){
    
}

#endif