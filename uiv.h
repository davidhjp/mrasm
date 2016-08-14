#ifndef _UINTVector
#define _UINTVector

#include <stdio.h>

typedef struct UINTVector {
	unsigned int value,upper,lower,*reference;

	UINTVector(	unsigned int value=0,
				unsigned int upper=31, 
				unsigned int lower=0, 
				unsigned int *reference=NULL, bool referenced=true) {
		this->value=value;
		this->upper=upper;
		this->lower=lower;
		this->reference=NULL;
		if(referenced)
			 this->reference=(reference ? reference : &(this->value));
		ValidityCheck();
	}

	operator unsigned int() {
		return Cast();
	}

	unsigned int operator=(unsigned int i) {
		return Assign(i);
	}

	unsigned int operator=(UINTVector vector) {
		return Assign(vector);
	}

	UINTVector operator()(unsigned int point) {
		return UINTVector(value,point,point,reference,reference!=NULL);
	}

	UINTVector operator()(unsigned int upper, unsigned int lower) {
		return UINTVector(value,upper,lower,reference,reference!=NULL);
	}

	UINTVector operator,(UINTVector vector) {
		unsigned long
			length	= (upper-lower)+1,
			extra	= (vector.upper-vector.lower)+1;
		return UINTVector(vector | (Cast() << extra),length+extra-1,0,NULL,false);
	}

	unsigned int Cast() {
		unsigned long
			ccval	= (reference ? (*reference) : value),
			length	= (upper-lower)+1,
			mask	= (-1);
		return ~(mask << length) & (ccval >> lower);
	}

	unsigned int Assign(unsigned int i) {
		unsigned long
			ccval	= (reference ? (*reference) : value),
			length	= (upper-lower)+1,
			mask	= (-1);
		mask 		= (mask << length);
		mask 		= (~mask << lower);
		value		= (ccval & ~mask) | ((i << lower) & mask);
		if(reference)
			*reference = value;
		return Cast();
	}

	void ValidityCheck() {
		if( (upper > 31) || (upper < 0) ) {
			printf("upper bound out of range\n");
			exit(1);
		}
		if( (lower > 31) || (lower < 0) ) {
			printf("lower bound out of range\n");
			exit(1);
		}
		if( lower > upper ) {
			printf("incorrect bounds\n");
			exit(1);
		}
	}
} uint_vector;

#endif
