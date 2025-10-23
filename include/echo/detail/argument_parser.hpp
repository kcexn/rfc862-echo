/* Copyright (C) 2025 Kevin Exton (kevin.exton@pm.me)
 *
 * Echo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Echo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Echo.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file argument_parser.hpp
 * @brief This file declares a CLI argument parser.
 */
#pragma once
#ifndef ECHO_ARGUMENT_PARSER_HPP
#define ECHO_ARGUMENT_PARSER_HPP
#include "generator.hpp"

#include <span>
#include <string_view>
/** @namespace For internal echo server implementation details. */
namespace echo::detail {
/** @brief A command line argument parser. */
struct argument_parser {
  /** @brief Command-line arguments are parsed into options. */
  struct option {
    std::string_view flag;
    std::string_view value;
  };
  /**
   * @brief Parse all command-line arguments.
   * @param args The command line arguments to parse.
   * @returns A generator of options.
   */
  static auto parse(std::span<char const *const> args) -> generator<option>;
  /**
   * @brief Parse all command-line arguments.
   * @param argc The number of command-line arguments.
   * @param argv The command-line arguments.
   * @returns A generator of options.
   */
  static auto parse(int argc, char const *const *argv) -> generator<option>
  {
    return parse({argv, static_cast<std::size_t>(argc)});
  }
};
} // namespace echo::detail
#endif // ECHO_ARGUMENT_PARSER_HPP
