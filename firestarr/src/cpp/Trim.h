/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include <string>
// from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
namespace tbd::util
{
/**
 * \brief Remove whitespace from left side of string
 * \param s string to trim
 */
void trim_left(string* s);
/**
 * \brief Remove whitespace from right side of string
 * \param s string to trim
 */
void trim_right(string* s);
/**
 * \brief Remove whitespace from both sides of string
 * \param s string to trim
 */
void trim(string* s);
/**
 * \brief Return new string with whitespace removed from left side
 * \param s string to trim
 * \return new string with whitespace removed from left side
 */
[[nodiscard]] string trim_left_copy(string s);
/**
 * \brief Return new string with whitespace removed from right side
 * \param s string to trim
 * \return new string with whitespace removed from right side
 */
[[nodiscard]] string trim_right_copy(string s);
/**
 * \brief Return new string with whitespace removed from both sides
 * \param s string to trim
 * \return new string with whitespace removed from both sides
 */
[[nodiscard]] string trim_copy(string s);
}
