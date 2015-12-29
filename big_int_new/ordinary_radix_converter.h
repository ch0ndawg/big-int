/*
 *  ordinary_radix_converter.h
 *  big_int
 *
 *  Created by Chris on 7/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <map>
#include "big_int.h"

struct bi_persistent_info;
class ordinary_radix_converter {
public: // typedef for the results
	typedef std::map<unsigned int,std::string> results_map;

protected:
	// protected data members: every derived class must have these items, but we can provide no reasonable 
	// default implementation and leave it up to derived classes to properly initialize these 
	typedef std::map<size_t,big_int> powers_map;
	typedef powers_map::value_type bi_pair;
	// the appropriate powers of 10
	powers_map powers;
	size_t orig_expected_size;

private:
	typedef results_map::value_type result_pair;
	// original number
	const big_int &num;
	//  resultant digit strings
	results_map results;

	// bi_thread_mutex q_mutex;
	
protected:
	
	void insert_result(unsigned int i, const std::string&s) { results.insert(result_pair(i,s)); }
	
public:
	ordinary_radix_converter(const big_int &bi)
	: num(bi), results(), powers(), orig_expected_size(0)
	{}
	
	size_t size() const { return orig_expected_size; }

	virtual int get_base() const = 0;
	virtual size_t min_rec_size() const = 0;
	virtual void operator()(const big_int &bi, unsigned int digit_group_id,
					unsigned int expected_size, bool is_first_group) = 0;

	virtual void start_thread( const  bi_persistent_info  &p) {}

	const results_map &get_results() const { return results; }
	const big_int& get_power(size_t expected_size) const { return powers.find(expected_size)->second; }
	//void join_all();
	virtual ~ordinary_radix_converter() {}
};

