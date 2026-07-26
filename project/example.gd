extends Node

@export var baseSpectrumRect: TextureRect
@export var currSpectrumRect: TextureRect
@export var gaussianRect: TextureRect
@export var use_tma := false

var baseSpectrumTex2DRD := Texture2DRD.new()
var currSpectrumTex2DRD := Texture2DRD.new()
var gaussianTex2DRD := Texture2DRD.new()

var spectrum_file = FSLFile.from_file("fsl/spectrums.fsl")
var spreading_file = FSLFile.from_file("fsl/spreadings.fsl")
var tess_file = FSLFile.from_file("fsl/tessendorf_funcs.fsl")
var fft_file = FSLFile.from_file("fsl/fft_testing/fft.fsl")

@onready var spectrums := spectrum_file.get_kernel_group()

#@onready var tessendorf_spectrum = spectrums.get_kernel("tessendorfSpectrum")
#@onready var no_spreading := spreading_file.get_kernel("noSpreading")
#@onready var update_spectrum := tess_file.get_kernel("updateSpectrum")

var N: int = 256
var tileLength: float = 250.0;

func _ready() -> void:
	tess_file.print_AST()
	#fft_file.print_AST()
	baseSpectrumRect.texture = baseSpectrumTex2DRD
	#currSpectrumRect.texture = currSpectrumTex2DRD
	#gaussianRect.texture = gaussianTex2DRD
	#var spectrum_kernel = spectrums.get_kernel("tmaSpectrum") if use_tma \
	#else spectrums.get_kernel("tessendorfSpectrum")
	#
	#var spectrumMap := spectrums.get_texture_2d("spectrumMap")
	#spectrumMap.set_texture(N, N)
	#no_spreading.assign_resource(spectrumMap, "spectrumMap")
	#
	#var baseSpectrum := no_spreading.get_texture_2d("baseSpectrum")
	#baseSpectrum.set_texture(N, N)
	#baseSpectrum.connect_and_call(func (rid):
			#baseSpectrumTex2DRD.texture_rd_rid = rid) 
		#
	#update_spectrum.assign_resource(baseSpectrum, "baseSpectrum")
#
	#update_spectrum.get_texture_2d("spectrumTexture").set_texture(N, N)
	#update_spectrum.get_storage_buffer("fft1_buffers").set_unsized_element_count(N * N)
	#update_spectrum.get_storage_buffer("fft2_buffers").set_unsized_element_count(N * N)
	#update_spectrum.get_texture_2d("spectrumTexture").connect_and_call(func (rid):
			#currSpectrumTex2DRD.texture_rd_rid = rid)
#
	#var spectrumCoefficients := no_spreading.get_texture_2d("spectrumCoefficients")
	#var rng = RandomNumberGenerator.new();
	#var gaussian = Image.create_empty(N, N, false, Image.Format.FORMAT_RGBAF);
	#for u in range(N):
		#for v in range(N):
			#var new_pixel = Color();
			#new_pixel.r = rng.randfn();
			#new_pixel.g = rng.randfn();
			#new_pixel.a = 1.0;
			#gaussian.set_pixel(u,v, new_pixel);
	#
	#spectrumCoefficients.set_texture(N, N, gaussian)
	#spectrumCoefficients.connect_and_call(
		#func (rid):
			#gaussianTex2DRD.texture_rd_rid = rid
			#)
	#
	#var compute_plan = ComputePlan.make_new(RenderingServer.get_rendering_device())
	#
	#
	#
	#var spectrum_push_constants : Dictionary[StringName, Variant] = {}
	#if use_tma:
		#spectrum_push_constants = {
			#"texSize": N,
			#"windSpeed": 17.5,
			#"fetch": 10000.0,
			#"depth": 100.0,
			#"tile_length": 250.0
		#}
	#else:
		#spectrum_push_constants = {
			#"texSize": N,
			#"windSpeed": 17.5,
			#"A": 0.02,
			#"tile_length": 250.0
		#}
		#
	#compute_plan.add_kernel(spectrum_kernel, N, N, 1, spectrum_push_constants)
	#compute_plan.add_barrier()
	#compute_plan.add_kernel(no_spreading, N, N, 1, {
		#"tile_length": tileLength,
		#"texSize": N,
		#"windDirection": deg_to_rad(0.0),
	#})
	#compute_plan.dispatch()

var time = 0.0
func _process(delta: float) -> void:
	
	time += delta
	#update_spectrum.dispatch(N, N, 1, {
		#"texSize": N,
		#"time": time,
		#"tile_length": 125,
		#"depth": 1000.0
	#})
