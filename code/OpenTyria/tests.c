#pragma once

void expect_parse_u16_equals(const char *input, uint16_t expected)
{
    uint16_t result;
    assert(parse_u16(&result, input, strlen(input), 0));
    assert(result == expected);
}

void test_parse_u16()
{
    uint16_t result;
    expect_parse_u16_equals("0", 0);
    expect_parse_u16_equals("-0", 0);
    expect_parse_u16_equals("+0", 0);
    expect_parse_u16_equals("001", 1);
    expect_parse_u16_equals("0b011", 3);
    expect_parse_u16_equals("0o11", 9);
    expect_parse_u16_equals("0x11", 17);
    expect_parse_u16_equals("00000", 0);
    assert(parse_u16(&result, "0x10000", strlen("0x10000"), 0) == false);
    assert(parse_u16(&result, "0x1", strlen("0x1"), 10) == false);
    assert(parse_u16(&result, "0b0", strlen("0b0"), 10) == false);
}

bool parse_ipv4_cstr(SocketAddr *result, const char *input)
{
    return parse_ipv4(&result->v4, input, strlen(input));
}

bool parse_ipv6_cstr(SocketAddr *result, const char *input)
{
    return parse_ipv6(&result->v6, input, strlen(input));
}

void test_parse_ipv4()
{
    SocketAddr result;
    assert(parse_ipv4_cstr(&result, "1.2.3.4:80") == true);
    assert(parse_ipv4_cstr(&result, "1.2.3.4:65535") == true);
    assert(parse_ipv4_cstr(&result, "1.2.3.4:65536") == false);
    assert(parse_ipv4_cstr(&result, ":65536") == false);
    assert(parse_ipv4_cstr(&result, "1.2.3.4") == false);
}

void test_parse_ipv6()
{
    SocketAddr result;
    assert(parse_ipv6_cstr(&result, "[e9c2:e2e3:ea8b:9f36:e151:9b65:b631:c87d]:80") == true);
    assert(parse_ipv6_cstr(&result, "[e9c2:e2e3:ea8b:9f36:e151:9b65:b631:c87d]:65535") == true);
    assert(parse_ipv6_cstr(&result, "[e9c2:e2e3:ea8b:9f36:e151:9b65:b631:c87d]:65536") == false);
    assert(parse_ipv6_cstr(&result, ":65536") == false);
    assert(parse_ipv6_cstr(&result, "e9c2:e2e3:ea8b:9f36:e151:9b65:b631:c87d:80") == false);
    assert(parse_ipv6_cstr(&result, "e9c2:e2e3:ea8b:9f36:e151:9b65:b631:c87d") == false);
}

int main()
{
    test_parse_u16();
    test_parse_ipv4();
    test_parse_ipv6();

    return 0;
}
