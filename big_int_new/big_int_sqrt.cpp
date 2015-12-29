/*
 *  big_int_sqrt.cpp
 *  big_int
 *
 *  Created by Chris on 7/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#include <math.h>
#include "big_int_sqrt.h"



bi_sqrt_t::bi_sqrt_t(const big_int &v,int n)
{
	const int MF=3;
	const  double BI=1.0/256.0;
	
	int m = v.size(), mm = (MF > m ? m : MF); // minimum of MF and m
	
	if (v.sign == '-' ) { // square root of negative
		throw big_int::invalid_argument(-2);
	}
	
	if (n < 0) n = m; 
	
	if (v.rep[m-mm] == 0) {
		u = 0;
		return;
	}
	double  fv = static_cast<long double>(static_cast<unsigned int>(v.rep[m- mm]));
	// compute a floating point approximation to the sqrt; this is the seed value to newton's method
	// (we use the 4 most significant "digits"
	for(int j= m - (mm-1); j < m; j++) {
		fv *= BI;
		fv += v.rep[j];
	}
	
	double fu =  1.0/sqrt(fv);
		
	u.rep.resize(n+MF+1); // construct a new vector of size n
	
	for (int j = n+MF; j >= 0; j--) { // fill it with the fractional expansion of our floating point approx
		int i = static_cast< int>(fu);
		u.rep[j] = static_cast<UInt16>(static_cast<UInt32>(i));
		fu = RADIX * (fu - i);
	}
	
	u.trim_zeros();
	
	
	for (;;) {
		using namespace std;
		
		//big_int r(u * u); // use Newton's method: Ui+1 = 1/2 Ui (3-VUi^2)
		
		
		big_int s(u * u * v); // UiV^2
		
		while (s.size() < 2*(n+MF) + v.size()) 
			s.rep.push_back(0); // if there was a zero (we shouldn't have trimmed zeros)
		
		// this fakes subtraction in the highest digit (easier than inserting a whole bunch of 0's
		// to 3, in order to use our genuine subtraction function
		ones_compl(s.rep);
		s.rep.back() += 3; // 3 - U^2 V
		
		s.fastdiv(2); // halve it.
		big_int r(u*s);
		
		Array::reverse_iterator t =
		(r.rep.rbegin() + n + MF > r.rep.rend() ? r.rep.rend() : r.rep.rbegin() + n + MF);
		copy(r.rep.rbegin(), t, u.rep.rbegin());
		//std::cout<<u.size() << std::endl;
		Array::iterator first_nonzero = find_if(s.rep.rbegin()+1,s.rep.rend(),big_int::non_zero()).base();
		if (s.rep.end() - first_nonzero >= n) { // not converged to 1
			big_int x(u*v); // u is the inverse square root, so get the real thing by multiplying by u
			w.rep.resize(n);
			copy(x.rep.rbegin(), x.rep.rbegin() + n, w.rep.rbegin()); 
			u.rep.erase(u.rep.begin(),u.rep.begin()+MF);
			break;
		}
		/*Array::reverse_iterator t =
				(r.rep.rbegin() + n + MF > r.rep.rend() ? r.rep.rend() : r.rep.rbegin() + n + MF);
		copy(r.rep.rbegin(), t, u.rep.rbegin());
		
		Array::iterator first_nonzero = find_if(s.rep.rbegin()+1,s.rep.rend(),non_zero()).base();
		if (s.rep.end()-first_nonzero >= n) {
			u.rep.erase(u.rep.begin(),u.rep.begin()+MF);
			break;
		}*/
	}
}

