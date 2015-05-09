// C++ BigNum template class
// AKA the integer doubler template.
// Copyright(C) 2007,2015 Jeremy Linton
//
// Source identity: DoubleInt_t.hpp
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
#ifndef DOUBLEINT_T_HPP
#define DOUBLEINT_T_HPP

#include "int128_t.hpp"


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
        DoubleInt_t &operator%=( const DoubleInt_t &rhs) { *this=DivideDouble(this,rhs); return *this;}



        DoubleInt_t &operator&=( const int64 &rhs) { this->Lo&=rhs; return *this;}
        DoubleInt_t &operator|=( const int64 &rhs) { this->Lo|=rhs; return *this;}
        DoubleInt_t &operator^=( const int64 &rhs) { this->Lo^=rhs; return *this;}

        DoubleInt_t &operator&=( const DoubleInt_t &rhs) { this->Lo&=rhs.Lo; this->Hi&=rhs.Hi; return *this;}
        DoubleInt_t &operator|=( const DoubleInt_t &rhs) { this->Lo|=rhs.Lo; this->Hi|=rhs.Hi; return *this;}
        DoubleInt_t &operator^=( const DoubleInt_t &rhs) { this->Lo^=rhs.Lo; this->Hi^=rhs.Hi; return *this;}


        DoubleInt_t operator+(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; AddDouble(&tmp,rhs,0); return tmp;}
        DoubleInt_t operator-(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; SubDouble(&tmp,rhs,0); return tmp;}
        DoubleInt_t operator/(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; DivideDouble(&tmp,rhs); return tmp;}
        DoubleInt_t operator%(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; tmp=DivideDouble(&tmp,rhs); return tmp;}
        DoubleInt_t operator*(   const DoubleInt_t &rhs) { DoubleInt_t tmp=*this; MultiplyDouble(&tmp,rhs); return tmp;}

        DoubleInt_t operator&(   const int64    &rhs) { DoubleInt_t tmp=*this; tmp.Lo&=rhs; return tmp;}
        DoubleInt_t operator|(   const int64    &rhs) { DoubleInt_t tmp=*this; tmp.Lo|=rhs; return tmp;}
        DoubleInt_t operator^(   const int64    &rhs) { DoubleInt_t tmp=*this; tmp.Lo^=rhs; return tmp;}


        DoubleInt_t operator>>(  const int      &rhs) { DoubleInt_t tmp=*this; tmp>>=rhs; return tmp;}
        DoubleInt_t operator<<(  const int      &rhs) { DoubleInt_t tmp=*this; tmp<<=rhs; return tmp;}



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








template<class BaseIntT> int DoubleInt_t<BaseIntT>::SubDouble(DoubleInt_t *A,const DoubleInt_t &B,const int borrow)
{
    int ret_borrow;
    ret_borrow=BaseIntT::SubDouble(&A->Lo,B.Lo,borrow);
    ret_borrow=BaseIntT::SubDouble(&A->Hi,B.Hi,ret_borrow);
    return ret_borrow;
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


template<class BaseIntT> int DoubleInt_t<BaseIntT>::shiftright(DoubleInt_t *Value,const int Carry_prm)
{
    int carry_ret;
    carry_ret=BaseIntT::shiftright(&Value->Hi,Carry_prm);
    carry_ret=BaseIntT::shiftright(&Value->Lo,carry_ret);
    return carry_ret;
}


template<class BaseIntT> int DoubleInt_t<BaseIntT>::shiftleft(DoubleInt_t *Value,const int Carry_prm)
{
    int carry_ret;
    carry_ret=BaseIntT::shiftleft(&Value->Lo,Carry_prm);
    carry_ret=BaseIntT::shiftleft(&Value->Hi,carry_ret);
    return carry_ret;
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



#endif // DOUBLEINT_T_HPP
