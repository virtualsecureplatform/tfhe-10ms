#ifndef TFHE_TEST_ENVIRONMENT
/* ***************************************************
TGSW fft operations
*************************************************** */

#include <cstdlib>
#include <iostream>
#include <random>
#include <cassert>
#include <ccomplex>
#include "tfhe_core.h"
#include "numeric_functions.h"
#include "lweparams.h"
#include "lwekey.h"
#include "lwesamples.h"
#include "lwe-functions.h"
#include "tlwe_functions.h"
#include "tgsw_functions.h"
#include "polynomials_arithmetic.h"
#include "lagrangehalfc_arithmetic.h"
#include "lwebootstrappingkey.h"

using namespace std;
#else
#undef EXPORT
#define EXPORT
#endif


//constructor content
EXPORT void init_TGswSampleFFT(TGswSampleFFT* obj, const TGswParams* params) {
    const int k = params->tlwe_params->k;
    const int l = params->l;
    TLweSampleFFT* all_samples = new_TLweSampleFFT_array((k+1)*l,params->tlwe_params);
    new(obj) TGswSampleFFT(params, all_samples);
}

//destructor content
EXPORT void destroy_TGswSampleFFT(TGswSampleFFT* obj) {
    int k = obj->k;
    int l = obj->l;
    delete_TLweSampleFFT_array((k+1)*l,obj->all_samples);
    obj->~TGswSampleFFT();
}


// For all the kpl TLWE samples composing the TGSW sample 
// It computes the inverse FFT of the coefficients of the TLWE sample   
EXPORT void tGswToFFTConvert(TGswSampleFFT* result, const TGswSample* source, const TGswParams* params) {
    const int kpl = params->kpl;
    
    for (int p=0; p<kpl; p++)
    tLweToFFTConvert(result->all_samples+p, source->all_sample+p, params->tlwe_params);
}

// For all the kpl TLWE samples composing the TGSW sample 
// It computes the FFT of the coefficients of the TLWEfft sample
EXPORT void tGswFromFFTConvert(TGswSample* result, const TGswSampleFFT* source, const TGswParams* params){
    const int kpl = params->kpl;
    
    for (int p=0; p<kpl; p++)
    tLweFromFFTConvert(result->all_sample+p, source->all_samples+p, params->tlwe_params);
}



// result = result + H
EXPORT void tGswFFTAddH(TGswSampleFFT* result, const TGswParams* params) {
    const int k = params->tlwe_params->k;
    const int l = params->l;
//cout<<"l  "<<l  <<endl;
    for (int j=0; j<l; j++) {
        Torus32 hj = params->h[j];
        for (int i=0; i<=k; i++)
       LagrangeHalfCPolynomialAddTorusConstant(&result->sample[i][j].a[i],hj); 
    }

}
EXPORT void tGswFFTAdd(TGswSampleFFT* result, TGswSampleFFT* result2, const TGswParams* params) {
    const int k = params->tlwe_params->k;
    const int l = params->l;

    for (int j=0; j<l; j++) {
        //Torus32 hj = params->h[j];
        for (int i=0; i<=k; i++)
       LagrangeHalfCPolynomialAddTorusConstant2(&result->sample[i][j].a[i],&result2->sample[i][j].a[i]); 
    }

}
// result = list of TLWE (0,0)
EXPORT void tGswFFTClear(TGswSampleFFT* result, const TGswParams* params) {
    const int kpl = params->kpl;

    for (int p=0; p<kpl; p++)
    tLweFFTClear(result->all_samples+p, params->tlwe_params);
}    
/*
// External product (*): accum = gsw (*) accum 
EXPORT void tGswFFTExternMulToTLwe(TLweSample* accum, const TGswSampleFFT* gsw, const TGswParams* params) {
    const TLweParams* tlwe_params=params->tlwe_params;
    const int k = tlwe_params->k;
    const int l = params->l;
    const int kpl = params->kpl;
    const int N = tlwe_params->N;
    //TODO attention, improve these new/delete...
    IntPolynomial* deca = new_IntPolynomial_array(kpl,N); //decomposed accumulator 
    LagrangeHalfCPolynomial* decaFFT=new_LagrangeHalfCPolynomial_array(kpl,N); //fft version
    TLweSampleFFT* tmpa = new_TLweSampleFFT(tlwe_params);

    for (int i=0; i<=k; i++)
	tGswTorus32PolynomialDecompH(deca+i*l,accum->a+i, params);
    for (int p=0; p<kpl; p++)
	IntPolynomial_ifft(decaFFT+p,deca+p);

    tLweFFTClear(tmpa, tlwe_params);
    for (int p=0; p<kpl; p++) {
	tLweFFTAddMulRTo(tmpa, decaFFT+p, gsw->all_samples+p, tlwe_params);
    }
    tLweFromFFTConvert(accum, tmpa, tlwe_params);

    delete_TLweSampleFFT(tmpa);
    delete_LagrangeHalfCPolynomial_array(kpl,decaFFT);
    delete_IntPolynomial_array(kpl,deca);
}
EXPORT void tGswFFTExternMulToTLwe(TLweSample* accum, const TGswSampleFFT* gsw, const TGswParams* params)
*/

EXPORT void tGswLagrangeHalfCPolynomialDecompH(LagrangeHalfCPolynomial* reps, const LagrangeHalfCPolynomial* pol, const TGswParams* params) {
    const int l = params->l;
    const int N = params->tlwe_params->N;
    //TODO attention, this prevents parallelization...
    static TorusPolynomial* a = new_TorusPolynomial(N);
    static IntPolynomial* deca = new_IntPolynomial_array(l,N);

    TorusPolynomial_fft(a,pol);
    tGswTorus32PolynomialDecompH(deca, a, params);
    for (int j=0; j<l; j++) {
	IntPolynomial_ifft(reps+j,deca+j);
    }
}

EXPORT void tGswFFTExternMulToTLwe(TLweSampleFFT* accum, TGswSampleFFT* gsw, const TGswParams* params) {
    const TLweParams* tlwe_params=params->tlwe_params;
    const int k = tlwe_params->k;
    const int l = params->l;
    const int kpl = params->kpl;
    const int N = tlwe_params->N;
    //TODO attention, this prevents parallelization...
    static LagrangeHalfCPolynomial* decomps=new_LagrangeHalfCPolynomial_array(kpl,N);

    for (int i=0; i<=k; i++)
	tGswLagrangeHalfCPolynomialDecompH(decomps+i*l,accum->a+i, params);
    tLweFFTClear(accum, tlwe_params);
    for (int p=0; p<kpl; p++)
	tLweFFTAddMulRTo(accum, decomps+p, gsw->all_samples+p, tlwe_params);
}
// result = (X^ai -1)*bki  

//This function is not used, but may become handy in a future release
//
EXPORT void tGswFFTMulByXai(TGswSampleFFT* result, const int ai,const int ai2,const int ai3, const TGswSampleFFT* bki, const TGswParams* params) {
    const TLweParams* tlwe_params=params->tlwe_params;
    const int k = tlwe_params->k;
    //const int l = params->l;
    const int kpl = params->kpl;
    const int N = tlwe_params->N;
    //on calcule x^ai-1 en fft
    //TODO attention, this prevents parallelization...
    static LagrangeHalfCPolynomial* xai=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai,ai);
    for (int p=0; p<kpl; p++) {
        const LagrangeHalfCPolynomial* in_s = bki->all_samples[p].a;
        LagrangeHalfCPolynomial* out_s = result->all_samples[p].a;
        for (int j=0; j<=k; j++)
            LagrangeHalfCPolynomialMul(&out_s[j], xai, &in_s[j]); 
    }
}

EXPORT void tGswFFTMulByXai8Add(TGswSampleFFT* result, const int ai,const int ai2,const int ai3, const TGswSampleFFT* bki,const TGswSampleFFT* bki2,const TGswSampleFFT* bki3,const TGswSampleFFT* bki4,const TGswSampleFFT* bki5,const TGswSampleFFT* bki6,const TGswSampleFFT* bki7,const TGswSampleFFT* bki8, const TGswParams* params) {
    const TLweParams* tlwe_params=params->tlwe_params;
    const int k = tlwe_params->k;
    //const int l = params->l;
    const int kpl = params->kpl;
    const int N = tlwe_params->N;
    //on calcule x^ai-1 en fft
    //TODO attention, this prevents parallelization...
    static LagrangeHalfCPolynomial* xai=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai,ai+ai2+ai3);

    static LagrangeHalfCPolynomial* xai2=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai2,ai+ai2);

    static LagrangeHalfCPolynomial* xai3=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai3,ai+ai3);

    static LagrangeHalfCPolynomial* xai4=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai4,ai2+ai3);

    static LagrangeHalfCPolynomial* xai5=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai5,ai);

    static LagrangeHalfCPolynomial* xai6=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai6,ai2);

    static LagrangeHalfCPolynomial* xai7=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai7,ai3);

    static LagrangeHalfCPolynomial* xai8=new_LagrangeHalfCPolynomial(N);
    LagrangeHalfCPolynomialSetXai(xai8,0);


    for (int p=0; p<kpl; p++) {
        const LagrangeHalfCPolynomial* in_s = bki->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s2 = bki2->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s3 = bki3->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s4 = bki4->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s5 = bki5->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s6 = bki6->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s7 = bki7->all_samples[p].a;
        const LagrangeHalfCPolynomial* in_s8 = bki8->all_samples[p].a;

        LagrangeHalfCPolynomial* out_s = result->all_samples[p].a;
        for (int j=0; j<=k; j++)
	{
		
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai, &in_s[j]);
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai2, &in_s2[j]);
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai3, &in_s3[j]);
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai4, &in_s4[j]);
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai5, &in_s5[j]);
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai6, &in_s6[j]);
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai7, &in_s7[j]);
	    cout << "2" << endl;
		LagrangeHalfCPolynomialAddMul(&out_s[j], xai8, &in_s8[j]);	
	    cout << "3" << endl;
	}
    }
}
//-------------------------------------------------------------------------------------
// autogenerated memory-related functions
//-------------------------------------------------------------------------------------

USE_DEFAULT_CONSTRUCTOR_DESTRUCTOR_IMPLEMENTATIONS1(TGswSampleFFT, TGswParams);

//
//----------------------------------------------------------------------------------------







#if 0
// BOOTSTRAPPING (as in CGGI16b - algo 3)
//  - modswitch: torus coefs multiplied by N/2
//  - set the test vector
//  - blind rotation by the phase
//  - sample extract 
//  - keyswitch
EXPORT void tfhe_bootstrapFFT(LweSample* result, const LweBootstrappingKeyFFT* bk, Torus32 mu1, Torus32 mu0, const LweSample* x){
    const Torus32 ab=(mu1+mu0)/2;
    const Torus32 aa = mu0-ab; // aa=(mu1-mu0)/2;
    const TGswParams* bk_params = bk->bk_params;
    const TLweParams* accum_params = bk_params->tlwe_params;
    const LweParams* extract_params = &accum_params->extracted_lweparams;
    const LweParams* in_out_params = bk->in_out_params;
    const int n=in_out_params->n;
    const int N=accum_params->N;
    const int Ns2=N/2;
    const int Nx2= 2*N;
    

    // Set the test vector (aa + aaX + ... + aaX^{N/2-1} -aaX^{N/2} - ... -aaX^{N-1})*X^{b}
    TorusPolynomial* testvect=new_TorusPolynomial(N);
    TorusPolynomial* testvectbis=new_TorusPolynomial(N);

    int barb=modSwitchFromTorus32(x->b,Nx2);
    //je definis le test vector (multiplié par a inclus !
    for (int i=0;i<Ns2;i++)
       testvect->coefsT[i]=aa;
    for (int i=Ns2;i<N;i++)
       testvect->coefsT[i]=-aa;
    torusPolynomialMulByXai(testvectbis, barb, testvect);



    // Accumulateur acc = fft((0, testvect))
    TLweSample* acc = new_TLweSample(accum_params);

    // acc will be used for tfhe_bootstrapFFT, acc1=acc will be used for tfhe_bootstrap
    tLweNoiselessTrivial(acc, testvectbis, accum_params);

    TGswSample* temp = new_TGswSample(bk_params);
    TGswSampleFFT* tempFFT = new_TGswSampleFFT(bk_params);


    // Blind rotation
//NICOLAS: j'ai ajouté ce bloc
#ifndef NDEBUG
    TorusPolynomial* phase = new_TorusPolynomial(N);
    int correctOffset = barb;
    cout << "starting the test..." << endl;
#endif
    // the index 1 is given when we don't use the fft
    for (int i=0; i<n; i++) {
        int bara=modSwitchFromTorus32(-x->a[i],Nx2);
        
        if (bara!=0) {
            tGswFFTMulByXaiMinusOne(tempFFT, bara, bk->bkFFT+i, bk_params);
            tGswFFTAddH(tempFFT, bk_params);
            tGswFFTExternMulToTLwe(acc, tempFFT, bk_params);
        }

//NICOLAS: et surtout, j'ai ajouté celui-ci!
#ifndef NDEBUG
	tLwePhase(phase,acc,debug_accum_key);  //celui-ci, c'est la phase de acc (FFT)
    if (debug_in_key->key[i]==1) correctOffset = (correctOffset+bara)%Nx2; 
        torusPolynomialMulByXai(testvectbis, correctOffset, testvect); //celui-ci, c'est la phase idéale (calculée sans bruit avec la clé privée)
    for (int j=0; j<N; j++) {
           printf("Iteration %d, index %d: phase %d vs noiseless %d\n",i,j,phase->coefsT[j], testvectbis->coefsT[j]);
    }
#endif

    }


    // Sample extract
    LweSample* u = new_LweSample(extract_params);
    tLweExtractLweSample(u, acc, extract_params, accum_params);
    u->b += ab;
    

    // KeySwitching
    lweKeySwitch(result, bk->ks, u);
    


    delete_LweSample(u);
    delete_TGswSampleFFT(tempFFT); 
    delete_TGswSample(temp);
    delete_TLweSample(acc);
    delete_TorusPolynomial(testvectbis);
    delete_TorusPolynomial(testvect);
}
#endif

#undef INCLUDE_ALL