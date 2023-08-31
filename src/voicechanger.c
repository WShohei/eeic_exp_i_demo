/*

*/

#include <assert.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/voicechanger.h"  
typedef short sample_t;

void die(char * s) {
    perror(s); 
    exit(1);
}

/* fd から 必ず n バイト読み, bufへ書く.
   n バイト未満でEOFに達したら, 残りは0で埋める.
   fd から読み出されたバイト数を返す */
ssize_t read_n(
    int fd,
    ssize_t n,
    void * buf
) {
    
    ssize_t re = 0;
    while ( re < n) {
        ssize_t r = read( fd, buf+re, n-re);
        if ( r==-1) die("read");
        if ( r==0) break;
        re += r;
    }
    memset(buf + re, 0, n - re);
    return re;

}

/* fdへ, bufからnバイト書く */
ssize_t write_n(
    int fd,
    ssize_t n, 
    void * buf
) {

    ssize_t wr = 0;
    while (wr < n) {
        ssize_t w = write(fd, buf + wr, n - wr);
        if (w == -1) die("write");
        wr += w;
    }
    return wr;

}

/* 標本(整数)を複素数へ変換 */
void sample_to_complex(
    sample_t * s, 
    complex double * X, 
    long n
) {

    long i;
    for (i = 0; i < n; i++) {
        X[i] = s[i];
    }

}

/* 複素数を標本(整数)へ変換. 虚数部分は無視 */
void complex_to_sample(
    complex double * X,
    sample_t * s, 
    long n
) {
    
    long i;
    for ( i=0; i<n; i++) {
        s[i] = creal(X[i]);
    }

}

/* 高速(逆)フーリエ変換;
   w は1のn乗根.
   フーリエ変換の場合   偏角 -2 pi / n
   逆フーリエ変換の場合 偏角  2 pi / n
   xが入力でyが出力.
   xも破壊される
 */
void fft_r(
    complex double * x, 
	complex double * y, 
	long n, 
	complex double w
) {

    if (n == 1) { y[0] = x[0]; }
    else {
        complex double W = 1.0;
        long i;
        for ( i=0; i<n/2; i++) {
            y[i] = (x[i] + x[i+n/2]); /* 偶数行 */
            y[i+n/2] = W * (x[i] - x[i+n/2]); /* 奇数行 */
            W *= w;
        }
        fft_r( y, x, n/2, w*w);
        fft_r( y+n/2, x+n/2, n/2, w*w);
        for ( i=0; i<n/2; i++) {
            y[2*i] = x[i];
            y[2*i+1] = x[i+n/2];
        }
    }

}   

void fft(
    complex double * x,
    complex double * y,
    long n
) {

    long i;
    double arg = 2.0 * M_PI / n;
    complex double w = cos(arg) - 1.0j * sin(arg);
    fft_r(x, y, n, w);
    for (i = 0; i < n; i++) y[i] /= n;

}

void ifft(
    complex double * y, 
	complex double * x, 
	long n
) {

    double arg = 2.0 * M_PI / n;
    complex double w = cos(arg) + 1.0j * sin(arg);
    fft_r(y, x, n, w);

}

void ytoa(
    complex double * y,
    double * a,
    long n
){
    
    for( long i=0; i<n/2+1; i++) {
        if( i==0){
            a[i] = creal(y[i]);
        } else if ( i==n/2) {
            a[i] = creal(y[i]);
        } else {
            a[i] = 2*creal(y[i]);
        }
    }

}

void ytob(
    complex double * y,
    double * b,
    long n
){
    
    for( long i=0; i<n/2+1; i++) {
        if ( i==0) {
            b[i] = 0;
        } else if ( i==n/2) {
            b[i] = 0;
        }
        b[i] = -2*cimag(y[i]);
    }
    
}

void abtoy(
    double * a,
    double * b,
    complex double * y,
    long n
) {

    for ( long i=0; i<n; i++) {
        if ( i==0) {
            y[i] = a[i];
        } else if ( i<n/2) {
            y[i] = 1.0/2.0*(a[i] - 1.0j * b[i]);
        } else if ( i==n/2) {
            y[i] = a[i];
        } else {
            y[i] = 1.0/2.0*(a[n-i] + 1.0j * b[n-i]);
        }
    }

}

int pow2check(long N) {
    long n = N;
    while ( n>1) {
        if ( n%2) return 0;
        n = n/2;
    }
    return 1;
}

void freqshift(  /*引数に与えられたデータを高周波にシフト。*/
    long n, /*DFTに渡すデータの標本数。２のべき乗である必要がある。*/
    long f,  /*シフトする周波数。*/
    sample_t * data /*DFTするデータ配列の先頭アドレス。*/
) {

    if (!pow2check(n)) {
        fprintf(stderr, "error : n (%ld) not a power of two\n", n);
        exit(1);
    }
    
    complex double * X = calloc(sizeof(complex double), n);
    complex double * Y = calloc(sizeof(complex double), n);
    double * A = calloc(sizeof(double), n/2+1);
    double * B = calloc(sizeof(double), n/2+1);
    
    /* 複素数の配列に変換 */
    sample_to_complex( data, X, n);
    
    /* FFT -> Y */
    fft( X, Y, n);
    /* 複素数領域のフーリエ変換を実数領域に変換*/
    ytoa( Y, A, n);
    ytob( Y, B, n);
    
    /*周波数シフトの中身*/
    if ( f>=0) {
        long l = round(n*f/F_S);
        for ( long i=0; i<l; i++) {
            double pre = A[i];
            A[i] = 0.0;
            long j = i + l;
            while( j<=n/2) {
                double temp = A[j];
                A[j] = pre;
                pre = temp;
                j = j + l;
            }            
        }
        for ( long i=0; i<l; i++) {
            double pre = B[i];
            B[i] = 0.0;
            long j = i + l;
            while( j<=n/2) {
                double temp = B[j];
                B[j] = pre;
                pre = temp;
                j = j + l;
            }            
        }
    } else {
        long l = -round(n*f/F_S);
        for ( long i=0; i<l; i++) {
            double pre = A[n/2-i];
            A[n/2-i] = 0.0;
            long j = i + l;
            while( j<=n/2) {
                double temp = A[n/2-j];
                A[n/2-j] = pre;
                pre = temp;
                j = j + l;
            }            
        }
        for ( long i=0; i<l; i++) {
            double pre = B[n/2-i];
            B[n/2-i] = 0.0;
            long j = i + l;
            while( j<=n/2) {
                double temp = B[n/2-j];
                B[n/2-j] = pre;
                pre = temp;
                j = j + l;
            }            
        }
    }
    abtoy( A, B, Y, n);
    /* IFFT -> Z */
    ifft(Y, X, n);
    /* 標本の配列に変換 */
    complex_to_sample(X, data, n);
    free(X);
    free(Y);
    free(A);
    free(B);

}

/*ボツ*/
short * bufrun(
    long f,/*シフトする周波数*/
    short * data,/*入力データの配列（short型16個=32bitを想定）*/
    int datasize,/*入力データ配列のサイズ（16を想定）*/
    short * buf,/*バッファ配列（short型長さはn）*/
    int bufsize/*バッファ配列のサイズ（一度にフーリエ変換するデータセット）*/
) {

    short * temp1 = (short *)calloc( sizeof(short), (size_t)datasize);
    short * temp2 = (short *)calloc( sizeof(short), (size_t)datasize);
    memcpy( temp1, data, (size_t)datasize);
    for ( int i=0; i<bufsize/datasize; i=i+1) {
        memcpy( temp2, buf+i*datasize, (size_t)datasize);
        memcpy( buf+i*datasize, temp1, (size_t)datasize);
        memcpy( temp1, temp2, (size_t)datasize);
    }
    sample_t * cp = (sample_t *)calloc(sizeof(sample_t),(size_t)bufsize);
    freqshift( (long)bufsize, f, cp);
    short  * p = cp+(bufsize-datasize-1)*datasize;
    return p;

}

int test( long N, long f) {

    short * data = (short *)calloc(sizeof(short),16);
    short * buf = (short *)calloc(sizeof(short),N);

    while (1) {

        /* 標準入力からn個標本を読む */
        int m = read_n( 0, (ssize_t)2*16, data);
        if ( m<=0) break;
        data = bufrun( 100, data, 16, buf, N);
        m = write_n( 1, (ssize_t)2*16, data);
        if ( m<=0) { break;}

    }
    return 0;

}