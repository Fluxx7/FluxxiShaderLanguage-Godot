extends Node


func _ready() -> void:
	var fsl_file0 = FSLFile.from_file("fsl/fft_testing/fft.fsl")
	var fsl_file1 = FSLFile.from_file("fsl/tessendorf_funcs.fsl")
	var fsl_file2 = FSLFile.from_file("fsl/spectrums.fsl")
	var fsl_file3 = FSLFile.from_file("fsl/spreadings.fsl")
	print("ifftStage .glsl code:")
	print(fsl_file0.get_kernel("ifftStage"))
	print("ifftUnpack .glsl code:")
	print(fsl_file1.get_kernel("ifftUnpack"))
	print("tessendorfSpectrum .glsl code:")
	print(fsl_file2.get_kernel("tessendorfSpectrum"))
	print("abSpectrum .glsl code:")
	print(fsl_file2.get_kernel("abSpectrum"))
	print("no spreading .glsl code:")
	print(fsl_file3.get_kernel("noSpreading"))
