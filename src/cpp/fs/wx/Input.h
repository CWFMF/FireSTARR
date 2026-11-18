/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INPUT_H
#define FS_INPUT_H
#include "../stdafx.h"
#include "FwiWeather.h"
namespace fs
{
/**
 * \brief Read all indices from a stream
 * \param iss Stream to parse
 * \param str string to read into
 */
FwiWeather read_fwi_weather(istringstream* iss, string* str);
/**
 * \brief Read only weather indices from a stream
 * \param iss Stream to parse
 * \param str string to read into
 */
FwiWeather read_weather(istringstream* iss, string* str);
}
#endif
