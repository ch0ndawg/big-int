/*
 *  multithreaded_fourier.h
 *  big_int
 *
 *  Created by Chris on 7/13/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#include <complex>
#include "fft.h"
#include "thread.h"

using namespace std;

template <class T>
class multithread_fourier {
	complex<T> *data;
	const int isign;
	const size_t n;
public: 
	enum {
		MIN_REC_SIZE = 32768 // min recursion size to justify the overhead incurred by parallel computation
	};
	multithread_fourier(complex<T>* d, int is, size_t nnn)
	: data(d), isign(is), n(nnn) {}
	void operator()()
	{ run();  // eventually we should handle standard exceptions here
	}
	void run();
};


// The actual routine.
//	Preconditions: the data consists of complex T's, namely pairs of T's. Must also have size a power of 2.
//	Throws:
//		std::bad_alloc for the creation of a vector<bool> in the transpose.
//		T's copy-assignment and -constructor exceptions (none if T is builtin like double)
//		Boost's thread resource exceptions
//		thread interrupted exceptions (which are ignored--this is what is supposed to happen since the only way 
//			interruption occurs is if the thread is canceled as a result of another exception)

template <class T>
void multithread_fourier<T>::run()
{
	// base case
	if (n <= MIN_REC_SIZE) {
		// need to reinterpret-cast, to treat a vector in C^n  as a vector in R^2n; the standard guarantees that complex
		// has a POD layout and that std::vectors are contiguous
#ifdef USE_BOOST_THREADS__
		boost::this_thread::interruption_point();
#endif
		fourier(reinterpret_cast<T*>(&data[0]),isign, n);
		return;
	}
	const size_t nn = n >> 1; // half
	
	// the phase factor
	T theta = 2*3.14159265358979323846264L*isign/n;
	const complex<T> w0(cos(theta), sin(theta)); // polar coordinates
	
	// split our vector in two
	complex<T> *v0 = data;
	complex<T> *v1 = data + nn;

	// regard the array a 2-by-n/2 array, wider than it is tall
	
	for (int i = 0; i < nn; i++) { // Trivially FT the 2-element columns
		complex<T> oldv0i = v0[i];
		v0[i] += v1[i];
		v1[i] = oldv0i - v1[i];
	}
	
	// now multiply the latter half by powers of the phase factor
	complex<T> w(1.0,0.0);
	for (int i=0; i < nn; i++, w *= w0) v1[i] *= w;

	// MULTITHREAD!!
	
	multithread_fourier<T> v0_compute(v0, isign, nn);
	multithread_fourier<T> v1_compute(v1, isign, nn);
		
	// spawn a new thread for the first half
	bi_thread first_half(v0_compute);
	
	// use this thread for the second half
		// (we could also have spawned another thread for this, making it look more "equitable"
		// but the overhead incurred by having excessive numbers of threads is not trivial. If two
		// were spawned, then one ends up with a large hierarchy of "master threads" idly waiting 
		// (but still taking up memory!) on the base case "slave threads" to complete.
	try {
		v1_compute.run();
		first_half.join(); // wait for the other half to complete.
	}
	catch (...) { // if we're interrupted
		// make sure the first half is interrupted too
		//std::cerr << "Exception occurred in a Fourier Transform thread; bailing...\n";
		first_half.interrupt();
		// disable interrupts
		first_half.join(); // wait for the other half to return from its interruption.
		throw; // now bail.
	}
	
	// Now we must transpose the array, i.e. interleave our two rows. We shall transpose the columns
	// via the cycle decomposition of the permutation of interleaving. 
	// The cycles are always half modulo n-1, e.g.
	// (0 1 2 3 4 5 6 7) ---> (0 4 1 5 2 6 3 7) gives (1 4 2)(3 5 6),
	// We observe: half of 1 mod 7 is half of 8 mod 7, which is 4, half of 4 is 2, half of 2 is 1
	// half of 3 mod 7 is half of 10 mod 7, which is 5; 5 is 12 mod 7, half of that is 6, and half of 6 is 3
	// It is not as straightforward as its moral equivalent, namely copying:
	
	/*
	 vector< complex<T> > oldv0(v0, v1);
	 vector< complex<T> > oldv1(v1, v1+nn);
	 
	 for (int i=0; i < nn; i++) {
	 data[2*i] = oldv0[i];
	 data[2*i +1] = oldv1[i];
	 }
	 */
	
	//  ... but it saves a lot of space and even gives a slight increase in speed!
	// you can't argue with an optimization that increases temporal and spatial performance!
	const size_t modulus = n-1;
	
	// setup: odd indices up to n/2 which have not occurred in a cycle
	// the infamous vector<bool>; we don't actually use any of the specialized functions
	// so this will not break when they change the standard. 
	// Currently it uses 1/128 of the memory of copying the whole vector 
	// when the  vector<bool> is removed, it's reduced to "only" 1/16 of it
	vector<bool> not_in_cycle(nn >> 1,true);
	
	// start with index 1 (index 0 and index n-1 are always preserved by interleaving)
	// we only need to go up to halfway, nn
	for (size_t start=1; start < nn; start += 2) {
		// if start has not already appeared in a cycle
		if (not_in_cycle[start>>1]) {
			size_t prev = start;
			// the temporary
			complex<T> temp = data[start]; 
			for(size_t j=0; ; prev = j) {
				// set j to the next index in the cycle
				if (prev & 1) { 
					// odd: add n-1 and halve
					j = (prev+modulus) >> 1;
					// mark prev as having appeared in a cycle (we only care if it is < n/2)
					if (prev < nn) not_in_cycle[prev>>1] = false;
				}
				// even: just halve; since we don't pay attention to evens in start, don't need to record it
				else j = prev >> 1;
				// if we closed the cycle, done!
				if (j == start) break;
				// actually permute
				data[prev] = data[j];
			}
			// set last one to the saved temporary before the cycle
			data[prev] = temp;
		}
	}
}

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
		multithread_fourier<T> mf(reinterpret_cast<complex<T> *>(&data[0]),1,n/2);
		mf.run();
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
		multithread_fourier<T> mf(reinterpret_cast<complex<T> *>(&data[0]),-1,n/2);
		mf.run();
	} // inverse transform
}