#include "console_string.h"
#include "fsl/fsl_defs.h"
#include "compute_shaders/compute_kernel.h"

struct KernelDef {
    ComputeKernel::KernelInfo info;
    String source;
};

class CodeBuilder {
private:
    struct CodeInfo {
        ConsoleString code;
        HashSet<StringName> refs;
        HashSet<StringName> local_vars; 
    };

    HashMap<StringName, KernelDef> kernel_defs;
    HashMap<StringName, CodeInfo> func_infos;
    HashSet<StringName> global_vars;
    HashMap<StringName, AST::ResourceNode> resources;
    HashMap<StringName, ComputeKernel::SpecializationConstant> spec_constants;
   
    // note: this system is dogshit and needs to be changed once we build FSLPrograms that can store something like a variable declaration with a default value
    // I'm not sure if conversion from fslAST to FSLProgram will happen before, during, or after validation, but once it happens at all, this stuff needs to go
    bool last_op_spec = false;
    StringName next_spec_const_name;
    ComputeKernel::SpecializationConstant next_spec_const;

    HashSet<uint32_t> used_constant_ids;
    ConsoleString string_builder;
    CodeBuilder(const AST::fslAST& ast);
    CodeInfo gen_var_decl(const AST::VariableDecl& var_decl);
    CodeInfo gen_operation(const AST::Operation &operation, bool is_first);
    CodeInfo gen_operation_list(const AST::OperationList &op_list);
    CodeInfo gen_expression(const AST::Expression &expression);
    CodeInfo gen_scope(const AST::ScopeNode &block);
    KernelDef gen_kernel(const AST::KernelNode &kernel);
    Pair<BufferInfo, String> gen_buffer(const String& buffer_name, const AST::BufferDef& buffer);
    Pair<ResourceInfo, String> gen_resource(const AST::ResourceNode &resource, uint32_t set, uint32_t binding);
public:
    static HashMap<StringName, KernelDef> get_kernels(const AST::fslAST& ast);
};

String to_original_type(const String &identifier);
String to_original_name(const String &name);
String gen_binding_code(const String& name, const String& bound_name);

String tex_to_glsl_name(const TextureType &tex_type);

Pair<TextureInfo, String> texture_to_glsl(const String &texture_name, const AST::TextureDef &texture);