#if !defined VERSE_TYPES
#define	VERSE_TYPES

#include <stdlib.h>

typedef unsigned char boolean;
typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;
typedef float real32;
typedef double real64;

#define V_REAL64_MAX         1.7976931348623158e+308 /* max value */
#define V_REAL32_MAX         3.40282347e+38F

#define TRUE 1
#define FALSE 0

typedef enum {
	V_NT_OBJECT = 0, 
	V_NT_GEOMETRY, 
	V_NT_MATERIAL, 
	V_NT_BITMAP, 
	V_NT_TEXT, 
	V_NT_PARTICLE,
	V_NT_CURVE, 
	V_NT_AUDIO, 
	V_NT_NUM_TYPES, 
	V_NT_SYSTEM = V_NT_NUM_TYPES, 
	V_NT_NUM_TYPES_NETPACK
} VNodeType;

typedef uint32		VNodeID;
typedef uint16		VLayerID;		/* Commonly used to identify layers. */
typedef uint16		VNMFragmentID;
typedef uint16		VNMBufferID;
typedef uint16		VNPEffectID;

typedef void *		VSession;

typedef enum {
	VN_O_METHOD_PTYPE_INTEGER8 = 0,
	VN_O_METHOD_PTYPE_INTEGER16,
	VN_O_METHOD_PTYPE_INTEGER32,

	VN_O_METHOD_PTYPE_UINTEGER8,
	VN_O_METHOD_PTYPE_UINTEGER16,
	VN_O_METHOD_PTYPE_UINTEGER32,

	VN_O_METHOD_PTYPE_REAL32,
	VN_O_METHOD_PTYPE_REAL64,

	VN_O_METHOD_PTYPE_STRING,

	VN_O_METHOD_PTYPE_VNODE,
	VN_O_METHOD_PTYPE_VLAYER,

	VN_O_METHOD_PTYPE_2_VECTOR32,
	VN_O_METHOD_PTYPE_3_VECTOR32,
	VN_O_METHOD_PTYPE_4_VECTOR32,

	VN_O_METHOD_PTYPE_2_VECTOR64,
	VN_O_METHOD_PTYPE_3_VECTOR64,
	VN_O_METHOD_PTYPE_4_VECTOR64,

	VN_O_METHOD_PTYPE_4_MATRIX32,
	VN_O_METHOD_PTYPE_9_MATRIX32,
	VN_O_METHOD_PTYPE_16_MATRIX32,

	VN_O_METHOD_PTYPE_4_MATRIX64,
	VN_O_METHOD_PTYPE_9_MATRIX64,
	VN_O_METHOD_PTYPE_16_MATRIX64
} VNOParamType;

typedef	union{
	int8		integer8;
	int16		integer16;
	int32		integer32;
	uint8		uinteger8;
	uint16		uinteger16;
	uint32		uinteger32;
	real32		real32;
	real64		real64;
	char		*string;
	VNodeID		vnode;
	VLayerID	vlayer;
	real32		vector32[4];
	real64		vector64[4];
	real32		matrix32[16];
	real64		matrix64[16];
} VNOParam;

#define VN_TAG_MAX_BLOB_SIZE 500

typedef enum {
	VN_TAG_BOOLEAN = 0,
	VN_TAG_INTEGER,
	VN_TAG_REAL,
	VN_TAG_STRING,
	VN_TAG_VECTOR,
	VN_TAG_LINK,
	VN_TAG_ANIMATION,
	VN_TAG_BLOB,
	VN_TAG_TYPE_COUNT
} VNTagType;

typedef union{
	boolean active;
	uint32	integer;
	real64	real;
	char	*string;
	real64	vector[3];
	VNodeID	link;
	struct{
		VNodeID curve;
		uint32 start;
		uint32 end;
	}animation;
	struct{
		uint16	blob_size;
		void	*blob;
	}blob;
} VNTag;

typedef enum {
	VN_S_CONNECT_NAME_SIZE = 32,
	VN_S_CONNECT_KEY_SIZE = 4,
	VN_S_CONNECT_DATA_SIZE = 32
} VNSConnectConstants;

typedef enum {
	VN_FORMAT_REAL32,
	VN_FORMAT_REAL64
} VNORealFormat;

typedef enum {
	VN_O_TAG_GROUP_SIZE = 16,
	VN_O_TAG_NAME_SIZE = 16,
	VN_O_TAG_FULL_NAME_SIZE = 64,
	VN_O_TAG_STRING_SIZE = 128
} VNOTagConstants;


typedef enum {
	VN_O_METHOD_GROUP_NAME_SIZE = 16,
	VN_O_METHOD_NAME_SIZE = 16,
	VN_O_METHOD_SIG_SIZE = 256
} VNOMethodConstants;

typedef enum {
	VN_G_LAYER_VERTEX_XYZ = 0,
	VN_G_LAYER_VERTEX_UINTEGER32,
	VN_G_LAYER_VERTEX_REAL64,
	VN_G_LAYER_POLYGON_CORNER_UINTEGER32 = 128,
	VN_G_LAYER_POLYGON_CORNER_REAL64,
	VN_G_LAYER_POLYGON_FACE_UINTEGER8,
	VN_G_LAYER_POLYGON_FACE_UINTEGER32,
	VN_G_LAYER_POLYGON_FACE_REAL64
} VNGLayerType;

typedef enum {
	VN_M_LIGHT_DIRECT = 0,
	VN_M_LIGHT_AMBIENT,
	VN_M_LIGHT_DIRECT_AND_AMBIENT,
	VN_M_LIGHT_BACK
} VNMLightType;

typedef enum {
	VN_M_NOISE_PERLIN = 0
} VNMNoiseType;

typedef enum {
	VN_M_RAMP_SQUARE = 0,
	VN_M_RAMP_LINEAR
} VNMRampType;

typedef enum {
	VN_M_RAMP_RED = 0,
	VN_M_RAMP_GREEN,
	VN_M_RAMP_BLUE
} VNMRampChannel;

typedef struct {
	double	pos;
	double	red;
	double	green;
	double	blue;
} VNMRampPoint;

typedef enum {
	VN_M_BLEND_FADE = 0,
	VN_M_BLEND_ADD,
	VN_M_BLEND_SUBTRACT,
	VN_M_BLEND_MULTIPLY,
	VN_M_BLEND_DIVIDE,
	VN_M_BLEND_DOT
} VNMBlendType;

typedef enum {
	VN_M_FT_COLOR = 0,
	VN_M_FT_LIGHT,
	VN_M_FT_REFLECTION,
	VN_M_FT_TRANSPARENCY,
	VN_M_FT_GEOMETRY,
	VN_M_FT_TEXTURE,
	VN_M_FT_TAG,
	VN_M_FT_NOISE,
	VN_M_FT_BLENDER,
	VN_M_FT_MATRIX,
	VN_M_FT_RAMP,
	VN_M_FT_ALTERNATIVE,
	VN_M_FT_OUTPUT
} VNMFragmentType;

typedef union{
	struct{
		real64 red;
		real64 green;
		real64 blue;
	}color;
	struct{
		uint8 type;
		real64 normal_falloff; 
		VNodeID brdf;
		char brdf_r[16];
		char brdf_g[16];
		char brdf_b[16];
	}light;
	struct{
		real64 normal_falloff;
	}reflection;
	struct{
		real64 normal_falloff;
		real64 refraction_index;
	}transparency;
	struct{
		char layer_r[16];
		char layer_g[16];
		char layer_b[16];
	}geometry;
	struct{
		VNodeID bitmap;
		char layer_r[16];
		char layer_g[16];
		char layer_b[16];
		VNMFragmentID control;
	}texture;
	struct{
		char name[16];
		char group[16];
	}tag;
	struct{
		uint8 type;
		VNMFragmentID mapping;
	}noise;
	struct{
		uint8 type;
		VNMFragmentID data_a;
		VNMFragmentID data_b; 
		VNMFragmentID control;
	}blender;
	struct{
		real64 matrix[16];
		VNMFragmentID data;
	}matrix;
	struct{
		uint8 type;
		uint8 channel;
		VNMFragmentID control; 
		uint8 point_count; 
		VNMRampPoint ramp[48];
	}ramp;
	struct{
		VNMFragmentID alt_a;
		VNMFragmentID alt_b;
	}alternative;
	struct{
		char type[16];
		VNMFragmentID front;
		VNMFragmentID back;
	}output;
} VMatFrag;

typedef enum {
	VN_B_LAYER_INTEGER1 = 0,
	VN_B_LAYER_INTEGER8,
	VN_B_LAYER_INTEGER16,
	VN_B_LAYER_REAL32,
	VN_B_LAYER_REAL64
} VNBLayerType;

#define VN_B_TILE_SIZE 4

typedef enum {
	VN_C_CONTENT_LANGUAGE_SIZE = 32,
	VN_C_CONTENT_INFO_SIZE = 256,
	VN_C_BUFFER_NAME_SIZE = 32,
	VN_C_TEXT_SIZE = 256
} VNCConstants;

#define VN_AUDIO_MAX_SAMPLE_SIZE_UNCOMPRESSED 1400
#define VN_AUDIO_MAX_SAMPLE_SIZE_COMPRESSED 5000

typedef enum {
    VN_A_LAYER_INTEGER8,
    VN_A_LAYER_INTEGER16,
    VN_A_LAYER_INTEGER24,
    VN_A_LAYER_INTEGER32,
    VN_A_LAYER_REAL32,
    VN_A_LAYER_REAL64,
} VNALayerType;

typedef enum {
    VN_A_UNCOMPRESSED_SUSTAINED,
    VN_A_COMPRESSED_SUSTAINED,
    VN_A_UNCOMPRESSED_UNSUSTAINED,
    VN_A_COMPRESSED_UNSUSTAINED
} VNATransferType;

typedef enum {
	VN_P_FORCE_RADIAL = 0,
	VN_P_FORCE_DIRECTIONAL
} VNPForceType;

typedef enum {
	VN_P_SPACE_LOCAL = 0,
	VN_P_SPACE_GLOBAL,
	VN_P_SPACE_PARTICLE
} VNPSpace;

extern void verse_set_connect_port(uint16 port);
extern void verse_callback_set(void *send_func, void *callback, void *user_data);
extern void verse_callback_update(uint32 milliseconds);
extern void verse_session_set(VSession session);
extern void verse_session_destroy(VSession session);
extern size_t verse_session_get_size(void);

extern void *verse_method_call_pack(uint32 param_count, const VNOParam *params, const VNOParamType *param_type);
extern boolean verse_method_call_unpack(const void *data, uint32 param_count, VNOParam *params, const VNOParamType *param_type);

extern void *verse_audio_compress(uint32 length, VNALayerType type, void *data);
extern void *verse_audio_uncompress(uint32 length, VNALayerType type, void *data);
/*
#define V_PRINT_SEND_COMMANDS
#define V_PRINT_RECEIVE_COMMANDS
*/

#endif		/* VERSE_TYPES */
