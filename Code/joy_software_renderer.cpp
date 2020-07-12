#include "joy_software_renderer_functions.cpp"

void SoftwareRenderStackToOutput(render_state* Render, bmp_info* buf, rc2 clipRect){
    u8* At = (u8*)Render->StackRegion.CreationBlock.Base;
	u8* RenderEnd = (u8*)Render->StackRegion.CreationBlock.Base + Render->StackRegion.CreationBlock.Used;
    
	while (At < RenderEnd) {
        render_entry_header* header = (render_entry_header*)At;
        
        At += sizeof(render_entry_header);
        
        switch(header->Type){
            case RenderEntry_ClearColor:{
                RENDER_GET_ENTRY(render_entry_clear_color);
                
#if defined(JOY_AVX)
                RenderClearSSE(buf, Entry->clearColor01, clipRect);
#else
                RenderClear(buf, Entry->clearColor01, clipRect);
#endif
            }break;
            
            case RenderEntry_Gradient:{
                RENDER_GET_ENTRY(render_entry_gradient);
                
                if(Entry->gradType == RenderEntryGradient_Horizontal){
                    
#if defined(JOY_AVX)
                    RenderGradientHorzSSE(buf, 
                                          Entry->rc, 
                                          Entry->color1, 
                                          Entry->color2, 
                                          clipRect);
#else
                    
                    RenderGradientHorz(buf, 
                                       Entry->rc, 
                                       Entry->color1, 
                                       Entry->color2, 
                                       clipRect);
#endif
                }
                else if(Entry->gradType == RenderEntryGradient_Vertical){
#if defined(JOY_AVX)
                    RenderGradientVertSSE(buf, 
                                          Entry->rc, 
                                          Entry->color1, 
                                          Entry->color2, 
                                          clipRect);
#else
                    RenderGradientVert(buf, 
                                       Entry->rc, 
                                       Entry->color1, 
                                       Entry->color2, 
                                       clipRect);
#endif
                }
                else{
                    INVALID_CODE_PATH;
                }
            }break;
            
            case RenderEntry_Rect:{
                RENDER_GET_ENTRY(render_entry_rect);
                
#if defined(JOY_AVX)
                RenderRectSSE(buf,
                              Entry->p,
                              Entry->dim,
                              Entry->modulationColor01,
                              clipRect);
#else
                RenderRect(buf,
                           Entry->p,
                           Entry->dim,
                           Entry->modulationColor01,
                           clipRect);
#endif
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(render_entry_bitmap);
                
#if defined(JOY_AVX)
                RenderBitmapSSE(buf, 
                                Entry->Bitmap,
                                Entry->P,
                                Entry->PixelHeight,
                                Entry->ModulationColor01,
                                clipRect);
#else
                RenderBitmap(buf, 
                             Entry->Bitmap,
                             Entry->P,
                             Entry->PixelHeight,
                             Entry->ModulationColor01,
                             clipRect);
#endif
            }break;
            
            At += header->DataSize;
        }
    }
}

struct Render_Queue_Work_Data{
    bmp_info* buf;
    rc2 clipRect;
    render_state* Render;
};

PLATFORM_CALLBACK(RenderQueueWork){
    Render_Queue_Work_Data* work = (Render_Queue_Work_Data*)Data;
    
    SoftwareRenderStackToOutput(
                                work->Render,
                                work->buf,
                                work->clipRect);
}

struct Render_Queue_rgba2bgra_Work{
    bmp_info* buf;
    rc2 clipRect;
};

PLATFORM_CALLBACK(RenderQueueRGBA2BGRAWork){
    Render_Queue_rgba2bgra_Work* work = (Render_Queue_rgba2bgra_Work*)Data;
    
#if defined(JOY_AVX)
    RenderRGBA2BGRASSE(work->buf, work->clipRect);
#else
    RenderRGBA2BGRA(work->buf, work->clipRect);
#endif
}

#define TILES_COUNT 32
void RenderMultithreaded(platform_job_queue* queue, render_state* Render, bmp_info* buf) {
    
#if 0
    rc2 clipRect;
    clipRect.min = V2(0, 0);
    clipRect.max = V2(buf->Width, buf->Height);
    
    SoftwareRenderStackToOutput(Render, buf, clipRect);
#else
    
#if 0
#define SIDE_TILES_COUNT 8
    
    RenderQueueWork works[SIDE_TILES_COUNT * SIDE_TILES_COUNT];
    
    int sideTileW = buf->Width / SIDE_TILES_COUNT;
    sideTileW = (sideTileW + 15) & (~15);
    int sideTileH = buf->Height / SIDE_TILES_COUNT;
    
    for (int j = 0; j < SIDE_TILES_COUNT; j++) {
        for (int i = 0; i < SIDE_TILES_COUNT; i++) {
            rc2 rect;
            
            rect.min = V2(sideTileW * i, sideTileH * j);
            rect.max = V2(sideTileW * (i + 1), sideTileH * (j + 1));
            
            if (j == SIDE_TILES_COUNT - 1) {
                rect.max.y = buf->Height;
            }
            
            if (i == SIDE_TILES_COUNT - 1) {
                rect.max.x = buf->Width;
            }
            
            RenderQueueWork* workData = &works[j * SIDE_TILES_COUNT + i];
            workData->buf = buf;
            workData->Render = Render;
            workData->clipRect = rect;
            
            //if ((j & 1) == (i & 1)) {
            PlatformAddEntry(Queue, RenderQueueWork, workData);
            //}
        }
    }
#else
    Render_Queue_Work_Data works[TILES_COUNT];
    
    int preferTileH = buf->Height / TILES_COUNT;
    if(preferTileH < 16){
        preferTileH = 16;
    }
    
    int curH = 0; 
    for(int i = 0; i < TILES_COUNT; i++){
        rc2 rect;
        
        b32 shouldExit = 0;
        int maxH = curH + preferTileH;
        if(maxH > buf->Height){
            maxH = buf->Height;
            shouldExit = 1;
        }
        
        rect.Min = V2(0.0f, curH);
        rect.Max = V2(buf->Width, maxH);
        
        Render_Queue_Work_Data* workData = &works[i];
        workData->buf = buf;
        workData->Render = Render;
        workData->clipRect = rect;
        
        //if (i & 1) {
        PlatformAddEntry(queue, RenderQueueWork, workData);
        //}
        
        curH += preferTileH;
        if(shouldExit){
            break;
        }
    }
#endif
    
    PlatformWaitForCompletion(queue);
#endif
}

void RenderMultithreadedRGBA2BGRA(platform_job_queue* queue, bmp_info* buf) {
    
    Render_Queue_rgba2bgra_Work works[TILES_COUNT];
    
#if 1
    int preferTileH = buf->Height / TILES_COUNT;
    if(preferTileH < 16){
        preferTileH = 16;
    }
    
    int currentH = 0; 
    for(int i = 0; i < TILES_COUNT; i++){
        rc2 rect;
        
        b32 shouldExit = 0;
        int maxH = currentH + preferTileH;
        if(maxH > buf->Height){
            maxH = buf->Height;
            shouldExit = 1;
        }
        
        rect.Min = V2(0.0f, currentH);
        rect.Max = V2(buf->Width, maxH);
        
        Render_Queue_rgba2bgra_Work* workData = &works[i];
        workData->buf = buf;
        workData->clipRect = rect;
        
        //if (i & 1) {
        PlatformAddEntry(queue, RenderQueueRGBA2BGRAWork, workData);
        //}
        
        currentH += preferTileH;
        if(shouldExit){
            break;
        }
    }
    
    PlatformWaitForCompletion(queue);
#endif
}