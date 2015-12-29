/*
 *  big_int_io.cpp
 *  big_int
 *
 *  Created by Chris on 7/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
 
#include <cctype>
#include "big_int_io.h"
#include "decimal_converter_func.h"

/*
struct Rem {
	const UInt16 divisor;
	UInt32 intermediate;
	UInt16 &rem;
		
	Rem(UInt16 div, UInt16& remain)  : divisor(div),  intermediate(0), rem(remain) {}
	UInt16 operator()(UInt16 x) {
		// intermediate was the remainder from last time; now tack on next digit
		intermediate += x;
		// divide in place: new digit this number / divisor
		x = intermediate / divisor;
		// rem is the remainder of that division
		rem = intermediate % divisor; 
	
		// now make it the next digit in intermediate
		intermediate = static_cast<UInt32>(rem) * (static_cast<UInt32>(BIG)+1);
		return x;
	}			
};
	
*/

big_int& big_int::operator/=(UInt16 rhs)
{
	fastdiv(rhs);
	return *this;
}

void big_int::init_string(const string& s) 
{
	using std::string;

	char saved_sign(sign);
	string::const_iterator p = s.begin();
	
	if (p == s.end()) { // string is empty 
		return; 
	} // empty string is 0
	if (*p == '-' || *p == '+') {
		saved_sign = *p; // save the sign; we affix it later (the algorithms will use actual multiplication) 
		++p; // advance
	}
	if (*p == 0 ) {
		if (*(p+1) == 'x') {
			process_hex_str(s);
			return;
		}
		
		if ( p+1 == s.end() ) {
			rep.push_back(0);
			return;
		}
		process_oct_str(s);
		return;
	}
	// kill off the sign
	size_t the_length = s.end() - p;
	
	// METHOD: we use synthetic division to do the radix conversion
	// to minimize the number of calls to *= we do it in chunks of 4
	// essentially we convert to base 10000 first, and then to base 65536 =USHRT_MAX
	
	// first, kill off misaligned term, since we're going left to right.
	// (recall we must group in 4s *from the right*, so we snip off any odd remainder from the left)
	int head = the_length % MAX_DDIGIT;
	
	UInt32 intermediate = 0;
	for (int i=0; i < head; ++i, ++p) {
		if (! std::isdigit(*p) ) throw invalid_argument(*p);
		intermediate *= 10;
		intermediate += *p - '0';
	}
	// even if it is 0, initialize the rep
	rep.push_back( intermediate);
	
	while (p != s.end() ) {
		// multiply by the large base
		*this *= MAX_DDIGIT_EXP;
		intermediate = 0;
		
		// convert the small one the usual way
		for (int i=0; i < MAX_DDIGIT; ++i, ++p) {
			// if not a digit, throw an exception
			if (! std::isdigit(*p) ) throw invalid_argument(*p);
			intermediate *= 10;
			intermediate += *p - '0';
		}
		
		// now tack on this newly computed digit
		*this += intermediate;
	}
	// everything done, save the sign. 
	sign = saved_sign;
}


using std::istream;
using std::ostream;
using std::string;

istream &operator >>(istream &str, big_int& bi)
{
	string s;
	str >> s; // read until whitespace
	
	big_int temp(s); // may throw, so create a new temporary object 
	
	bi = temp; // read the string using init_string
	return str;
}


// save & restore fill character
class auto_fill {
	ostream &str;
	char fillchar;
public :
	auto_fill(ostream&os) : str(os), fillchar(os.fill('0')) {}
	~auto_fill() { str.fill(fillchar); }
};

class ostream_output {
	ostream &str;
public:	
	ostream_output(ostream &stream) : str(stream) {}
	// output the digits; set the width to 4 digits, and the fill to 0 (placeholder 0s)
	void operator()(UInt16 y) { str << std::setw(MAX_DDIGIT)  << static_cast<UInt32>(y); }
};


ostream &operator<<(ostream &os, const big_int &bi)
{	
#ifdef MULTITHREAD
	typedef boost::shared_ptr<fancy_radix_converter> smart_frc_ptr;
	bi_ldiv_t *const null_quotrem = 0;
#endif
	decimal_converter_func ordinary_convert_radix(bi);
	//fancy_radix_converter<decimal_converter_func> the_computer(bi, ordinary_convert_radix);
	// calculate!
	
	//the_computer();
	// create another fancy radix converter on the heap, because we need local objects to persist after this thread returns
	// (notation is ewww, i know)
#ifdef MULTITHREAD
	smart_frc_ptr the_computer ( 
		new fancy_radix_converter(bi, ordinary_convert_radix)
	);
	// spawn a new thread to calculate the left "digit"
	ordinary_convert_radix.start_thread(
		bi_persistent_info(the_computer,smart_quotrem_ptr(null_quotrem))
	);

	ordinary_convert_radix.join_all(); // can't output until they're all done.
#else
	fancy_radix_converter the_computer(bi, ordinary_convert_radix);
	the_computer.f00();
#endif
	if (bi.sign == '+') {
		// if it is positive, AND showpos is enabled, output a + sign
		if (os.flags() & std::ios::showpos) os << '+';
	}
	else {
		os << '-'; // show a negative sign
	}
	
	const decimal_converter_func::results_map& the_digits = ordinary_convert_radix.get_results();
	
	int num_nonempties = 0;
	decimal_converter_func::results_map::const_iterator q = 
			the_digits.end();
	for (decimal_converter_func::results_map::const_iterator p = 
			the_digits.begin(); p != q; ++p) {
		if (!p->second.empty()) {
			num_nonempties++;
			os  << p->second;
		}
	}
	if (num_nonempties == 0) os << 0;
	return os;
} 

const string big_int::to_string() const
{
	// make a string stream
	std::ostringstream the_str;
	// use our operator << on it
	the_str << *this;
	
	return the_str.str();
}

// amusing application... 
/*
const string big_int::to_literal_ascii() const
{
	string result;
	for (int i=0; i < size(); i++) {
		if (rep[i] < 0x20) {
			result.push_back('^');
			result.push_back(rep[i] + '@');
		}
		else if (rep[i] == '^') {
				result.push_back('^');
				result.push_back('%');
		}
		else if (rep[i] == 0x7F) {
				result.push_back('^');
				result.push_back('\?');
		}
		else {
			result.push_back(rep[i]);
		}
 	}
	return result;
}
*/