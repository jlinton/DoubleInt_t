// C++ BigNum template class
// AKA the integer doubler template.
// Copyright(C) 2007,2015 Jeremy Linton
//
// This started as an amusing exercise in template meta-programming.. 
// Or in this case nestable classes. AKA std::list<std::list<>>
// But the optimizer can see into the nesting, so the generated assembly 
// actually looks really sort of native. Especially since the base class
// is just gcc inline assembly...
//
// So, what we have is actually a pretty good class for "small" bignums
// AKA ones less than say 1k bits. Larger than that and the more intelligent
// algorithms used in GNU MP result in faster code.
//
// This is also an _AWESOME_ test of your compiler. Have you ever seen a
// single module take hours to compile? Well, that is possible with this one
// if the integer types are large enough!!!!! It can also create some crazy big
// functions...
//
// One of the nicer things about this code is the fact that I've created 
// an x86 int128 class that can provide information about overflow/carry
// Its this class which is used as the basis for the integer doubler class.
//
// The amusing code is where we have:
// typedef class DoubleInt_t<int128>    int256;
// typedef class DoubleInt_t<int256>    int512;
// typedef class DoubleInt_t<int512>    int1024;
// typedef class DoubleInt_t<int1024>   int2048;
// 
// While these are called int256/etc they are actually unsigned 
// (because at the time, I thought that the default int class in C++ should be
// unsigned with a "signed" keyword to provide for signed math...)
//
// There is a signed template too called SignedInt_t<> which adds a sign
// bit to the given unsigned class, and transforms all the basic operations
// so that they work correctly depending on the state of the sign bit.
//
// combined with some inline assembly to create a integer class
// that could export information about whether an operation had overflow/carry
//
// these classes create an arbitrary size integer similar to GMP
//
// This module is both the template classes as well as some small "unit tests"
// which demonstrate how it can be used. Of particular note is the AsString() 
// and FromString() routines which provide human readable input/output from
// the DoubleInt_t
// 
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

#include <stdio.h>
#include <string>
#include <sys/time.h>
using std::string;

typedef long long int64;
typedef int       int32;

// This class is the base class for the Doubler, it provides the
// helper routines like         
//    int SubDouble(int128_t *A,const int128_t &B,const int borrow);       
//    int128_t DivideDouble(int128_t *A,const int128_t &B);
//     int shiftleft(int128_t *Value,const int Carry_prm);
//
// Which are used by DoubleInt_t to double the size of the passed class.
// By itself this class is fairly useful if you happen to need something 
// slightly larger than a 64-bit int (large(er) bitfields come to mind). 
//
typedef class int128_t
{
    public:
        // construction/casting
        int128_t()                    :Hi(0),Lo(0) {}
        int128_t(const int128_t &orig):Hi(orig.Hi),Lo(orig.Lo) {}
        int128_t(const int64    &orig):Hi(0),Lo(orig) {}
        // assignment
        int128_t &operator= (const int128_t &rhs) {Hi=rhs.Hi;Lo=rhs.Lo;return *this;}
//      int128_t &operator= (const int64    &rhs) {Hi=0;Lo=rhs;return *this;}
//      int128_t &operator= (const int32    &rhs) {Hi=0;Lo=rhs;return *this;}
        // compariston
        bool     operator==(const int128_t &rhs) { if ((Hi==rhs.Hi) && (Lo==rhs.Lo)) return true; return false;}
        bool     operator!=(const int128_t &rhs) { if ((Hi==rhs.Hi) && (Lo==rhs.Lo)) return false; return true;}
        bool     operator>=(const int128_t &rhs) { if ((Hi>rhs.Hi) || ( (Hi==rhs.Hi) && (Lo>=rhs.Lo))) return true; return false;}
        bool     operator<=(const int128_t &rhs) { if ((Hi<rhs.Hi) || ( (Hi==rhs.Hi) && (Lo<=rhs.Lo))) return true; return false;}
        bool     operator> (const int128_t &rhs) { if ((Hi>rhs.Hi) || ( (Hi==rhs.Hi) && (Lo>rhs.Lo ))) return true; return false;}
        bool     operator< (const int128_t &rhs) { if ((Hi<rhs.Hi) || ( (Hi==rhs.Hi) && (Lo<rhs.Lo ))) return true; return false;}
        // operations (these are exported for user use)
        int128_t &operator>>=(const int      rhs)  { for (int x=0;x<rhs;x++) shiftright(this,0); return *this;}
        int128_t &operator<<=(const int      rhs)  { for (int x=0;x<rhs;x++) shiftleft(this,0); return *this;}
        int128_t &operator-=( const int128_t &rhs) { SubDouble(this,rhs,0); return *this;}
        int128_t &operator+=( const int128_t &rhs) { AddDouble(this,rhs,0); return *this;}
        int128_t &operator*=( const int128_t &rhs) { MultiplyDouble(this,rhs); return *this;}
        int128_t &operator/=( const int128_t &rhs) { DivideDouble(this,rhs); return *this;}

        int128_t &operator&=( const int64 &rhs) { this->Lo&=rhs; return *this;}
        int128_t &operator|=( const int64 &rhs) { this->Lo|=rhs; return *this;}
        int128_t &operator^=( const int64 &rhs) { this->Lo^=rhs; return *this;}

        int128_t &operator&=( const int128_t &rhs)  { this->Lo&=rhs.Lo; this->Hi&=rhs.Hi; return *this;}
        int128_t &operator|=( const int128_t &rhs)  { this->Lo|=rhs.Lo; this->Hi|=rhs.Hi; return *this;}
        int128_t &operator^=( const int128_t &rhs)  { this->Lo^=rhs.Lo; this->Hi^=rhs.Hi; return *this;}

//          int128_t &operator~=( const int64 &rhs) { this->Lo~=rhs; return *this;}

        int128_t operator+(   const int128_t &rhs) { int128_t tmp=*this; AddDouble(&tmp,rhs,0); return tmp;}
        int128_t operator-(   const int128_t &rhs) { int128_t tmp=*this; SubDouble(&tmp,rhs,0); return tmp;}
        int128_t operator/(   const int128_t &rhs) { int128_t tmp=*this; DivideDouble(&tmp,rhs); return tmp;}
        int128_t operator*(   const int128_t &rhs) { int128_t tmp=*this; MultiplyDouble(&tmp,rhs); return tmp;}

        int128_t operator&(   const int64    &rhs) { int128_t tmp=*this; tmp&=rhs; return tmp;}
        int128_t operator|(   const int64    &rhs) { int128_t tmp=*this; tmp|=rhs; return tmp;}
        int128_t operator^(   const int64    &rhs) { int128_t tmp=*this; tmp^=rhs; return tmp;}

        int128_t operator>>(  const int      &rhs) { int128_t tmp=*this; tmp>>=rhs; return tmp;}
        int128_t operator<<(  const int      &rhs) { int128_t tmp=*this; tmp<<=rhs; return tmp;}

        // the following pretty much the same as above, in both cases we are in temp hell
/*
        friend int128_t operator+(const int128_t &lhs,const int128_t &rhs) { int128_t tmp=lhs; Add128(&tmp,&rhs,0); return tmp;}
        friend int128_t operator-(const int128_t &lhs,const int128_t &rhs) { int128_t tmp=lhs; Sub128(&tmp,&rhs,0); return tmp;}
        friend int128_t operator/(const int128_t &lhs,const int128_t &rhs) { int128_t tmp=lhs; Divide128(&tmp,&rhs); return tmp;}
        friend int128_t operator*(const int128_t &lhs,const int128_t &rhs) { int128_t tmp=lhs; Multiply128(&tmp,&rhs); return tmp;}
*/

        // input/output routines
        string AsString(const char *format);
        char GetLowByte() {return Lo&0xFF;}
//  protected:
        // these operations are exported for higher level use
        // they don't use the this variable...
        static int SubDouble(int128_t *A,const int128_t &B,const int borrow);       
        static int AddDouble(int128_t *A,const int128_t &B,const int carry);
        static int128_t DivideDouble(int128_t *A,const int128_t &B);
        static int128_t MultiplyDouble(int128_t *A,const int128_t &B);
        static int shiftleft(int128_t *Value,const int Carry_prm);
        static int shiftright(int128_t *Value,const int Carry_prm);
//  private:
        int64 Hi;
        int64 Lo;
        static const int size; //clean up the memory allocation slightly by moving this out of band..
} int128;
const int int128::size=128;


// This is the core doubler template. It takes either itself or the int128_t
// Class and creates a class which has exactly 2x the number of bits. This allows us 
// To create somewhat arbitrary sized integers, although its not really useful beyond 
// maybe 2k bits. If you need more, consider something like GNU MP.
template<class BaseIntT> class DoubleInt_t
{
    public:
        // construction/casting
        DoubleInt_t()                    :Hi(0),Lo(0),size(Hi.size*2) {}
        DoubleInt_t(const DoubleInt_t &orig):Hi(orig.Hi),Lo(orig.Lo),size(Hi.size*2) {}
        DoubleInt_t(const BaseIntT    &orig):Hi(0),Lo(orig),size(Hi.size*2) {}
        DoubleInt_t(const int64       &orig):Hi(0),Lo(orig),size(Hi.size*2) {}
        // assignment
        DoubleInt_t &operator= (const DoubleInt_t &rhs) {Hi=rhs.Hi;Lo=rhs.Lo;return *this;}
//      DoubleInt_t &operator= (const int64    &rhs) {Hi=0;Lo=rhs;return *this;}
//      DoubleInt_t &operator= (const int32    &rhs) {Hi=0;Lo=rhs;return *this;}
        // compariston
        bool     operator==(const DoubleInt_t &rhs) { if ((Hi==rhs.Hi) && (Lo==rhs.Lo)) return true; return false;}
        bool     operator!=(const DoubleInt_t &rhs) { if ((Hi==rhs.Hi) && (Lo==rhs.Lo)) return false; return true;}
        bool     operator>=(const DoubleInt_t &rhs) { if ((Hi>rhs.Hi) || ( (Hi==rhs.Hi) && (Lo>=rhs.Lo))) return true; return false;}
        bool     operator<=(const DoubleInt_t &rhs) { if ((Hi<rhs.Hi) || ( (Hi==rhs.Hi) && (Lo<=rhs.Lo))) return true; return false;}
        bool     operator> (const DoubleInt_t &rhs) { if ((Hi>rhs.Hi) || ( (Hi==rhs.Hi) && (Lo>rhs.Lo ))) return true; return false;}
        bool     operator< (const DoubleInt_t &rhs) { if ((Hi<rhs.Hi) || ( (Hi==rhs.Hi) && (Lo<rhs.Lo ))) return true; return false;}
        // operations (these are exported for user use)
        DoubleInt_t &operator>>=(const int      rhs)  { for (int x=0;x<rhs;x++) shiftright(this,0); return *this;}
        DoubleInt_t &operator<<=(const int      rhs)  { for (int x=0;x<rhs;x++) shiftleft(this,0); return *this;}
        DoubleInt_t &operator-=( const DoubleInt_t &rhs) { SubDouble(this,rhs,0); return *this;}
        DoubleInt_t &operator+=( const DoubleInt_t &rhs) { AddDouble(this,rhs,0); return *this;}
        DoubleInt_t &operator*=( const DoubleInt_t &rhs) { MultiplyDouble(this,rhs); return *this;}
        DoubleInt_t &operator/=( const DoubleInt_t &rhs) { DivideDouble(this,rhs); return *this;}


        DoubleInt_t &operator&=( const int64 &rhs) { this->Lo&=rhs; return *this;}
        DoubleInt_t &operator|=( const int64 &rhs) { this->Lo|=rhs; return *this;}
        DoubleInt_t &operator^=( const int64 &rhs) { this->Lo^=rhs; return *this;}

        DoubleInt_t &operator&=( const DoubleInt_t &rhs) { this->Lo&=rhs.Lo; this->Hi&=rhs.Hi; return *this;}
        DoubleInt_t &operator|=( const DoubleInt_t &rhs) { this->Lo|=rhs.Lo; this->Hi|=rhs.Hi; return *this;}
        DoubleInt_t &operator^=( const DoubleInt_t &rhs) { this->Lo^=rhs.Lo; this->Hi^=rhs.Hi; return *this;}


        DoubleInt_t operator+(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; AddDouble(&tmp,rhs,0); return tmp;}
        DoubleInt_t operator-(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; SubDouble(&tmp,rhs,0); return tmp;}
        DoubleInt_t operator/(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; DivideDouble(&tmp,rhs); return tmp;}
        DoubleInt_t operator*(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; MultiplyDouble(&tmp,rhs); return tmp;}

        DoubleInt_t operator&(   const int64    &rhs) { DoubleInt_t tmp=*this; tmp.Lo&=rhs; return tmp;}
        DoubleInt_t operator|(   const int64    &rhs) { DoubleInt_t tmp=*this; tmp.Lo|=rhs; return tmp;}
        DoubleInt_t operator^(   const int64    &rhs) { DoubleInt_t tmp=*this; tmp.Lo^=rhs; return tmp;}


        DoubleInt_t operator>>(  const int      &rhs) { DoubleInt_t tmp=*this; tmp>>=rhs; return tmp;}
        DoubleInt_t operator<<(  const int      &rhs) { DoubleInt_t tmp=*this; tmp<<=rhs; return tmp;}

        // the following pretty much the same as above, in both cases we are in temp hell
/*
        friend DoubleInt_t operator+(const DoubleInt_t &lhs,const DoubleInt_t &rhs) { DoubleInt_t tmp=lhs; Add128(&tmp,&rhs,0); return tmp;}
        friend DoubleInt_t operator-(const DoubleInt_t &lhs,const DoubleInt_t &rhs) { DoubleInt_t tmp=lhs; Sub128(&tmp,&rhs,0); return tmp;}
        friend DoubleInt_t operator/(const DoubleInt_t &lhs,const DoubleInt_t &rhs) { DoubleInt_t tmp=lhs; Divide128(&tmp,&rhs); return tmp;}
        friend DoubleInt_t operator*(const DoubleInt_t &lhs,const DoubleInt_t &rhs) { DoubleInt_t tmp=lhs; Multiply128(&tmp,&rhs); return tmp;}
*/


        // consider overridding printf until then use AsString
        string AsString(const char *format);
        void   FromString(const char *Source_prm);
        char   GetLowByte() {return Lo.GetLowByte();}
//  protected:
        // these operations are exported for higher level use
        // they don't use the this variable...
        static int SubDouble(DoubleInt_t *A,const DoubleInt_t &B,const int borrow);     
        static int AddDouble(DoubleInt_t *A,const DoubleInt_t &B,const int carry);
        static DoubleInt_t DivideDouble(DoubleInt_t *A,const DoubleInt_t &B);
        static DoubleInt_t MultiplyDouble(DoubleInt_t *A,const DoubleInt_t &B);
        static int shiftleft(DoubleInt_t *Value,const int Carry_prm);
        static int shiftright(DoubleInt_t *Value,const int Carry_prm);
//  private:
        BaseIntT Hi;
        BaseIntT Lo;
        const int size;
};

// throw exceptions on overflow/underflow etc..
template<class BaseInnT> class OverflowException_t
{

};

template<class BaseIntT, class ExponentT> class Floating_t
{
};

// normal number systems don't support -0 so we don't either...
// this class just takes an unsigned base and adds a sign to it, 
// it does this without changing the bit encoding of the value, so its
// not exactly efficient for certain operations, on the other hand we don't then have 
// to worry about sign extension or anything like that...
template<class BaseIntT> class SignedInt_t
{
    public:
        // construction/casting
        SignedInt_t()                    :Value(0),Negative(0) {}
        SignedInt_t(const SignedInt_t &orig):Value(orig.Value),Negative(orig.Negative) {}
        SignedInt_t(const BaseIntT    &orig):Value(orig),Negative(0) {}
        SignedInt_t(const int64       &orig):Value(orig),Negative(0) { if (orig<0) { Negative=1; Value^=BaseIntT(-1); Value+=BaseIntT(1);}} //ugly!
        // assignment
        SignedInt_t &operator= (const SignedInt_t &rhs) {Value=rhs.Value; Negative=rhs.Negative; return *this;}
        // compariston (some of thse operators could use a little teaking for efficiency)
        bool     operator==(const SignedInt_t &rhs) { if ((Value==rhs.Value) && (Negative==rhs.Negative)) return true; return false;}
        bool     operator!=(const SignedInt_t &rhs) { if ((Value==rhs.Value) && (Negative==rhs.Negative)) return false; return true;}
        bool     operator>=(const SignedInt_t &rhs) { if ((Negative==1) && (rhs.Negative==1))   {   if (Value<=rhs.Value)   {   return true;    }   return false;   }
                                                      else if ((Negative==0) && (rhs.Negative==0))  {   if (Value>=rhs.Value)   {   return true;    }   return false; }
                                                      else if (Negative==1) {   return false;   }   else    {   return true;    }   return false; }
        bool     operator<=(const SignedInt_t &rhs) { if (*this==rhs) return true; else if (*this>=rhs) return false; else return true;}
        bool     operator> (const SignedInt_t &rhs) { if (*this==rhs) return false; else return (*this>=rhs);}
        bool     operator< (const SignedInt_t &rhs) { if (*this==rhs) return false; else return !(*this>=rhs);}
        // operations (these are exported for user use), many of these operations are "wierd" because they don't affect the sign aka shift and bit instructions maintain signage
        SignedInt_t &operator>>=(const int      rhs)  { Value>>=rhs; return *this;} 
        SignedInt_t &operator<<=(const int      rhs)  { Value<<=rhs; return *this;}
        SignedInt_t &operator-=( const SignedInt_t &rhs) { SubDouble(this,rhs,0);  return *this;}
        SignedInt_t &operator+=( const SignedInt_t &rhs) { AddDouble(this,rhs,0); return *this;}
        SignedInt_t &operator*=( const SignedInt_t &rhs) { MultiplyDouble(this,rhs); return *this;}
        SignedInt_t &operator/=( const SignedInt_t &rhs) { DivideDouble(this,rhs); return *this;}


        SignedInt_t &operator&=( const int64 &rhs) { Value&=rhs; return *this;}
        SignedInt_t &operator|=( const int64 &rhs) { Value|=rhs; return *this;}
        SignedInt_t &operator^=( const int64 &rhs) { Value^=rhs; return *this;}

        SignedInt_t &operator&=( const SignedInt_t &rhs) { Value&=rhs.Value; return *this;}
        SignedInt_t &operator|=( const SignedInt_t &rhs) { Value|=rhs.Value; return *this;}
        SignedInt_t &operator^=( const SignedInt_t &rhs) { Value^=rhs.Value; return *this;}


        SignedInt_t operator+(   const SignedInt_t &rhs) { SignedInt_t tmp=*this; AddDouble(&tmp,rhs,0); return tmp;}
        SignedInt_t operator-(   const SignedInt_t &rhs) { SignedInt_t tmp=*this; SubDouble(&tmp,rhs,0); return tmp;}
        SignedInt_t operator/(   const SignedInt_t &rhs) { SignedInt_t tmp=*this; DivideDouble(&tmp,rhs); return tmp;}
        SignedInt_t operator*(   const SignedInt_t &rhs) { SignedInt_t tmp=*this; MultiplyDouble(&tmp,rhs); return tmp;}

        SignedInt_t operator&(   const int64    &rhs) { SignedInt_t tmp=*this; tmp.Value&=rhs; return tmp;}
        SignedInt_t operator|(   const int64    &rhs) { SignedInt_t tmp=*this; tmp.Value|=rhs; return tmp;}
        SignedInt_t operator^(   const int64    &rhs) { SignedInt_t tmp=*this; tmp.Value^=rhs; return tmp;}


        SignedInt_t operator>>(  const int      &rhs) { SignedInt_t tmp=*this; tmp>>=rhs; return tmp;}
        SignedInt_t operator<<(  const int      &rhs) { SignedInt_t tmp=*this; tmp<<=rhs; return tmp;}

        // the following pretty much the same as above, in both cases we are in temp hell


        // consider overridding printf until then use AsString
        //string AsString(const SignedInt_t &Value,char *format);
        string AsString(const char *format) { string ret=Value.AsString(format); if (Negative) ret.insert(0,"-"); return ret;}
        void   FromString(const char *Source_prm);
        char   GetLowByte() {return Value.GetLowByte();}
//  protected:
        // these operations are exported for higher level use
        // they don't use the this variable...
        static int SubDouble(SignedInt_t *A,const SignedInt_t &B,const int borrow);     
        static int AddDouble(SignedInt_t *A,const SignedInt_t &B,const int carry);
        static SignedInt_t DivideDouble(SignedInt_t *A,const SignedInt_t &B);
        static SignedInt_t MultiplyDouble(SignedInt_t *A,const SignedInt_t &B);
        static int shiftleft(SignedInt_t *Value_prm,const int Carry_prm) { return shiftleft(Value_prm->Value,Carry_prm);}
        static int shiftright(SignedInt_t *Value_prm,const int Carry_prm) { return shiftright(Value_prm->Value,Carry_prm);}
        
//  private:
        BaseIntT Value;
        int Negative;
};

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



/*bool operator==(const int128 &a,const int128 &b)
{
    if ((a.Hi==b.Hi) && (a.Lo==b.Lo)) return true;
    return false;
}*/



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

//
//
// Assembly implementations of some functions used
// by the int128 class.. These are just lightweight 
// inline assembly wrappers for x86 instructions
// 
//


static inline int64 Multiply64(int64 *A, int64 *B)
{
    int64 ret; //unessisary the ret should be in RAX
    asm ("mul %3    \n\t"
         :"=a" (*A), "=d" (ret)
         : "a" (*A), "r" (*B)
         : 
     );
    return ret;
}

// take a long word B:A and divide by C, result in A and remainder is returned
static inline int64 Divide64(int64 *A, int64 *B,int64 *C)
{
    int64 ret; 
    if (*C==0)
    {
        throw "Divide by zero";
    }
    if (*C>*B)
    {
        throw "Underflow";
    }
    asm ("div %4    \n\t"
         :"=a" (*A), "=d" (ret)
         : "a" (*A), "d" (*B), "r" (*C)
         : 
     );
    return ret;
}

/*
//returns carry and sum in A
static inline int Add64(int64 *A,const int64 *B,const int carry)
{
    int ret_carry=0;

    asm ("clc           \n\t"
         "cmp  $0,%3    \n\t"
         "je  0f        \n\t"
         "stc           \n\t"
         "0: adc %4, %0 \n\t"
         "jnc 0f        \n\t"
         "mov $1, %1    \n\t"
         "0:"
         :"=r" (*A), "=r" (ret_carry)
         :"0" (*A), "r" (carry), "r" (*B), "1" (ret_carry)
         : "cc"
    );
    return ret_carry;
}*/


//returns carry and sum in A
static inline int Add64(int64 *A,const int64 *B,const int64 carry)
{
    char ret_carry =0;
    asm (
         "add %3, %0  \n\t"
         "adc %4, %0  \n\t"
         "adc %1, %1  \n\t"
//       "setc %1     \n\t" //appears slower than the add
//       "cmovcq %3, %1   \n\t"
         :"=r" (*A), "=r" (ret_carry)
         :"0" (*A), "r" (carry), "r" (*B), "1" (ret_carry)
         : "cc"
    );
    return (int)ret_carry;
}


// returns if it borroed one...
static inline int Sub64(int64 *A, const int64 *B,const int borrow)
{
    int ret_borrow=0;
    asm (
        "clc                \n\t"
         "cmp $0,%[bor]      \n\t"
         "je  0f             \n\t"
         "stc                \n\t"
         "0:                 \n\t"
         "sbb %[src], %[dst] \n\t"
         "adc %[ret_bor], %[ret_bor]\n\t"
         : [dst] "=r" (*A), [ret_bor] "=r" (ret_borrow)
         :        "0" (*A),           "1"  (ret_borrow), [src] "r" (*B), [bor] "r" (borrow)
         : "cc"
        );
    return ret_borrow;
}

//
//
//          The int128_t methods
//
//
//

// A-=B; returns borrow
inline int int128_t::SubDouble(int128_t *A,const int128_t &B,const int borrow)
{
    int ret_borrow;
    ret_borrow=Sub64(&A->Lo,&B.Lo,borrow);
    ret_borrow=Sub64(&A->Hi,&B.Hi,ret_borrow);
    return ret_borrow;
}

template<class BaseIntT> int DoubleInt_t<BaseIntT>::SubDouble(DoubleInt_t *A,const DoubleInt_t &B,const int borrow)
{
    int ret_borrow;
    ret_borrow=BaseIntT::SubDouble(&A->Lo,B.Lo,borrow);
    ret_borrow=BaseIntT::SubDouble(&A->Hi,B.Hi,ret_borrow);
    return ret_borrow;
}


inline int int128_t::AddDouble(int128_t *A,const int128_t &B,const int carry)
{
    int carry_ret=0;
    carry_ret=Add64(&A->Lo,&B.Lo,carry);
    carry_ret=Add64(&A->Hi,&B.Hi,carry_ret);
    return carry_ret;
}



//
//
//          The DoubleInt_t methods
//
//
//



template<class BaseIntT> int DoubleInt_t<BaseIntT>::AddDouble(DoubleInt_t *A,const DoubleInt_t &B,const int carry)
{
    int carry_ret=0;
    carry_ret=BaseIntT::AddDouble(&A->Lo,B.Lo,carry);
    carry_ret=BaseIntT::AddDouble(&A->Hi,B.Hi,carry_ret);
    return carry_ret;
}


// Ha, this is a 256bit multiply, it takes two 128 bit sources and creates a 128bit destination and 128bit overflow..
// this general code path can be abstracted into a template to generate any arbitraty length multiply as long as 
// we have a 64bit base class... For template testing we could create a 32-bit base class and compare the results 
// with a 64-bit result
inline int128_t int128_t::MultiplyDouble(int128_t *A,const int128_t &B)
{
    int128 ret;
    int64  tmp=0;
    int64  col3=0;

    // a 128 bit multiply works like when you were in grade school except that instead of the max value per column being
    // a 9 the max value is 2^64. 

    //   ab
    //*  cd
    //------
    //   bd
    //  ad
    //  bc
    //+ac
    //-------
    // wxyz
    int64 a=A->Hi;
    int64 b=A->Lo;
    int64 c=B.Hi;
    int64 d=B.Lo;
    int64 w=0;
    int64 x=0;
    int64 y=0;
    int64 z=0;
    int   carry=0;
    int   carry2=0;
    int64 xp=0;
    
    y=Multiply64(&b,&d);
    z=b; b=A->Lo;
    x=Multiply64(&a,&d);
    carry=Add64(&y,&a,0);// y+=a; 
    a=A->Hi;
    xp=Multiply64(&b,&c); 
    carry=Add64(&x,&xp,carry); //x+=xp+carry;
    carry2=Add64(&y,&b,0);   //y+=b; 
    w=Multiply64(&a,&c); 
    carry2=Add64(&x,&a,carry2);//x+=a; 
    // final w fixup
    int64 longcarry=carry2;
    Add64(&w,&longcarry,carry2);


    A->Lo=z;
    A->Hi=y;
    ret.Lo=x;
    ret.Hi=w;

    return ret;
}


template<class BaseIntT> DoubleInt_t<BaseIntT> DoubleInt_t<BaseIntT>::MultiplyDouble(DoubleInt_t *A,const DoubleInt_t &B)
{
    DoubleInt_t ret;
    BaseIntT  tmp=0;
    BaseIntT  col3=0;

    // a 128 bit multiply works like when you were in grade school except that instead of the max value per column being
    // a 9 the max value is 2^64. 

    //   ab
    //*  cd
    //------
    //   bd
    //  ad
    //  bc
    //+ac 
    //-------
    // wxyz
    BaseIntT a=A->Hi;
    BaseIntT b=A->Lo;
    BaseIntT c=B.Hi;
    BaseIntT d=B.Lo;
    BaseIntT w=0;
    BaseIntT x=0;
    BaseIntT y=0;
    BaseIntT z=0;
    int   carry=0;
    int   carry2=0;
    BaseIntT xp=0;
    
    y=BaseIntT::MultiplyDouble(&b,d);
    z=b; b=A->Lo;
    x=BaseIntT::MultiplyDouble(&a,d);
    carry=BaseIntT::AddDouble(&y,a,0);//    y+=a; 
    a=A->Hi;
    xp=BaseIntT::MultiplyDouble(&b,c); 
    carry=BaseIntT::AddDouble(&x,xp,carry); //x+=xp+carry;
    carry2=BaseIntT::AddDouble(&y,b,0);   //y+=b; 
    w=BaseIntT::MultiplyDouble(&a,c); 
    carry2=BaseIntT::AddDouble(&x,a,carry2);//x+=a; 
    // final w fixup
    BaseIntT longcarry=carry2;
    BaseIntT::AddDouble(&w,longcarry,carry2);


    A->Lo=z;
    A->Hi=y;
    ret.Lo=x;
    ret.Hi=w;

    return ret;
}


// Sort of satanic because of all the carry nonsense being propagated in and out of the flags
// for an arbitrary length integer the fastest way to do this is to put all the rcr's in a loop
// or unroll them instead of putting all the carry flag checking everywhere
int int128_t::shiftright(int128 *Value,const int Carry_prm)
{
    int Carry_ret=0;
    if (Carry_prm)
    {
        asm ("stc\n\t"
             "rcr $1,%0 \n\t"
             "rcr $1,%1 \n\t" 
             "adc %2,%2 \n\t"
             : "=r" (Value->Hi), "=r" (Value->Lo), "=r" (Carry_ret)    //output
             : "0" (Value->Hi), "1" (Value->Lo), "2" (Carry_ret)     //input
             : "cc"                                  //clobber
            );      
    }
    else
    {
        asm ("clc\n\t"
             "rcr $1,%0 \n\t"
             "rcr $1,%1 \n\t"
             "adc %2,%2 \n\t"
             : "=r" (Value->Hi), "=r" (Value->Lo), "=r" (Carry_ret)   //output
             : "0" (Value->Hi), "1" (Value->Lo), "2" (Carry_ret)     //input
             : "cc"                                  //clobber
            );

    }
    return Carry_ret;
}

template<class BaseIntT> int DoubleInt_t<BaseIntT>::shiftright(DoubleInt_t *Value,const int Carry_prm)
{
    int carry_ret;
    carry_ret=BaseIntT::shiftright(&Value->Hi,Carry_prm);
    carry_ret=BaseIntT::shiftright(&Value->Lo,carry_ret);
    return carry_ret;
}


// This is the left shift version of the above right shift 
int int128_t::shiftleft(int128 *Value,const int Carry_prm)
{
    char Carry_ret=0;
    if (Carry_prm)
    {
        asm ("stc\n\t"
             "rcl $1,%1 \n\t" 
             "rcl $1,%0 \n\t"
//           "setc %2     \n\t" //appears slower than the add
             "adc %2,%2 \n\t"
             : "=r" (Value->Hi), "=r" (Value->Lo), "=r" (Carry_ret)    //output
             : "0" (Value->Hi), "1" (Value->Lo), "2" (Carry_ret)     //input
             : "cc"                                  //clobber
            );      
    }
    else
    {
        asm ("clc\n\t"
             "rcl $1,%1 \n\t"
             "rcl $1,%0 \n\t"
//           "setc %2     \n\t" //appears slower than the add
             "adc %2,%2 \n\t"
             : "=r" (Value->Hi), "=r" (Value->Lo), "=r" (Carry_ret)   //output
             : "0" (Value->Hi), "1" (Value->Lo), "2" (Carry_ret)     //input
             : "cc"                                  //clobber
            );

    }
    return Carry_ret;
}

template<class BaseIntT> int DoubleInt_t<BaseIntT>::shiftleft(DoubleInt_t *Value,const int Carry_prm)
{
    int carry_ret;
    carry_ret=BaseIntT::shiftleft(&Value->Lo,Carry_prm);
    carry_ret=BaseIntT::shiftleft(&Value->Hi,carry_ret);
    return carry_ret;
}

// from what I understand there are really only a couple of algorithms
// useful for really long integer divides. My original plan was to 
// do some form of broken up radix 2^64 divide, but that doesn't actually
// work without making sure the dividend is less than 2^64, if thats the case
// we could make a really fast arbitrarily long dividend division. I'm afraid I don't
// want to make that restriction so, I'm back to the old school, shift and subtract method.
// A=A/B Ret=Remainder
int128_t int128_t::DivideDouble(int128_t *A,const int128_t &B)
{
    int128 quotient=*A;
    int128 remainder;//==0

    if ((int128_t)(B)==0) //TODO: fix this const mess
    {
        throw "division by zero";
    }
    
    for (int x=0;x<A->size;x++)
    {
        int hibit=shiftleft(&quotient,0);
        shiftleft(&remainder,hibit);
        if (remainder>=B)
        {
            SubDouble(&remainder,B,0); //remainder-=*B;
            quotient.Lo|=1; //quotient|=1;
        }
    }

    *A=quotient;
    return remainder;
}

template<class BaseIntT> DoubleInt_t<BaseIntT> DoubleInt_t<BaseIntT>::DivideDouble(DoubleInt_t *A,const DoubleInt_t &B)
{
    DoubleInt_t quotient=*A;
    DoubleInt_t remainder;//==0

    if ((DoubleInt_t)(B)==0) //TODO: fix this const mess
    {
        throw "division by zero";
    }
    
    for (int x=0;x<A->size;x++)
    {
        int hibit=shiftleft(&quotient,0);
        shiftleft(&remainder,hibit);
        if (remainder>=B)
        {
            SubDouble(&remainder,B,0); //remainder-=*B;
            quotient.Lo|=1; //quotient|=1;
        }
    }

    *A=quotient;
    return remainder;
}




// this takes the 64-bit value and prints it out given the formatter
// this is about as far from efficient as can be
string AsString(const int64 &Value,char *format)
{
    char temp[256];
    string ret;
    switch (format[1])
    {
        case 'd':
            sprintf(temp,"%lld",Value);
            ret=temp;
            break;
        case 'b':
            sprintf(temp,"%llb",Value);
            ret=temp;
            break;
        case 'X':
            sprintf(temp,"%llX",Value);
            ret=temp;
            break;
        case 'x':
            sprintf(temp,"%llx",Value);
            ret=temp;
            break;
    }
    return ret;
}


//string int128_t::AsString(const int128_t &Value,char *format)
string int128_t::AsString(const char *format)
{
    char temp[256];
    string ret;
    switch (format[1])
    {
        case 'd':
            break;
        case 'b':
            break;
        case 'X':
        case 'x':
        {
            int128_t tmp;
            tmp=*this;
            temp[(tmp.size>>2)]=0;
            for (int x=(tmp.size>>2)-1;x>=0;x--)
            {
                temp[x]=(tmp.Lo)&0xF;
                if (temp[x]>0x9) 
                {
                    temp[x]+='A'-10;
                }
                else
                {
                    temp[x]+='0';
                }
                shiftright(&tmp,0);
                shiftright(&tmp,0);
                shiftright(&tmp,0);
                shiftright(&tmp,0);
            }
        }
        break;
    }
    ret=temp;
    return ret;
}


template<class BaseIntT> string DoubleInt_t<BaseIntT>::AsString(const char *format)
{
    string ret;
    switch (format[1])
    {
        case 'd':
        {
            char temp[2];
            DoubleInt_t tmp;
            int x=0;
            tmp=*this;
            if (tmp==0)
            {
                ret="0";
            }
            else
            {
                ret="";
                temp[1]='\0';
                
                while (tmp!=0)
                {
                    DoubleInt_t remainder=DivideDouble(&tmp,DoubleInt_t(10)); //TODO: convert to something a little faster...
                    temp[0]=remainder.GetLowByte()+'0';
                    ret.insert(0,string(temp));
                }
            }
        }
        break;
        case 'b':
        {
            DoubleInt_t tmp;
            int x=0;
            tmp=*this;
            char temp[tmp.size+2];
    
            for (int x=0;x<tmp.size;x++)
            {
                temp[x]=tmp.GetLowByte()&0x01+'0';
                shiftright(&tmp,0);
            }
            temp[tmp.size]='\0';
            ret=temp;
        }
        break;
        case 'X':
        case 'x':
        {
            DoubleInt_t tmp;
            tmp=*this;
            char *temp=new char[(tmp.size>>2)+2];
            temp[(tmp.size>>2)]=0;
            for (int x=(tmp.size>>2)-1;x>=0;x--)
            {
                temp[x]=(tmp.Lo).GetLowByte()&0xF;
                if (temp[x]>0x9) 
                {
                    temp[x]+='A'-10;
                }
                else
                {
                    temp[x]+='0';
                }
                shiftright(&tmp,0);
                shiftright(&tmp,0);
                shiftright(&tmp,0);
                shiftright(&tmp,0);
            }
            ret=temp;
            delete temp;
        }
        break;
    }
    return ret;
}

// takes the value as a base 10 or base 16 string and converts it to the big integer type
template<class BaseIntT> void DoubleInt_t<BaseIntT>::FromString(const char *Source_prm)
{
    int start=0;
    int base=10;
    Hi=Lo=0;
    while (Source_prm[start]!='\0')
    {
        if (Source_prm[start]=='0')
        {
            if (Source_prm[start+1]=='x')
            {
                base=16;
                start+=2;
            }
            break;
        }
        if ((Source_prm[start]>='0') && (Source_prm[start]<='9'))
        {
            break;
        }
        start++;
    }
    // ok we found the beginning of the string..
    if (base==10)
    {
        DoubleInt_t multconst(10);
        while ((Source_prm[start]<='9') && (Source_prm[start]>='0'))
        {
            char tmp=Source_prm[start]-'0';
            MultiplyDouble(this,multconst);
            AddDouble(this,DoubleInt_t(tmp),0);
            start++;
        }
    }
    else
    {
        char upper=toupper(Source_prm[start]);
        while (((upper<='9') && (upper>='0')) || ((upper>='A') && (upper<='F')))
        {
            char tmp;
            if (upper<='9')
            {
                tmp=upper-'0';
            }
            else
            {
                tmp=upper-'A';
            }
            shiftleft(this,0);
            shiftleft(this,0);
            shiftleft(this,0);
            shiftleft(this,0);
            Lo|=int64(tmp);
            start++;
            upper=toupper(Source_prm[start]);
        }
    }
}


template<class BaseIntT> int SignedInt_t<BaseIntT>::AddDouble(SignedInt_t *A,const SignedInt_t &B,const int carry)
{
    if (carry!=0)
    {
        throw "non zero carry passed to SignedInt add";
    }
    if (A->Negative==B.Negative) //same sign.. just add them, don't modify the sign
    {
        return BaseIntT::AddDouble(&A->Value,B.Value,carry);
    }
    else
    {
        // ok now here is the fun with non two complement numbers

        // first detect which one has the larger magnitude
        if (A->Value>B.Value)
        {
            //A will maintain its sign, but subtract B from A
            if (BaseIntT::SubDouble(&A->Value,B.Value,0)!=0)
            {
                throw "borrow nessisary during add!"; //probably a bug...
            }
            return 0; //there shouldn't be a carry in this case
        }
        else
        {
            if (A->Value==B.Value)
            {
                //rare shortcut....
                A->Value=0;
                A->Negative=0;
                return 0;
            }
            else // (B.Value>A.Value)
            {
                //A will have B's Sign..

                // this is a lot sucky...
                BaseIntT tmp=A->Value;
                A->Value=B.Value;
                if (BaseIntT::SubDouble(&A->Value,tmp,0)!=0)
                {
                    throw "borrow nessisary during add!"; //probably a bug...
                }
                A->Negative=B.Negative;
                return 0; //there shouldn't be a carry in this case
            }
        }
    }
}

// flip the sign of A and add them...
template<class BaseIntT> int SignedInt_t<BaseIntT>::SubDouble(SignedInt_t *A,const SignedInt_t &B,const int borrow)
{
    A->Negative^=1; //negate the sign with xor <chuckle>
    return AddDouble(A,B,0);
}

//TODO make a finalizer class which sits between this class and the double template and throws overflow exceptions...
template<class BaseIntT> SignedInt_t<BaseIntT> SignedInt_t<BaseIntT>::MultiplyDouble(SignedInt_t *A,const SignedInt_t &B)
{
    BaseIntT::MultiplyDouble(&A->Value,B.Value);
    if (A->Value==0)
    {
        A->Negative=0;
    }
    else
    {
        A->Negative^=B.Negative; //flip sign as nessiary
    }
}

template<class BaseIntT> SignedInt_t<BaseIntT> SignedInt_t<BaseIntT>::DivideDouble(SignedInt_t *A,const SignedInt_t &B)
{
    SignedInt_t tmp;
    tmp.Value=BaseIntT::DivideDouble(&A->Value,B.Value);
    if (A->Value==0)
    {
        A->Negative=0;
    }
    else
    {
        A->Negative^=B.Negative; //flip sign as nessiary
    }
    return tmp.Value;
}

template<class BaseIntT> void SignedInt_t<BaseIntT>::FromString(const char *Source_prm)
{
    int start=0;
    Negative=0;

    while (Source_prm[start]!='\0')
    {
        if (Source_prm[start]=='0')
        {
            if (Source_prm[start+1]=='x')
            {
                // hex start
                Value.FromString(&Source_prm[start]);
                break;
            }
        }
        if ((Source_prm[start]>='0') && (Source_prm[start]<='9'))
        {
            // normal decimal start
            Value.FromString(&Source_prm[start]);
            break;
        }
        if (Source_prm[start]=='-')
        {
            Negative=1;
        }
        start++;
    }
    if (Source_prm[start]=='\0')
    {
        Value=0;
    }
}








//
//
//          Some unit test methods, most of these 
//          output data which must be verified by hand
//          or just diff the output from multiple runs.
//
//
//




#define _UNITTEST_ //for now just leave the unittest on
#ifdef _UNITTEST_



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
/*  a=0x10000;
    b=0x10;
    c=0x03;
    overflow=0;
    for (int x=0;x<20;x++)
    {
        printf("remainder=%llX, a=%llX b=%llX c=%llX\n",overflow,a,b,c);
        overflow=Divide64(&a,&b,&c);
    }*/

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
    char *teststrs[]=
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
