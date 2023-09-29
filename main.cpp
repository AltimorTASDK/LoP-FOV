#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <tuple>
#include <stdexcept>
#include <string>

#include <Windows.h>
#include <Psapi.h>

constexpr auto M_PI = 3.14159265f;

struct FVector {
	float X, Y, Z;
};

struct FLPlayerCameraData {
	float FOV;
	float PitchMin;
	float PitchMax;
	float TargetArmLength;
	FVector SocketOffset;
	FVector TargetOffset;
	bool bEnableCameraLag : 1;
	bool bEnableCameraRotationLag : 1;
	float CameraLagSpeed;
	float CameraRotationLagSpeed;
	float CameraLagMaxDistance;
	float DialogCamZoom;
};

struct ALCameraManager {
	char pad0000[0x2A78];
	FLPlayerCameraData DefaultCameraData;
	char pad2AB4[0x2C00 - 0x2AB4];
};

struct UCameraComponent {
	char pad000[0x1E8];
	float FieldOfView;
	char pad1EC[0x810 - 0x1EC];
};

struct USpringArmComponent {
	char pad000[0x1E8];
	float TargetArmLength;
	char pad1EC[0x210 - 0x1EC];
	float CameraLagSpeed;
	char pad214[0x270 - 0x214];
};

constexpr auto DEFAULT_FOV = 75.f;
constexpr auto DEFAULT_DISTANCE = 350.f;

extern "C" float fov_multiplier;
extern "C" float distance_multiplier;

float fov_multiplier;
float distance_multiplier;

static float scale_fov(float fov, float multiplier)
{
	return atanf(tanf(fov * M_PI / 360) * multiplier) * 360 / M_PI;
}

extern "C" void set_camera_component_fov(const ALCameraManager *manager, UCameraComponent *camera)
{
	camera->FieldOfView = scale_fov(manager->DefaultCameraData.FOV, fov_multiplier);
}

extern "C" void set_spring_arm_length(USpringArmComponent *spring_arm, float length)
{
	spring_arm->TargetArmLength = length * distance_multiplier;
}

extern "C" void set_spring_arm_lag(USpringArmComponent *spring_arm, float lag)
{
	spring_arm->CameraLagSpeed = lag / distance_multiplier;
}

static void read_config()
{
	auto file = std::ifstream("fov.ini");
	auto fov = DEFAULT_FOV;
	auto distance = DEFAULT_DISTANCE;

	std::string line;

	while (std::getline(file, line)) {
		// Remove comments
		if (const auto comment = line.find(';'); comment != std::string::npos)
			line.erase(comment);

		// Remove whitespace
		std::erase(line, ' ');
		std::erase(line, '\t');

		const auto delim = line.find('=');

		if (delim == std::string::npos)
			continue;

		const auto key = line.substr(0, delim);

		char *parse_end;
		const auto value = strtof(line.substr(delim + 1).c_str(), &parse_end);

		// Must be a valid float
		if (*parse_end != '\0')
			continue;

		if (key == "FOV")
			fov = value;
		else if (key == "Distance")
			distance = value;
	}

	fov_multiplier = tanf(fov * M_PI / 360) / tanf(DEFAULT_FOV * M_PI / 360);
	distance_multiplier = distance / DEFAULT_DISTANCE / fov_multiplier;
}

static std::tuple<std::byte*, std::byte*> get_module_bounds(const char *name)
{
	const auto handle = GetModuleHandle(name);
	if (handle == nullptr)
		return {nullptr, nullptr};

	MODULEINFO info;
	GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info));
	auto *start = (std::byte*)info.lpBaseOfDll;
	auto *end = start + info.SizeOfImage;
	return {start, end};
}

static std::byte *sigscan(const char *name, const char *sig, const char *mask)
{
	auto [start, end] = get_module_bounds(name);
	auto *last_scan = end - strlen(mask) + 1;

	for (auto *addr = start; addr < last_scan; addr++) {
		for (size_t i = 0; ; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != (char)addr[i])
				break;
		}
	}

	throw std::runtime_error("Signature not found");
}

static void apply_call_hook(void *target, const void *hook, size_t pad_size)
{
	constexpr size_t patch_size = 12;
	const auto total_size = max(patch_size, pad_size);

	DWORD old_protect;
	VirtualProtect(target, total_size, PAGE_EXECUTE_READWRITE, &old_protect);

	// mov rax, hook
	*(uint16_t*)target = 0xB848;
	*(const void**)((std::byte*)target + 2) = hook;
	// call rax
	*(uint16_t*)((std::byte*)target + 10) = 0xD0FF;
	// nop
	memset((std::byte*)target + patch_size, 0x90, total_size - patch_size);

	VirtualProtect(target, total_size, old_protect, &old_protect);
}

extern "C" void hook_ALCameraManager_SetupDefaultCamera_set_fov();
extern "C" void hook_ALCameraManager_UpdateCamera_set_dist();
extern "C" void hook_ALCameraManager_UpdateCamera_set_lag();
extern "C" void hook_FLRotationAccordingToMovement_UpdateRotation_scale();

static void apply_hooks()
{
	auto *fov_target = sigscan(
		"LOP-Win64-Shipping.exe",
		// mov eax, [rdi+0x2A78]
		// mov [r9+0x1E8], eax
		"\x8B\x87\x78\x2A\x00\x00\x41\x89\x81\xE8\x01\x00\x00",
		"xxxxxxxxxxxxx");

	apply_call_hook(fov_target, hook_ALCameraManager_SetupDefaultCamera_set_fov, 13);

	auto *dist_target = sigscan(
		"LOP-Win64-Shipping.exe",
		// movss [r9+0x1E8], xmm0
		// xorps xmm0, xmm0
		"\xF3\x41\x0F\x11\x81\xE8\x01\x00\x00\x0F\x57\xC0",
		"xxxxxxxxxxxx");

	apply_call_hook(dist_target, hook_ALCameraManager_UpdateCamera_set_dist, 12);

	auto *lag_target = sigscan(
		"LOP-Win64-Shipping.exe",
		// movss [r9+0x210], xmm0
		// movaps xmm1, xmm6
		"\xF3\x41\x0F\x11\x81\x10\x02\x00\x00\x0F\x28\xCE",
		"xxxxxxxxxxxx");

	apply_call_hook(lag_target, hook_ALCameraManager_UpdateCamera_set_lag, 12);

	auto *rotate_target = sigscan(
		"LOP-Win64-Shipping.exe",
		// movss [rsp+0x48], xmm0
		// addss xmm6, [rsp+0x34]
		"\xF3\x0F\x11\x44\x24\x48\xF3\x0F\x58\x74\x24\x34",
		"xxxxxxxxxxxx");

	apply_call_hook(rotate_target, hook_FLRotationAccordingToMovement_UpdateRotation_scale, 12);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
	if (reason != DLL_PROCESS_ATTACH)
		return FALSE;

	read_config();
	apply_hooks();

	return TRUE;
}
