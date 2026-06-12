#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/classes/rd_shader_source.hpp"
#include <unordered_map>

#include "fsl/parsing/fsl_parser.h"

using namespace godot;

class FSLFile : public RefCounted {
    GDCLASS(FSLFile, RefCounted)
private:
	FSLFile();
protected:
	static void _bind_methods();
    
    void load_shader();
    

    unsigned long prev_transpile_time;
    HashMap<StringName, String> kernel_sources;  
    StringName path;
    fslAST currAst;
public:

    FSLFile(String file_path);

	~FSLFile() override = default;
    String get_kernel(StringName kernel_name);
    static Ref<FSLFile> from_file(String file_path);
    
    void test();
};