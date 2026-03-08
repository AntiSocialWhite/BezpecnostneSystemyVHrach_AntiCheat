#include "DllModule.h"

void hooks::get_local_player()
{
    auto lp_idx = interfaces::engine->get_local_player();
    hooks::local_player = lp_idx && interfaces::engine->is_connected() ? static_cast<entity_t*>(interfaces::entity_list->get_client_entity(lp_idx)) : nullptr;
}

bool interfaces::initialize() {
	client = get_interface<i_base_client_dll, interface_type::index>("client.dll", "VClient018");
	input = *reinterpret_cast<i_input**>(sigs::input + 1);
    engine = get_interface<iv_engine_client, interface_type::index>("engine.dll", "VEngineClient014");
    entity_list = get_interface<i_client_entity_list, interface_type::index>("client.dll", "VClientEntityList003");
    key_values_system = reinterpret_cast<void* (__stdcall*)()>(GetProcAddress(GetModuleHandle("vstdlib.dll"), "KeyValuesSystem"))();
    clientstate = **reinterpret_cast<i_client_state***>(sigs::clientstate + 1);
    console = get_interface<i_console, interface_type::index>("vstdlib.dll", "VEngineCvar007");
    globals = **reinterpret_cast<c_global_vars_base***>((*reinterpret_cast<uintptr_t**>(client))[11] + 10);

    return true;
}

void sigs::initialize()
{
    keyvalues_engine = hooks::pattern_scan("engine.dll", "E8 ? ? ? ? 83 C4 08 84 C0 75 10 FF 75 0C");
    keyvalues_client = hooks::pattern_scan("client.dll", "E8 ? ? ? ? 83 C4 08 84 C0 75 10");
    input = hooks::pattern_scan("client.dll", "B9 ? ? ? ? F3 0F 11 04 24 FF 50 10");
    clientstate = hooks::pattern_scan("engine.dll", "A1 ? ? ? ? 8B 80 ? ? ? ? C3");
}

bool hooks::initialize() {
	const auto create_move_target = reinterpret_cast<void*>(get_virtual(interfaces::client, 22));
    const auto key_values_system_target = reinterpret_cast<void*>(get_virtual(interfaces::key_values_system, 2));
    const auto frame_stage_notify_target = reinterpret_cast<void*>(get_virtual(interfaces::client, 37));

	if (MH_Initialize() != MH_OK)
		throw std::runtime_error("failed to initialize MH_Initialize.");

    if (MH_CreateHook(key_values_system_target, &alloc_key_values::hook, reinterpret_cast<void**>(&alloc_key_values::original)) != MH_OK)
        throw std::runtime_error("failed to initialize alloc_key_values. (outdated index?)");

	if (MH_CreateHook(create_move_target, &create_move::hook, reinterpret_cast<void**>(&create_move::original)) != MH_OK)
		throw std::runtime_error("failed to initialize create_move. (outdated index?)");

    if (MH_CreateHook(frame_stage_notify_target, &frame_stage_notify::hook, reinterpret_cast<void**>(&frame_stage_notify::original)) != MH_OK)
        throw std::runtime_error("failed to initialize frame_stage_notify. (outdated index?)");

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        throw std::runtime_error("failed to enable hooks.");

    return true;
}

inline std::uintptr_t GetAbsoluteAddress(const std::uintptr_t uRelativeAddress)
{
    return uRelativeAddress + 0x4 + *reinterpret_cast<std::int32_t*>(uRelativeAddress);
}

void* __stdcall hooks::alloc_key_values::hook(const std::int32_t size) {
    static auto uAllocKeyValuesEngine = GetAbsoluteAddress((uintptr_t)(sigs::keyvalues_engine)+0x1) + 0x4A;
    static auto uAllocKeyValuesClient = GetAbsoluteAddress((uintptr_t)(sigs::keyvalues_client + 0x1)) + 0x3E;
    const auto address = std::uintptr_t(_ReturnAddress());
    if (address == uAllocKeyValuesEngine || address == uAllocKeyValuesClient)
    {
        return nullptr;
    }

    return hooks::alloc_key_values::original(interfaces::key_values_system, size);
}

static void __stdcall createmove(int nSequenceNumber, float input_sample_frametime, bool bIsActive, bool& send_packet) {
	hooks::create_move::original(nSequenceNumber, input_sample_frametime, bIsActive);

	auto cmd = &interfaces::input->GetVerifiedCmd(nSequenceNumber - 1)->userCmd; // only check verified CMDs, cheats have to "verify" them anyways.
	if (!cmd || !cmd->command_number || !cmd->tick_count || !cmd->randomseed)
		return;

    AC_Funcs::determine_ban_createmove(interfaces::clientstate->iChokedCommands > 0, cmd);

    // have to save this after running the detection, so the next tick uses the previous tick data.
    vec3_t engine_angles{};
    interfaces::engine->get_view_angles(&engine_angles);
    AC_Funcs::info.last_engine_angle = engine_angles;
    AC_Funcs::info.last_cmd_number_for_engine_angle = nSequenceNumber;//cmd->command_number;

    return;
}

void __stdcall hooks::frame_stage_notify::hook(client_frame_stage_t frame_stage)
{
    if (frame_stage == FRAME_RENDER_END) {
        //AC_Funcs::determine_ban_framestage();
    }
    frame_stage_notify::original(interfaces::client, frame_stage);
}

// fix for createmove hook after riptide update (last leaked game source is from 2020, gotta use asm)
__declspec(naked) void __stdcall hooks::create_move::hook(int sequence, float frame_time, bool is_active) {
    __asm
    {
        push ebp
        mov ebp, esp
        push ebx
        lea ecx, [esp]
        push ecx
        push dword ptr[ebp + 10h]
        push dword ptr[ebp + 0Ch]
        push dword ptr[ebp + 8]
        call createmove
        pop ebx
        pop ebp
        retn 0Ch
    }
}