//
// crypto.cpp
// Created by Diogo Martins on 14/05/15.

#include "crypto.h"

/*******************************************************************************
	XOR.cpp
	 
	This function takes in two strings (value,key) and uses XOR encryption to
	encrypt (and decrypt) the value using the key.  it returns the encrypted
	value as a C++ string.
	 
	John Shao
	
For more on XOR encryption: http://www.cprogramming.com/tutorial/xor.html 
********************************************************************************
	This work is hereby released into the Public Domain. To view a copy of the
	public domain dedication, visit
	http://creativecommons.org/licenses/publicdomain/
 
	or send a letter to
 
	Creative Commons
	559 Nathan Abbott Way
	Stanford, California 94305
	USA.
*******************************************************************************/
 
string XOR(string value,string key)
{
	string retval(value);
 
	short unsigned int klen = key.length();
	short unsigned int vlen = value.length();
	short unsigned int k = 0;
	short unsigned int v = 0;
	 
	for(v = 0; v < vlen; v++)
	{
		retval[v] = value[v] ^ key[k];
		k = (++k < klen ? k : 0);
	}
	 
	return retval;
}
