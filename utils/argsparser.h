#ifndef FFMPEG_EXAMPLES_ARGS_PARSER_H
#define FFMPEG_EXAMPLES_ARGS_PARSER_H

#include <optional>
#include <vector>
#include <string>
#include <unordered_map>
#include <any>
#include <regex>
#include "logging.h"
#include "fmt/format.h"

namespace args
{
    template<typename T>
    struct is_string
            : public std::disjunction<
                    std::is_same<char *, typename std::decay_t<T>>,
                    std::is_same<const char *, typename std::decay_t<T>>,
                    std::is_same<std::string, typename std::decay_t<T>>
            > {
    };

    template<typename T>
    inline constexpr bool is_string_v = is_string<T>::value;

    template<class, class _ = void>
    struct is_vector : public std::false_type {};

    template<typename T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    template <class T>
    struct is_vector<T, typename std::enable_if<std::is_same<T, std::vector<typename T::value_type, typename T::allocator_type>>::value>::type>
            : public std::true_type {};

    enum value_t : uint8_t
    {
        unsupported = 0x00,

        string = 0x01,
        boolean = 0x02,
        floating = 0x03,
        integer = 0x04,

        vector = 0x10,
    };

    template<class T, class _ = void>
    struct to_inner {
        static constexpr value_t value = value_t::unsupported;
        using value_type = value_t;
        using type = T;
    };

    template<class T>
    inline constexpr value_t to_inner_v = to_inner<T>::value;

    template<class T>
    using to_inner_t = to_inner<T>::type;

    template<class T>
    struct to_inner<T, typename std::enable_if<std::is_same<T, bool>::value>::type>
    {
        static constexpr value_t value = value_t::boolean;
        using value_type = value_t;
        using type = bool;
    };

    template<class T>
    struct to_inner<T, typename std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
    {
        static constexpr value_t value = value_t::integer;
        using value_type = value_t;
        using type = int64_t;
    };

    template<class T>
    struct to_inner<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
    {
        static constexpr value_t value = value_t::floating;
        using value_type = value_t;
        using type = double;
    };

    template<class T>
    struct to_inner<T, typename std::enable_if_t<is_string_v<T>>>
    {
        static constexpr value_t value = value_t::string;
        using value_type = value_t;
        using type = std::string;
    };

    template<class T>
    struct to_inner<T, typename std::enable_if_t<is_vector_v<T>>>
    {
        static constexpr value_t value = (value_t)(value_t::vector | to_inner_v<typename T::value_type>);
        using value_type = value_t;
        using type = std::vector<typename to_inner<typename T::value_type>::type>;
    };

    template<value_t T>
    struct to_type
    {
        static constexpr value_t value = value_t::unsupported;
        using value_type = value_t;
        using type = void;
    };

    template<>
    struct to_type<value_t::boolean>
    {
        static constexpr value_t value = value_t::boolean;
        using value_type = value_t;
        using type = bool;
    };

    template<>
    struct to_type<value_t::integer>
    {
        static constexpr value_t value = value_t::integer;
        using value_type = value_t;
        using type = int64_t;
    };

    template<>
    struct to_type<value_t::floating>
    {
        static constexpr value_t value = value_t::floating;
        using value_type = value_t;
        using type = double;
    };

    template<>
    struct to_type<value_t::string>
    {
        static constexpr value_t value = value_t::string;
        using value_type = value_t;
        using type = std::string;
    };

    template<>
    struct to_type<static_cast<value_t>(value_t::vector | value_t::boolean)>
    {
        static constexpr value_t value = static_cast<value_t>(value_t::vector | value_t::boolean);
        using value_type = value_t;
        using type = std::vector<bool>;
    };

    template<>
    struct to_type<static_cast<value_t>(value_t::vector | value_t::integer)>
    {
        static constexpr value_t value = static_cast<value_t>(value_t::vector | value_t::integer);
        using value_type = value_t;
        using type = std::vector<int64_t>;
    };

    template<>
    struct to_type<static_cast<value_t>(value_t::vector | value_t::floating)>
    {
        static constexpr value_t value = static_cast<value_t>(value_t::vector | value_t::floating);
        using value_type = value_t;
        using type = std::vector<double>;
    };

    template<>
    struct to_type<static_cast<value_t>(value_t::vector | value_t::string)>
    {
        static constexpr value_t value = static_cast<value_t>(value_t::vector | value_t::string);
        using value_type = value_t;
        using type = std::vector<std::string>;
    };

    template<class R>
    bool value_cast(const R& v, std::enable_if_t<std::is_same_v<R, bool>, bool> = {})
    {
        return v;
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == value_t::integer, to_inner_t<R>> = {})
    {
        return static_cast<int64_t>(v);
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == value_t::floating, to_inner_t<R>> = {})
    {
        return static_cast<double>(v);
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == value_t::string, to_inner_t<R>> = {})
    {
        return std::string(v);
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == (value_t::vector | value_t::boolean), to_inner_t<R>> = {})
    {
        return v;
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == (value_t::vector | value_t::integer), to_inner_t<R>> = {})
    {
        return to_inner_t<R>{v.begin(), v.end()};
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == (value_t::vector | value_t::floating), to_inner_t<R>> = {})
    {
        return to_inner_t<R>{v.begin(), v.end()};
    }

    template<class R>
    to_inner_t<R> value_cast(const R& v, std::enable_if_t<to_inner_v<R> == (value_t::vector | value_t::string), to_inner_t<R>> = {})
    {
        return to_inner_t<R>{v.begin(), v.end()};
    }

    struct arg_t
    {
        value_t type{};
        std::any value{ std::nullopt };
        std::string help{};
        bool declared{};
    };

    class parser
    {
    public:
        explicit parser(bool ul = true) : unlimited(ul) { }

        static std::optional<std::string> parse_key(const std::string& str)
        {
            std::smatch keys;

            if (std::regex_match(str, keys, std::regex("-{1,}([a-zA-z]+)"))) {
                if (keys.size() == 2) {
                    return keys[1];
                }
            }
            return std::nullopt;
        }

        template<class ValueType>
        void add(const char * opt, const ValueType& value = {}, const std::string& desc = {})
        {
            static_assert(to_inner_v < ValueType > != value_t::unsupported, "unsupported type.");

            std::optional<std::string> key{};
            if (!(key = parse_key(opt))) {
                LOG(FATAL) << "invalid key : " << opt << '.';
            }

            args_[key.value()] = {to_inner_v < ValueType >, value_cast(value), { desc }, true };
        }


        template<class ValueType>
        std::optional<ValueType> get(const std::string& key)
        {
            if(args_.contains(key)) {
                if (args_[key].value.has_value()) {
                    return std::any_cast<ValueType>(args_[key].value);
                }
                LOG(INFO) << "no value for '" << key << "'";
                return std::nullopt;
            }

            LOG(WARNING) << "doest not have the key '" << key << "'";
            return std::nullopt;
        }

        void set(const std::string& key, const std::string& value)
        {
            switch (args_[key].type) {
                case value_t::unsupported:
                    LOG(ERROR) << "can not set value for an unsupported type.";
                    break;

                case value_t::boolean:
                    args_[key].value = (value == "1" || value == "true");
                    break;

                case value_t::floating:
                    args_[key].value = std::stod(value);
                    break;

                case value_t::integer:
                    args_[key].value = std::stoi(value);
                    break;

                case value_t::string:
                    args_[key].value = value;
                    break;

                default:
                    if ((uint8_t)args_[key].type & (uint8_t)value_t::vector) {
                        switch ((value_t)((uint8_t)args_[key].type & ~(uint8_t)value_t::vector))
                        {
                            case value_t::boolean:
                                std::any_cast<std::vector<bool>&>(args_[key].value).push_back((value == "1" || value == "true"));
                                break;

                            case value_t::floating:
                                std::any_cast<std::vector<double>&>(args_[key].value).push_back(std::stod(value));
                                break;

                            case value_t::integer:
                                std::any_cast<std::vector<int64_t>&>(args_[key].value).push_back(std::stoi(value));
                                break;

                            default:
                                std::any_cast<std::vector<std::string>&>(args_[key].value).push_back(value);
                                break;
                        }

                        break;
                    }
                    LOG(ERROR) << "unknown type.";
                    break;
            }
        }

        void clear(const std::string& key)
        {
            switch (args_[key].type) {
                case value_t::unsupported:
                    LOG(ERROR) << "can not set value for an unsupported type.";
                    break;

                case value_t::boolean:
                    args_[key].value = false;
                    break;

                case value_t::floating:
                    args_[key].value = static_cast<int64_t>(0);
                    break;

                case value_t::integer:
                    args_[key].value = static_cast<double>(0);
                    break;

                case value_t::string:
                    args_[key].value = std::string{};
                    break;

                default:
                    if ((uint8_t)args_[key].type & (uint8_t)value_t::vector) {
                        switch ((value_t)((uint8_t)args_[key].type & ~(uint8_t)value_t::vector))
                        {
                            case value_t::boolean:
                                std::any_cast<std::vector<bool>&>(args_[key].value).clear();
                                break;

                            case value_t::floating:
                                std::any_cast<std::vector<double>&>(args_[key].value).clear();
                                break;

                            case value_t::integer:
                                std::any_cast<std::vector<int64_t>&>(args_[key].value).clear();
                                break;

                            default:
                                std::any_cast<std::vector<std::string>&>(args_[key].value).clear();
                                break;
                        }

                        break;
                    }
                    LOG(ERROR) << "unknown type.";
                    break;
            }
        }

        int parse(int argc, char *argv[])
        {
            if (argc < 2) return -1;

            std::optional<std::string> key{};
            for (int i = 1; i < argc; ++i) {
                LOG(INFO) << fmt::format("{:3d} / {}: {}", i, argc, argv[i]);

                std::string arg{ argv[i] };
                if (arg[0] == '-') { // is key
                    if (!(key = parse_key(arg)) || (!args_.contains(key.value()) && !unlimited)) {
                        LOG(ERROR) << "invalid argument '" << arg << "' as a key.";
                    }

                    if (args_.contains(key.value()))
                        clear(key.value());
                    continue;
                }

                if (!key) {
                    LOG(WARNING) << "ignore the argument '" << arg << "' cased by the front invalid key argument.";
                    continue;
                }

                if (args_.contains(key.value()) && args_[key.value()].declared) {
                    set(key.value(), arg);
                }
                else if (unlimited) {
                    if (!args_[key.value()].declared && args_[key.value()].type == value_t::unsupported) {
                        args_[key.value()] = arg_t{ value_t::string, arg,  "", false };
                    }
                    else if (!args_[key.value()].declared && args_[key.value()].type == value_t::string) {
                        args_[key.value()].type = static_cast<value_t>(value_t::vector | value_t::string);
                        args_[key.value()].value = std::vector{std::any_cast<std::string>(args_[key.value()].value)};
                        set(key.value(), arg);
                    }
                    else {
                        set(key.value(), arg);
                    }
                }
            }
        }

        std::string help()
        {
            std::string str{"\n"};
            for (const auto& [key, value] : args_) {
                if (value.declared) {
                    str += fmt::format(" -{:32} {}\n", key, value.help);
                }
            }
            return str;
        }

    private:
        std::unordered_map<std::string, arg_t> args_;
        bool unlimited{ true };
    };
}

#endif // !FFMPEG_EXAMPLES_ARGS_PARSER_H