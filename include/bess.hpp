#pragma once
// https://github.com/LIJI32/SameBoy/blob/master/BESS.md

#include <cstdint>
#include <string>

struct BESS_NAME_BLOCK {};
struct BESS_INFO_BLOCK {};
struct BESS_CORE_BLOCK {};
struct BESS_XOAM_BLOCK {};
struct BESS_MBC_BLOCK {};
struct BESS_RTC_BLOCK {};
struct BESS_HUC_BLOCK {};
struct BESS_FOOTER_BLOCK {};

struct SAVE_STATE {
  BESS_NAME_BLOCK name;
  BESS_INFO_BLOCK info;
  BESS_CORE_BLOCK core;
  BESS_XOAM_BLOCK xoam;
  BESS_MBC_BLOCK mbc;
  BESS_RTC_BLOCK rtc;
  
};