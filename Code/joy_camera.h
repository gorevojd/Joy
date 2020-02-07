#ifndef JOY_CAMERA_H
#define JOY_CAMERA_H

#include "joy_math.h"
#include "joy_types.h"

struct game_camera{
    v3 P;
    v3 dP;
    
    Euler_Angles Angles;
    
    quat Rotation;
    
    m44 GetMatrix(){
        m44 Result = InverseTranslationMatrix(this->P) * Transpose(Quat2M44(this->Rotation));
        
        return(Result);
    }
};

m44 GetCameraMatrix(game_camera* camera);
void UpdateCameraRotation(game_camera* camera,
                          float dPitch,
                          float dYaw,
                          float dRoll);

#endif