/*
 *  big_int_add_sub.cpp
 *  big_int
 *
 *  Created by Chris on 7/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#include "big_int_add_sub.h"
#include <iterator>
#ifdef DEBUG___
#include <iostream>

#endif

/*inline UInt16 hi_word(UInt32 x)
{ return BIG & x >> 8*sizeof(UInt16); }
inline UInt16 lo_word(UInt32 x)
{ return BIG & x; } */


big_int& big_int::operator+=(UInt16 rhs) 
{
	if  (sign == '-') {
		throw invalid_argument(-1); // negs not supported, yet
	} 
	// reserve one extra space in advance
	rep.reserve(rep.size() + 1); // may throw
	UInt16 carry = rhs;
	for ( int i = 0; carry>0 && i < rep.size(); i++) {
		UInt32 result = carry + rep[i];
		rep[i] = hi_word(result); // lo word
		carry = lo_word(result); // hi word
	}
	if (carry > 0)  // we need a new place for the last carry
		rep.push_back(carry); // will not throw; we reserved it
	
	return *this;  
}
// safe interface to unsigned sub:

// we must cast away the const, sigh
// it is safe, because the last bool parameter will distinguish the mutable 
// from the immutable parameter; true says it's the second.
// (this is better than doing unary minus and *copying* the whole big int)
void unsigned_sub(big_int &min, big_int&s,bool);
inline void unsigned_sub( big_int& lhs, const big_int& rhs)
{ unsigned_sub(lhs,const_cast<big_int&>(rhs), false); } 

inline void unsigned_sub(const big_int& lhs, big_int& rhs)
{ unsigned_sub(const_cast<big_int&>(lhs), rhs, true); }


void unsigned_add(big_int &lhs, big_int&rhs,bool);
inline void unsigned_add( big_int& lhs, const big_int& rhs)
{ unsigned_add(lhs,const_cast<big_int&>(rhs), false); } 

inline void unsigned_add(const big_int& lhs, big_int& rhs)
{ unsigned_add(const_cast<big_int&>(lhs), rhs, true); }

struct add_algorithm : public std::binary_function<UInt16,UInt16,UInt16> {
	UInt32 &intermediate;
	add_algorithm(UInt32& inter) : intermediate(inter) {}
	UInt16 operator()(UInt16 x, UInt16 y) {
		intermediate = hi_word(intermediate) + x+y;
		return lo_word(intermediate);
	}
};

struct sub_algorithm : public std::binary_function<UInt16,UInt16,UInt16> {
	UInt32 &intermediate;
	sub_algorithm(UInt32& inter) : intermediate(inter) {}
	UInt16 operator()(UInt16 x, UInt16 y) {
		intermediate = BIG + hi_word(intermediate) + x-y;
		return lo_word(intermediate);
	}
};


struct continue_carry_algorithm : public std::unary_function<UInt16,UInt16> {
	UInt32 &intermediate;
	continue_carry_algorithm(UInt32& inter) : intermediate(inter) {}
	UInt16 operator()(UInt16 x) {
		intermediate = hi_word(intermediate) + x;
		return lo_word(intermediate);
	}
};

struct continue_borrow_algorithm : public std::unary_function<UInt16,UInt16> {
	UInt32 &intermediate;
	continue_borrow_algorithm(UInt32& inter) : intermediate(inter) {}
	UInt16 operator()(UInt16 x) {
		intermediate = BIG+ hi_word(intermediate) + x;
		return lo_word(intermediate);
	}
};

void unsigned_sub( big_int& min,  big_int& s, bool assign_to_second)
{
	// data validation is checked in +=; we may assume that the minuend is always > subtrahend
	// last parameter denotes which one is mutable (which can be assigned to)
	UInt32 intermediate = BIG +1;
	
	//  it would not be possible to do this with if/then statements because references must be initialized
	big_int &dest = assign_to_second ? s : min;
		
	dest.rep.reserve(min.rep.size()+1);
	
	int n = s.rep.size();
	int p = dest.rep.size();
	
	sub_algorithm sub_alg(intermediate);
	
	Array::iterator current = std::transform(min.rep.begin(), min.rep.begin() + n, s.rep.begin(), dest.rep.begin(),sub_alg);
	
	continue_borrow_algorithm borrow_alg(intermediate);
	
	if (p > n)  { // no back inserter
		std::transform(min.rep.begin() + n, min.rep.end(), current , borrow_alg);
	}
	else { // with a back inserter
		std::transform(min.rep.begin() + n, min.rep.end(), std::back_inserter(dest.rep) , borrow_alg);
	}
	// subtract
	/*for ( i = 0; i < m; i++) {
		result = BIG + min.rep[i] + hi_word(result);
		// as long as there's still rhs's, add them
		if (i < n) result -= s.rep[i];
		// keep carrying regardless
		(i < p )? dest.rep[i] = lo_word(result) : dest.rep.push_back(lo_word(result));
	}*/
}


void unsigned_add(big_int& min, big_int &s, bool assign_to_second)
{
	big_int& dest = assign_to_second ? s : min;
	
	dest.rep.reserve(min.rep.size()+1);
	
	UInt32 result;
	UInt16 carry = 0;
	int i;
	
	// add rhs to lhs
	for ( i = 0; i < min.rep.size(); i++) {
		result = carry+ min.rep[i];
		// as long as there's still rhs's, add them
		if (i < s.rep.size()) result += s.rep[i];
		// keep carrying regardless
		dest.rep[i] = static_cast<UInt16>(result); // lo word
		carry= result / (BIG+1); // hi word
	}
	
	// if still not done with rhs,
	for ( ;  i < s.rep.size(); i++) {
		result = carry + s.rep[i];
		dest.rep.push_back(static_cast<UInt16>(result));
		carry = result / (BIG+1);
	}
	if (carry > 0) dest.rep.push_back(carry);
	


/*


		// data validation is checked in +=; we may assume that the minuend is always > subtrahend
	// last parameter denotes which one is mutable (which can be assigned to)
	UInt32 intermediate = 0;
	
	//  it would not be possible to do this with if/then statements because references must be initialized
	big_int &dest = assign_to_second ? s : min;
		
	dest.rep.reserve(min.rep.size()+1);
	
	int n = s.rep.size();
	int p = dest.rep.size();
	
	add_algorithm add_alg(intermediate);
	
	Array::iterator current = std::transform(min.rep.begin(), min.rep.begin() + n, s.rep.begin(), dest.rep.begin(),add_alg);
	
	continue_carry_algorithm carry_alg(intermediate);
	
	if (p > n)  { // no back inserter
		std::transform(min.rep.begin() + n, min.rep.end(), current , carry_alg);
	}
	else { // with a back inserter
		std::transform(min.rep.begin() + n, min.rep.end(), std::back_inserter(dest.rep) , carry_alg);
	}
	UInt16 lastcarry = hi_word(intermediate);
	if (lastcarry > 0) dest.rep.push_back(lastcarry);*/
}

big_int & big_int::operator-=(const big_int &rhs)
{
	sign = (sign == '+' ? '-' : '+'); // switch signs
	operator+=(rhs); // add rhs
	sign = (sign == '+' ? '-' : '+'); // switch signs again
	return *this;
}

bool less_absolute(const Array& lhs, const Array &rhs)
{
	size_t n = lhs.size(), m= rhs.size();
	
	if (n < m) return true;
	if (n == m) {
		return lexicographical_compare(lhs.rbegin(),lhs.rend(),rhs.rbegin(), rhs.rend());
	/*	Array::const_reverse_iterator p1 = lhs.rbegin();
		Array::const_reverse_iterator p2 = rhs.rbegin();
		for ( ; p1 != lhs.rend(); p1++,p2++ ) {
			if (*p1 < *p2) return true;
		}
	*/
	}
	return false;
}
struct non_zero {
	bool operator()(UInt16 x) { return x!= 0;}
};

big_int& big_int::operator+=(const big_int &rhs) 
{
	if (sign == rhs.sign) {
		if (rep.size() >= rhs.rep.size()) {
			unsigned_add(*this, rhs); // same sign: do classic addition; the sign remains the same
		}
		else {
			unsigned_add(rhs, *this);
		}
	}
	// if we're of smaller magnitude
	else if (
	// if of equal size, lexicographically compare from the MOST significant digit
		less_absolute(rep,rhs.rep) ) { 
		
		unsigned_sub(rhs, *this);
		sign = rhs.sign; // copy sign of larger magnitude
	}
	else {
		// yes, i know, that is such a hack.
		unsigned_sub(*this, rhs);
	}
	// trim zeros
	// while (rep.size() > 1 && rep.back() == 0) rep.pop_back();
	
	// trim zeros via sophisticated fancy-schmancy STL methods
	Array::iterator first_nonzero = find_if(rep.rbegin(),rep.rend(),non_zero()).base();
	if (first_nonzero == rep.begin()) ++first_nonzero; // don't delete if it's the only 0.
	rep.erase(first_nonzero,rep.end());
	//if (rep.empty()) rep.push_back(0);
	return *this; 
}


big_int& big_int::operator-=(UInt16 rhs)
{
	if (sign == '+') { // again, we'll do this later
		throw invalid_argument(-1);
	} else {
		try {
			sign  = '+';
			operator +=(rhs);
		}
		catch(...) {
			sign = '-';
			throw;
		}
		sign = '-';
	}
	return *this;
}
