#!/bin/bash
//usr/bin/tail -n +2 $0 | g++ -O3 -o ${0%.cpp} -x c++ - && ./${0%.cpp} && rm ./${0%.cpp} ; exit
//
// This unit test can be directly executed, just `chmod u+x Double_t_UNITTEST.cpp` it
//
// C++ BigNum template class
// AKA the integer doubler template.
// Copyright(C) 2007,2015 Jeremy Linton
//
// Source identity: DoubleInt_t_UNITTEST.cpp
//
// This module provides some low level unit tests for the 
// "private" ????Double() routines in the int128 and doubler template
// It also does some basic verification of the exported class operators.
// 
// See DoubleInt_t.hpp for more information
//
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies 
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
// USE OR OTHER DEALINGS IN THE SOFTWARE.




//
//
//          Some unit test methods, most of these 
//          output data which must be verified by hand
//          or just diff the output from multiple runs.
//
//
//


#include "DoubleInt_t.hpp"


#define _UNITTEST_ //for now just leave the unittest on
#ifdef _UNITTEST_


typedef class DoubleInt_t<int128>    int256;
typedef class DoubleInt_t<int256>    int512;
typedef class DoubleInt_t<int512>    int1024;
typedef class DoubleInt_t<int1024>   int2048;
typedef class DoubleInt_t<int2048>   int4096;
typedef class DoubleInt_t<int4096>   int8192;
typedef class DoubleInt_t<int8192>   int16384;
typedef class DoubleInt_t<int16384>  int32768;
typedef class DoubleInt_t<int32768>  int65536;
typedef class DoubleInt_t<int65536>  int131072;
typedef class DoubleInt_t<int131072> int32kB;
typedef class DoubleInt_t<int32kB>   int64kB;
typedef class DoubleInt_t<int64kB>   int128kB;
typedef class DoubleInt_t<int128kB>  int256kB;
typedef class DoubleInt_t<int256kB>  int512kB;
typedef class DoubleInt_t<int512kB>  int1MB;
// Much beyond 1MB and this won't compile in any reasonable amount of 
// time. Plus the operations are _REALLY_ slow. 

typedef class SignedInt_t<int256>    sint256; //signed 256 bit int


// for 64-bit x86
unsigned long long rdtsc(void)
{
    unsigned int tickl, tickh;
    __asm__ __volatile__("rdtsc":"=a"(tickl),"=d"(tickh));
    return ((unsigned long long)tickh << 32)|tickl;
}

// for 32-bit x86
//#define rdtscll(val)  __asm__ __volatile__ ("rdtsc" : "=A" (val))
// for x86-64
#define rdtscll(val) val=rdtsc()


// this is here to assure that the basic assembly language constructs for the base class are working ok
void Test64BitBase(void)
{
    int64 a,b,c;
    int carry;
    int64 overflow;

    // 64 bit add...
    a=0x10;
    b=0x2;
    carry=0;
    for (int x=0;x<100;x++)
    {
        b=a;
        printf("Carry=%X, a=%llX b=%llX\n",carry,a,b);
        carry=Add64(&a,&b,carry);
    }

    // 64 bit sub...
    a=0x10000;
    b=0x2;
    carry=0;
    for (int x=0;x<20;x++)
    {
        b*=2;
        printf("Borrow=%X, a=%llX b=%llX\n",carry,a,b);
        carry=Sub64(&a,&b,carry);
    }

    // 64 bit multiply
    a=0x10;
    b=0x1234;
    overflow=0;
    for (int x=0;x<10;x++)
    {
        printf("overflow=%llX, a=%llX b=%llX\n",overflow,a,b);
        overflow=Multiply64(&a,&b);
    }


    // 64 bit divide
	// Pick these constants carefully
	// if the result cannot fit into a 64-bit 
	// register then this will sigfpe (or throw)
	a=0x10000;
    b=0x1; //b=0x10 will sigfpe
    c=0x03;
    overflow=0;
    for (int x=0;x<20;x++)
    {
        printf("remainder=%llX, a=%llX b=%llX c=%llX\n",overflow,a,b,c);
        overflow=Divide64(&a,&b,&c);
    }
}

void Test128BitTemplate(void)
{
    int64 test=1024*1024*1024;
    test*=64;
    printf("Hello there %s\n",AsString(test,"%d").c_str());
    printf("Hello there 0x%s\n",AsString(test,"%X").c_str());
    int128 t128, over;

    t128=test;
    printf("Hello there 0x%s\n",t128.AsString("%X").c_str());
    over=int128::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=int128::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());


    test=0xFFFFFFFFFFFFFFFF;
    t128=test;
    printf("Hello there 0x%s\n",t128.AsString("%X").c_str());
    over=int128::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=int128::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());


    // shift left test
    test=1;
    t128=test;
    int carryres=0;
    for (int x=0;x<129;x++)
    {
        printf("shift value=%s carry=%d\n",t128.AsString("%X").c_str(),carryres);
        carryres=int128::shiftleft(&t128,0);
    }

    t128=test;
    carryres=0;
    for (int x=0;x<129;x++)
    {
        printf("carry shift value=%s carry=%d\n",t128.AsString("%X").c_str(),carryres);
        carryres=int128::shiftleft(&t128,1);
    }

    // divide test
    test=0xFFFFFFFFFFFFFFFF;
    t128=test;
    int128 t2;
    t2=int128(16);
    int128::MultiplyDouble(&t128,int128(0xFFFFF));
    int128 remainder;
    for (int x=0;x<10;x++)
    {
        printf("divide value=%s by=%s (remainder=%s)\n",t128.AsString("%X").c_str(),t2.AsString("%X").c_str(),remainder.AsString("%X").c_str());
        remainder=int128::DivideDouble(&t128,t2);
    }

    // class shift test
    test=0x1;
    t128=test;
    for (int x=0;x<100;x++)
    {
        printf("shift value=%s\n",t128.AsString("%X").c_str());
        t128<<=1;
    }
    for (int x=0;x<100;x++)
    {
        printf("shift value=%s\n",t128.AsString("%X").c_str());
        t128>>=1;
    }

}


void Test256BitTemplate(void)
{
    int128 test=1024*1024*1024;
    DoubleInt_t<int128> t128,t2, over;

    t128=test;
    printf("Hello there 0x%s\n",t128.AsString("%X").c_str());
    over=DoubleInt_t<int128>::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=DoubleInt_t<int128>::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());


    test=0xFFFFFFFFFFFFFFFF;
    t128=test;
    printf("Hello there 0x%s\n",t128.AsString("%X").c_str());
    over=DoubleInt_t<int128>::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=DoubleInt_t<int128>::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=DoubleInt_t<int128>::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());

    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    for (int x=0;x<66;x++)
    {
        printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
        over=DoubleInt_t<int128>::MultiplyDouble(&t128,t2);
        
    }

    // shift left test
    test=1;
    t128=test;
    int carryres=0;
    for (int x=0;x<129;x++)
    {
        printf("shift value=%s carry=%d\n",t128.AsString("%X").c_str(),carryres);
        carryres=DoubleInt_t<int128>::shiftleft(&t128,0);
    }

    t128=test;
    carryres=0;
    for (int x=0;x<129;x++)
    {
        printf("carry shift value=%s carry=%d\n",t128.AsString("%X").c_str(),carryres);
        carryres=DoubleInt_t<int128>::shiftleft(&t128,1);
    }

    // divide test
    test=0xFFFFFFFFFFFFFFFF;
    t128=test;
    t2=int128(16);
    DoubleInt_t<int128>::MultiplyDouble(&t128,DoubleInt_t<int128>(0xFFFFF));
    DoubleInt_t<int128> remainder;
    for (int x=0;x<10;x++)
    {
        printf("divide value=%s by=%s (remainder=%s)\n",t128.AsString("%X").c_str(),t2.AsString("%X").c_str(),remainder.AsString("%X").c_str());
        remainder=DoubleInt_t<int128>::DivideDouble(&t128,t2);
    }

    // class shift test
    test=0x1;
    t128=test;
    for (int x=0;x<200;x++)
    {
        printf("shift value=%s\n",t128.AsString("%X").c_str());
        t128<<=1;
    }
    for (int x=0;x<200;x++)
    {
        printf("shift value=%s\n",t128.AsString("%X").c_str());
        t128>>=1;
    }


    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    int64 start,end;
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        over=DoubleInt_t<int128>::MultiplyDouble(&t128,t2);
    }
    rdtscll(end);
    printf("MultiplyDouble Took %lld cycles a loop\n",(end-start)/6);
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        t128*=t2;
    }
    rdtscll(end);
    printf("operator *= %d cycles a loop\n",(end-start)/6);
}

void Test512BitTemplate(void)
{
    int256 test=1024*1024*1024;
    int512 t128,t2, over;

    t128=test;
    printf("Hello there 0x%s\n",t128.AsString("%X").c_str());
    over=int512::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=int512::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());


    test=0xFFFFFFFFFFFFFFFF;
    t128=test;
    printf("Hello there 0x%s\n",t128.AsString("%X").c_str());
    over=int512::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=int512::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
    over=int512::MultiplyDouble(&t128,t128);
    printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());

    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    for (int x=0;x<128;x++)
    {
        printf("Hello there 0x%s over=%s\n",t128.AsString("%X").c_str(),over.AsString("%X").c_str());
        over=int512::MultiplyDouble(&t128,t2);
        
    }

    // shift left test
    test=1;
    t128=test;
    int carryres=0;
    for (int x=0;x<500;x++)
    {
        printf("shift value=%s carry=%d dec=%s\n",t128.AsString("%X").c_str(),carryres,t128.AsString("%d").c_str());
        carryres=int512::shiftleft(&t128,0);
    }

    t128=test;
    carryres=0;
    for (int x=0;x<500;x++)
    {
        printf("carry shift value=%s carry=%d\n",t128.AsString("%X").c_str(),carryres);
        carryres=int512::shiftleft(&t128,1);
    }

    // divide test
    test=0xFFFFFFFFFFFFFFFF;
    t128=test;
    t2=int512(16);
    int512::MultiplyDouble(&t128,int512(0xFFFFF));
    int512 remainder;
    for (int x=0;x<10;x++)
    {
        printf("divide value=%s by=%s (remainder=%s)\n",t128.AsString("%X").c_str(),t2.AsString("%X").c_str(),remainder.AsString("%X").c_str());
        remainder=int512::DivideDouble(&t128,t2);
    }

    // class shift test
    test=0x1;
    t128=test;
    for (int x=0;x<200;x++)
    {
        printf("shift value=%s\n",t128.AsString("%X").c_str());
        t128<<=1;
    }
    for (int x=0;x<200;x++)
    {
        printf("shift value=%s\n",t128.AsString("%X").c_str());
        t128>>=1;
    }


    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    int64 start,end;
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        over=int512::MultiplyDouble(&t128,t2);
    }
    rdtscll(end);
    printf("int512 Took %llu cycles a loop\n",(end-start)/6);
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        t128*=t2;
    }
    rdtscll(end);
    printf("int512 operator *= %llu cycles a loop\n",(end-start)/6);
}


void Test16384BitTemplate(void)
{
    int8192  test=1024*1024*1024;
    int16384 t128,t2, over;

    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    int64 start,end;
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        over=int16384::MultiplyDouble(&t128,t2);
    }
    rdtscll(end);
    printf("16k MultiplyDouble Took %llu cycles a loop\n",(end-start)/6);
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        t128*=t2;
    }
    rdtscll(end);
    printf("16k operator *= %llu cycles a loop\n",(end-start)/6);

    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        t128/=t2;
    }
    rdtscll(end);
    printf("16k operator /= %llu cycles a loop\n",(end-start)/6);   
}

void Test131072BitTemplate(void)
{
    int65536  test=1024*1024*1024;
    int131072 t128,t2, over;

    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    int64 start,end;
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        over=int131072::MultiplyDouble(&t128,t2);
    }
    rdtscll(end);
    printf("128k MultiplyDouble Took %llu cycles a loop\n",(end-start)/6);
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        t128*=t2;
    }
    rdtscll(end);
    printf("128k operator *= %llu cycles a loop\n",(end-start)/6);
    rdtscll(start);
    for (int x=0;x<6;x++)
    {
        t128/=t2;
    }
    rdtscll(end);
    printf("128k operator /= %llu cycles a loop\n",(end-start)/6);
}


void Test1MBTemplate(void)
{
    int512kB  test=1024*1024*1024;
    int1MB t128,t2, over;

    printf("This is going to take a while, if it crashes verify your stack space...\n");

    test=0xF;
    t128=test;
    test=0x10;
    t2=test;
    test=0;
    over=test;
    int64 start,end;
    rdtscll(start);
    for (int x=0;x<2;x++)
    {
        over=int1MB::MultiplyDouble(&t128,t2);
    }
    rdtscll(end);
    printf("1M MultiplyDouble Took %llu cycles a loop\n",(end-start)/2);
    rdtscll(start);
    for (int x=0;x<2;x++)
    {
        t128*=t2;
    }
    rdtscll(end);
    printf("1M operator *= %llu cycles a loop\n",(end-start)/2);
    rdtscll(start);
    for (int x=0;x<2;x++)
    {
        t128/=t2;
    }
    rdtscll(end);
    printf("1M operator /= %llu cycles a loop\n",(end-start)/2);
}

void TestSignedValue(void)
{
    sint256 x,y,z;
    int64 testvals[][2]=
    {
        {11, -10},
        {10, -10},
        {9 , -10},
        {1 , -10},
        {0,  -10},
        {-1 , -10},
        {-9, -10},
        {-10, -10},
        {-11, -10},
        {9999,9999}
    };
    const char *teststrs[]=
    {
        "0x10",
        "10",
        "-10",
        "0x0010000000000000000000000",
        "309485009821345068724781056",
        NULL
    };
    int cnt=0;
    while (testvals[cnt][0]!=9999)
    {
        printf("signed int x=%d y=%d\n",testvals[cnt][0],testvals[cnt][1]);
        x=sint256(testvals[cnt][0]);
        y=sint256(testvals[cnt][1]);

        printf("%s>=%s is %s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),x>=y?"true":"false");
        printf("%s<=%s is %s\n",y.AsString("%d").c_str(),x.AsString("%d").c_str(),y<=x?"true":"false");
        printf("%s<=%s is %s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),x<=y?"true":"false");
        printf("%s>=%s is %s\n",y.AsString("%d").c_str(),x.AsString("%d").c_str(),y>=x?"true":"false");


        printf("%s>%s is %s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),x>y?"true":"false");
        printf("%s<%s is %s\n",y.AsString("%d").c_str(),x.AsString("%d").c_str(),y<x?"true":"false");
        printf("%s<%s is %s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),x<y?"true":"false");
        printf("%s>%s is %s\n",y.AsString("%d").c_str(),x.AsString("%d").c_str(),y>x?"true":"false");
        z=x+y;
        printf("%s+%s=%s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),z.AsString("%d").c_str());
        z=x-y;
        printf("%s-%s=%s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),z.AsString("%d").c_str());
        z=x*y;
        printf("%s*%s=%s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),z.AsString("%d").c_str());
        z=x/y;
        printf("%s/%s=%s\n",x.AsString("%d").c_str(),y.AsString("%d").c_str(),z.AsString("%d").c_str());
        cnt++;
    }

    cnt=0;
    while (teststrs[cnt]!=NULL)
    {
        x.FromString(teststrs[cnt]);
        printf("%s should be equal to\n%s\n",teststrs[cnt],x.AsString("%d").c_str());
        cnt++;
    }
}


// run the basic tests..
#include <sys/resource.h>
int main(int argc,char *argv[])
{
    // for the 1MB objects the stack size needs to be adjusted..
    rlimit newlimit;
    if (getrlimit(RLIMIT_STACK,&newlimit)!=0)
    {
        perror("Unable to get stack limit");
    }
    else
    {
        printf("current stack size %d, max limit %d\n",newlimit.rlim_cur,newlimit.rlim_max);
    }
    //newlimit.rlim_max=1024L*1024L*64L; //64M stack...
    newlimit.rlim_cur=1024L*1024L*64L; //64M stack...
    if (setrlimit(RLIMIT_STACK, &newlimit)!=0)
    {
        perror("Unable to set stack limit, check ulimit -s unlimited, or comment out the exit(1) and the Test1MBTemplate");
        return 1;
    }

    Test64BitBase();
    Test128BitTemplate();
    Test256BitTemplate();
    Test512BitTemplate();
    TestSignedValue();
    Test16384BitTemplate();
    Test131072BitTemplate();
    Test1MBTemplate();
}

#endif //_UNITTEST_
