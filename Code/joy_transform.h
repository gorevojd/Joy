#ifndef JOY_TRANSFORM_H
#define JOY_TRANSFORM_H

struct transform{
    v3 P;
    quat R;
    v3 S;
    
    transform(){
        this->P = P;
    }
};

inline transform Transform(v3 P, quat R, v3 S){
    transform Result = {};
    
    Result.P = P;
    Result.R = R;
    Result.S = S;
    
    return(Result);
}

inline m44 GetTransformMatrix(transform Transform){
    m44 Result = ScalingMatrix(transform.S) * RotationMatrix(transform.R) * TranslationMatrix(Transform.P);
    
    return(Result);
}

#endif