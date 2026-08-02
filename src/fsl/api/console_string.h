#pragma once
#include "godot_imports.h"
#include "std_imports.h"

#include "fsl/parsing/fsl_parser.h"



class ConsoleString {
private:
    enum Indent {
        Increment,
        Decrement,
    };
    Vector<sumtype<String, Indent>> out_log;
    String curr_string;
public:
    template<typename... VarArgs>
    void add_line(const String &text, const VarArgs... vargs) {
        curr_string += vformat(text, vargs...);
        flush();
    }
    template<typename... VarArgs>
    void add(const String &text, const VarArgs... vargs) {
        curr_string += vformat(text, vargs...);
    }
    void add(ConsoleString&& console_string) {
        if (!console_string.out_log.is_empty()) {
            if(const String* first_string = get_if<String>(&console_string.out_log[0])) {
                curr_string += *first_string;
                console_string.out_log.remove_at(0);
            }
            flush();
            out_log.append_array(std::move(console_string.out_log));
            return;
        }
        if (!console_string.curr_string.is_empty()) {
            curr_string += console_string.curr_string;
        }
    }
    void add(const ConsoleString& console_string) {
        if (!console_string.out_log.is_empty()) {
            Vector<std::variant<String, Indent>> string_list = console_string.out_log;
            if(const String* first_string = get_if<String>(&string_list[0])) {
                curr_string += *first_string;
                string_list.remove_at(0);
            }
            flush();
            out_log.append_array(std::move(string_list));
        }
        if (!console_string.curr_string.is_empty()) {
            curr_string += console_string.curr_string;
        }
    }
    template<typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, ConsoleString>>>
    void add_line(T&& console_string) {
        add(std::forward<T>(console_string));
        flush();
    }

    void flush() {
        if (!curr_string.is_empty()) { 
            out_log.push_back(curr_string);
            curr_string = "";
        }
    }
    void indent(uint32_t amount = 1) {
        flush();
        for (auto i = 0; i < amount; i++) {
            out_log.push_back(Increment);
        }
    }
    void unindent(uint32_t amount = 1) {
        flush();
        for (auto i = 0; i < amount; i++) {
            out_log.push_back(Decrement);
        }
    }
    String get_output() const {
        String output = "";
        uint32_t curr_indent = 0; 

        for (const auto& str_or_int : out_log) {
            match(str_or_int, [&](String text) {
                    String indents = "";
                    for (auto i = 0; i < curr_indent; i++) {
                        indents += "\t";
                    }
                    output += vformat("%s%s\n", indents, text);
                },
                [&](Indent indent) {
                    if (indent == Increment) {
                        curr_indent += 1;
                    } else {
                        curr_indent -= 1;
                    }   
                });
        }

        String indents = "";
        for (auto i = 0; i < curr_indent; i++) {
            indents += "\t";
        }
        output += vformat("%s%s", indents, curr_string);
        return output;
    }
    void print() const {
        printvf(get_output());
    }
};