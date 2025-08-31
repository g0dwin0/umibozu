#pragma once
#include "common.hpp"
#include <unordered_map>


const std::unordered_map<u8, std::string_view> cart_types = {
    {0x00,                       "ROM ONLY"},
    {0x01,                           "MBC1"},
    {0x02,                       "MBC1+RAM"},
    {0x03,               "MBC1+RAM+BATTERY"},
    {0x05,                           "MBC2"},
    {0x06,                   "MBC2+BATTERY"},
    {0x08,                      "ROM+RAM 1"},
    {0x09,              "ROM+RAM+BATTERY 1"},
    {0x0B,                          "MMM01"},
    {0x0C,                      "MMM01+RAM"},
    {0x0D,              "MMM01+RAM+BATTERY"},
    {0x0F,             "MBC3+TIMER+BATTERY"},
    {0x10,                    "MBC30 + RTC"},
    {0x11,                           "MBC3"},
    {0x12,                    "MBC30 + RAM"},
    {0x13,                "MBC30 + BATTERY"},
    {0x19,                           "MBC5"},
    {0x1A,                       "MBC5+RAM"},
    {0x1B,               "MBC5+RAM+BATTERY"},
    {0x1C,                    "MBC5+RUMBLE"},
    {0x1D,                "MBC5+RUMBLE+RAM"},
    {0x1E,        "MBC5+RUMBLE+RAM+BATTERY"},
    {0x20,                           "MBC6"},
    {0x22, "MBC7+SENSOR+RUMBLE+RAM+BATTERY"},
    {0xFC,                  "POCKET CAMERA"},
    {0xFD,                   "BANDAI TAMA5"},
    {0xFE,                           "HuC3"},
    {0xFF,               "HuC1+RAM+BATTERY"},
};