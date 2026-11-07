#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace apiary {

class JsonObjectScalarParser {
public:
    explicit JsonObjectScalarParser(const std::string& input) : input_(input) {}

    bool parse(std::map<std::string, std::string>& values) {
        if (input_.size() > 1048576) return false;
        skipWhitespace();
        if (!parseObject(values, 0, "")) return false;
        skipWhitespace();
        return position_ == input_.size();
    }

private:
    const std::string& input_;
    size_t position_ = 0;

    void skipWhitespace() {
        while (position_ < input_.size() &&
               (input_[position_] == ' ' || input_[position_] == '\t' ||
                input_[position_] == '\r' || input_[position_] == '\n')) ++position_;
    }

    static void appendUtf8(std::string& out, uint32_t cp) {
        if (cp <= 0x7f) out.push_back(static_cast<char>(cp));
        else if (cp <= 0x7ff) {
            out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0xffff) {
            out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }

    bool readHex4(uint32_t& value) {
        if (position_ + 4 > input_.size()) return false;
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = input_[position_++];
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<uint32_t>(c - 'A' + 10);
            else return false;
        }
        return true;
    }

    bool parseString(std::string& out) {
        if (position_ >= input_.size() || input_[position_++] != '"') return false;
        out.clear();
        while (position_ < input_.size()) {
            const unsigned char c = static_cast<unsigned char>(input_[position_++]);
            if (c == '"') return true;
            if (c < 0x20) return false;
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            if (position_ >= input_.size()) return false;
            const char escaped = input_[position_++];
            switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    uint32_t cp = 0;
                    if (!readHex4(cp)) return false;
                    if (cp >= 0xd800 && cp <= 0xdbff) {
                        if (position_ + 2 > input_.size() ||
                            input_[position_] != '\\' || input_[position_ + 1] != 'u') return false;
                        position_ += 2;
                        uint32_t low = 0;
                        if (!readHex4(low) || low < 0xdc00 || low > 0xdfff) return false;
                        cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                    } else if (cp >= 0xdc00 && cp <= 0xdfff) return false;
                    appendUtf8(out, cp);
                    break;
                }
                default: return false;
            }
        }
        return false;
    }

    bool parseNumber(std::string& out) {
        const size_t start = position_;
        if (position_ < input_.size() && input_[position_] == '-') ++position_;
        if (position_ >= input_.size()) return false;
        if (input_[position_] == '0') ++position_;
        else {
            if (input_[position_] < '1' || input_[position_] > '9') return false;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') ++position_;
        }
        if (position_ < input_.size() && input_[position_] == '.') {
            ++position_;
            const size_t digits = position_;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') ++position_;
            if (digits == position_) return false;
        }
        if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
            ++position_;
            if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) ++position_;
            const size_t digits = position_;
            while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') ++position_;
            if (digits == position_) return false;
        }
        out = input_.substr(start, position_ - start);
        return true;
    }

    bool parseObject(std::map<std::string, std::string>& out, unsigned depth,
                     const std::string& prefix) {
        if (depth > 32 || position_ >= input_.size() || input_[position_++] != '{') return false;
        skipWhitespace();
        if (position_ < input_.size() && input_[position_] == '}') {
            ++position_;
            return true;
        }
        while (position_ < input_.size()) {
            std::string key;
            if (!parseString(key)) return false;
            const std::string full_key = prefix.empty() ? key : prefix + "." + key;
            skipWhitespace();
            if (position_ >= input_.size() || input_[position_++] != ':') return false;
            skipWhitespace();
            std::string value;
            if (!parseValue(value, depth + 1, full_key, out)) return false;
            if (!out.emplace(full_key, value).second) return false;
            skipWhitespace();
            if (position_ < input_.size() && input_[position_] == '}') {
                ++position_;
                return true;
            }
            if (position_ >= input_.size() || input_[position_++] != ',') return false;
            skipWhitespace();
        }
        return false;
    }

    bool parseArray(std::map<std::string, std::string>& out, unsigned depth,
                    const std::string& prefix) {
        if (depth > 32 || position_ >= input_.size() || input_[position_++] != '[') return false;
        skipWhitespace();
        if (position_ < input_.size() && input_[position_] == ']') {
            ++position_;
            return true;
        }
        size_t index = 0;
        while (position_ < input_.size()) {
            std::string value;
            const std::string item_key = prefix + "[" + std::to_string(index++) + "]";
            if (!parseValue(value, depth + 1, item_key, out)) return false;
            if (!out.emplace(item_key, value).second) return false;
            skipWhitespace();
            if (position_ < input_.size() && input_[position_] == ']') {
                ++position_;
                return true;
            }
            if (position_ >= input_.size() || input_[position_++] != ',') return false;
            skipWhitespace();
        }
        return false;
    }

    bool parseValue(std::string& out, unsigned depth, const std::string& path,
                    std::map<std::string, std::string>& values) {
        if (depth > 32 || position_ >= input_.size()) return false;
        const char c = input_[position_];
        if (c == '"') return parseString(out);
        if (c == '{') {
            if (!parseObject(values, depth + 1, path)) return false;
            out.clear();
            return true;
        }
        if (c == '[') {
            if (!parseArray(values, depth + 1, path)) return false;
            out.clear();
            return true;
        }
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(out);
        for (const char* literal : {"true", "false", "null"}) {
            const std::string token(literal);
            if (input_.compare(position_, token.size(), token) == 0) {
                position_ += token.size();
                out = token;
                return true;
            }
        }
        return false;
    }

};

inline bool parseJsonObjectScalars(const std::string& input,
                                   std::map<std::string, std::string>& values) {
    values.clear();
    JsonObjectScalarParser parser(input);
    return parser.parse(values);
}

} // namespace apiary
