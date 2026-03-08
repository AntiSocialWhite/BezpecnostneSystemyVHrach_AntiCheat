#pragma once
#include <cstdint>
#include <Windows.h>
#include <thread>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <map>
#include <intrin.h>
#include "minhook/minhook.h"
#include "EngineClient.h"
#include "Netvars.h"
#include "crc.h"
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

#define time_to_ticks(dt) ((int)(0.5 + (float)(dt) / interfaces::globals->interval_per_tick))
#define ticks_to_time(dt) ( interfaces::globals->interval_per_tick * (float)(dt) )

class i_app_system;

typedef void* (*create_interface_fn)(const char* name, int* return_code);

class i_app_system {
public:
	virtual bool connect(create_interface_fn factory) = 0;
	virtual void disconnect() = 0;
	virtual void* query_interface(const char* interface_name) = 0;
	virtual int init() = 0;
	virtual void shutdown() = 0;
	virtual const void* get_dependencies() = 0;
	virtual int  get_tier() = 0;
	virtual void reconnect(create_interface_fn factory, const char* interface_name) = 0;
	virtual void unknown() = 0;
};


class i_console;
class convar;
class con_command;
class con_command_base;

typedef int cvar_dll_indentifier_t;

class i_console_display_func {
public:
	virtual void color_print(const uint8_t* clr, const char* msg) = 0;
	virtual void print(const char* msg) = 0;
	virtual void drint(const char* msg) = 0;
};

class i_console : public i_app_system {
public:
	virtual cvar_dll_indentifier_t	allocate_dll_indentifier() = 0;
	virtual void			register_con_command(con_command_base* base) = 0;
	virtual void			unregister_con_command(con_command_base* base) = 0;
	virtual void			unregister_con_commands(cvar_dll_indentifier_t id) = 0;
	virtual const char* get_command_line_value(const char* name) = 0;
	virtual con_command_base* find_command_base(const char* name) = 0;
	virtual const con_command_base* find_command_base(const char* name) const = 0;
	virtual convar* get_convar(const char* var_name) = 0;
	virtual const convar* get_convar(const char* var_name) const = 0;
	virtual con_command* find_command(const char* name) = 0;
	virtual const con_command* find_command(const char* name) const = 0;
	virtual void			install_global_change_callback(fn_change_callback_t callback) = 0;
	virtual void			remove_global_change_callback(fn_change_callback_t callback) = 0;
	virtual void			call_global_change_callbacks(convar* var, const char* old_str, float old_val) = 0;
	virtual void			install_console_display_func(i_console_display_func* func) = 0;
	virtual void			remove_console_display_func(i_console_display_func* func) = 0;
	virtual void			console_color_printf(unsigned char* clr, const char* format, ...) const = 0;
	virtual void			console_printf(const char* format, ...) const = 0;
	virtual void			dconsole_dprintf(const char* format, ...) const = 0;
	virtual void			rever_flagged_convars(int flag) = 0;
};

class i_client_entity_list {
public:
	void* get_client_entity(int index) {
		using original_fn = void* (__thiscall*)(i_client_entity_list*, int);
		return (*(original_fn**)this)[3](this, index);
	}
	void* get_client_entity_handle(uintptr_t handle) {
		using original_fn = void* (__thiscall*)(i_client_entity_list*, uintptr_t);
		return (*(original_fn**)this)[4](this, handle);
	}
	int get_highest_index() {
		using original_fn = int(__thiscall*)(i_client_entity_list*);
		return (*(original_fn**)this)[6](this);
	}
	int get_max_entities() {
		using original_fn = int(__thiscall*)(i_client_entity_list*);
		return (*(original_fn**)this)[8](this);
	}
};

class c_usercmd {
public:
	virtual	~c_usercmd() { }		// 0x00
	int command_number;
	int tick_count;
	vec3_t viewangles;
	vec3_t aimdirection;
	float forwardmove;
	float sidemove;
	float upmove;
	int buttons;
	char impulse;
	int weaponselect;
	int weaponsubtype;
	int randomseed;
	short mousedx;
	short mousedy;
	bool hasbeenpredicted;
	vec3_t vecHeadAngles;		// 0x4C
	vec3_t vecHeadOffset;		// 0x58

	[[nodiscard]] CRC32_t GetChecksum() const
	{
		CRC32_t uHashCRC = 0UL;

		CRC32::Init(&uHashCRC);
		CRC32::ProcessBuffer(&uHashCRC, &command_number, sizeof(command_number));
		CRC32::ProcessBuffer(&uHashCRC, &tick_count, sizeof(tick_count));
		CRC32::ProcessBuffer(&uHashCRC, &viewangles, sizeof(viewangles));
		CRC32::ProcessBuffer(&uHashCRC, &aimdirection, sizeof(aimdirection));
		CRC32::ProcessBuffer(&uHashCRC, &forwardmove, sizeof(forwardmove));
		CRC32::ProcessBuffer(&uHashCRC, &sidemove, sizeof(sidemove));
		CRC32::ProcessBuffer(&uHashCRC, &upmove, sizeof(upmove));
		CRC32::ProcessBuffer(&uHashCRC, &buttons, sizeof(buttons));
		CRC32::ProcessBuffer(&uHashCRC, &impulse, sizeof(impulse));
		CRC32::ProcessBuffer(&uHashCRC, &weaponselect, sizeof(weaponselect));
		CRC32::ProcessBuffer(&uHashCRC, &weaponsubtype, sizeof(weaponsubtype));
		CRC32::ProcessBuffer(&uHashCRC, &randomseed, sizeof(randomseed));
		CRC32::ProcessBuffer(&uHashCRC, &mousedx, sizeof(mousedx));
		CRC32::ProcessBuffer(&uHashCRC, &mousedy, sizeof(mousedy));
		CRC32::Final(&uHashCRC);

		return uHashCRC;
	}
};
static_assert(sizeof(c_usercmd) == 0x64);

class c_verifiedusercmd
{
public:
	c_usercmd	userCmd;	// 0x00
	CRC32_t		uHashCRC;	// 0x64
};
static_assert(sizeof(c_verifiedusercmd) == 0x68);

class i_input {
public:
	std::byte			pad0[0xC];				// 0x00
	bool				bTrackIRAvailable;		// 0x0C
	bool				bMouseInitialized;		// 0x0D
	bool				bMouseActive;			// 0x0E
	std::byte			pad1[0x9A];				// 0x0F
	bool				bCameraInThirdPerson;	// 0xA9
	std::byte			pad2[0x2];				// 0xAA
	float				vecCameraOffset[3];
	std::byte			pad3[0x38];				// 0xB8
	c_usercmd* pCommands;				// 0xF0
	c_verifiedusercmd* pVerifiedCommands;		// 0xF4

	[[nodiscard]] c_usercmd* GetUserCmd(const int nSequenceNumber) const
	{
		return &pCommands[nSequenceNumber % 150];
	}

	[[nodiscard]] c_verifiedusercmd* GetVerifiedCmd(const int nSequenceNumber) const
	{
		return &pVerifiedCommands[nSequenceNumber % 150];
	}
};

class i_client_state
{
public:
	std::byte        pad0[0x00A0];                //0x0000
	int                iChallengeNr;            //0x00A0
	std::byte        pad1[0x64];                //0x00A4
	int                iSignonState;            //0x0108
	std::byte        pad2[0x8];                //0x010C
	float            flNextCmdTime;            //0x0114
	int                iServerCount;            //0x0118
	int                iCurrentSequence;        //0x011C
	char _0x0120[4];
	__int32 m_iClockDriftMgr; //0x0124 
	char _0x0128[68];
	__int32 m_iServerTick; //0x016C 
	__int32 m_iClientTick; //0x0170 
	int                iDeltaTick;                //0x0174
	bool            bPaused;                //0x0178
	std::byte        pad4[0x7];                //0x0179
	int                iViewEntity;            //0x0180
	int                iPlayerSlot;            //0x0184
	char            szLevelName[260];        //0x0188
	char            szLevelNameShort[80];    //0x028C
	char            szGroupName[80];        //0x02DC
	std::byte        pad5[0x5C];                //0x032C
	int                nMaxClients;            //0x0388
	std::byte        pad6[0x4984];            //0x038C
	float            flLastServerTickTime;    //0x4D10
	bool            bInSimulation;            //0x4D14
	std::byte        pad7[0xB];                //0x4D15
	int                iOldTickcount;            //0x4D18
	float            flTickRemainder;        //0x4D1C
	float            flFrameTime;            //0x4D20
	int                nLastOutgoingCommand;    //0x4D38
	int                iChokedCommands;        //0x4D30
	int                nLastCommandAck;        //0x4D2C
	int                iCommandAck;            //0x4D30
	int                iSoundSequence;            //0x4D34
	std::byte        pad8[0x50];                //0x4D38
	vec3_t			angViewPoint;			// 0x4D90
};

class client_class;
class i_client_networkable;
class i_client_mode;
typedef i_client_networkable* (*create_client_class_fn)(int ent_number, int serial_number);
typedef i_client_networkable* (*create_event_fn)();

enum class_ids {
	ctesttraceline = 224,
	cteworlddecal = 225,
	ctespritespray = 222,
	ctesprite = 221,
	ctesparks = 220,
	ctesmoke = 219,
	cteshowline = 217,
	cteprojecteddecal = 214,
	cfeplayerdecal = 71,
	cteplayerdecal = 213,
	ctephysicsprop = 210,
	cteparticlesystem = 209,
	ctemuzzleflash = 208,
	ctelargefunnel = 206,
	ctekillplayerattachments = 205,
	cteimpact = 204,
	cteglowsprite = 203,
	cteshattersurface = 216,
	ctefootprintdecal = 200,
	ctefizz = 199,
	cteexplosion = 197,
	cteenergysplash = 196,
	cteeffectdispatch = 195,
	ctedynamiclight = 194,
	ctedecal = 192,
	cteclientprojectile = 191,
	ctebubbletrail = 190,
	ctebubbles = 189,
	ctebspdecal = 188,
	ctebreakmodel = 187,
	ctebloodstream = 186,
	ctebloodsprite = 185,
	ctebeamspline = 184,
	ctebeamringpoint = 183,
	ctebeamring = 182,
	ctebeampoints = 181,
	ctebeamlaser = 180,
	ctebeamfollow = 179,
	ctebeaments = 178,
	ctebeamentpoint = 177,
	ctebasebeam = 176,
	ctearmorricochet = 175,
	ctemetalsparks = 207,
	csteamjet = 168,
	csmokestack = 158,
	dusttrail = 277,
	cfiretrail = 74,
	sporetrail = 283,
	sporeexplosion = 282,
	rockettrail = 280,
	smoketrail = 281,
	cpropvehicledriveable = 145,
	particlesmokegrenade = 279,
	cparticlefire = 117,
	movieexplosion = 278,
	ctegaussexplosion = 202,
	cenvquadraticbeam = 66,
	cembers = 55,
	cenvwind = 70,
	cprecipitation = 138,
	cprecipitationblocker = 139,
	cbasetempentity = 18,
	nextbotcombatcharacter = 0,
	ceconwearable = 54,
	cbaseattributableitem = 4,
	ceconentity = 53,
	cweaponzonerepulsor = 274,
	cweaponxm1014 = 273,
	cweapontaser = 268,
	ctablet = 172,
	csnowball = 159,
	csmokegrenade = 156,
	cweaponshield = 266,
	cweaponsg552 = 264,
	csensorgrenade = 152,
	cweaponsawedoff = 260,
	cweaponnova = 256,
	cincendiarygrenade = 99,
	cmolotovgrenade = 113,
	cmelee = 112,
	cweaponm3 = 248,
	cknifegg = 108,
	cknife = 107,
	chegrenade = 96,
	cflashbang = 77,
	cfists = 76,
	cweaponelite = 239,
	cdecoygrenade = 47,
	cdeagle = 46,
	cweaponusp = 272,
	cweaponm249 = 247,
	cweaponump45 = 271,
	cweapontmp = 270,
	cweapontec9 = 269,
	cweaponssg08 = 267,
	cweaponsg556 = 265,
	cweaponsg550 = 263,
	cweaponscout = 262,
	cweaponscar20 = 261,
	cscar17 = 150,
	cweaponp90 = 259,
	cweaponp250 = 258,
	cweaponp228 = 257,
	cweaponnegev = 255,
	cweaponmp9 = 254,
	cweaponmp7 = 253,
	cweaponmp5navy = 252,
	cweaponmag7 = 251,
	cweaponmac10 = 250,
	cweaponm4a1 = 249,
	cweaponhkp2000 = 246,
	cweaponglock = 245,
	cweapongalilar = 244,
	cweapongalil = 243,
	cweapong3sg1 = 242,
	cweaponfiveseven = 241,
	cweaponfamas = 240,
	cweaponbizon = 235,
	cweaponawp = 233,
	cweaponaug = 232,
	cak47 = 1,
	cweaponcsbasegun = 237,
	cweaponcsbase = 236,
	cc4 = 34,
	cbumpmine = 32,
	cbumpmineprojectile = 33,
	cbreachcharge = 28,
	cbreachchargeprojectile = 29,
	cweaponbaseitem = 234,
	cbasecsgrenade = 8,
	csnowballprojectile = 161,
	csnowballpile = 160,
	csmokegrenadeprojectile = 157,
	csensorgrenadeprojectile = 153,
	cmolotovprojectile = 114,
	citem_healthshot = 104,
	citemdogtags = 106,
	cdecoyprojectile = 48,
	cphyspropradarjammer = 127,
	cphyspropweaponupgrade = 128,
	cphyspropammobox = 125,
	cphysproplootcrate = 126,
	citemcash = 105,
	cenvgascanister = 63,
	cdronegun = 50,
	cparadropchopper = 116,
	csurvivalspawnchopper = 171,
	cbrc4target = 27,
	cinfomapregion = 102,
	cfirecrackerblast = 72,
	cinferno = 100,
	cchicken = 36,
	cdrone = 49,
	cfootstepcontrol = 79,
	ccsgamerulesproxy = 39,
	cweaponcubemap = 0,
	cweaponcycler = 238,
	cteplantbomb = 211,
	ctefirebullets = 198,
	cteradioicon = 215,
	cplantedc4 = 129,
	ccsteam = 43,
	ccsplayerresource = 41,
	ccsplayer = 40,
	cplayerping = 131,
	ccsragdoll = 42,
	cteplayeranimevent = 212,
	chostage = 97,
	chostagecarriableprop = 98,
	cbasecsgrenadeprojectile = 9,
	chandletest = 95,
	cteamplayroundbasedrulesproxy = 174,
	cspritetrail = 166,
	cspriteoriented = 165,
	csprite = 164,
	cragdollpropattached = 148,
	cragdollprop = 147,
	cpropcounter = 142,
	cpredictedviewmodel = 140,
	cposecontroller = 136,
	cgrassburn = 94,
	cgamerulesproxy = 93,
	cinfoladderdismount = 101,
	cfuncladder = 85,
	ctefoundryhelpers = 201,
	cenvdetailcontroller = 61,
	cdangerzone = 44,
	cdangerzonecontroller = 45,
	cworldvguitext = 276,
	cworld = 275,
	cwaterlodcontrol = 231,
	cwaterbullet = 230,
	cmapvetopickcontroller = 110,
	cvotecontroller = 229,
	cvguiscreen = 228,
	cpropjeep = 144,
	cpropvehiclechoreogeneric = 0,
	ctriggersoundoperator = 227,
	cbasevphysicstrigger = 22,
	ctriggerplayermovement = 226,
	cbasetrigger = 20,
	ctest_proxytoggle_networkable = 223,
	ctesla = 218,
	cbaseteamobjectiveresource = 17,
	cteam = 173,
	csunlightshadowcontrol = 170,
	csun = 169,
	cparticleperformancemonitor = 118,
	cspotlightend = 163,
	cspatialentity = 162,
	cslideshowdisplay = 155,
	cshadowcontrol = 154,
	csceneentity = 151,
	cropekeyframe = 149,
	cragdollmanager = 146,
	cphysicspropmultiplayer = 123,
	cphysboxmultiplayer = 121,
	cpropdoorrotating = 143,
	cbasepropdoor = 16,
	cdynamicprop = 52,
	cprop_hallucination = 141,
	cpostprocesscontroller = 137,
	cpointworldtext = 135,
	cpointcommentarynode = 134,
	cpointcamera = 133,
	cplayerresource = 132,
	cplasma = 130,
	cphysmagnet = 124,
	cphysicsprop = 122,
	cstatueprop = 167,
	cphysbox = 120,
	cparticlesystem = 119,
	cmoviedisplay = 115,
	cmaterialmodifycontrol = 111,
	clightglow = 109,
	citemassaultsuituseable = 0,
	citem = 0,
	cinfooverlayaccessor = 103,
	cfunctracktrain = 92,
	cfuncsmokevolume = 91,
	cfuncrotating = 90,
	cfuncreflectiveglass = 89,
	cfuncoccluder = 88,
	cfuncmovelinear = 87,
	cfuncmonitor = 86,
	cfunc_lod = 81,
	ctedust = 193,
	cfunc_dust = 80,
	cfuncconveyor = 84,
	cfuncbrush = 83,
	cbreakablesurface = 31,
	cfuncareaportalwindow = 82,
	cfish = 75,
	cfiresmoke = 73,
	cenvtonemapcontroller = 69,
	cenvscreeneffect = 67,
	cenvscreenoverlay = 68,
	cenvprojectedtexture = 65,
	cenvparticlescript = 64,
	cfogcontroller = 78,
	cenvdofcontroller = 62,
	ccascadelight = 35,
	cenvambientlight = 60,
	centityparticletrail = 59,
	centityfreezing = 58,
	centityflame = 57,
	centitydissolve = 56,
	cdynamiclight = 51,
	ccolorcorrectionvolume = 38,
	ccolorcorrection = 37,
	cbreakableprop = 30,
	cbeamspotlight = 25,
	cbasebutton = 5,
	cbasetoggle = 19,
	cbaseplayer = 15,
	cbaseflex = 12,
	cbaseentity = 11,
	cbasedoor = 10,
	cbasecombatcharacter = 6,
	cbaseanimatingoverlay = 3,
	cbonefollower = 26,
	cbaseanimating = 2,
	cai_basenpc = 0,
	cbeam = 24,
	cbaseviewmodel = 21,
	cbaseparticleentity = 14,
	cbasegrenade = 13,
	cbasecombatweapon = 7,
	cbaseweaponworldmodel = 23,
};

class d_variant;
class recv_table;
class recv_prop;
class c_recv_proxy_data;

using recv_var_proxy_fn = void(*)(const c_recv_proxy_data* data, void* struct_ptr, void* out_ptr);
using array_length_recv_proxy_fn = void(*)(void* struct_ptr, int object_id, int current_array_length);
using data_table_recv_var_proxy_fn = void(*)(const recv_prop* prop, void** out_ptr, void* data_ptr, int object_id);

enum send_prop_type {
	_int = 0,
	_float,
	_vec,
	_vec_xy,
	_string,
	_array,
	_data_table,
	_int_64,
};
class d_variant {
public:
	union {
		float m_float;
		long m_int;
		char* m_string;
		void* m_data;
		float m_vector[3];
		__int64 m_int64;
	};
	send_prop_type type;
};
class c_recv_proxy_data {
public:
	const recv_prop* recv_prop;
	d_variant value;
	int element_index;
	int object_id;
};
class recv_prop {
public:
	char* prop_name;
	send_prop_type prop_type;
	int prop_flags;
	int buffer_size;
	int is_inside_of_array;
	const void* extra_data_ptr;
	recv_prop* array_prop;
	array_length_recv_proxy_fn array_length_proxy;
	recv_var_proxy_fn proxy_fn;
	data_table_recv_var_proxy_fn data_table_proxy_fn;
	recv_table* data_table;
	int offset;
	int element_stride;
	int elements_count;
	const char* parent_array_prop_name;
};
class recv_table {
public:
	recv_prop* props;
	int props_count;
	void* decoder_ptr;
	char* table_name;
	bool is_initialized;
	bool is_in_main_list;
};

class c_client_class {
public:
	create_client_class_fn create_fn;
	create_event_fn create_event_fn;
	char* network_name;
	recv_table* recvtable_ptr;
	c_client_class* next_ptr;
	class_ids class_id;
};

enum client_frame_stage_t {
	FRAME_UNDEFINED = -1,			// (haven't run any frames yet)
	FRAME_START,

	// A network packet is being recieved
	FRAME_NET_UPDATE_START,
	// Data has been received and we're going to start calling PostDataUpdate
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	// Data has been received and we've called PostDataUpdate on all data recipients
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	// We've received all packets, we can now do interpolation, prediction, etc..
	FRAME_NET_UPDATE_END,

	// We're about to start rendering the scene
	FRAME_RENDER_START,
	// We've finished rendering the scene.
	FRAME_RENDER_END
};

class i_base_client_dll {
public:
	c_client_class* get_client_classes() {
		using original_fn = c_client_class * (__thiscall*)(i_base_client_dll*);
		return (*(original_fn**)this)[8](this);
	}
};


namespace sigs {
	inline uint8_t* keyvalues_engine = nullptr;
	inline uint8_t* keyvalues_client = nullptr;
	inline uint8_t* input = nullptr;
	inline uint8_t* clientstate = nullptr;
	void initialize();
}

namespace hooks {
	inline unsigned int get_virtual(void* _class, unsigned int index) { return static_cast<unsigned int>((*static_cast<int**>(_class))[index]); }


	inline std::uint8_t* pattern_scan(const char* module_name, const char* signature) noexcept {
		const auto module_handle = GetModuleHandleA(module_name);

		if (!module_handle)
			return nullptr;

		static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + std::strlen(pattern);

			for (auto current = start; current < end; ++current) {
				if (*current == '?') {
					++current;

					if (*current == '?')
						++current;

					bytes.push_back(-1);
				}
				else {
					bytes.push_back(std::strtoul(current, &current, 16));
				}
			}
			return bytes;
			};

		auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
		auto nt_headers =
			reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

		auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
		auto pattern_bytes = pattern_to_byte(signature);
		auto scan_bytes = reinterpret_cast<std::uint8_t*>(module_handle);

		auto s = pattern_bytes.size();
		auto d = pattern_bytes.data();

		for (auto i = 0ul; i < size_of_image - s; ++i) {
			bool found = true;

			for (auto j = 0ul; j < s; ++j) {
				if (scan_bytes[i + j] != d[j] && d[j] != -1) {
					found = false;
					break;
				}
			}
			if (found)
				return &scan_bytes[i];
		}

		throw std::runtime_error(std::string("Wrong signature: ") + signature);
	}

	void get_local_player();

	bool initialize();

	inline entity_t* local_player;

	namespace frame_stage_notify {
		using fn = void(__thiscall*)(void*, client_frame_stage_t);
		void __stdcall hook(client_frame_stage_t frame_stage);
		inline fn original = nullptr;
	}

	namespace create_move {
		using fn = void(__stdcall*)(int, float, bool);
		void __stdcall hook(int sequence, float frame_time, bool is_active);
		inline fn original = nullptr;
	}

	namespace alloc_key_values {
		using fn = void* (__thiscall*)(void*, const std::int32_t);
		void* __stdcall hook(const std::int32_t size);
		inline fn original = nullptr;
	}
}

class c_global_vars_base {
public:
	float realtime;
	int frame_count;
	float absolute_frametime;
	float absolute_frame_start_time;
	float cur_time;
	float frame_time;
	int max_clients;
	int tick_count;
	float interval_per_tick;
	float interpolation_amount;
	int sim_ticks_this_frame;
	int network_protocol;
	void* p_save_data;
	bool is_client;
	bool is_remote_client;
	int timestamp_networking_base;
	int timestamp_randomize_window;
};

namespace interfaces {
	enum class interface_type { index, bruteforce };

	template <typename ret, interface_type type>
	ret* get_interface(const std::string& module_name, const std::string& interface_name) {
		using create_interface_fn = void* (*)(const char*, int*);
		const auto fn = reinterpret_cast<create_interface_fn>(GetProcAddress(GetModuleHandleA(module_name.c_str()), "CreateInterface"));

		if (fn) {
			void* result = nullptr;

			switch (type) {
			case interface_type::index:
				result = fn(interface_name.c_str(), nullptr);

				break;
			case interface_type::bruteforce:
				char buf[128];

				for (uint32_t i = 0; i <= 100; ++i) {
					memset(static_cast<void*>(buf), 0, sizeof buf);

					result = fn(interface_name.c_str(), nullptr);

					if (result)
						break;
				}

				break;
			}

			if (!result)
				throw std::runtime_error(interface_name + " wasn't found in " + module_name);

			return static_cast<ret*>(result);
		}

		throw std::runtime_error(module_name + " wasn't found");
	}

	inline i_base_client_dll* client;
	inline i_input* input;
	inline iv_engine_client* engine;
	inline i_client_entity_list* entity_list;
	inline i_client_state* clientstate;
	inline c_global_vars_base* globals;
	inline i_console* console;
	inline void* key_values_system;
	bool initialize();
}

namespace AC_Funcs {
	bool using_fakelag();

	bool using_micromovement(c_usercmd* cmd);

	bool possible_aimbot_zero_mouse(c_usercmd* cmd);

	bool possible_desync(bool send_packet, c_usercmd* cmd);

	bool possible_backtrack(c_usercmd* cmd);

	bool possible_rage_aim(c_usercmd* cmd, c_usercmd* prev_cmd);

	bool shifting();

	void determine_ban_createmove(bool send_packet, c_usercmd* cmd);

	inline struct ZeroMouseAimInfo {
		int windowTicks = 0;
		int hits = 0;

		int coolDown = 0; // anti-spam

		void reset() { windowTicks = 0; hits = 0; coolDown = 0; }
	} g_zm;


	class cheater_info_ {
	public:
		int last_choke_cycle = 0;
		int current_choke_peak = 0;
		int last_choke_peak = 0;
		int fakelag_score = 0;
		int fakelag_cooldown = 0;
		float sent_angle;
		float choked_angle;
		int previous_cmd_tickcount;
		int cmd_tickcount;
		vec3_t last_engine_angle{};
		int last_cmd_number_for_engine_angle;
		float previous_simtime;
		c_usercmd* prev_cmd;
		int ticks_sidemoving;
		bool shifting;
	};

	inline cheater_info_ info;

	enum AC_EventFlags : uint32_t
	{
		AC_NONE = 0,
		AC_FAKELAG_PATTERN = 1 << 0,
		AC_DESYNC_POSSIBLE = 1 << 1,
		AC_ROLL_ANGLE = 1 << 2,
		AC_BACKTRACK = 1 << 3,
		AC_TICKBASE_SHIFT = 1 << 4,
		AC_FAKEDUCK = 1 << 5,
		AC_RAGE_AIM = 1 << 6,
		AC_MICROMOVE_PATTERN = 1 << 7,
		AC_ZERO_MOUSE = 1 << 8
	};

	#pragma pack(push, 1)
	struct AC_Packet
	{
		uint64_t steam_id;
		uint32_t tick;
		uint32_t flags;
		char session_id[33];
	};
	#pragma pack(pop)

	bool send_ac_batch(const AC_Packet* pkts, uint32_t count);

	inline std::string g_sessionId;
}