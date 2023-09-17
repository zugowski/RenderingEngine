#include "FileUtil.h"

namespace {
    std::string ToUTF8(const std::wstring& value)
    {
        auto length = WideCharToMultiByte(
            CP_UTF8, 0U, value.data(), -1, nullptr, 0, nullptr, nullptr);
        auto buffer = new char[length];

        WideCharToMultiByte(
            CP_UTF8, 0U, value.data(), -1, buffer, length, nullptr, nullptr);

        std::string result(buffer);
        delete[] buffer;
        buffer = nullptr;

        return result;
    }

    std::string Replace
    (
        const std::string& input,
        std::string         pattern,
        std::string         replace)
    {
        std::string result = input;
        auto pos = result.find(pattern);

        while (pos != std::string::npos)
        {
            result.replace(pos, pattern.length(), replace);
            pos = result.find(pattern, pos + replace.length());
        }

        return result;
    }

    std::wstring Replace
    (
        const std::wstring& input,
        std::wstring         pattern,
        std::wstring         replace)
    {
        std::wstring result = input;
        auto pos = result.find(pattern);

        while (pos != std::wstring::npos)
        {
            result.replace(pos, pattern.length(), replace);
            pos = result.find(pattern, pos + replace.length());
        }

        return result;
    }

} // namespace

bool SearchFilePathW(const wchar_t* filename, std::wstring& result)
{
    if (filename == nullptr)
    {
        return false;
    }

    if (wcscmp(filename, L" ") == 0 || wcscmp(filename, L"") == 0)
    {
        return false;
    }

    wchar_t exePath[520];
    memset(exePath, L'\0', sizeof(exePath));
    GetModuleFileNameW(nullptr, exePath, 520);
    PathRemoveFileSpecW(exePath);

    wchar_t dstPath[520];
    memset(dstPath, L'\0', sizeof(exePath));

    wcscpy_s(dstPath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"..\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"..\\..\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"\\res\\%s", filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"%s\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"%s\\..\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"%s\\..\\..\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    swprintf_s(dstPath, L"%s\\res\\%s", exePath, filename);
    if (PathFileExistsW(dstPath) == TRUE)
    {
        result = Replace(dstPath, L"\\", L"/");
        return true;
    }

    return false;
}

bool SearchFilePathA(const char* filename, std::string& result)
{
    if (filename == nullptr)
    {
        return false;
    }

    if (strcmp(filename, " ") == 0 || strcmp(filename, "") == 0)
    {
        return false;
    }

    char exePath[520] = { 0 };
    memset(exePath, '\0', sizeof(exePath));
    GetModuleFileNameA(nullptr, exePath, 520);
    PathRemoveFileSpecA(exePath);

    char dstPath[520] = { 0 };
    memset(dstPath, '\0', sizeof(dstPath));

    strcpy_s(dstPath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "..\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "..\\..\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "\\res\\%s", filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "%s\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "%s\\..\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "%s\\..\\..\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    sprintf_s(dstPath, "%s\\res\\%s", exePath, filename);
    if (PathFileExistsA(dstPath) == TRUE)
    {
        result = Replace(dstPath, "\\", "/");
        return true;
    }

    return false;
}

std::string RemoveDirectoryPathA(const std::string& path)
{
    auto temp = Replace(path, "\\", "/");
    auto pos = temp.rfind('/');

    if (pos != std::string::npos)
    {
        return temp.substr(pos + 1);
    }

    return path;
}

std::wstring RemoveDirectoryPathW(const std::wstring& path)
{
    auto temp = Replace(path, L"\\", L"/");
    auto pos = temp.rfind(L'/');

    if (pos != std::wstring::npos)
    {
        return temp.substr(pos + 1);
    }

    return path;
}

std::string GetDirectoryPathA(const char* filePath)
{
    std::string path = Replace(filePath, "\\", "/");
    size_t idx = path.find_last_of("/");
    if (idx != std::string::npos)
    {
        return path.substr(0, idx + 1);
    }

    return std::string();
}

std::wstring GetDirectoryPathW(const wchar_t* filePath)
{
    std::wstring path = Replace(filePath, L"\\", L"/");
    size_t idx = path.find_last_of(L"/");
    if (idx != std::wstring::npos)
    {
        return path.substr(0, idx + 1);
    }

    return std::wstring();
}