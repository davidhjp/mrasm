#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct LinkedValue {
private:
	unsigned int value;
	LinkedValue* link;
	bool protect;

	static int instances;

public:
	LinkedValue(unsigned int value=0, bool constant=false):protect(false) {
		Disconnect(value);
		protect=constant;
		instances++;
	}

	LinkedValue(LinkedValue* link):protect(false) {
		Connect(link);
		instances++;
	}

	~LinkedValue() {
		instances--;
		if(instances == 0)
			printf("linkedvalue clear\n");
		else if(instances < 0) {
			printf("linkedvalue error, constructor unknown instance\n");
			exit(1);
		}
	}

	operator unsigned int() {
		return GetRootValue();
	}

	LinkedValue* operator+() {
		return GetLink();
	}
	LinkedValue* operator++() {
		return GetLink()->GetLink();
	}
	LinkedValue* operator*() {
		return GetRootLink();
	}

	unsigned int operator=(unsigned int value) {
		return SetRootValue(value);
	}
	unsigned int operator=(LinkedValue& value) {
		return SetRootValue(value);
	}

	unsigned int operator()(unsigned int value) {
		return Disconnect(value);
	}
	unsigned int operator()(LinkedValue* link) {
		return Connect(link);
	}

private:
	unsigned int SetRootValue(unsigned int value) {
		if(!protect) {
			if(link)
				return link->SetRootValue(value);
			this->value=value;
		}
		return GetRootValue();
	}
	unsigned int GetRootValue() {
		if(link)
			return link->GetRootValue();
		return value;
	}

	LinkedValue* GetLink() {
		if(link)
			return link;
		return this;
	}
	LinkedValue* GetRootLink() {
		if(link)
			return link->GetRootLink();
		return this;
	}

	unsigned int Connect(LinkedValue* link) {
		if(!protect) {
			bool linkvalid=true;
			if(link) {
				if( (link==this) || (**link==this) )
					linkvalid=false;
			}
			if(linkvalid) {
				Disconnect();
				this->link=link;
			}
		}
		return GetRootValue();
	}
	unsigned int Disconnect(unsigned int value=0) {
		if(!protect)
			this->link=NULL;
		return SetRootValue(value);
	}
} linkedvalue;

int linkedvalue::instances=0;

typedef struct LinkedVector {
private:
	linkedvalue* bit;
	unsigned int length;
	bool permanent;
	static int instances;
public:
	static linkedvalue one, zero;
	LinkedVector(char* value) {
		this->permanent=false;
		this->length=strlen(value);
		this->bit = new linkedvalue[length];
		AssignConstant(value);

		instances++;
	}
	LinkedVector(unsigned int length, bool permanent=true) {
		this->permanent=permanent;
		this->length=length;
		this->bit = new linkedvalue[length];
		instances++;
	}

	~LinkedVector() {
		delete [] bit;

		instances--;
		if(instances == 0)
			printf("linkedvector clear\n");
		else if(instances < 0) {
			printf("linkedvector error, unknown instance existed\n");
			exit(1);
		}
	}

	operator unsigned int() {
		return Cast();
	}
	LinkedVector operator=(unsigned int value) {
		return Assign(value);
	}
	LinkedVector operator=(LinkedVector vector) {
		return Assign(vector);
	}
	LinkedVector operator<=(unsigned int value) {
		return Link(value);
	}
	LinkedVector operator<=(LinkedVector vector) {
		return Link(vector);
	}
	LinkedVector operator()(unsigned int upper, unsigned int lower) {
		return Select(false,upper,lower);
	}	
	LinkedVector operator()(unsigned int element) {
		return Select(false,element,element);
	}
	LinkedVector operator*() {
		return Select();
	}
	LinkedVector operator,(LinkedVector vector) {
		return Concatenate(vector);
	}
private:
	LinkedVector Select(bool allselect=true,unsigned int upper=0,unsigned int lower=0) {
		int length;

		if(allselect)
			 length=this->length;
		else length=upper-lower+1;

		LinkedVector tmpvector(length, false);

		for(int i=0; i < length; i++) {
			if(permanent)
				 tmpvector.bit[i]( &bit[ i + lower ] );
			else tmpvector.bit[i]( +bit[ i + lower ] );
		}
		return tmpvector;
	}

	LinkedVector Concatenate(LinkedVector& vector) {
		LinkedVector tmpvector(length + vector.length, false);

		for(int i=0; i < vector.length; i++) {
			if(vector.permanent)
				 tmpvector.bit[i]( &vector.bit[i] );
			else tmpvector.bit[i]( +vector.bit[i] );
		}
		for(int i=0; i < length; i++) {
			if(permanent)
				 tmpvector.bit[i + vector.length]( &bit[i] );
			else tmpvector.bit[i + vector.length]( +bit[i] );
		}
		return tmpvector;
	}

	LinkedVector Link(unsigned int value) {
		linkedvalue* reference;
		for(int i=0; i < length; i++) {
			if(permanent)
				 reference=(&bit[i]);
			else reference=(+bit[i]);
			(*reference)( (value >> i) & 1 );
		}
		return Select();
	}

	LinkedVector Link(LinkedVector& vector) {
		linkedvalue* reference;
		for(int i=0; i < length; i++) {
			if(permanent)
				 reference=(&bit[i]);
			else reference=(+bit[i]);
			(*reference)( +vector.bit[i] );
		}
		return Select();
	}

	LinkedVector AssignConstant(char* value) {
		for(int i=length; i > 0; i--) {
			if(*value == '1')
				 bit[i-1]( &one );
			else bit[i-1]( &zero );
			value++;
		}
		return Select();
	}

	LinkedVector Assign(unsigned int value) {
		for(int i=0; i < length; i++)
			bit[i]=( (value >> i) & 1 );
		return Select();
	}
	LinkedVector Assign(LinkedVector& vector) {
		for(int i=0; i < length; i++)
			bit[i]=( vector.bit[i] );
		return Select();
	}

	unsigned int Cast() {
		unsigned int value=0;
		for(int i=length; i > 0; i--)
			value=(value << 1) | (bit[i-1] & 1);
		return value;
	}
} linkedvector,bv;

linkedvalue linkedvector::one(1,true);
linkedvalue linkedvector::zero(0,true);
int linkedvector::instances=0;

void print_binary(unsigned int value);

linkedvalue create_temp() {
	linkedvalue tmp(5);
	return tmp;
}

int main(int argc, char** argv) {

/*
	linkedvector x(8),y(8),z(16);
	z<=(*y,bv("01010011"));
	y=0xc4;

	print_binary(y);
	print_binary(z);
	printf("%d, %d\n", (uint)linkedvector::one, (uint)linkedvector::zero);

	z(15,12)<=( bv("1101") * bv("0110") );

//	z=0;
//	z(7,0)<=( bv("1101"),bv("0110") );
	print_binary(y);
	print_binary(z);
	printf("%d, %d\n", (uint)linkedvector::one, (uint)linkedvector::zero);*/
/*	{
		linkedvector a(4),b(4),c(4);
		b<=(*c);
		a<=(*c);
		c.Assign(0x7);
		print_binary(a.Cast());
		print_binary(b.Cast());
		print_binary(c.Cast());
		b.Assign(0x9);
		printf("count: %d\n",count);
		print_binary(a.Cast());
		print_binary(b.Cast());
		print_binary(c.Cast());
	}*/
/*	linkedvector a(8),b(8),muxout(8),adderin(8),d(16);
	d<=(*a,*b);
	muxout<=(*a);
	adderin<=(*muxout);
	a=0x7e;
	b=0x94;
	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("d: "); print_binary(d);
	printf("m: "); print_binary(muxout);
	printf("o: "); print_binary(adderin);	
	printf("\n");
	d(3,0)<=(b(4),b(5),b(6),b(7));
	muxout<=(*b);
	a=0xff;
	b=0x78;
	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("d: "); print_binary(d);
	printf("m: "); print_binary(muxout);

	linkedvector result(9);
	result=(adderin + a);
	printf("o: "); print_binary(result(8,6));
	printf("\n");

	linkedvector v00(4),v01(4),v02(4);
	linkedvector v10(8),v11(4);
	linkedvector v20(16);
	linkedvector v30(12);

	v10(7,0)<=(*v00,*v01);
	v20<=(*v10,(*v11<=*v01),*v02);
	v30<=(v20(15,12),*v01,*v00);
	v00=0xe;
	v11(3,2)<=(uint)0x3;
	v01=0x9;
	v02=0x5;
	//v10.Assign(0x9f);
	printf("v11: "); print_binary( v11 );
	printf("v20: "); print_binary( v20 );
	printf("v30: "); print_binary( v30 );
*/
/*	linkedvector v0(8),v1(8),v2(4),v3(18),two(2);

	two.Assign(1);

	v3 <= (v0,v1,two);
	v2 <= v1(3,0);

	v0.Assign(0x9C);
	v1.Assign(0x137);

	printf("v0: "); print_binary(v0.Cast());
	printf("v1: "); print_binary(v1.Cast());
	printf("v2: "); print_binary(v2.Cast());
	printf("\n");
	printf("v3: "); print_binary(v3.Cast());*/

	linkedvalue a,b,c;

	a=1;
	b=3;
	c=4;

	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("c: "); print_binary(c);
	printf("\n");

	b(&a);
	c(&b);

	a=14;

	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("c: "); print_binary(c);
	printf("\n");

//	c.root();
	printf("%x,%x,%x\n",&a,&b,&c);
	printf("%x,%x\n",++c,+c);
	c(*c);
	b(1);
	c=create_temp();
	b(13);
//	b=13;

	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("c: "); print_binary(c);
	printf("\n");

	c(b);
	b=15;
	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("c: "); print_binary(c);
	printf("\n");

	a(0xc);
	b(&a);
	c(&b);
	a(&c);
	a(&a);
	printf("a: "); print_binary(a);
	printf("b: "); print_binary(b);
	printf("c: "); print_binary(c);
	printf("\n");

	return 0;
}

void print_binary(unsigned int value) {
	unsigned int bit=(1 << 31);
	for(int i=0; i < 32; i++) {
		if(i && !(i%8))
			 printf(",");
		if(value & bit)
			 printf("1");
		else printf("0");
		bit=(bit >> 1);
	}
	printf("\n");
}

