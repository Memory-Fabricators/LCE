#include "InputStream.h"
#include "File.h"
#include "InputOutputStream.h"
#include "stdafx.h"

InputStream *InputStream::getResourceAsStream(const wstring &fileName)
{
    File file(fileName);

    return file.exists() ? new FileInputStream(file) : NULL;
}
