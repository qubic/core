#pragma once

std::string wchar_to_string(const wchar_t* wstr) {
    std::u16string u16str;
    for (size_t i = 0; wstr[i]; i++) {
        uint16_t c = static_cast<uint16_t>(wstr[i]); // truncate to 16-bit
        u16str += c;
    }

    // Convert UTF-16 to UTF-8
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(u16str);
}

void print_wstr(const wchar_t* wstr) {
    std::cout << wchar_to_string(wstr) << std::endl;
}