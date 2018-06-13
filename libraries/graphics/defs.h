//
//  defs.h
//  graphics
//
//  Created by Neil Singh on 2/5/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef DEFS_H
#define DEFS_H

// Buffer Creation Flags
#define GRAPHICS_BUFFER_STATIC				(1 << 1)
#define GRAPHICS_BUFFER_DYNAMIC				(1 << 2)
#define GRAPHICS_BUFFER_INDEXBUFFER			(1 << 3)
#define GRAPHICS_BUFFER_VERTEXBUFFER		(1 << 4)
#define GRAPHICS_BUFFER_TEXTURE				(1 << 5)
#define GRAPHICS_BUFFER_RENDERTARGET		(1 << 6)
#define GRAPHICS_BUFFER_WRITEONLY			(1 << 8)

// Clear flags
#define GRAPHICS_CLEAR_COLOR				(1 << 0)
#define GRAPHICS_CLEAR_DEPTH 				(1 << 1)
#define GRAPHICS_CLEAR_STENCIL				(1 << 2)

// Transform types
#define GRAPHICS_TRANSFORM_WORLD			1
#define GRAPHICS_TRANSFORM_VIEW 			2
#define GRAPHICS_TRANSFORM_PROJECTION		3
#define GRAPHICS_TRANSFORM_TEXTURE0			4
#define GRAPHICS_TRANSFORM_TEXTURE1			5
#define GRAPHICS_TRANSFORM_TEXTURE2			6
#define GRAPHICS_TRANSFORM_TEXTURE3			7
#define GRAPHICS_TRANSFORM_TEXTURE4			8
#define GRAPHICS_TRANSFORM_TEXTURE5			9
#define GRAPHICS_TRANSFORM_TEXTURE6			10
#define GRAPHICS_TRANSFORM_TEXTURE7			11
#define GRAPHICS_TRANSFORM_WORLD1			12
#define GRAPHICS_TRANSFORM_WORLD2			13
#define GRAPHICS_TRANSFORM_WORLD3			14

// Primitive types
#define GRAPHICS_PRIMITIVE_TRIANGLELIST		1
#define GRAPHICS_PRIMITIVE_POINTLIST		2
#define GRAPHICS_PRIMITIVE_LINELIST			3
#define GRAPHICS_PRIMITIVE_LINESTRIP		4
#define GRAPHICS_PRIMITIVE_TRIANGLESTRIP	5
#define GRAPHICS_PRIMITIVE_TRIANGLEFAN		6

// Types
#define GRAPHICS_TYPE_FLOAT1				0
#define GRAPHICS_TYPE_FLOAT2				1
#define GRAPHICS_TYPE_FLOAT3				2
#define GRAPHICS_TYPE_FLOAT4				3
#define GRAPHICS_TYPE_D3DCOLOR				4
#define GRAPHICS_TYPE_UBYTE4				5
#define GRAPHICS_TYPE_SHORT2				6
#define GRAPHICS_TYPE_SHORT4				7
#define GRAPHICS_TYPE_UBYTE4N				8
#define GRAPHICS_TYPE_SHORT2N				9
#define GRAPHICS_TYPE_SHORT4N				10
#define GRAPHICS_TYPE_USHORT2N				11
#define GRAPHICS_TYPE_USHORT4N				12
#define GRAPHICS_TYPE_UDEC3					13
#define GRAPHICS_TYPE_DEC3N					14
#define GRAPHICS_TYPE_FLOAT16_2				15
#define GRAPHICS_TYPE_FLOAT16_4				16

// Usage
#define GRAPHICS_USAGE_POSITION				0
#define GRAPHICS_USAGE_BLENDWEIGHT			1
#define GRAPHICS_USAGE_BLENDINDICES			2
#define GRAPHICS_USAGE_NORMAL				3
#define GRAPHICS_USAGE_PSIZE				4
#define GRAPHICS_USAGE_TEXCOORD				5
#define GRAPHICS_USAGE_TANGENT				6
#define GRAPHICS_USAGE_BINORMAL				7
#define GRAPHICS_USAGE_TESSFACTOR			8
#define GRAPHICS_USAGE_POSITIONT			9
#define GRAPHICS_USAGE_COLOR				10
#define GRAPHICS_USAGE_FOG					11
#define GRAPHICS_USAGE_DEPTH				12
#define GRAPHICS_USAGE_SAMPLE				13

// Render Target
#define GRAPHICS_RENDERTARGET_DEPTH			0
#define GRAPHICS_RENDERTARGET_STENCIL		1
#define GRAPHICS_RENDERTARGET_COLOR0		2
#define GRAPHICS_RENDERTARGET_COLOR1		3
#define GRAPHICS_RENDERTARGET_COLOR2		4
#define GRAPHICS_RENDERTARGET_COLOR3		5
#define GRAPHICS_RENDERTARGET_COLOR4		6
#define GRAPHICS_RENDERTARGET_COLOR5		7
#define GRAPHICS_RENDERTARGET_COLOR6		8
#define GRAPHICS_RENDERTARGET_COLOR7		9

// Render State
#define GRAPHICS_RENDERSTATE_ZENABLE			1
#define GRAPHICS_RENDERSTATE_ZWRITEENABLE		2
#define GRAPHICS_RENDERSTATE_ALPHATESTENABLE	3
#define GRAPHICS_RENDERSTATE_DITHERENABLE		4
#define GRAPHICS_RENDERSTATE_BLENDENABLE		5
#define GRAPHICS_RENDERSTATE_FOGENABLE			6
#define GRAPHICS_RENDERSTATE_SPECULARENABLE		7
#define GRAPHICS_RENDERSTATE_STENCILENABLE		8
#define GRAPHICS_RENDERSTATE_LIGHTINGENABLE		9
#define GRAPHICS_RENDERSTATE_NORMALIZENORMALS	10
#define GRAPHICS_RENDERSTATE_POINTSPRITEENABLE	11
#define GRAPHICS_RENDERSTATE_POINTSCALEENABLE	12
#define GRAPHICS_RENDERSTATE_STENCILREF			13
#define GRAPHICS_RENDERSTATE_STENCILMASK		14
#define GRAPHICS_RENDERSTATE_STENCILWRITEMASK	15
#define GRAPHICS_RENDERSTATE_FOGSTART			16
#define GRAPHICS_RENDERSTATE_FOGEND				17
#define GRAPHICS_RENDERSTATE_FOGDENSITY			18
#define GRAPHICS_RENDERSTATE_POINTSIZE			19
#define GRAPHICS_RENDERSTATE_POINTSIZEMIN		20
#define GRAPHICS_RENDERSTATE_POINTSIZEMAX		21
#define GRAPHICS_RENDERSTATE_POINTSCALE_A		22
#define GRAPHICS_RENDERSTATE_POINTSCALE_B		23
#define GRAPHICS_RENDERSTATE_POINTSCALE_C		24
#define GRAPHICS_RENDERSTATE_FOGCOLOR			25
#define GRAPHICS_RENDERSTATE_AMBIENT			26
#define GRAPHICS_RENDERSTATE_CLIPPLANEENABLE	27
#define GRAPHICS_RENDERSTATE_FOGMODE			28
#define GRAPHICS_RENDERSTATE_FILLMODE			29
#define GRAPHICS_RENDERSTATE_SHADEMODE			30
#define GRAPHICS_RENDERSTATE_LINEPATTERN		31
#define GRAPHICS_RENDERSTATE_SRCBLEND			32
#define GRAPHICS_RENDERSTATE_DSTBLEND			33
#define GRAPHICS_RENDERSTATE_BLENDEQUATION		34
#define GRAPHICS_RENDERSTATE_CULLMODE			35
#define GRAPHICS_RENDERSTATE_ZFUNC				36
#define GRAPHICS_RENDERSTATE_ALPHAFUNC			37
#define GRAPHICS_RENDERSTATE_STENCILFUNC		38
#define GRAPHICS_RENDERSTATE_STENCILFAIL		39
#define GRAPHICS_RENDERSTATE_STENCILZFAIL		40
#define GRAPHICS_RENDERSTATE_STENCILPASS		41
#define GRAPHICS_RENDERSTATE_ALPHAREF			42
#define GRAPHICS_RENDERSTATE_FRONTWINDING		43
#define GRAPHICS_RENDERSTATE_COORDINATETYPE		44
#define GRAPHICS_RENDERSTATE_ZBIAS				45
#define GRAPHICS_RENDERSTATE_RANGEFOGENABLE		46
#define GRAPHICS_RENDERSTATE_COLORWRITEENABLE	47
#define GRAPHICS_RENDERSTATE_VERTEXMATERIALENABLE	48
#define GRAPHICS_RENDERSTATE_DIFFUSEMATERIALSOURCE	49
#define GRAPHICS_RENDERSTATE_SPECULARMATERIALSOURCE	50
#define GRAPHICS_RENDERSTATE_AMBIENTMATERIALSOURCE	51
#define GRAPHICS_RENDERSTATE_EMISSIVEMATERIALSOURCE	52
#define GRAPHICS_RENDERSTATE_TEXTUREFACTOR		53
#define GRAPHICS_RENDERSTATE_LOCALVIEWER		54
#define GRAPHICS_RENDERSTATE_SCISSORTESTENABLE	55
#define GRAPHICS_RENDERSTATE_BLENDCOLOR			56
#define GRAPHICS_RENDERSTATE_STENCILENABLE2SIDED	57
#define GRAPHICS_RENDERSTATE_CCWSTENCILFUNC		58
#define GRAPHICS_RENDERSTATE_CCWSTENCILFAIL		59
#define GRAPHICS_RENDERSTATE_CCWSTENCILZFAIL	60
#define GRAPHICS_RENDERSTATE_CCWSTENCILPASS		61
#define GRAPHICS_RENDERSTATE_VERTEXBLEND		62
#define GRAPHICS_RENDERSTATE_SLOPESCALEDEPTHBIAS	63
#define GRAPHICS_RENDERSTATE_DEPTHBIAS			64
#define GRAPHICS_RENDERSTATE_OUTPUTGAMMA		65
#define GRAPHICS_RENDERSTATE_ZVISIBLE			66
#define GRAPHICS_RENDERSTATE_LASTPIXEL			67
#define GRAPHICS_RENDERSTATE_CLIPPING			68
#define GRAPHICS_RENDERSTATE_WRAP0				69
#define GRAPHICS_RENDERSTATE_WRAP1				70
#define GRAPHICS_RENDERSTATE_WRAP2				71
#define GRAPHICS_RENDERSTATE_WRAP3				72
#define GRAPHICS_RENDERSTATE_WRAP4				73
#define GRAPHICS_RENDERSTATE_WRAP5				74
#define GRAPHICS_RENDERSTATE_WRAP6				75
#define GRAPHICS_RENDERSTATE_WRAP7				76
#define GRAPHICS_RENDERSTATE_WRAP8				77
#define GRAPHICS_RENDERSTATE_WRAP9				78
#define GRAPHICS_RENDERSTATE_WRAP10				79
#define GRAPHICS_RENDERSTATE_WRAP11				80
#define GRAPHICS_RENDERSTATE_WRAP12				81
#define GRAPHICS_RENDERSTATE_WRAP13				82
#define GRAPHICS_RENDERSTATE_WRAP14				83
#define GRAPHICS_RENDERSTATE_WRAP15				84
#define GRAPHICS_RENDERSTATE_MULTISAMPLEANTIALIAS	85
#define GRAPHICS_RENDERSTATE_MULTISAMPLEMASK		86
#define GRAPHICS_RENDERSTATE_INDEXEDVERTEXBLENDENABLE	87
#define GRAPHICS_RENDERSTATE_TWEENFACTOR		88
#define GRAPHICS_RENDERSTATE_ANTIALIASEDLINEENABLE	89
#define GRAPHICS_RENDERSTATE_COLORWRITEENABLE1	90
#define GRAPHICS_RENDERSTATE_COLORWRITEENABLE2	91
#define GRAPHICS_RENDERSTATE_COLORWRITEENABLE3	92
#define GRAPHICS_RENDERSTATE_SEPARATEALPHABLENDENABLE	93
#define GRAPHICS_RENDERSTATE_SRCBLENDALPHA		94
#define GRAPHICS_RENDERSTATE_DSTBLENDALPHA		95
#define GRAPHICS_RENDERSTATE_BLENDEQUATIONALPHA	96
#define GRAPHICS_RENDERSTATE_TRANSPARENCYANTIALIAS	97
#define GRAPHICS_RENDERSTATE_LINEAA				98
#define GRAPHICS_RENDERSTATE_LINEWIDTH			99

// Texture States
#define GRAPHICS_TEXTURESTATE_BIND_TEXTURE		1
#define GRAPHICS_TEXTURESTATE_COLOROP			2
#define GRAPHICS_TEXTURESTATE_COLORARG1			3
#define GRAPHICS_TEXTURESTATE_COLORARG2			4
#define GRAPHICS_TEXTURESTATE_ALPHAOP			5
#define GRAPHICS_TEXTURESTATE_ALPHAARG1			6
#define GRAPHICS_TEXTURESTATE_ALPHAARG2			7
#define GRAPHICS_TEXTURESTATE_ADDRESSU			8
#define GRAPHICS_TEXTURESTATE_ADDRESSV			9
#define GRAPHICS_TEXTURESTATE_MIPFILTER			10
#define GRAPHICS_TEXTURESTATE_MAGFILTER			11
#define GRAPHICS_TEXTURESTATE_MINFILTER			12
#define GRAPHICS_TEXTURESTATE_BORDERCOLOR		13
#define GRAPHICS_TEXTURESTATE_TEXCOORDINDEX		14
#define GRAPHICS_TEXTURESTATE_TEXTURETRANSFORMFLAGS	15
#define GRAPHICS_TEXTURESTATE_TEXCOORDGEN		16
#define GRAPHICS_TEXTURESTATE_BUMPENVMAT00		17
#define GRAPHICS_TEXTURESTATE_BUMPENVMAT01		18
#define GRAPHICS_TEXTURESTATE_BUMPENVMAT10		19
#define GRAPHICS_TEXTURESTATE_BUMPENVMAT11		20
#define GRAPHICS_TEXTURESTATE_TEXTURE_MIPMAP_LEVEL	21
#define GRAPHICS_TEXTURESTATE_TEXTURE_LOD_BIAS	22
#define GRAPHICS_TEXTURESTATE_TEXTURE_ANISOTROPIC_LEVEL	23
#define GRAPHICS_TEXTURESTATE_ADDRESSW			24
#define GRAPHICS_TEXTURESTATE_GAMMA				25
#define GRAPHICS_TEXTURESTATE_BUMPENVLSCALE		26
#define GRAPHICS_TEXTURESTATE_BUMPENVLOFFSET	27
#define GRAPHICS_TEXTURESTATE_COLORARG0			28
#define GRAPHICS_TEXTURESTATE_ALPHAARG0			29

// Texture Combiner
#define GRAPHICS_TC_DISABLE						1
#define GRAPHICS_TC_SELECTARG1					2
#define GRAPHICS_TC_SELECTARG2					3
#define GRAPHICS_TC_MODULATE					4
#define GRAPHICS_TC_ADD							5
#define GRAPHICS_TC_ADDSIGNED					6
#define GRAPHICS_TC_SUBTRACT					7
#define GRAPHICS_TC_BLENDTEXTUREALPHA			8
#define GRAPHICS_TC_BLENDDIFFUSEALPHA			9
#define GRAPHICS_TC_BLENDCURRENTALPHA			10
#define GRAPHICS_TC_BLENDFACTORALPHA			11
#define GRAPHICS_TC_MODULATE2X					12
#define GRAPHICS_TC_MODULATE4X					13
#define GRAPHICS_TC_DSDT						14
#define GRAPHICS_TC_DOTPRODUCT3					15
#define GRAPHICS_TC_BLENDTEXTUREALPHAPM			16
#define GRAPHICS_TC_ADDSIGNED2X					17
#define GRAPHICS_TC_ADDSMOOTH					18
#define GRAPHICS_TC_PREMODULATE					19
#define GRAPHICS_TC_MODULATEALPHA_ADDCOLOR		20
#define GRAPHICS_TC_MODULATECOLOR_ADDALPHA		21
#define GRAPHICS_TC_MODULATEINVALPHA_ADDCOLOR	22
#define GRAPHICS_TC_MODULATEINVCOLOR_ADDALPHA	23
#define GRAPHICS_TC_BUMPENVMAPLUMINANCE			24
#define GRAPHICS_TC_MULTIPLYADD					25
#define GRAPHICS_TC_LERP						26

// Texture Arguments
#define GRAPHICS_TA_CONSTANT					1
#define GRAPHICS_TA_PREVIOUS					2
#define GRAPHICS_TA_DIFFUSE						3
#define GRAPHICS_TA_TEXTURE						4
#define GRAPHICS_TA_SPECULAR					5

// Texture Address
#define GRAPHICS_TEXTURE_ADDRESS_WRAP			1
#define GRAPHICS_TEXTURE_ADDRESS_MIRROR			2
#define GRAPHICS_TEXTURE_ADDRESS_CLAMP			3
#define GRAPHICS_TEXTURE_ADDRESS_BORDER			4
#define GRAPHICS_TEXTURE_ADDRESS_MIRRORONCE		5
#define GRAPHICS_TEXTURE_ADDRESS_EDGE			6

// Texture Filters
#define GRAPHICS_TEXTURE_FILTER_NONE			0
#define GRAPHICS_TEXTURE_FILTER_NEAREST			1
#define GRAPHICS_TEXTURE_FILTER_LINEAR			2
#define GRAPHICS_TEXTURE_FILTER_ANISOTROPIC		3

// Texture Transform Flags
#define GRAPHICS_TEXTURE_TRANSFORM_OFF			0
#define GRAPHICS_TEXTURE_TRANSFORM_S			(1 << 0)
#define GRAPHICS_TEXTURE_TRANSFORM_T			(1 << 1)
#define GRAPHICS_TEXTURE_TRANSFORM_R			(1 << 2)
#define GRAPHICS_TEXTURE_TRANSFORM_Q			(1 << 3)
#define GRAPHICS_TEXTURE_PROJECTED				(1 << 15)

// Texture Coord Gen
#define GRAPHICS_TEXCOORD_GEN_OFF				0
#define GRAPHICS_TEXCOORD_GEN_EYE_POSITION		1
#define GRAPHICS_TEXCOORD_GEN_EYE_NORMAL		2
#define GRAPHICS_TEXCOORD_GEN_REFLECTIONVECTOR	3
#define GRAPHICS_TEXCOORD_GEN_SPHERE			4

// Stencil Ops
#define GRAPHICS_STENCILOP_INVALID				0
#define GRAPHICS_STENCILOP_KEEP					1
#define GRAPHICS_STENCILOP_ZERO					2
#define GRAPHICS_STENCILOP_REPLACE				3
#define GRAPHICS_STENCILOP_INCRSAT				4
#define GRAPHICS_STENCILOP_DECRSAT				5
#define GRAPHICS_STENCILOP_INVERT				6
#define GRAPHICS_STENCILOP_INCR					7
#define GRAPHICS_STENCILOP_DECR					8

// Blend Ops
#define GRAPHICS_BLENDOP_ZERO					1
#define GRAPHICS_BLENDOP_ONE					2
#define GRAPHICS_BLENDOP_SRCCOLOR				3
#define GRAPHICS_BLENDOP_INVSRCCOLOR 			4
#define GRAPHICS_BLENDOP_SRCALPHA				5
#define GRAPHICS_BLENDOP_INVSRCALPHA 			6
#define GRAPHICS_BLENDOP_DESTALPHA				7
#define GRAPHICS_BLENDOP_INVDESTALPHA			8
#define GRAPHICS_BLENDOP_DESTCOLOR				9
#define GRAPHICS_BLENDOP_INVDESTCOLOR			10
#define GRAPHICS_BLENDOP_SRCALPHASAT			11
#define GRAPHICS_BLENDOP_BLENDFACTOR			12
#define GRAPHICS_BLENDOP_INVBLENDFACTOR			13

// Blend Equation
#define GRAPHICS_BLENDEQ_ADD					1
#define GRAPHICS_BLENDEQ_SUBTRACT				2
#define GRAPHICS_BLENDEQ_REVSUBTRACT			3
#define GRAPHICS_BLENDEQ_MINIMUM				4
#define GRAPHICS_BLENDEQ_MAXIMUM				5

// Face Winding
#define GRAPHICS_FRONTWINDING_CW				1
#define GRAPHICS_FRONTWINDING_CCW				2

// Face culling
#define GRAPHICS_FACE_NONE						1
#define GRAPHICS_FACE_FRONT						2
#define GRAPHICS_FACE_BACK						3
#define GRAPHICS_FACE_FRONT_BACK				4

// Compare Functions
#define GRAPHICS_CMP_NEVER						1
#define GRAPHICS_CMP_LESS						2
#define GRAPHICS_CMP_EQUAL						3
#define GRAPHICS_CMP_LESSEQUAL					4
#define GRAPHICS_CMP_GREATER					5
#define GRAPHICS_CMP_NOTEQUAL					6
#define GRAPHICS_CMP_GREATEREQUAL				7
#define GRAPHICS_CMP_ALWAYS						8

// Buffer formats
#define GRAPHICS_FORMAT_X8R8G8B8				1
#define GRAPHICS_FORMAT_A8R8G8B8				2
#define GRAPHICS_FORMAT_R5G6B5					3
#define GRAPHICS_FORMAT_X1R5G5B5				4
#define GRAPHICS_FORMAT_A1R5G5B5				5
#define GRAPHICS_FORMAT_A4R4G4B4				6
#define GRAPHICS_FORMAT_Z_D32					7
#define GRAPHICS_FORMAT_Z_D16					8
#define GRAPHICS_FORMAT_Z_D24S8					9
#define GRAPHICS_FORMAT_Z_D15S1					10
#define GRAPHICS_FORMAT_LUMINANCE8				11
#define GRAPHICS_FORMAT_LUMINANCE4_ALPHA4		12
#define GRAPHICS_FORMAT_LUMINANCE16				13
#define GRAPHICS_FORMAT_LUMINANCE8_ALPHA8		14
#define GRAPHICS_FORMAT_DXT1					15
#define GRAPHICS_FORMAT_DXT2					16
#define GRAPHICS_FORMAT_DXT3					17
#define GRAPHICS_FORMAT_DXT4					18
#define GRAPHICS_FORMAT_DXT5					19
#define GRAPHICS_FORMAT_BUMPU8V8				20
#define GRAPHICS_FORMAT_BUMPL6V5U5				21
#define GRAPHICS_FORMAT_BUMPX8L8V8U8			22
#define GRAPHICS_FORMAT_BUMPL8V8U8				23
#define GRAPHICS_FORMAT_ARGB_S10E5				24
#define GRAPHICS_FORMAT_ARGB_S23E8				25
#define GRAPHICS_FORMAT_A2R10G10B10				26
#define GRAPHICS_FORMAT_V8U8					27
#define GRAPHICS_FORMAT_Q8W8V8U8				28
#define GRAPHICS_FORMAT_CxV8U8					29
#define GRAPHICS_FORMAT_X8L8V8U8				30
#define GRAPHICS_FORMAT_A2W10V10U10				31
#define GRAPHICS_FORMAT_ALPHA8					32
#define GRAPHICS_FORMAT_R_S10E5					33
#define GRAPHICS_FORMAT_R_S23E8					34
#define GRAPHICS_FORMAT_RG_S10E5				35
#define GRAPHICS_FORMAT_RG_S23E8				36
#define GRAPHICS_FORMAT_BUFFER					37
#define GRAPHICS_FORMAT_Z_D24X8					38
#define GRAPHICS_FORMAT_V16U16					39
#define GRAPHICS_FORMAT_G16R16					40
#define GRAPHICS_FORMAT_A16B16G16R16			41
#define GRAPHICS_FORMAT_UYVY					42
#define GRAPHICS_FORMAT_YUY2					43
#define GRAPHICS_FORMAT_NV12					44
#define GRAPHICS_FORMAT_AYUV					45
#define GRAPHICS_FORMAT_BC4_UNORM				108
#define GRAPHICS_FORMAT_BC5_UNORM				111
#define GRAPHICS_FORMAT_Z_DF16					118
#define GRAPHICS_FORMAT_Z_DF24					119
#define GRAPHICS_FORMAT_Z_D24S8_INT				120

typedef struct {
	uint32_t resolution_x;
	uint32_t resolution_y;
} graphics_info_t;

typedef struct {
	uint32_t cid;
	uint32_t width;
	uint32_t height;
	uint32_t color_bits;
	uint32_t depth_bits;
	uint32_t stencil_bits;
	
	uint32_t color_surface;
	uint32_t depth_surface;	// also stencil surface
} graphics_context_t;

typedef struct {
	uint32_t bid;
	uint32_t offset;
	uint32_t stride;
	
	uint32_t type;
	uint32_t usage;
} graphics_vertex_array_t;

#endif /* DEFS_H */
