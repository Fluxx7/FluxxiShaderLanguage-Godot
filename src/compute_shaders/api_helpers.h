#pragma once
#include "godot_imports.h"
#include "std_imports.h"
#include "fsl/fsl_defs.h"

PackedByteArray fsl_type_to_bytes(const FSLType& fsl_type, Variant value, BufferFormat format = STD430);

Variant bytes_to_variant(const FSLType& fsl_type, PackedByteArray bytes, BufferFormat format = STD430);