#include "console_string.h"

struct KernelDef {
    ComputeKernel::KernelInfo info;
    String source;
};

class CodeBuilder {
private:
    HashMap<StringName, KernelDef> kernel_defs;
    ConsoleString string_builder;
    CodeBuilder(const fslAST& ast);
    ConsoleString gen_statement(const Statement &statement, const HashMap<StringName, String> &renames = {});
    ConsoleString gen_operation(const Operation &operation, const HashMap<StringName, String> &renames = {});
    ConsoleString gen_expression(const Expression &expression, const HashMap<StringName, String> &renames = {});
    ConsoleString gen_scope(const ScopeNode &block, const HashMap<StringName, String> &renames = {});
    KernelDef gen_kernel(const KernelNode &kernel, const HashMap<StringName, ResourceNode> &resources);
    Pair<ResourceInfo, String> gen_resource(const ResourceNode &resource, uint32_t set, uint32_t binding);
public:
    static HashMap<StringName, KernelDef> get_kernels(const fslAST& ast);
};

String to_original_type(const String &identifier);
String to_original_name(const String &name);

String tex_to_glsl_name(const TextureType &tex_type);

Pair<BufferInfo, String> buffer_to_glsl(const String &buffer_name, const BufferDef &buffer);
Pair<TextureInfo, String> texture_to_glsl(const String &texture_name, const TextureDef &texture);