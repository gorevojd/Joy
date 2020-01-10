#ifndef JOY_FFT_H
#define JOY_FFT_H

#include <std::complex>

#endif

#if defined(JOY_FFT_IMPLEMENTATION) && !defined(JOY_FFT_IMPLEMENTATION_DONE)
#define JOY_FFT_IMPLEMENTATION_DONE


void Dft(std::complex<float>* in, std::complex<float>* out, int count) {
	float invCount = 1.0f / (float)count;
	
	for (int n = 0; n < count; n++)
	{
		std::complex<float> xn = {};
        
		for (int k = 0; k < count; k++) {
			float angle = 2.0f * PI * (float)k * (float)n * invCount;
            
			float angleCos = cosf(angle);
			float angleSin = sinf(angle);
            
			xn.re += in[k].re * angleCos + in[k].im * angleSin;
			xn.im += in[k].re * -angleSin + in[k].im * angleCos;
		}
        
		out[n] = xn;
	}
}

void DftInv(std::complex<float>* in, int count, std::complex<float>* out) {
	float invCount = 1.0f / (float)count;
    
	for (int k = 0; k < count; k++)
	{
		std::complex<float> xk = {};
        
		for (int n = 0; n < count; n++)
		{
			float angle = 2.0f * PI * (float)k * (float)n * invCount;
            
			float angleCos = cosf(angle);
			float angleSin = sinf(angle);
            
			xk.re += in[k].re * angleCos + in[k].im * -angleSin;
			xk.im += in[k].re * angleSin + in[k].im * angleCos;
		}
        
		out[k] = xk * invCount;
	}
}

#endif