#include "joy_software_renderer.h"
#include "joy_render_stack.h"
#include "joy_platform.h"

#include <intrin.h>

void SoftwareRenderStackToOutput(render_stack* stack, bmp_info* buf, rc2 clipRect){
    u8* at = (u8*)stack->MemRegion.CreationBlock.Base;
	u8* stackEnd = (u8*)stack->MemRegion.CreationBlock.Base + stack->MemRegion.CreationBlock.Used;
    
	while (at < stackEnd) {
        render_entry_header* header = (render_entry_header*)at;
        
        at += sizeof(render_entry_header);
        
        switch(header->type){
            case RenderEntry_ClearColor:{
                RENDER_GET_ENTRY(render_entry_clear_color);
                
                RenderClearSSE(buf, entry->clearColor01, clipRect);
            }break;
            
            case RenderEntry_Gradient:{
                RENDER_GET_ENTRY(render_entry_gradient);
                
                if(entry->gradType == RenderEntryGradient_Horizontal){
                    RenderGradientHorzSSE(buf, 
                                          entry->rc, 
                                          entry->color1, 
                                          entry->color2, 
                                          clipRect);
                }
                else if(entry->gradType == RenderEntryGradient_Vertical){
                    RenderGradientVertSSE(buf, 
                                          entry->rc, 
                                          entry->color1, 
                                          entry->color2, 
                                          clipRect);
                }
                else{
                    INVALID_CODE_PATH;
                }
            }break;
            
            case RenderEntry_Rect:{
                RENDER_GET_ENTRY(render_entry_rect);
                
#if 1
                RenderRectSSE(buf,
                              entry->p,
                              entry->dim,
                              entry->modulationColor01,
                              clipRect);
#else
                RenderRect(buf,
                           entry->p,
                           entry->dim,
                           entry->modulationColor01,
                           clipRect);
#endif
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(render_entry_bitmap);
                
#if 1
                RenderBitmapSSE(buf, 
                                entry->Bitmap,
                                entry->P,
                                entry->PixelHeight,
                                entry->ModulationColor01,
                                clipRect);
#else
                RenderBitmap(buf, 
                             entry->Bitmap,
                             entry->P,
                             entry->PixelHeight,
                             entry->ModulationColor01,
                             clipRect);
#endif
            }break;
            
            at += header->dataSize;
        }
    }
}

struct Render_Queue_Work_Data{
    bmp_info* buf;
    rc2 clipRect;
    render_stack* stack;
};

PLATFORM_CALLBACK(RenderQueueWork){
    Render_Queue_Work_Data* work = (Render_Queue_Work_Data*)Data;
    
    SoftwareRenderStackToOutput(
        work->stack,
        work->buf,
        work->clipRect);
}

struct Render_Queue_rgba2bgra_Work{
    bmp_info* buf;
    rc2 clipRect;
};

PLATFORM_CALLBACK(RenderQueueRGBA2BGRAWork){
    Render_Queue_rgba2bgra_Work* work = (Render_Queue_rgba2bgra_Work*)Data;
    
    RenderRGBA2BGRASSE(work->buf, work->clipRect);
}

#define TILES_COUNT 32
void RenderMultithreaded(platform_job_queue* queue, render_stack* stack, bmp_info* buf) {
    
#if 0
    rc2 clipRect;
    clipRect.min = V2(0, 0);
    clipRect.max = V2(buf->Width, buf->Height);
    
    SoftwareRenderStackToOutput(stack, buf, clipRect);
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
            workData->stack = stack;
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
        workData->stack = stack;
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