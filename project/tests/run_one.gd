extends SceneTree
# Loads one .fsl file through the FSL stack and prints machine-readable
# markers for run_fsl_tests.sh to evaluate. Not meant to be run by hand,
# but you can: godot --headless -s res://tests/run_one.gd -- res://path/to.fsl [kernels...]

func _init() -> void:
	var args := OS.get_cmdline_user_args()
	if args.is_empty():
		printerr("usage: godot --headless -s res://tests/run_one.gd -- <fsl path> [expected kernel names...]")
		quit(2)
		return
	if not ClassDB.class_exists("FSLFile"):
		printerr("FSL_TEST_NO_EXTENSION: FSLFile class not registered (gdextension failed to load?)")
		quit(3)
		return

	var fsl_path: String = args[0]
	var kernels := args.slice(1)

	print("FSL_TEST_BEGIN ", fsl_path)
	var file = FSLFile.from_file(fsl_path)
	print("FSL_TEST_LOADED")
	for k in kernels:
		var src: String = file.get_kernel_source(k)
		if src.is_empty():
			print("FSL_TEST_KERNEL_MISSING ", k)
		else:
			print("FSL_TEST_KERNEL_OK ", k)
	print("FSL_TEST_END")
	quit(0)
