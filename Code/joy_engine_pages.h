#ifndef JOY_ENGINE_PAGES_H
#define JOY_ENGINE_PAGES_H

enum engine_page_type{
    EnginePage_Render,
    EnginePage_Audio,
    EnginePage_Gui,
    EnginePage_Memory,
    EnginePage_Platform,
    EnginePage_Profile,
    EnginePage_Test,
    
    EnginePage_Count,
};

#define ENGINE_PAGE_ELEMENT_CALLBACK(name) void name()
typedef ENGINE_PAGE_ELEMENT(engine_page_element_callback);

struct engine_page{
    u32 Type;
    
    char Name[64];
    
    engine_page_element_callback* ElementCallback;
};

engine_page Pages[EnginePage_Count];

inline b32 PageIndexIsValid(int Index){
    b32 Result = (Index >= 0) && (Index < EnginePage_Count);
}

inline engine_page* GetPage(engine_page* Pages, int Index){
    engine_page* Result = 0;
    
    if(PageIndexIsValid(Index)){
        Result = Pages + Index;
    }
    
    return(Result);
}

inline void DescribeEnginePage(engine_page* Pages, 
                               int Index, 
                               char* Name, 
                               engine_page_element_callback* Callback)
{
    engine_page* Page = GetPage(Pages, Index);
    
    if(Page){
        Page->ElementCallback = Callback;
        CopyStrings(Page->Name, Name);
    }
}

#endif