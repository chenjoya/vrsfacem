/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Strings.h"

#include <cctype>
#include <climits>
#include <cmath>
#include <cstring>
#include <ctime>

#include <sstream>

#include <fmt/format.h>

#include <vrs/os/Platform.h>

namespace vrs {
namespace helpers {

using namespace std;

string trim(const string& text, const char* whiteChars) {
  size_t end = text.length();
  while (end > 0 && strchr(whiteChars, text[end - 1]) != nullptr) {
    end--;
  }
  if (end == 0) {
    return {};
  }
  size_t start = 0;
  while (start < end && strchr(whiteChars, text[start]) != nullptr) {
    start++;
  }
  if (start > 0 || end < text.length()) {
    return text.substr(start, end - start);
  }
  return text;
}

bool startsWith(const string& text, const string& prefix) {
  return text.length() >= prefix.length() &&
      vrs::helpers::strncasecmp(text.c_str(), prefix.c_str(), prefix.length()) == 0;
}

bool endsWith(const string& text, const string& suffix) {
  return text.length() >= suffix.length() &&
      vrs::helpers::strncasecmp(
          text.c_str() + text.length() - suffix.length(), suffix.c_str(), suffix.length()) == 0;
}

string humanReadableFileSize(int64_t bytes) {
  const int64_t unit = 1024;
  if (bytes < unit) {
    return to_string(bytes) + " B";
  }
  double exp = floor(log(bytes) / log(unit));
  char pre = string("KMGTPE").at(static_cast<size_t>(exp - 1));
  string str;
  double r = bytes / pow(unit, exp);
  if (r < 1000.) {
    str = fmt::format("{:.2f} {}B", r, pre);
  } else {
    // avoid scientific notation switch for 1000-1023...
    str = fmt::format("{} {}B", static_cast<int>(r), pre);
  }
  return str;
}

string humanReadableDuration(double seconds) {
  string str;
  str.reserve(30);
  if (seconds < 0) {
    str.push_back('-');
    seconds = -seconds;
  }
  const double kYear = 31557600; // Julian astronomical year
  if (seconds < 1e9 * kYear) {
    const double kMinute = 60;
    const double kHour = 60 * kMinute;
    const double kDay = 24 * kHour;
    const double kWeek = 7 * kDay;
    bool showNext = false;
    if (seconds > kYear) {
      int years = static_cast<int>(seconds / kYear);
      str.append(to_string(years)).append((years == 1) ? " year, " : " years, ");
      seconds -= years * kYear;
      showNext = true;
    }
    if (showNext || seconds > kWeek) {
      int weeks = static_cast<int>(seconds / kWeek);
      str.append(to_string(weeks)).append((weeks == 1) ? " week, " : " weeks, ");
      seconds -= weeks * kWeek;
      showNext = true;
    }
    if (showNext || seconds > kDay) {
      int days = static_cast<int>(seconds / kDay);
      str.append(to_string(days)).append((days == 1) ? " day, " : " days, ");
      seconds -= days * kDay;
      showNext = true;
    }
    if (showNext || seconds > kHour) {
      int hours = static_cast<int>(seconds / kHour);
      str.append(to_string(hours)).append("h ");
      seconds -= hours * kHour;
      showNext = true;
    }
    if (showNext || seconds > kMinute) {
      int minutes = static_cast<int>(seconds / kMinute);
      str.append(to_string(minutes)).append("m ");
      seconds -= minutes * kMinute;
      showNext = true;
    }
    if (showNext || seconds == 0 || seconds >= 1) {
      str.append(humanReadableTimestamp(seconds)).append("s");
    } else if (seconds >= 2e-3) {
      str.append(fmt::format("{:.0f}ms", seconds * 1e03));
    } else if (seconds >= 2e-6) {
      str.append(fmt::format("{:.0f}us", seconds * 1e06));
    } else if (seconds >= 2e-9) {
      str.append(fmt::format("{:.0f}ns", seconds * 1e09));
    } else if (seconds >= 2e-12) {
      str.append(fmt::format("{:.0f}ps", seconds * 1e12));
    } else if (seconds >= 2e-15) {
      str.append(fmt::format("{:.0f}fs", seconds * 1e15));
    } else if (seconds >= 2e-18) {
      str.append(fmt::format("{:.3f}fs", seconds * 1e15));
    } else {
      str.append(fmt::format("{:g}fs", seconds * 1e15));
    }
  } else {
    str.append(humanReadableTimestamp(seconds)).append("s");
  }
  return str;
}

string humanReadableTimestamp(double seconds, uint8_t precision) {
  // This code must work with Ubuntu's 20.4 fmt and the newest compilers/fmt.
  // Bottom line: fmt::format must use a constant format string to compile everywhere.
  enum Format { f3, f6, f9, e3, e9 };
  Format format = f3;
  double cTinyLimit = 1e-3;
  const double cHugeLimit = 1e10;
  if (precision > 3) {
    if (precision <= 6) {
      cTinyLimit = 1e-6;
      format = f6;
    } else {
      cTinyLimit = 1e-9;
      format = f9;
    }
  }
  double atimestamp = abs(seconds);
  if (atimestamp < cTinyLimit) {
    if (atimestamp > 0) {
      format = e3;
    }
  } else if (atimestamp >= cHugeLimit) {
    format = e9;
  }
  switch (format) {
    case f3:
      return fmt::format("{:.3f}", seconds);
    case f6:
      return fmt::format("{:.6f}", seconds);
    case f9:
      return fmt::format("{:.9f}", seconds);
    case e3:
      return fmt::format("{:.3e}", seconds);
    case e9:
      return fmt::format("{:.9e}", seconds);
  }
}

string humanReadableDateTime(double secondsSinceEpoch) {
  string date(std::size("YYYY-MM-DD HH:MM:SS"), 0);
  time_t creationTimeSec = static_cast<time_t>(secondsSinceEpoch);
  struct tm ltm {};
#if IS_WINDOWS_PLATFORM()
  localtime_s(&ltm, &creationTimeSec); // because "Windows"
#else
  localtime_r(&creationTimeSec, &ltm);
#endif
  date.resize(std::strftime(date.data(), date.size(), "%F %T", &ltm));
  return date;
}

string make_printable(const string& str) {
  string sanitized;
  if (!str.empty()) {
    sanitized.reserve(str.size() + 10);
    for (unsigned char c : str) {
      if (c < 128 && isprint(c)) {
        sanitized.push_back(c);
      } else if (c == '\n') {
        sanitized += "\\n"; // LF
      } else if (c == '\r') {
        sanitized += "\\r"; // CR
      } else if (c == '\t') {
        sanitized += "\\t"; // tab
      } else if (c == '\b') {
        sanitized += "\\b"; // backspace
      } else if (c == 0x1B) {
        sanitized += "\\e"; // esc
      } else {
        // stringstream methods just won't work right, even with Stackoverflow's help...
        static const char* digits = "0123456789abcdef";
        sanitized += "\\x";
        sanitized.push_back(digits[(c >> 4) & 0xf]);
        sanitized.push_back(digits[c & 0xf]);
      }
    }
  }
  return sanitized;
}

bool getBool(const map<string, string>& m, const string& field, bool& outValue) {
  const auto iter = m.find(field);
  if (iter != m.end() && !iter->second.empty()) {
    outValue = iter->second != "0" && iter->second != "false";
    return true;
  }
  return false;
}

bool getInt(const map<string, string>& m, const string& field, int& outValue) {
  const auto iter = m.find(field);
  if (iter != m.end() && !iter->second.empty()) {
    try {
      outValue = stoi(iter->second);
      return true;
    } catch (logic_error&) {
      /* do nothing */
    }
  }
  return false;
}

bool getInt64(const map<string, string>& m, const string& field, int64_t& outValue) {
  const auto iter = m.find(field);
  if (iter != m.end() && !iter->second.empty()) {
    try {
      outValue = stoll(iter->second);
      return true;
    } catch (logic_error&) {
      /* do nothing */
    }
  }
  return false;
}

bool getUInt64(const map<string, string>& m, const string& field, uint64_t& outValue) {
  const auto iter = m.find(field);
  if (iter != m.end() && !iter->second.empty()) {
    try {
      outValue = stoull(iter->second);
      return true;
    } catch (logic_error&) {
      /* do nothing */
    }
  }
  return false;
}

bool getDouble(const map<string, string>& m, const string& field, double& outValue) {
  const auto iter = m.find(field);
  if (iter != m.end() && !iter->second.empty()) {
    try {
      outValue = stod(iter->second);
      return true;
    } catch (logic_error&) {
      /* do nothing */
    }
  }
  return false;
}

bool readUInt32(const char*& str, uint32_t& outValue) {
  char* newStr = nullptr;
  errno = 0;
  long long int readInt = strtoll(str, &newStr, 10);
  if (readInt < 0 || (readInt == LLONG_MAX && errno == ERANGE) ||
      readInt > numeric_limits<uint32_t>::max() || str == newStr) {
    return false;
  }

  outValue = static_cast<uint32_t>(readInt);

  str = newStr;
  return true;
}

bool replaceAll(string& inOutString, const string& token, const string& replacement) {
  bool replaced = false;
  if (!token.empty()) {
    size_t pos = inOutString.find(token, 0);
    while (pos != string::npos) {
      inOutString.replace(pos, token.length(), replacement);
      replaced = true;
      pos = inOutString.find(token, pos + replacement.length());
    }
  }
  return replaced;
}

void split(
    const std::string& inputString,
    char delimiter,
    std::vector<std::string>& tokens,
    bool skipEmpty,
    const char* trimChars) {
  std::stringstream ss(inputString);
  std::string item;

  while (getline(ss, item, delimiter)) {
    if (trimChars != nullptr) {
      item = helpers::trim(item, trimChars);
    }
    if (!(item.empty() && skipEmpty)) {
      tokens.push_back(item);
    }
  }
}

} // namespace helpers
} // namespace vrs
