#ifndef VOICECHANGER_H
#define VOICECHANGER_H
#define F_S 3000

typedef short sample_t;

void die(char * s);

ssize_t read_n(
    int fd,
    ssize_t n,
    void * buf
);

ssize_t write_n(
    int fd,
    ssize_t n, 
    void * buf
);

void sample_to_complex(
    sample_t * s, 
    complex double * X, 
    long n
);

void complex_to_sample(
    complex double * X,
    sample_t * s, 
    long n
);

void fft_r(
    complex double * x, 
	complex double * y, 
	long n, 
	complex double w
);

void fft(
    complex double * x,
    complex double * y,
    long n
);

void ifft(
    complex double * y, 
	complex double * x, 
	long n
);

void ytoa(
    complex double * y,
    double * a,
    long n
);

void ytob(
    complex double * y,
    double * b,
    long n
);

void abtoy(
    double * a,
    double * b,
    complex double * y,
    long n
);

int pow2check(long N);

void freqshift(  /*引数に与えられたデータを高周波にシフト。*/
    long n, /*DFTに渡すデータの標本数。２のべき乗である必要がある。*/
    long f,  /*シフトする周波数。*/
    sample_t * data /*DFTするデータ配列の先頭アドレス。*/
);

int test( long N, long f);

#endif