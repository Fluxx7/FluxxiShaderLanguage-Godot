#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
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
#include "code_builder.h"

using namespace godot;

class FSLFile : public RefCounted {
    GDCLASS(FSLFile, RefCounted)
private:
	FSLFile() = default;
protected:
	static void _bind_methods();
    
    void load_shader();
    

    unsigned long prev_transpile_time;
    
    HashMap<StringName, KernelDef> compute_kernels;
    StringName path;
    fslAST currAst;
public:
    FSLFile(String file_path);

	~FSLFile() override = default;
    String get_kernel_source(StringName kernel_name);
    Ref<ComputeKernel> get_kernel(StringName kernel_name, RenderingDevice *rd);
    Ref<ComputeGroup> get_kernel_group(RenderingDevice *rd = nullptr);


    static Ref<FSLFile> from_file(String file_path);
    
    void print_AST();
    void test();
};