/*
 *  fft.h
 *  big_int
 *
 *  Created by Chris on 7/3/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
 
// Adapted from Numerical Recipes
#include <vector>
#include <tgmath.h>


// Preconditions:
//	Data is a complex-flattened array, namely each pair of T's forms a complex number in higher level code
//	Data is a power of 2 in size.
// Postconditions:
//	Data is again
// Throws: 
//	const char*, "N must be a power of 2"
//	anything that may be thrown by copy-construction and -assignment of T
//		in particular, for builtins, we are guaranteed to no throw anything else
//	all operations are on primitive data

template<class T>
void fourier(T* data, int isign, size_t n) /* throw (const char*) */
{
	using namespace std;
	int nn, mmax, m, j, istep, i;
	
	nn = n << 1;
	// validation
	if (n< 2) return;
	if (n & (n-1)) throw("n must be a power of 2");

	// Bit-reverse exchange (i.e. convert to row major instead of col major)
	j = 1;
	for (i = 1; i < nn; i += 2) {
		if (j > i) { // swap
			T temp = data[j-1];
			data[j-1] = data[i-1];
			data[i-1] = temp;
		
			temp = data[j];
			data[j] = data[i];
			data[i] = temp;
		}
		m = n;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1; // halve the size
		}
		j += m;
	}
	
	// Danielson-Lanczos: calculate the FFT from rearranged data
	
	mmax = 2;
	while (nn > mmax) {
		T tempr, tempi;
		
		istep = mmax << 1; // calculate trig functions by recurrence
		T theta(isign*(2*3.14159265358979323846264L/mmax));
		T wtemp (sin(0.5*theta));
		T wpr(-2.0*wtemp*wtemp);
		T wpi(sin(theta));
		T wr(1.0);
		T wi(0.0);
		for (m = 1; m < mmax ; m +=2 ) {
			for (i= m; i <= nn; i += istep) {
				j = i + mmax; // the D-L formula
				tempr = wr*data[j-1] - wi*data[j]  ; // complex product w * complex data[j/2]
				tempi = wr*data[j]   + wi*data[j-1];
				data[j-1] = data[i-1] - tempr;
				data[j] = data[i] - tempi;
				data[i-1] += tempr;
				data[i] += tempi;
			}
			// trig recurrence
			wr = (wtemp=wr) * wpr - wi*wpi + wr;
			wi = wi*wpr + wtemp*wpi+wi;
		
		}
		mmax = istep;
	}
}
// fft for real-valued functions
//	preconditions: data is a vector of T which represents true real values, not complex values, of size a power of 2
//	throws:
//		If multithreaded, everything that the multithreaded fourier may throw, which includes:
//			Boost::thread exceptions (resource allocation errors and such)
//			T's copy assignment and construction exceptions (none if T is built-in)
//			std::bad_alloc, from transpose operations in thread
//			thread-interrupted exception.
//		Fourier itself may throw const char * if length is not a power of 2.
#ifndef MULTITHREAD
template <class T>
void realft(std::vector<T> & data, int isign)
{
	using namespace std;
	int i, i1, i2, i3, i4, n = data.size();
	T c1 = 0.5, c2, h1r, h1i, h2r, h2i, wr, wi, wpr, wpi, wtemp;
	T theta = 3.14159265358979323846264L/(n>> 1);
	
	if (isign == 1 ) {
		c2 = -0.5;
		// fourierize the data
		fourier(&data[0],1,n/2);
	} else {
		c2 = 0.5;
		theta = -theta;
	}
	
	wtemp = sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi = sin(theta);
	wr = 1.0 + wpr;
	wi = wpi; 
	
	for (i = 1; i < (n>>2) ; i++ ) {
		i2 = 1 + (i1 = i + i);
		i4 = 1 + (i3 = n - i1);
		
		h1r = c1*(data[i1] + data[i3]); // separate transforms
		h1i = c1*(data[i2] - data[i4]);
		h2r = -c2*(data[i2] + data[i4]);
		h2i = c2*(data[i1] - data[i3]);
		
		data[i1] = h1r +  wr*h2r - wi*h2i; // recombination 
		data[i2] = h1i +  wr*h2i + wi*h2r;
		data[i3] = h1r -  wr*h2r + wi*h2i;
		data[i4] = -h1i + wr*h2i + wi*h2r;
		
		wr = (wtemp = wr) * wpr - wi*wpi + wr; // recurrence
		wi = wi*wpr + wtemp*wpi + wi;
	}
	
	if (isign == 1) {
		data[0] = (h1r = data[0]) + data[1];
		data[1] = h1r -data[1];
	} else {
		data[0] = c1*((h1r = data[0]) + data[1]);
		data[1] = c1*(h1r-data[1]);
		fourier(&data[0],-1,n/2);
	} // inverse transform
}
#endif
#define FOURIER