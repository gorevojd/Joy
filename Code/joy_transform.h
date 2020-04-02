#ifndef JOY_TRANSFORM_H
#define JOY_TRANSFORM_H

#include "joy_math.h"

struct transform{
    v3 T;
    quat R;
    v3 S;
};

inline transform CreateTransform(v3 T, quat R, v3 S){
    transform Result = {};
    
    Result.T = T;
    Result.R = R;
    Result.S = S;
    
    return(Result);
}

inline m44 ToMatrix(const transform& Tran){
    m44 Result = ScalingMatrix(Tran.S) * RotationMatrix(Tran.R) * TranslationMatrix(Trans.T);
    
    return(Result);
}

inline transform DecomposeTransformMatrix(const m44& Matrix){
    transform Result = {};
    
    Result.T = Matrix.Rows[3];
    
    v3 Row0 = Matrix.Rows[0];
    v3 Row1 = Matrix.Rows[1];
    v3 Row2 = Matrix.Rows[2];
    
    float Row0Len = Magnitude(Row0);
    float Row1Len = Magnitude(Row1);
    float Row2Len = Magnitude(Row2);
    
    Result.S = V3(Row0Len, Row1Len, Row2Len);
    
    Row0 = Row0 / Row0Len;
    Row1 = Row1 / Row1Len;
    Row2 = Row2 / Row2Len;
    
    m33 RotMat = MatrixFromRows(Row0, Row1, Row2);
    
    Result.R = QuatFromM33(RotMat);
    
    return(Result);
}

#endif