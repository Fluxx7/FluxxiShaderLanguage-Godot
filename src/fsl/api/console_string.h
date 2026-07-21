#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include "godot_cpp/classes/rd_shader_source.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include <unordered_map>

#include "fsl/parsing/fsl_parser.h"
#include "fsl/fsl_defs.h"
#include "compute_shaders/compute_kernel.h"
#include "compute_shaders/compute_group.h"

using namespace godot;

class ConsoleString {
private:
    enum Indent {
        Increment,
        Decrement,
    };
    Vector<std::variant<String, Indent>> out_log;
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
        console_string.flush();
        if (console_string.out_log.is_empty()) {
            return;
        }
        if (const String* first_string = std::get_if<String>(&(console_string.out_log[0]))) {
            curr_string += *first_string;
            console_string.out_log.remove_at(0);
            if (console_string.out_log.is_empty()) {
                return;
            }
        }
        flush();
        out_log.append_array(std::move(console_string.out_log));
    }
    void add(const ConsoleString& console_string) {
        Vector<std::variant<String, Indent>> string_list = console_string.out_log;
        if (!console_string.curr_string.is_empty()) {
            string_list.append(console_string.curr_string);
        }
        if (string_list.is_empty()) {
            return;
        }
        if (const String* first_string = std::get_if<String>(&(string_list[0]))) {
            curr_string += *first_string;
            string_list.remove_at(0);
        }
        flush();
        out_log.append_array(std::move(string_list));
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
            std::visit(overload{
                [&](String text) {
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
                }
            }, str_or_int);
        }
        String indents = "";
        for (auto i = 0; i < curr_indent; i++) {
            indents += "\t";
        }
        output += vformat("%s%s\n", indents, curr_string);
        return output;
    }
    void print() const {
        uint32_t curr_indent = 0; 
        for (const auto& str_or_int : out_log) {
            std::visit(overload{
                [&](String text) {
                    String indents = "";
                    for (auto i = 0; i < curr_indent; i++) {
                        indents += "\t";
                    }
                    printvf("%s%s", indents, text);
                },
                [&](Indent indent) {
                    if (indent == Increment) {
                        curr_indent += 1;
                    } else {
                        curr_indent -= 1;
                    }   
                }
            }, str_or_int);
        }
        String indents = "";
        for (auto i = 0; i < curr_indent; i++) {
            indents += "\t";
        }
        printvf("%s%s", indents, curr_string);
    }
};