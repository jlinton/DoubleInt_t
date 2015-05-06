# DoubleInt_t

C++ template that doubles the integer class it is given, when nested this can create large bignums



This started as an amusing exercise in template meta-programming.. 
Or in this case nestable classes. AKA std::list&<std::list<>&>
But the optimizer can see into the nesting, so the generated assembly 
actually looks really sort of native. Especially since the base class
is just gcc inline assembly...

So, what we have is actually a pretty good class for "small" bignums
AKA ones less than say 1k bits. Larger than that and the more intelligent
algorithms used in GNU MP result in faster code.

This is also an _AWESOME_ test of your compiler. Have you ever seen a
single module take hours to compile? Well, that is possible with this one
if the integer types are large enough!!!!! It can also create some crazy big
functions... More recent versions of GCC actually are much more capable 
of compiling this code (especially with O3 than older ones)!

One of the nicer things about this code is the fact that I've created 
an x86 int128 class that can provide information about overflow/carry
Its this class which is used as the basis for the integer doubler class.

The amusing code is where we have:
 typedef class DoubleInt_t<int128>    int256;
 typedef class DoubleInt_t<int256>    int512;
 typedef class DoubleInt_t<int512>    int1024;
 typedef class DoubleInt_t<int1024>   int2048;
 
While these are called int256/etc they are actually unsigned 
(because at the time, I thought that the default int class in C++ should be
unsigned with a "signed" keyword to provide for signed math...)

There is a signed template too called SignedInt_t<> which adds a sign
bit to the given unsigned class, and transforms all the basic operations
so that they work correctly depending on the state of the sign bit.

combined with some inline assembly to create a integer class
that could export information about whether an operation had overflow/carry

The doubler type mostly behaves as would be expected for a normal integer
type, although it only supports the unary operators. The global binary 
operators (aka x=y+z) could be easily added but you then need a set 
for each type along with appropriate conversions. So the example
must be coded as x=y; x+=z;

See my vector class for some examples of global binary operators if you
don't know how to implement them yourself.


