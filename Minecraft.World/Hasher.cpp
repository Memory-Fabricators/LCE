#include "stdafx.h"
#include <functional>
#ifndef _SDL3
#include <xhash>
#endif

#include "Hasher.h"

#if defined _SDL3
static unsigned int hash_value(const std::wstring &s)
{
    return (unsigned int)std::hash<std::wstring>()(s);
}
#endif

Hasher::Hasher(wstring &salt)
{
    this->salt = salt;
}

wstring Hasher::getHash(wstring &name)
{
    // 4J Stu - Removed try/catch
    // try {
    wstring s = wstring(salt).append(name);
    // MessageDigest m;
    // m = MessageDigest.getInstance("MD5");
    // m.update(s.getBytes(), 0, s.length());
    // return new BigInteger(1, m.digest()).toString(16);

    // TODO 4J Stu - Will this hash us with the same distribution as the MD5?
    return _toString(::hash_value(s));
    //}
    // catch (NoSuchAlgorithmException e)
    //{
    //	throw new RuntimeException(e);
    //}
}
