#ifndef FFMPEG_EXAMPLES_ARGS_PARSER_H
#define FFMPEG_EXAMPLES_ARGS_PARSER_H

#include <optional>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <any>
#include <regex>
#include "logging.h"
#include "fmt/format.h"
#include "enum.h"


namespace args
{
    // string
    template<typename T>
    struct is_string
            : public std::disjunction<
                    std::is_same<char *, typename std::decay_t<T>>,
                    std::is_same<const char *, typename std::decay_t<T>>,
                    std::is_same<std::string, typename std::decay_t<T>>
            > {
    };

    template<class T>
    inline constexpr bool is_string_v = is_string<T>::value;

    // vector
    template<class, class _ = void> struct is_vector : public std::false_type {};

    template<class T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    template <class T>
    struct is_vector<T, std::enable_if_t<std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>>>>
            : std::true_type {};

    // pair
    template<class, class _ = void> struct is_pair : public std::false_type {};

    template<class T>
    inline constexpr bool is_pair_v = is_pair<T>::value;

    template <class T>
    struct is_pair<T, std::enable_if_t<std::is_same_v<T, std::pair<typename T::first_type, typename T::second_type>>>>
            : std::true_type {};

    // map
    template<class, class _ = void> struct is_map : public std::false_type {};

    template<class T>
    inline constexpr bool is_map_v = is_map<T>::value;

    template <class T>
    struct is_map<T, std::enable_if_t<std::is_same_v<T,
            std::map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>>>>
            : std::true_type {};


    template<typename T> concept String = is_string_v<T>;
    template<typename T> concept Vector = is_vector_v<T>;
    template<typename T> concept Pair = is_pair_v<T>;
    template<typename T> concept Map = is_map_v<T>;

    enum class value_t
    {
        unsupported     = 0x00,

        string          = 0x0001,
        boolean         = 0x0002,
        floating        = 0x0004,
        integer         = 0x0008,

        vector          = 0x1000,
        pair            = 0x2000,
        map             = 0x4000,

        wrapper_mask    = 0xf000,

        ENABLE_BITMASK_OPERATORS()
    };

    template <class T> struct is_base_type : std::false_type { };

    template<class T>
    inline constexpr bool is_base_type_v = is_base_type<T>::value;

    template<class T> using is_base_type_t = is_base_type<T>::type;

    template <class T>
    requires (std::is_integral_v<T> || std::is_floating_point_v<T> || is_string_v<T>)
    struct is_base_type<T> : std::true_type { };


    // convert to inner type
    template<class T>
    struct to_inner : std::integral_constant<value_t, value_t::unsupported> { };

    template<class T>
    inline constexpr value_t to_inner_v = to_inner<T>::value;

    template<class T> using to_inner_t = to_inner<T>::type;

    template<>
    struct to_inner<bool> : std::integral_constant<value_t, value_t::boolean> { using type = bool; };

    template<class T>
    requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    struct to_inner<T> : std::integral_constant<value_t, value_t::integer> { using type = int64_t; };

    template<std::floating_point T>
    struct to_inner<T> : std::integral_constant<value_t, value_t::floating> { using type = double; };

    template<String T>
    struct to_inner<T> : std::integral_constant<value_t, value_t::string> { using type = std::string; };

    // inner pair traits
    template<class, class _ = void> struct is_inner_pair : public std::false_type {};

    template<class T>
    inline constexpr bool is_inner_pair_v = is_inner_pair<T>::value;

    template <class T>
    struct is_inner_pair<T, std::enable_if_t<is_pair_v<T> &&
                                             is_base_type_v<typename T::first_type> &&
                                             is_base_type_v<typename T::second_type>>>
            : std::true_type {};

    // inner vector traits
    template<class, class _ = void> struct maybe_inner_vector : public std::false_type {};

    template<class T>
    inline constexpr bool maybe_inner_vector_v = maybe_inner_vector<T>::value;

    template <class T>
    struct maybe_inner_vector<T, std::enable_if_t<is_vector_v<T> &&
                                                  is_base_type_v<typename T::value_type>>>
            : std::true_type {};

    // inner map
    template<class, class _ = void> struct maybe_inner_map : public std::false_type {};

    template<class T>
    inline constexpr bool maybe_inner_map_v = maybe_inner_map<T>::value;

    template <class T>
    struct maybe_inner_map<T, std::enable_if_t<is_map_v<T> &&
                                               is_base_type_v<typename T::key_type> &&
                                               is_base_type_v<typename T::mapped_type>>>
            : std::true_type {};

    template<class T>
    requires (is_inner_pair_v<T>)
    struct to_inner<T> : std::integral_constant<
            value_t,
            value_t::pair | to_inner_v<typename T::first_type> | (to_inner_v<typename T::second_type> << 4)
    >
    {
        using type = std::pair<to_inner_t<typename T::first_type>, to_inner_t<typename T::second_type>>;
    };

    template<class T>
    requires (maybe_inner_vector_v<T>)
    struct to_inner<T> : std::integral_constant<value_t, value_t::vector | to_inner_v<typename T::value_type>>
    {
        using type = std::vector<to_inner_t<typename T::value_type>>;
    };

    template<class T>
    requires (maybe_inner_map_v<T>)
    struct to_inner<T>
            : std::integral_constant<
                    value_t,
                    value_t::map | to_inner_v<typename T::key_type> | (to_inner_v<typename T::mapped_type> << 4)
            >
    {
        using type = std::map<to_inner_t<typename T::key_type>, to_inner_t<typename T::mapped_type>>;
    };

    // supported
    template<class T>
    concept Supported = requires { to_inner_v<T> != value_t::unsupported; };


    // cast value to inner type value
    template<class T> requires (std::is_integral_v<T> || std::is_floating_point_v<T> || is_inner_pair_v<T>)
    to_inner_t<T> value_cast(T v)
    {
        return static_cast<to_inner_t<T>>(v);
    }

    template<class T> requires (is_string_v<T>)
    to_inner_t<T> value_cast(const T& v)
    {
        return std::string(v);
    }

    template<class T> requires (maybe_inner_vector_v<T> || maybe_inner_map_v<T>)
    to_inner_t<T> value_cast(const T& v)
    {
        return to_inner_t<T>{v.begin(), v.end()};
    }

    struct arg_t
    {
        value_t type{};
        std::any value{ std::nullopt };
        std::string help{};
        bool declared{};
        bool is_default{};
    };

    class parser
    {
    public:
        explicit parser(std::string desc = {}, bool ul = true) : desc_(std::move(desc)), unlimited_(ul) { }

        static std::optional<std::string> parse_key(const std::string& str)
        {
            std::smatch keys;

            if (std::regex_match(str, keys, std::regex("-{1,2}([a-zA-Z][a-zA-Z0-9]*)"))) {
                if (keys.size() == 2) {
                    return keys[1];
                }
            }
            return std::nullopt;
        }

        template<Supported T>
        void add(const char * opt, const T& value = {}, const std::string& help = {})
        {
            std::optional<std::string> key{};
            if (!(key = parse_key(opt))) {
                LOG(FATAL) << "invalid key '" << opt << "'";
            }

            args_[key.value()] = {
                    .type=to_inner_v<T>,
                    .value=value_cast(value),
                    .help=help,
                    .declared=true,
                    .is_default=true
            };
        }


        template<Supported T>
        std::optional<T> get(const std::string& key)
        {
            if(args_.contains(key)) {
                if (args_[key].value.has_value() && (args_[key].type == to_inner_v<T>)) {
                    return std::any_cast<T>(args_[key].value);
                }
            }
            return std::nullopt;
        }

        template<Supported T>
        T get(const std::string& key, const T& dft)
        {
            return get<T>(key).value_or(dft);
        }

        static std::optional<std::pair<std::string, std::string>> parse_pair(const std::string& str)
        {
            std::smatch keys;

            if (std::regex_match(str, keys, std::regex("([a-zA-Z0-9]+):([a-zA-Z0-9]+)"))) {
                if (keys.size() == 3) {
                    return std::pair{ keys[1], keys[2] };
                }
            }
            return std::nullopt;
        }

        static bool b(const std::string& s) { return s == "1" || s == "true" || s == "on"; }
        static int64_t i(const std::string& s) { return std::stoi(s); }
        static double d(const std::string& s) { return std::stod(s); }

        void set_pair(const std::string& key, const std::string& value)
        {
            auto pv = parse_pair(value);
            LOG(WARNING) << "invalid pair value : " << value;

            if (pv.has_value()) {
                auto& [first, second] = pv.value();

                switch (args_[key].type & ~value_t::wrapper_mask) {
                    using enum value_t;
                    case boolean | boolean << 4:    args_[key].value = std::pair{b(first), b(second)}; break;
                    case integer | integer << 4:    args_[key].value = std::pair{i(first), i(second)}; break;
                    case floating | floating << 4:  args_[key].value = std::pair{d(first), d(second)}; break;
                    case string | string << 4:      args_[key].value = std::pair{first, second}; break;

                    case boolean | integer << 4:    args_[key].value = std::pair{b(first), i(second)}; break;
                    case integer | boolean << 4:    args_[key].value = std::pair{i(first), b(second)}; break;

                    case boolean | floating << 4:   args_[key].value = std::pair{b(first), d(second)}; break;
                    case floating | boolean << 4:   args_[key].value = std::pair{d(first), b(second)}; break;

                    case boolean | string << 4:     args_[key].value = std::pair{b(first), second}; break;
                    case string | boolean << 4:     args_[key].value = std::pair{first, b(second)}; break;

                    case integer | floating << 4:   args_[key].value = std::pair{i(first), d(second)}; break;
                    case floating | integer << 4:   args_[key].value = std::pair{d(first), i(second)}; break;

                    case integer | string << 4:     args_[key].value = std::pair{i(first), second}; break;
                    case string | integer << 4:     args_[key].value = std::pair{first, i(second)}; break;

                    case floating | string << 4:    args_[key].value = std::pair{d(first), second}; break;
                    case string | floating << 4:    args_[key].value = std::pair{first, d(second)}; break;
                }
            }
        }

        void set_map(const std::string& key, const std::string& value)
        {
            auto pv = parse_pair(value);
            LOG(WARNING) << "invalid pair value : " << value;

            if (pv.has_value()) {
                auto& [first, second] = pv.value();

                if (args_[key].value.has_value()) {
                    switch (args_[key].type & ~value_t::wrapper_mask) {
                        using enum value_t;
                        case boolean | boolean << 4:    std::any_cast<std::map<bool, bool>&>(args_[key].value)[b(first)] = b(second); break;
                        case integer | integer << 4:    std::any_cast<std::map<int64_t, int64_t>&>(args_[key].value)[i(first)] = i(second); break;
                        case floating | floating << 4:  std::any_cast<std::map<double, double>&>(args_[key].value)[d(first)] = d(second); break;
                        case string | string << 4:      std::any_cast<std::map<std::string, std::string>&>(args_[key].value)[first] = second; break;

                        case boolean | integer << 4:    std::any_cast<std::map<bool, int64_t>&>(args_[key].value)[b(first)] = i(second); break;
                        case integer | boolean << 4:    std::any_cast<std::map<int64_t, bool>&>(args_[key].value)[i(first)] = b(second); break;

                        case boolean | floating << 4:   std::any_cast<std::map<bool, double>&>(args_[key].value)[b(first)] = d(second); break;
                        case floating | boolean << 4:   std::any_cast<std::map<double, bool>&>(args_[key].value)[d(first)] = b(second); break;

                        case boolean | string << 4:     std::any_cast<std::map<bool, std::string>&>(args_[key].value)[b(first)] = second; break;
                        case string | boolean << 4:     std::any_cast<std::map<std::string, bool>&>(args_[key].value)[first] = b(second); break;

                        case integer | floating << 4:   std::any_cast<std::map<int64_t, double>&>(args_[key].value)[i(first)] = d(second); break;
                        case floating | integer << 4:   std::any_cast<std::map<double, int64_t>&>(args_[key].value)[d(first)] = i(second); break;

                        case integer | string << 4:     std::any_cast<std::map<int64_t, std::string>&>(args_[key].value)[i(first)] = second; break;
                        case string | integer << 4:     std::any_cast<std::map<std::string, int64_t>&>(args_[key].value)[first] = i(second); break;

                        case floating | string << 4:    std::any_cast<std::map<double, std::string>&>(args_[key].value)[d(first)] = second; break;
                        case string | floating << 4:    std::any_cast<std::map<std::string, double>&>(args_[key].value)[first] = d(second); break;
                    }
                }
                else {
                    switch (args_[key].type & ~value_t::wrapper_mask) {
                        using enum value_t;
                        case boolean | boolean << 4:    args_[key].value = std::map<bool, bool>{{b(first), b(second)}}; break;
                        case integer | integer << 4:    args_[key].value = std::map<int64_t, int64_t>{{i(first), i(second)}}; break;
                        case floating | floating << 4:  args_[key].value = std::map<double, double>{{d(first), d(second)}}; break;
                        case string | string << 4:      args_[key].value = std::map<std::string, std::string>{{first, second}}; break;

                        case boolean | integer << 4:    args_[key].value = std::map<bool, int64_t>{{b(first), i(second)}}; break;
                        case integer | boolean << 4:    args_[key].value = std::map<int64_t, bool>{{i(first), b(second)}}; break;

                        case boolean | floating << 4:   args_[key].value = std::map<bool, double>{{b(first), d(second)}}; break;
                        case floating | boolean << 4:   args_[key].value = std::map<double, bool>{{d(first), b(second)}}; break;

                        case boolean | string << 4:     args_[key].value = std::map<bool, std::string>{{b(first), second}}; break;
                        case string | boolean << 4:     args_[key].value = std::map<std::string, bool>{{first, b(second)}}; break;

                        case integer | floating << 4:   args_[key].value = std::map<int64_t, double>{{i(first), d(second)}}; break;
                        case floating | integer << 4:   args_[key].value = std::map<double, int64_t>{{d(first), i(second)}}; break;

                        case integer | string << 4:     args_[key].value = std::map<int64_t, std::string>{{i(first), second}}; break;
                        case string | integer << 4:     args_[key].value = std::map<std::string, int64_t>{{first, i(second)}}; break;

                        case floating | string << 4:    args_[key].value = std::map<double, std::string>{{d(first), second}}; break;
                        case string | floating << 4:    args_[key].value = std::map<std::string, double>{{first, d(second)}}; break;
                    }
                }
            }
        }

        void set(const std::string& key, const std::string& value)
        {
            switch (args_[key].type) {
                case value_t::boolean:  args_[key].value = b(value); break;
                case value_t::floating: args_[key].value = d(value); break;
                case value_t::integer:  args_[key].value = i(value); break;
                case value_t::string:   args_[key].value = value;       break;

                case value_t::vector | value_t::boolean:
                    if (!args_[key].value.has_value())
                        args_[key].value = std::vector<bool>{ b(value) };
                    else
                        std::any_cast<std::vector<bool>&>(args_[key].value).emplace_back(b(value));
                    break;

                case value_t::vector | value_t::integer:
                    if (!args_[key].value.has_value())
                        args_[key].value = std::vector<int64_t>{i(value)};
                    else
                        std::any_cast<std::vector<int64_t>&>(args_[key].value).emplace_back(i(value));
                    break;

                case value_t::vector | value_t::floating:
                    if (!args_[key].value.has_value())
                        args_[key].value = std::vector<double>{ d(value) };
                    else
                        std::any_cast<std::vector<double>&>(args_[key].value).emplace_back(d(value));
                    break;

                case value_t::vector | value_t::string:
                    if (!args_[key].value.has_value())
                        args_[key].value = std::vector<std::string>{value };
                    else
                        std::any_cast<std::vector<std::string>&>(args_[key].value).emplace_back(value);
                    break;

                default:
                    if ((args_[key].type & value_t::wrapper_mask) == value_t::pair) {
                        set_pair(key, value);
                    }
                    else if ((args_[key].type & value_t::wrapper_mask) == value_t::map) {
                        set_map(key, value);
                    }
                    else {
                        LOG(ERROR) << "unknown type for '[" << key << "] " << value << "'";
                    }
                    break;
            }
        }

        int parse(int argc, char *argv[])
        {
            if (argc < 2) return -1;

            std::optional<std::string> key{};
            for (int i = 1; i < argc && argv[i]; ++i) {
                std::string arg{ argv[i] };

                // keys
                if (arg.starts_with('-') && (key = parse_key(arg))) {
                    // ignore the undeclared key on limited mode
                    if (!args_.contains(key.value()) && !unlimited_) {
                        key = {};
                        LOG(ERROR) << "ignore the invalid key '" << arg << "' which is not declared.";
                        continue;
                    }

                    // clear the default value at the first time
                    if (args_.contains(key.value()) && args_[key.value()].is_default) {
                        args_[key.value()].value = {};
                        args_[key.value()].is_default = false;

                        // set value for boolean option while not offer a value
                        if ((i + 1 < argc && parse_key(argv[i+1]))) {
                            if (args_[key.value()].type == value_t::boolean) {
                                args_[key.value()].value = true;
                            }
                            else {
                                LOG(ERROR) << "not offer a value for key '"<< key.value() << "'.";
                            }
                        }

                        continue;
                    }

                    // assign default value for the undeclared key on unlimited mode
                    // the undeclared key can only be bool, std::string or promoted to std::vector<std::string>
                    if (!args_.contains(key.value()) && unlimited_) {
                        args_[key.value()] = (i + 1 < argc && parse_key(argv[i+1])) ?
                                             arg_t{ .type=value_t::boolean, .value=true } :
                                             arg_t{ .type=value_t::string, .value={} };
                        continue;
                    }

                    continue;
                }

                // values
                // ignore the values for the invalid keys
                if (!key) {
                    LOG(WARNING) << "ignore the value '" << arg << "' since there is no valid key.";
                    continue;
                }

                // valid keys and values
                // promote std::string to std::vector<std::string>
                if (!args_[key.value()].declared && args_[key.value()].type == value_t::string && args_[key.value()].value.has_value()) {
                    args_[key.value()].type = value_t::vector | value_t::string;
                    args_[key.value()].value = std::vector<std::string>{ std::any_cast<std::string>(args_[key.value()].value) };
                }
                set(key.value(), arg);
            }

            return 0;
        }

        std::string help()
        {
            std::string str{};
            str += "\n usage: " + desc_ + "\n\nOptions: \n";
            for (const auto& [key, value] : args_) {
                if (value.declared) {
                    str += fmt::format("\t-{:24} {}\n", key, value.help);
                }
            }
            return str;
        }

    private:
        std::unordered_map<std::string, arg_t> args_{};
        std::string desc_;
        bool unlimited_{true };
    };
}

#endif // !FFMPEG_EXAMPLES_ARGS_PARSER_H