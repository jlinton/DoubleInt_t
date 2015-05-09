// C++ BigNum template class
// AKA the integer doubler template.
// Copyright(C) 2007,2015 Jeremy Linton
//
// Source identity: int128_t.hpp
//
// This class provides an int128 with borrow/carry/overflow/etc 
// logic for use by the Double_t class which can then create 
// semi arbitary bit depth integers.
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



#ifndef INT128_T_HPP
#define INT128_T_HPP

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
        int128_t &operator%=( const int128_t &rhs) { this=DivideDouble(this,rhs); return *this;}

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
        int128_t operator%(   const int128_t &rhs) { int128_t tmp=*this; this=DivideDouble(&tmp,rhs); return tmp;}
        int128_t operator*(   const int128_t &rhs) { int128_t tmp=*this; MultiplyDouble(&tmp,rhs); return tmp;}

        int128_t operator&(   const int64    &rhs) { int128_t tmp=*this; tmp&=rhs; return tmp;}
        int128_t operator|(   const int64    &rhs) { int128_t tmp=*this; tmp|=rhs; return tmp;}
        int128_t operator^(   const int64    &rhs) { int128_t tmp=*this; tmp^=rhs; return tmp;}

        int128_t operator>>(  const int      &rhs) { int128_t tmp=*this; tmp>>=rhs; return tmp;}
        int128_t operator<<(  const int      &rhs) { int128_t tmp=*this; tmp<<=rhs; return tmp;}

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

/*bool operator==(const int128 &a,const int128 &b)
{
    if ((a.Hi==b.Hi) && (a.Lo==b.Lo)) return true;
    return false;
}*/

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
// This is provided for completeness but isn't used because the 
// divide double is implemented as shifts/subtracts like the doubler is
static inline int64 Divide64(int64 *A, int64 *B,int64 *C)
{
    int64 ret; 
    if (*C==0)
    {
        throw "Divide by zero";
    }
    if (*B>*C) //if the part in the high 64-bits is larger than the divisor then the 
    {          //cannot fit in RAX.
        throw "Underflow";
    }
    asm ("div %4    \n\t"
         :"=a" (*A), "=d" (ret)
         : "a" (*A), "d" (*B), "r" (*C)
         : 
     );
    return ret;
}


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


inline int int128_t::AddDouble(int128_t *A,const int128_t &B,const int carry)
{
    int carry_ret=0;
    carry_ret=Add64(&A->Lo,&B.Lo,carry);
    carry_ret=Add64(&A->Hi,&B.Hi,carry_ret);
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


// this takes the 64-bit value and prints it out given the formatter
// this is about as far from efficient as can be
string AsString(const int64 &Value,const char *format)
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
#endif //INT128_T_HPP
