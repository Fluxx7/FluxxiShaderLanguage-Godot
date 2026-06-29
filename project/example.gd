extends Node

@export var baseSpectrumRect: TextureRect
@export var currSpectrumRect: TextureRect
@export var gaussianRect: TextureRect

# file path -> expected kernel names
const KERNEL_MANIFEST := {
	"fsl/fft_testing/fft.fsl": ["twiddleGen", "ifftStage", "ifftTranspose"],
	"fsl/spectrums.fsl": ["tessendorfSpectrum", "abSpectrum", "jonswapSpectrum", "tmaSpectrum"],
	"fsl/spreadings.fsl": ["noSpreading", "positiveCosSpreading"],
	"fsl/tessendorf_funcs.fsl": ["updateSpectrum", "ifftUnpack"],
}

var spectrum_file = FSLFile.from_file("fsl/spectrums.fsl")
var spreading_file = FSLFile.from_file("fsl/spreadings.fsl")
var tess_file = FSLFile.from_file("fsl/tessendorf_funcs.fsl")

@onready var tessendorf_spectrum = spectrum_file.get_kernel("tessendorfSpectrum")
@onready var no_spreading = spreading_file.get_kernel("noSpreading")
@onready var update_spectrum = tess_file.get_kernel("updateSpectrum")
var N: int = 256
var tileLength: float = 250.0;

func _ready() -> void:
	var baseSpectrum = tessendorf_spectrum.create_texture(N, N)
	var spectrumCoefficients = no_spreading.create_texture(N, N)
	var currSpectrum = update_spectrum.create_texture(N, N)
	var fft1_buffer = update_spectrum.create_buffer(ComputeKernel.STORAGE, N * N * 16 * 2)
	var fft2_buffer = update_spectrum.create_buffer(ComputeKernel.STORAGE, N * N * 16 * 2)
	tessendorf_spectrum.bind_texture_callback(baseSpectrum, 
		func (tex_rd):
			baseSpectrumRect.texture = tex_rd
			)
	tessendorf_spectrum.bind_texture_callback(currSpectrum, 
		func (tex_rd):
			currSpectrumRect.texture = tex_rd
			)
	var rng = RandomNumberGenerator.new();
	var gaussian = Image.create_empty(N, N, false, Image.Format.FORMAT_RGBAF);
	for u in range(N):
		for v in range(N):
			var new_pixel = Color();
			new_pixel.r = rng.randfn();
			new_pixel.g = rng.randfn();
			new_pixel.a = 1.0;
			gaussian.set_pixel(u,v, new_pixel);
	
	no_spreading.set_texture(spectrumCoefficients, N, N, gaussian)
	no_spreading.bind_texture_callback(spectrumCoefficients,
	func (texRd):
		gaussianRect.texture = texRd)
	
	
	tessendorf_spectrum.assign_resource(baseSpectrum, "baseSpectrum")
	no_spreading.assign_resource(baseSpectrum, "baseSpectrum")
	no_spreading.assign_resource(spectrumCoefficients, "spectrumCoefficients")
	
	update_spectrum.assign_resource(baseSpectrum, "baseSpectrum")
	update_spectrum.assign_resource(currSpectrum, "spectrumTexture")
	update_spectrum.assign_resource(fft1_buffer, "fft1_buffers")
	update_spectrum.assign_resource(fft2_buffer, "fft2_buffers")
	
	print(spectrum_file.get_kernel_source("tessendorfSpectrum"))

	tessendorf_spectrum.dispatch(N, N, 1, {
		"texSize": N,
		"windSpeed": 17.0,
		"A": 0.02,
		"tile_length": tileLength,
	})
	no_spreading.dispatch(N, N, 1, {
		"texSize": N,
		"tile_length": tileLength,
		"windDirection": deg_to_rad(0.0),
		})

var time = 0.0
func _process(delta: float) -> void:
	
	time += delta
	update_spectrum.dispatch(N, N, 1, {
		"texSize": N,
		"time": time,
		"tile_length": 125,
		"depth": 1000.0
	})
