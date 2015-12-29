/*
 *  big_int_divide.cpp
 *  big_int
 *
 *  Created by Chris on 7/8/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */ 
#include <iterator>
#include "big_int_divide.h"

#ifndef BASE_65536
const double RADIX = 256.0;
#else
const double RADIX = 65536.0;
#endif 

const big_int big_int::recip(int n) const
{
	#ifndef BASE_65536
	const int MF=4;
	#else
	const int MF=8;
	#endif
	const long double BI = 1.0L/RADIX;
	const big_int &v = *this; // alias to conform to NR
	
	//trim_zeros(); 
	
	int m = size(), mm = (MF > m ? m : MF); // minimum of MF and m
	
	if (m == 0 || m ==1 && v.rep.back() == 0 ) { // divide by zero
		throw div_0_exception();
	}
	
	if (n < 0) n = m; 
	
	long double  fv = static_cast<long double>(static_cast<unsigned int>(v.rep[m- mm]));
	
	// compute a floating point approximation to the reciprocal; this is the seed value to newton's method
	// (we use the 4 most significant "digits"
	for(int j= m - (mm-1); j < m; j++) {
		fv *= BI;
		fv += rep[j];
	}
	
	long double fu =  1.0L/fv;
	
	big_int u;
		
	u.rep.resize(n+MF+1); // construct a new vector of size n
	
	for (int j = n+MF; j >= 0; j--) { // fill it with the fractional expansion of our floating point approx
		int i = static_cast< int>(fu);
		u.rep[j] = static_cast<UInt16>(static_cast<UInt32>(i));
		fu = RADIX * (fu - i);
	}
	
	u.trim_zeros();
	//int j  = 0;
	for (int j=0;;j++) {
		using namespace std;
		big_int s(u * v);
		
		if (s.size() < n+MF+m) 
		s.rep.push_back(0); // if there was a zero (we shouldn't have trimmed zeros)
		
		ones_compl(s.rep);
		s.rep.back() += 2; // 2 - UV
		 
		big_int r(u*s);
		
		Array::reverse_iterator t =
				(r.rep.rbegin() + n + MF > r.rep.rend() ? r.rep.rend() : r.rep.rbegin() + n + MF);
		copy(r.rep.rbegin(), t, u.rep.rbegin());
	
		Array::iterator first_nonzero = find_if(s.rep.rbegin()+1,s.rep.rend(),non_zero()).base();
		if (j > 25) throw overflow_exception(); // this is a protection; 
		if (s.rep.end()-first_nonzero >= n) {
			u.rep.erase(u.rep.begin(),u.rep.begin()+MF);
			break;
		}
	}
	//std::cout << "It took " << j << " iterations to do this inversion.\n";
	u.sign = sign; // inverse has same sign as us
	return u;
}


// short division: divide into most sig. digit; quotient is the digit of answer
// and remainder + next digit is the dividend for the next step

UInt16 big_int::fastdiv( UInt16 divisor) // does not allocate memory; cannot throw
{
	
	UInt16 rem;
	UInt32 intermediate = 0;
	// VIOLATION!
	if (!divisor) throw div_0_exception();
	if (divisor == 1) return 0; // don't do work for divisors of 1
  
	for (register int j = rep.size()-1; j >= 0; j--) {
		intermediate += rep[j];
		// divide in place: new digit this number / divisor
		rep[j] = intermediate / divisor;
		// rem is the remainder of that division
		rem = intermediate % divisor; 
	
		// now make it the next digit in intermediate
		intermediate = static_cast<UInt32>(rem) * (static_cast<UInt32>(BIG)+1);
	}
	if (rep.size() > 0 && rep.back() == 0) rep.pop_back(); // get rid of initial zeros
	// std::transform(rep.rbegin(), rep.rend(), rep.rbegin(), Rem(theInt, rem));
	return rem;
}


bi_ldiv_t::bi_ldiv_t( const big_int &u, const big_int & v) : q(0), r(0)
{	
	using namespace std;
	
	size_t m = v.size(), n = u.size();
	if (m > n) {
		r = u; // remainder is the dividend
		return;
	}
	
	// take care of division by multiples of powers of the base 
	size_t shift = 0;
	while( shift < m  && v.rep [shift] == 0)  shift++;
	
	

	
	q.rep.reserve(n-m+1); // reserve memory
	// pretend the divisor is the shifted version
	m -= shift;
	
	if (m == 1) {
		q = u;
		r = q.fastdiv(v.rep[shift]);
		q.trim_zeros();
		if (shift > 0) { // must recompute the remainder
			q.rep.erase(q.rep.begin(), q.rep.begin()+ shift	);
			r = u - q*v;
		}
		
		r.sign = u.sign;
		q.sign = ((u.sign == v.sign) ? '+' : '-');
		r.trim_zeros();
		return;
	}
	
	big_int s(v.recip(n-m+1));
	big_int rr(s*u); // intermediate quotient; need to adjust precision
	s.sign = '+'; // we'll fix the sign later; it's the same as v's, remember?
	
	//if (rr.sign == '+') --rr; // this handles borderline case: ratio of two powers of 256
	//else ++rr;
	//--s;
	// this is to check if we have filled the left end or not; decrement takes care of borderline cases
	/*	
		cout << "\nDividend u:\n" << u.size() << " Digits; should be " << n << '\n' ;
		std::copy(u.rep.rbegin(), u.rep.rend(), ostream_iterator<unsigned int>(cout, " "));
		cout << "\n";
		cout << "Divisor v:\n" << v.size() << " Digits; should be " << m << '\n' ;
		std::copy(v.rep.rbegin(), v.rep.rend(), ostream_iterator<unsigned int>(cout, " "));
		cout << "\n";
		cout << "Reciprocal s:\n" << s.size() << " Digits; should be " << n-m+1 << '\n' ;
		std::copy(s.rep.rbegin(), s.rep.rend(), ostream_iterator<unsigned int>(cout, " "));
		cout << "\n";
		cout << "Intermediate Quotient rr:\n" << rr.size() << " Digits; should be " << m+n << '\n' ;
		std::copy(rr.rep.rbegin(), rr.rep.rend(), ostream_iterator<unsigned int>(cout, " "));
		cout << "\n\n";
	*/
	
	
	int FUDGE = (rr.size() == s.size() + u.size());
	
	// shift
	Array a(rr.rep.end()-n+m-FUDGE + shift, rr.rep.end());
	big_int qq(a);
	++qq; // increment as a check for roundoff error
	
	qq.sign = rr.sign;
	// remainder
	big_int t(u-qq*v);
	if (t==0) t.sign = '+'; // negative zero
	qq.sign = '+';
	
	if (t.sign != u.sign) {
		--qq; // quotient was too small
		if (rr.sign == '+') t+= v;
		else t-=v;
	}
	q = qq;
	q.sign = rr.sign;
	r = t;
	
	// trim zeros.
	q.trim_zeros();
	r.trim_zeros();
}

const big_int gcd(const big_int &a, const big_int &b)
{
	if (b == 0) return a;
	return gcd(b, a%b);
	/*
	if (b == 0) {
		std::cout << "BASE CASE! Answer:\n" << a <<"\n";
		return a; 
	}
	std::cout<< a << "\n" << b <<"\n\n";
	return gcd(b, a%b);
	*/
}