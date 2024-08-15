#include "utility/url.h"
#include <iomanip>
#include <sstream>

using avant::utility::url;

url::url()
{
}

url::url(const std::string &s)
{
}

std::string url::get_scheme() const
{
    return "";
}

std::string url::get_username() const
{
    return "";
}

std::string url::get_password() const
{
    return "";
}

std::string url::get_host() const
{
    return "";
}

unsigned short url::get_port() const
{
    return 0;
}

std::string url::get_path() const
{
    return "";
}

std::string url::get_query() const
{
    return "";
}

const std::multimap<std::string, std::string> &url::get_query_parameters() const
{
    static std::multimap<std::string, std::string> tmp{};
    return tmp;
}

std::string url::get_fragment() const
{
    return "";
}

// path+query+fragment
std::string url::get_full_path() const
{
    return "";
}

void url::from_string(const std::string &s)
{
}

bool avant::utility::operator==(const url &a, const url &b)
{
    return false;
}

bool avant::utility::operator!=(const url &a, const url &b)
{
    return false;
}

bool avant::utility::operator<(const url &a, const url &b)
{
    return false;
}

void url::set_secure(bool secure)
{
}

bool url::is_ipv6() const
{
    return false;
}

bool url::is_secure() const
{
    return false;
}

std::string url::to_string() const
{
    return "";
}
url::operator std::string() const
{
    return this->to_string();
}

bool url::unescape_path(const std::string &in, std::string &out)
{
    return false;
}

std::string_view url::capture_up_to(const std::string_view right_delimiter, const std::string &error_message /*= ""*/)
{
    return "";
}

bool url::move_before(const std::string_view right_delimiter)
{
    return false;
}

bool url::exists_forward(const std::string_view right_delimiter)
{
    return false;
}

std::string url::decode(const std::string &str)
{
    std::ostringstream decoded;
    std::string::size_type len = str.length();
    for (std::string::size_type i = 0; i < len; ++i)
    {
        if (str[i] == '%' && i + 2 < len) // two char
        {
            int hex_value;
            std::istringstream hex_stream(str.substr(i + 1, 2));
            if (hex_stream >> std::hex >> hex_value)
            {
                decoded << static_cast<char>(hex_value);
                i += 2;
            }
            else
            {
                decoded << str[i];
            }
        }
        else if (str[i] == '+')
        {
            decoded << ' ';
        }
        else
        {
            decoded << str[i];
        }
    }
    return decoded.str();
}

std::string url::encode(const std::string &str)
{
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;
    for (char c : str)
    {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/')
        {
            encoded << c;
        }
        else
        {
            encoded << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return encoded.str();
}