/* ************************************************************************
*   File: random.c                                      Part of CircleMUD *
*  Usage: random number generator                                         *
************************************************************************ */

//
// File: random.c                      -- Part of TempusMUD
//
// All modifications and additions are
// Copyright 1998 by John Watson, all rights reserved.
//

/*
 * I am bothered by the non-portablility of 'rand' and 'random' -- rand
 * is ANSI C, but on some systems such as Suns, rand has seriously tragic
 * spectral properties (the low bit alternates between 0 and 1!).  random
 * is better but isn't supported by all systems.  So, in my quest for Ultimate
 * CircleMUD Portability, I decided to include this code for a simple but
 * relatively effective random number generator.  It's not the best RNG code
 * around, but I like it because it's very short and simple, and for our
 * purposes it's "random enough".
 *               --Jeremy Elson  2/23/95
 */

/***************************************************************************/

/*
 *
 * This program is public domain and was written by William S. England
 * (Oct 1988).  It is based on an article by:
 *
 * Stephen K. Park and Keith W. Miller. RANDOM NUMBER GENERATORS:
 * GOOD ONES ARE HARD TO FIND. Communications of the ACM,
 * New York, NY.,October 1988 p.1192

 The following is a portable c program for generating random numbers.
 The modulus and multipilier have been extensively tested and should
 not be changed except by someone who is a professional Lehmer generator
 writer.  THIS GENERATOR REPRESENTS THE MINIMUM STANDARD AGAINST WHICH
 OTHER GENERATORS SHOULD BE JUDGED. ("Quote from the referenced article's
 authors. WSE" )
*/

#define	m  (unsigned long)2147483647
#define	q  (unsigned long)127773

#define	a (unsigned int)16807
#define	r (unsigned int)2836

/*
** F(z)	= (az)%m
**	= az-m(az/m)
**
** F(z)  = G(z)+mT(z)
** G(z)  = a(z%q)- r(z/q)
** T(z)  = (z/q) - (az/m)
**
** F(z)  = a(z%q)- rz/q+ m((z/q) - a(z/m))
** 	 = a(z%q)- rz/q+ m(z/q) - az
*/

#include <limits.h>

static unsigned long seed;

void
my_srand(unsigned long initial_seed)
{
	seed = initial_seed;
}


unsigned long
my_rand(void)
{
	register int lo, hi, test;

	hi = seed / q;
	lo = seed % q;

	test = a * lo - r * hi;

	if (test > 0)
		seed = test;
	else
		seed = test + m;

	return seed;
}

//
// creates a random number in interval [from;to]'
//
int
number(int from, int to)
{
	if (to <= from)
		return (from);
	return (int)(((long long)my_rand() * (long long)(to - from + 1) / INT_MAX) + from);
}

double
rand_float(void)
{
	return (double)my_rand() / (double)INT_MAX;
}

//
// returns a random boolean value
//
bool
random_binary()
{
	return !number(0,1);
}

//
// returns a random boolean value, true 1/num of returns
//
bool
random_fractional(unsigned int num)
{
	if (num == 0)
		return true;
	return !number(0, num - 1);
}

//
// returns a random boolean value, true 1/3 of returns (33% tru)
//
bool
random_fractional_3()
{
	return !number(0, 2);
}

//
// returns a random boolean value, true 1/4 of returns (25% true)
//
bool
random_fractional_4()
{
	return !number(0, 3);
}

//
// returns a random boolean value, true 1/5 of returns (20% true)
//
bool
random_fractional_5()
{
	return !number(0, 4);
}

//
// returns a random boolean value, true 1/10 of returns (10% true)
//
bool
random_fractional_10()
{
	return !number(0, 9);
}

//
// returns a random boolean value, true 1/20 of returns (5% true)
//
bool
random_fractional_20()
{
	return !number(0, 19);
}

//
// returns a random boolean value, true 1/50 of returns (2% true)
//
bool
random_fractional_50()
{
	return !number(0, 49);
}

//
// returns a random boolean value, true 1/100 of returns (1% true)
//
bool
random_fractional_100()
{
	return !number(0, 99);
}

//
// returns a random value between and including 1-100
//
int
random_percentage()
{
	return number(1, 100);
}

//
// returns a random value between and including 0-99
//
int
random_percentage_zero_low()
{
	return number(0, 99);
}

//
// return a random value between 0 and num
//
int
random_number_zero_low(unsigned int num)
{
	if (num == 0)
		return 0;
	return number(0, num);
}

//
// returns a random value of val +/- variance not < min or > max
int
rand_value(int val, int variance, int min, int max)
{
    if (min == -1 || val - variance > min)
        min = val - variance;
    if (max == -1 || val + variance < max)
        max = val + variance;
    return number(min, max);
}
//
//
//

double
float_number(double from, double to)
{
	if (to <= from)
		return (from);
	return rand_float() * (to - from) + from;
}

//
// simulates dice roll
//
int
dice(int num, int size)
{
	int sum = 0;

	if (size <= 0 || number <= 0)
		return 0;

	while (num-- > 0)
		sum += number(1, size);

	return sum;
}
