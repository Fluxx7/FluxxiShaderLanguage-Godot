extends Node

var file_target := "validation_error/ve05_unknown_type.fsl"

func _ready() -> void:
	#var _compTest = FSLFile.from_file("res://tests/fsl/" + file_target)
	#var _cacheTest = FSLFile.from_file("res://tests/fsl/validation_error/ve25_type_no_name.fsl")
	#compTest.get_kernel("blah")
	#compTest.print_AST()
	var structTest = FSLFile.from_file("res://fsl/cbtrees.fsl")
	var structKernel = structTest.get_kernel("buildTris")
	structKernel.print_info()
	
	
func _process(_delta: float) -> void:
	pass
