/*
 *  bigint.cpp
 *  big_int
 *
 *  Created by Chris on 6/30/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "big_int.h" 
#ifdef MULTITHREAD
#include "multithreaded_fourier.h"
#else
#include "fft.h"
#endif
#ifdef BASE_65536
typedef unsigned char UInt8;
inline UInt8 hi_byte(UInt16 x)
{ return 0xFF & (x >> 8*sizeof(UInt8)); }
inline UInt8 lo_byte(UInt16 x)
{ return 0xFF & x; } 
#endif
#ifdef NO_LONG_DOUBLE
typedef double bi_double;
const size_t MAX_SIZE = 2097152;
#else
typedef long double bi_double;
#endif

big_int& big_int::operator*=(UInt16 rhs)
{	
	UInt16 carry = 0;
	// if it is 1, bail.
	if (rhs == 1) return *this;
	// allocate enough memory
	if (rep.capacity() < rep.size() + 1) {
		rep.reserve(rep.size() + 1); // this act may cause a throw, so do it before any modification
	}
	for ( int i = 0; i < rep.size(); i++) {
		UInt32 result = rep[i] * rhs + carry;
		rep[i] = result & BIG; // lo word
		carry = result /(BIG+1); // hi word
	}
	if (carry > 0) { // we need a new place for the last carry
		rep.push_back(carry); // guaranteed not to throw because we reserved space for it
	}
	
	return *this;
}


void big_int::init_ulong(unsigned int x)
{
	// note that representation stores the "digits" backward; this eases multiplication
	while (x > 0) {
		rep.push_back( static_cast<UInt16>(x & BIG) ); // lowest 16 bits
		x >>= 8*sizeof(UInt16); // shift right 16 bits
	}
	if (rep.empty()) rep.push_back(0); // if there was nothing
}

// The actual work of assignment-multiplication is done here
// Preconditions: none (even if zeros are not trimmed, which is the invariant, this is unaffected
// Throws: All thrown by FFT (bad_alloc, thread exceptions, thread interruption exceptions)
//	std::bad_alloc from copy-construction of many vectors
//	big_int::overflow_exception, if double is too short to represent the intermediate sums (in base 256), or long double too short (in base 65536) 
// Postconditions: the product has all its leading zeros trimmed. (and the state of leading zeros of the other factor is unchanged)
void mult_helper(big_int &lhs, big_int &rhs, bool assign_to_second)
{
	const double rx = 256.0;
	size_t n = lhs.size(), m = rhs.size(), p = m+n;
	
	big_int& bi_result = ( assign_to_second ? rhs : lhs);
	//size_t n_min = (assign_to_second? m:n);

#ifdef NO_LONG_DOUBLE
	if (n > MAX_SIZE  || m > MAX_SIZE)
		throw big_int::overflow_exception(); // intermediate results will exceed precision in FFT
#endif
	
	

#ifdef FOURIER
	size_t shift = 0;
	while( shift < m  && rhs.rep [shift] == 0)  
		shift++;
	m -= shift;
	if (m == 1) { // with shift,
		if (!assign_to_second) bi_result *= rhs.rep[shift];
		else bi_result = lhs * rhs.rep[shift];
		// add the zeros
		bi_result.rep.insert(bi_result.rep.begin(), shift, 0);
		return;
	}
	size_t nn = 1; // assert n > m
	// prepare floating point FFT vectors
	while (nn < n) nn <<= 1;
#ifdef BASE_65536
	nn <<= 2; // double nn until it is at or exceeds n (nearest power of 2)
#else
	nn <<= 1;
#endif
	std::vector<bi_double> a(nn,0.0L), b(nn,0.0L);

#ifdef BASE_65536
	// pack them in pairs
	for (int j=0;j<n;j++) {
		a[2*j] = lo_byte(lhs.rep[j]);
		a[2*j+1] = hi_byte(lhs.rep[j]);
	}
	for (int j=0;j<m;j++)  {
		b[2*j] = lo_byte(rhs.rep[j+shift]);
		b[2*j+1] = hi_byte(rhs.rep[j+shift]);
	}
#else
	// copy vectors
	for (int j=0;j<n;j++) a[j] = lhs.rep[j];
	for (int j=0;j<m;j++) b[j] = rhs.rep[j];
#endif
	// fft0rize0rize0rize0rize0rize0rize0rize0rize0rize0rize0rize0rize0rize
	realft(a,1); // may throw, but is operating on a copy
	realft(b,1); // "
	
	// complex multiply corresponding
	b[0] *= a[0];
	b[1] *= a[1];
	for (int j =2; j<nn; j+=2) {
		bi_double t = b[j];
		b[j] = t*a[j] - b[j+1]*a[j+1];
		b[j+1] = t*a[j+1] + b[j+1]*a[j];
	}
	
	// inverse fft0rize0rize0rize0rize0rize;
	realft(b,-1);

#else
	// implementation: Cauchy product.

	std::vector<bi_double> b(2*n, 0.0L);
	for (int i= 0 ; i < 2*n; i++) {
		for (int j = 0; j <= i; j++){
			if (i-j < n && j < m) {
				// use a bi_double for intermediate result
				b[i] += static_cast<bi_double>(lhs.rep[i-j]) * rhs.rep[j];
			}
		}
	}
#endif
	// do carries
	bi_double result=0.0, carry =0.0;
	
	bi_result.rep.resize(p+shift); // Make room for the result; may throw
	// shift: clear the end
	for (int j=0;j < shift; j++) bi_result.rep[j] = 0;
	// This section cannot throw.
	int i;
#ifdef BASE_65536
	for (i=0; i < 2*p-1; i++) { // note: inverse FT is N times larger
		result = b[i]/(nn >> 1) + carry + 0.5; // take care of roundoff

		carry = static_cast<bi_double> (static_cast<unsigned int>(result/rx)); // next carry is round off (t/rx)
		b[i] = result - carry * rx;
	}  // last component
	b[i] = carry;
	for (int j=0; j < p; j++ ) { // pack it
		bi_result.rep[j+shift] = static_cast<unsigned int>(rx) *
		static_cast<unsigned int>(b[2*j+1]) + static_cast<unsigned int>(b[2*j]);
	}

#else
	for (i=0; i < p-1; i++) { // note: inverse FT is N times larger
#ifdef FOURIER
		result = b[i]/(nn >> 1) + carry + 0.5; // take care of roundoff
#else
		result = b[i] + carry + 0.5; // take care of roundoff
#endif
		// CAUTION: we use unsigned int to cast  
		carry = static_cast<bi_double> (static_cast<unsigned int>(result/rx)); // next carry is round off (t/rx)
		bi_result.rep[i] =  static_cast<unsigned int> (result - carry * rx);
	}  // last component
	bi_result.rep[i] =  static_cast<unsigned int> (carry);
	// trim excess zeros
#endif
	bi_result.trim_zeros();

	// rule of signs
	if (rhs.sign != lhs.sign) bi_result.sign = '-';
	else bi_result.sign = '+';
}

// this call is extremely dangerous; for small arguments you could easily cripple your system
big_int& big_int::pow_eq(const big_int& y) 
{ 
	//return *this = pow(*this,y);
	
	// we use the method of repeated squaring 
	if (y.sign == '-') { // negative powers
		throw invalid_argument('-');
		/*big_int temp = recip(size()); //invert (exception safety)
		temp.pow_eq(-y);
		*this = temp;*/
	}
	if (y==0) { 
		return *this = big_int(1u); 
	}
	if (y==1) return *this;
	

	if (y.rep[0] & 1)  { //odd: multiply by the power  one below
	 	big_int old_this(*this); // this is needed to take care of aliasing: *= is implicitly assignment to self
		return pow_eq(y-1) *= old_this;
	}	
	 
	// square ourselves
	operator*=(*this);
	return pow_eq(y/2); // now raise to 1/2 the power
}

const big_int pow(const big_int& x, const big_int& y)
{ 
	if (y==0) return big_int(1u);
	return big_int(x).pow_eq(y);
}
