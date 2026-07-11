#include "InputStream.h"
#include "File.h"
#include "InputOutputStream.h"
#include "stdafx.h"

InputStream *InputStream::getResourceAsStream(const wstring &fileName)
{
    return new FileInputStream(File(fileName));
}
