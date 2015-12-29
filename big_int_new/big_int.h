/*
 *  bigint.h
 *  big_int
 *
 *  Created by Chris on 6/30/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef ___BIG_INT_H___
#define ___BIG_INT_H___

#include <string>
#include <limits>
#include <vector>
#include <iostream>

/*********************************************************************************************
For standard machines in which sizeof(int) =4 , sizeof(short) = 2 
******************************************************************************************** */
#ifndef BASE_65536
typedef unsigned short  UInt32;
typedef unsigned char UInt16;
enum { MAX_DDIGIT = 2, MAX_DDIGIT_EXP = 100,
 REALLY_BIG = 0x0000FFFF, 
BIG = 0xFF,
LITT0 = BIG/10
};
#else
typedef unsigned int UInt32;
typedef unsigned short UInt16;

enum { MAX_DDIGIT = 4, MAX_DDIGIT_EXP = 10000,
 REALLY_BIG = 0xFFFFFFFF, 
BIG = 0xFFFF,
LITT0 = BIG/10
};
#endif
typedef  std::vector< UInt16 > Array;

/*
**********************************************************************************************/
using std::vector;
using std::string;
struct bi_ldiv_t; 
class big_int;

void mult_helper(big_int &lhs, big_int &rhs, bool assign_to_second);

class big_int {
public:
	// exception classes
	struct exception { 
		virtual exception *clone() const = 0;
		virtual string what() const = 0;
		virtual void repropagate() const = 0;
		virtual ~exception() {}
		//static void operator delete(void *T) { std::cerr << "Deallocating exception...\n"; ::operator delete(T);}
	};
	struct div_0_exception : public exception 
	{
		virtual div_0_exception *clone() const  { return new div_0_exception(*this); }
		virtual string what() const  { return "Division by zero occurred within intermediate calculation!"; }
		virtual void repropagate() const  { throw *this; }
	};
	struct overflow_exception : public exception {
		virtual overflow_exception *clone() const { return new overflow_exception(*this); }
		virtual string what() const { return "Internal precision exceeded within intermediate calculation!"; }
		virtual void repropagate() const { throw *this; }
	};

	struct invalid_argument : public exception { 
		int x; 
		invalid_argument(int y) : x(y) {} 
		virtual invalid_argument *clone() const { return new invalid_argument (*this); }
		virtual string what() const { return "Invalid argument within intermediate calculation!"; }
		virtual void repropagate() const { throw *this; }
	};
	
	// ***********************************************Constructors/Destructor ****************************
	
	big_int(int  x) : sign(x >= 0 ? '+' : '-'), rep() {init_ulong(x >= 0 ? x : -x); } // initialize with a long long int
	big_int(unsigned int  x=0ul): sign('+'), rep() { init_ulong(x); } // initialize with an unsigned long int
	big_int(const string &s) : sign('+'), rep() { init_string(s); }  // initialize with a string 
	// big_int(const big_int &rhs)  = default;
	// big_int& operator=(const big_int &rhs)= default; // (someday we will reference count and will have to update this)
	// ~big_int() =default;
	
	// ***********************************************Conversions **************************************
	
	const string to_string() const;
	// amusing application!
	//const string to_literal_ascii() const;
	int to_int() const;
	// long long to_longlong() const;
	
	//***************************************** Addition/Subtraction *************************************
	
	big_int& operator+=(const big_int &rhs);
	big_int& operator-=(const big_int &rhs);
	// add/sub helper functions 
	friend void unsigned_sub(big_int& min,  big_int& s, bool assign_to_second);
	friend void unsigned_add(big_int& min,  big_int& s, bool assign_to_second);
	
	// single-digit versions
	big_int& operator+=(UInt16 rhs);
	big_int& operator-=(UInt16 rhs);
	
	big_int& operator++()
	{ return operator+=(UInt16(1)); }
	
	// post-increment (highly inefficient.)
	const big_int operator++(int)
	{ big_int old_this(*this); operator++(); return old_this; }
	
	big_int& operator--()
	{ return operator-=(UInt16(1)); }
	
	// post-increment (highly inefficient.)
	const big_int operator--(int)
	{ big_int old_this(*this); operator--(); return old_this; }

	const big_int operator-() const { return big_int(*this).neg(); }
	// redundant... actually it does do something: it returns a copy
	const big_int operator+() const { return *this; } 
	
	// directly modify the sign
	big_int& neg() { sign = (sign == '+' ?  '-' : '+');  return *this; }
	
	friend void mult_helper(big_int &lhs, big_int &rhs, bool assign_to_second);
	
	
	//************************************** Multiplication/Division**************************************
	big_int& operator*=(const big_int &rhs) {
		if (size() < rhs.size()) mult_helper(rhs,*this);
		else mult_helper(*this,rhs);
		return *this;
	}
private:
	// reciprocal function; we interpret radix point at 0 and the reciprocal to n digits of precision
	// because it is an implementation detail used to do division, it is declared as private.
	//	if we actually attempt to use this function as a reciprocal, we would NOT get what we would expect,
	//	even if we accepted a convention for the placement of the radix point.
	// if it has the default (invalid) value of -1, then the precision will be made identical to the incoming number
	const big_int recip(int n = -1) const;
public:
	// division with remainder by a small number
	UInt16 fastdiv( UInt16 ); // the modulus is the return value

	// general single-digit versions
	big_int& operator*=(UInt16 rhs);
	big_int& operator/=(UInt16 rhs);
	
	const big_int operator*(UInt16 rhs) const { return big_int(*this) *= rhs; }
	const big_int operator/(UInt16 rhs) const { return big_int(*this) /= rhs; }

	// long division structures
	friend struct bi_ldiv_t;
	friend struct bi_sqrt_t;
	friend void div_helper(big_int& q, big_int &r, const big_int &u, const big_int &v);
	
	// general division operators
	big_int& operator/=(const big_int &rhs);
	big_int& operator%=(const big_int &rhs); 
	
	
	//********************************* exponentiation  *******************************************
	big_int& pow_eq(const big_int & y); // assign a power
	
	//********************************* size approximately in bytes **********************************
	size_t size() const { return rep.size(); }
	
	//*** *************Comparison operators*******************************************************
	
	
	bool operator==(const big_int& rhs) const { 
	// assumes zeros are trimmed
		for (int i=0;i < rep.size();i++) {
			if (rep[i] != rhs.rep[i]) return false;
		}
		return true;
	}
	// short form comparison
	bool operator==(UInt16 rhs) const { if (size() == 1) return rep.back() == rhs; return false; }
	bool operator!=(const big_int& rhs) const { return ! operator==(rhs);}
	bool operator<(const big_int&rhs) const;
	bool operator>(const big_int&rhs) const { return (rhs < *this); }
	bool operator<=(const big_int& rhs) const { return ! operator>(rhs); }
	bool operator>=(const big_int& rhs) const { return ! operator<(rhs); }
	
	//************************* I/O **********************************************************
	//formatted output
	friend std::ostream & operator<<(std::ostream&, const big_int &);
	friend std::istream & operator>>(std::istream&,  big_int &);
	
	//binary output
	void write(std::ostream &thefile)
	{ thefile.write(reinterpret_cast<const char*>(to_data_vec()),sizeof(UInt16) * size()); }
private:
	// construct a big_int from an array; this is private, as it refers to an implementation detail
	explicit big_int(const Array &a, char sgn='+') : rep(a), sign(sgn) {}
	const UInt16 *to_data_vec() const { return &rep[0]; }
	
	// arithmetic helper functions

	void mult_helper(const big_int &lhs, big_int &rhs) {::mult_helper(const_cast<big_int &>(lhs), rhs, true);}
	void mult_helper(big_int &lhs, const big_int &rhs) {::mult_helper(lhs, const_cast<big_int &>(rhs), false);}
	
	// ********************************The Actual Data Members! ****************
	char sign;
	Array rep; // array object
	
	// predicate function
	struct non_zero {
		bool operator()(UInt16 x) { return x!= 0;}
	};

	// trim excess zeros at the top of the representation -- essential for the correct functioning of some algorithms
	// this establishes a class invariant: a big_int is in a valid state when all its zeros are trimmed,
	// unless it is equal to zero; and it is always guaranteed to be nonempty (we always make sure a zero is added in that case)
	big_int& trim_zeros()  {
		Array::iterator first_nonzero = find_if(rep.rbegin(),rep.rend(),non_zero()).base();
		rep.erase(first_nonzero,rep.end());
		if (rep.empty()) rep.push_back(0);
		return *this;
	} 
	
	// eventually we'll want a reference count, which can be done even in the presence of const
	// mutable size_t ref_count;
	
	//**************************************Utility Initialization Routines*****************************
	
	void init_ulong(unsigned int x);
	void init_string(const string &s);
	void process_hex_str(const string &s) {}
	void process_oct_str(const string &s) {}
};

// bitwise utility functions (it assists in carries)
inline UInt16 hi_word(UInt32 x)
{ return BIG & x >> 8*sizeof(UInt16); }
inline UInt16 lo_word(UInt32 x)
{ return BIG & x; } 

// the base of the system (it is defined in the radix conversion module)

extern const double RADIX;

// Ones complement  allows us to fake subtraction at different places in the number
/*
struct ones_complement : public std::unary_function<UInt16,UInt16> {
	UInt32 &intermediate;
	ones_complement(UInt32& inter) : intermediate(inter) {}
	UInt16 operator()(UInt16 x) {
		intermediate = BIG+ hi_word(intermediate) - x;
		return lo_word(intermediate);
	}
};*/
inline void ones_compl(Array & x)
{
	UInt32 intermediate = BIG+1;
	size_t n = x.size();
	for (register int i=0; i < n; i++) {
		intermediate = BIG+ hi_word(intermediate) - x[i];
		x[i] = lo_word(intermediate);
	}
	//std::transform(x.begin(), x.end(), x.begin(), ones_complement(intermediate));
}

// long division structure: calculates both quotient and remainder together

struct bi_ldiv_t {
	big_int q;
	big_int r;
		
	bi_ldiv_t(const big_int &dividend, const big_int &divisor);
};

// implementation of the division assignment functions

inline	big_int& big_int::operator/=(const big_int &rhs)
{ return *this = bi_ldiv_t(*this,rhs).q; }

inline	big_int& big_int::operator%=(const big_int &rhs)
{	return *this = bi_ldiv_t(*this,rhs).r; }
/* Global 2-parameter arithmetic operators */

// throws std::bad_alloc, although not likely, since it will request only one additional byte
inline const big_int operator+(const big_int& lhs, const big_int &rhs)
{	return big_int(lhs) += rhs; } 

// throws std::bad_alloc, although not likely, since it will request only one additional byte
inline const big_int operator-(const big_int& lhs, const big_int &rhs)
{ return big_int(lhs) -= rhs; } 

// throws std::bad_alloc, big_int::overflow_exception, thread resource exceptions, thread-interrupt exceptions
inline const big_int operator*(const big_int& lhs, const big_int &rhs)
{ return big_int(lhs) *= rhs; } 

inline const big_int operator/(const big_int&lhs,const big_int&rhs)
{ return bi_ldiv_t(lhs,rhs) .q; }

inline const big_int operator%(const big_int&lhs,const big_int&rhs)
{ return bi_ldiv_t(lhs,rhs) .r; }

const big_int pow(const big_int& x, const big_int& y);

// square root (broken)
struct bi_sqrt_t {
	big_int w;
	big_int u;
	bi_sqrt_t(const big_int&r, int n=-1);
};

inline const big_int sqrt(const big_int &v, int n=-1)
{	return bi_sqrt_t(v,n).w; }

inline const big_int sqrtinv(const big_int &v, int n=-1)
{
	if (v==0) throw big_int::div_0_exception();
	return bi_sqrt_t(v,n).u; 
}

// GCD: one of the principal applications of big numbers
const big_int gcd(const big_int &a, const big_int& b);
#endif