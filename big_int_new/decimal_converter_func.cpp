/*
 *  decimal_converter_func.cpp
 *  big_int
 *
 *  Created by Chris on 7/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */ 

#include "decimal_converter_func.h"
#ifndef BASE_65536
const double decimal_converter_func::LOGBASE = 2.4082399653118495617099111577959; 
#else
const double decimal_converter_func::LOGBASE = 2*2.4082399653118495617099111577959; 
#endif

decimal_converter_func::decimal_converter_func(const big_int &bi)
: ordinary_radix_converter(bi) //the_info(), results_mutex()
{
	big_int the_base(THE_BASE);
	
	int a = bi.size()*LOGBASE;
	int b = a; 
	while (b > MIN_REC_SIZE * LOGBASE) b >>= 1; // halve until smaller than min recursion size
	
	
	powers.insert(bi_pair(b,the_base.pow_eq(b)));
	
	while ((a>>1) > b)  {
		b <<= 1; // double again; find the highest power of 2 times the smallest size that works
		the_base *= the_base;
		// insert it into our map.
		powers.insert(bi_pair(b,the_base));
	}
	
	orig_expected_size = b << 1;
} 


void decimal_converter_func::operator()(const big_int &bu, unsigned int digit_group_id, 
									unsigned int expected_size, bool is_first_group)
{ 
	static  const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	big_int bi(bu);
	using namespace std;
	vector<char> big_digits;
	
	big_digits.reserve(expected_size);
	
	while (bi.size() > 0) { // repeat until zero
		// divide by 10, get remainder, put in a new vector
		big_digits.push_back(digits[bi.fastdiv(THE_BASE)]);
	}
	
	// pad with zeros
	if (!is_first_group) { // this is not the first group; pad with zeros
		int num_zeros = expected_size - big_digits.size();
		if (num_zeros < 0) num_zeros = 0;
		string zero_pad = (num_zeros > 0 ? string(num_zeros,'0') : "");
#ifdef MULTITHREAD
		bi_mutex_lock l(results_mutex);
#endif
		insert_result(digit_group_id, zero_pad +
								   string(big_digits.rbegin(),big_digits.rend())
		);
	} 
	else if (!big_digits.empty()) {
		while (!big_digits.empty() && big_digits.back() == '0') big_digits.pop_back();	
#ifdef MULTITHREAD
		bi_mutex_lock l(results_mutex);
#endif
		insert_result(digit_group_id, string(big_digits.rbegin(),big_digits.rend()));
	}
	// return the string in reverse order
}

#ifdef MULTITHREAD
void decimal_converter_func::join_all()
{ 
	bool interrupt_remaining = false;
	std::auto_ptr<const big_int::exception> e(0);
	// do not obtain a lock; size() may change before the next iteration is complete!
	//for (info_queue::iterator p = the_info.begin(); p!= the_info.end(); ++p) {
	while (!the_info.empty()) {
		bi_persistent_info *p = &the_info.front();
		if (interrupt_remaining) p->the_thread->interrupt();
		p->the_thread->join();
		// exceptional condition
		std::auto_ptr<const big_int::exception> ex(p->frc->get_exception());
		if (ex.get() && !interrupt_remaining) {
			e = ex;
			interrupt_remaining = true;
		}
		//bi_mutex_lock l(q_mutex);
		the_info.pop_front();
	}
	// repropagate the exception
	if (e.get()) e->repropagate();
}
#endif