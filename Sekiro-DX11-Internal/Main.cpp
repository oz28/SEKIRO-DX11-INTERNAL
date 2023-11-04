#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "MinHook/MinHook.h"
#if _WIN64 
#pragma comment(lib, "MinHook/libMinHook.x64.lib")
#else
#pragma comment(lib, "MinHook/libMinHook.x86.lib")
#endif

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#include <d3d11.h>
#include <string>
#include <list>
#include "./Hack/hack.h"
#include <vector>
#include "./Mathtools/mathtools.h"
#include <sstream>
#include <iostream>
#include "images.h"
#include "./Hooks/hooks.h"


using namespace std;
#pragma comment(lib, "d3d11.lib")


bool boneMenu = false;
bool hackMenu = true;


std::vector<bonePoint> allPoints;
std::vector<bone> bones;
std::vector<boneNumber> allNumber;

int index2;

static bool esp = false;
int boneIndex = 0;
int entIndex = 0;
int drawRange1 = 0;




// Globals
HINSTANCE dll_handle;

typedef long(__stdcall* present)(IDXGISwapChain*, UINT, UINT);
present p_present;
present p_present_target;
bool get_present_pointer()
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = GetForegroundWindow();
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	IDXGISwapChain* swap_chain;
	ID3D11Device* device;

	const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(
		NULL, 
		D3D_DRIVER_TYPE_HARDWARE, 
		NULL, 
		0, 
		feature_levels, 
		2, 
		D3D11_SDK_VERSION, 
		&sd, 
		&swap_chain, 
		&device, 
		nullptr, 
		nullptr) == S_OK)
	{
		void** p_vtable = *reinterpret_cast<void***>(swap_chain);
		swap_chain->Release();
		device->Release();
		//context->Release();
		p_present_target = (present)p_vtable[8];
		return true;
	}
	return false;
}

WNDPROC oWndProc;
bool is_inactive = false;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
	if (io.WantCaptureMouse)
	{
		if (!is_inactive) {
			CallWindowProc(reinterpret_cast<WNDPROC>(oWndProc), hWnd, WM_ACTIVATE, WA_INACTIVE, 0);
			is_inactive = true;
		}
		return TRUE;
	}
	else if (is_inactive) {
		CallWindowProc(reinterpret_cast<WNDPROC>(oWndProc), hWnd, WM_ACTIVATE, WA_ACTIVE, 0);
		is_inactive = false;
	}

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


bool init = false;
HWND window = NULL;
ID3D11Device* p_device = NULL;
ID3D11DeviceContext* p_context = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load from disk into a raw RGBA buffer
	int image_width = 0;
	int image_height = 0;
	//unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	unsigned char* image_data = stbi_load_from_memory(sekiroAdamRawData, sizeof(sekiroAdamRawData), &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = image_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	p_device->CreateTexture2D(&desc, &subResource, &pTexture);
	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	p_device->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}

int my_image_width = 0;
int my_image_height = 0;
ID3D11ShaderResourceView* my_texture = NULL;


ImGuiWindowFlags window_flags = 0;


RECT rect;

void SetupImGuiStyle()
{
	// Comfy style by Giuseppe from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.1000000014901161f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 10.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(30.0f, 30.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 5.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 10.0f;
	style.PopupBorderSize = 0.0f;
	style.FramePadding = ImVec2(5.0f, 3.5f);
	style.FrameRounding = 5.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(5.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 5.0f;
	style.ColumnsMinSpacing = 5.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 15.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.3605149984359741f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.54935622215271f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3803921639919281f, 0.4235294163227081f, 0.572549045085907f, 0.5490196347236633f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2588235437870026f, 0.2588235437870026f, 0.2588235437870026f, 0.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.2352941185235977f, 0.2352941185235977f, 0.2352941185235977f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509803950786591f, 1.0f, 0.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 0.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.5490196347236633f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.2918455004692078f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03433477878570557f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}


static long __stdcall detour_present(IDXGISwapChain* p_swap_chain, UINT sync_interval, UINT flags) {
	if (!init) {
		if (SUCCEEDED(p_swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&p_device)))
		{
			p_device->GetImmediateContext(&p_context);
			DXGI_SWAP_CHAIN_DESC sd;
			p_swap_chain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			p_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			p_device->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
			ImGui_ImplWin32_Init(window);
			ImGui_ImplDX11_Init(p_device, p_context);
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoResize;
			init = true;
			SetupImGuiStyle();

		}
		else
			return p_present(p_swap_chain, sync_interval, flags);
	}
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	
	ImGui::NewFrame();

	if (boneMenu)
	{
		ImGui::Begin("Bone Finder Toole");
		static char address[64];
		static char offsetinput[64];
		static char offsetBonesInput[64];
		ImGui::InputText("adress", address, IM_ARRAYSIZE(address));
		ImGui::SameLine();
		ImGui::InputText("offsetBonePos", offsetinput, IM_ARRAYSIZE(offsetinput));
		ImGui::InputText("offsetBones", offsetBonesInput, IM_ARRAYSIZE(offsetBonesInput));
		static size_t adress2;
		static size_t offsetBonePos = 0;
		static size_t offsetBone = 0;

		std::istringstream ss(address);
		ss >> std::hex >> adress2;

		std::istringstream ss1(offsetinput);
		ss1 >> std::hex >> offsetBonePos;

		std::istringstream ss2(offsetBonesInput);
		ss2 >> std::hex >> offsetBone;


		ImGui::InputInt("DrawRange", &drawRange1);

		if (ImGui::Button("FASTADD"))
		{

			for (size_t i = 0; i < drawRange1; i++)
			{
				bone boni;
				boni.x = (float*)(adress2+ offsetBone * i + offsetBonePos * 0x0);
				boni.y = (float*)(adress2+ offsetBone * i + offsetBonePos * 0x1);
				boni.z = (float*)(adress2 + offsetBone * i + offsetBonePos * 0x2);

				//int index = bones.size();
				//sprintf(boni.info, "%d", index);

				index2++;
				sprintf(boni.info, "%d", index2);

				bones.push_back(boni);


			}

		}



		if (ImGui::Button("Clear List")) {
			bones.clear();
			index2 = 0;
		}

		for (size_t i = 0; i < bones.size(); i++)
		{
			Vector3 bonesconvert;
			bonesconvert.x = *bones[i].x;
			bonesconvert.y = *bones[i].y;
			bonesconvert.z = *bones[i].z;
		}
		ImGui::End();

	
		ImGui::Begin("Bone Finder Toole");
		ImGui::Text("Fastest Bone Finder");
		static float p1X = 0.0f;
		ImGui::InputFloat("P1 X", &p1X, 0.05f, 0, "%.3f",1);
		ImGui::NextColumn();
		static float p1Y = 0.0f;
		ImGui::InputFloat("P1 Y", &p1Y, 0.05f, 0, "%.3f", 1);
		ImGui::NextColumn();
		static float p1Z = 0.0f;
		ImGui::InputFloat("P1 Z", &p1Z, 0.05f, 0, "%.3f", 1);
		ImGui::NextColumn();

		ImGui::Separator();
		static float p2X = 0.0f;
		ImGui::InputFloat("P2 X", &p2X, 0.05f, 0, "%.3f", 1);
		ImGui::NextColumn();
		static float p2Y = 0.0f;
		ImGui::InputFloat("P2 Y", &p2Y, 0.05f, 0, "%.3f", 1);
		ImGui::NextColumn();
		static float p2Z = 0.0f;
		ImGui::InputFloat("P2 Z", &p2Z, 0.05f, 0, "%.3f", 1);
		ImGui::NextColumn();
	
		if (ImGui::Button("ADD"))
		{
			Vector3 p1; p1.x = p1X; p1.y = p1Y; p1.z = p1Z;
			Vector3 p2; p2.x = p2X; p2.y = p2Y; p2.z = p2Z;
	
			bonePoint point;
			point.p1 = p1;
			point.p2 = p2;

			boneNumber number;
			number.p1 = p1;


			int index = allPoints.size();
			int indexNumber = allNumber.size();

			sprintf(point.info, "%d", index);
			sprintf(number.info, "%d", indexNumber);
	
			allPoints.push_back(point);
			allNumber.push_back(number);

	
		}

		if (ImGui::BeginListBox("coords")){
			for (size_t i = 0; i < allPoints.size(); i++)
			{
				bonePoint point = allPoints[i];
				if (ImGui::Selectable(point.info, boneIndex == i)) {
					boneIndex = i;

				}

			}

			ImGui::EndListBox();

	
		}

		if (ImGui::Button("DEL"))
		{
			if(allPoints.size()>0)
			allPoints.erase(allPoints.begin() + boneIndex);
		}


		ImGui::End();
	}

	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureMouse = hackMenu;
	io.MouseDrawCursor = hackMenu;

	if (hackMenu)
	{
		ImGui::Begin("Sekiro Internal Multihack", &hackMenu, window_flags);

		ImGui::Text("Sekiro App Ver. 1.06");
		ImGui::Text("   ");
		ImGui::Text("   ");
		ImGui::SameLine();;
		ImGui::SetCursorPos(ImVec2(290, 0));
		ImGui::Image((void*)my_texture, ImVec2(36, 36));
		ImGui::Separator();
		GetFPS();

		ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "FPS: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), fpsconvert);

		if (ImGui::CollapsingHeader("ESP Settings"))
		{
			ImGui::SliderFloat("Draw Range ", &drawRange, 0.0f, 100.0f);;
			if (ImGui::Checkbox("Snapline", &snapline)) { clearEntityList(); };
			ImGui::Checkbox("Show Rotation", &dirESP);
			if (ImGui::Checkbox("Entity Health", &healthesp)) {
				clearEntityList();
			};
			ImGui::Checkbox("Entity Coords", &coordsesp);
			ImGui::Checkbox("Player Skeleton ESP", &playerskeletesp);			
		}

		if (ImGui::CollapsingHeader("CAM Settings"))
		{
			static float f1 = 0.02f;
			fovasm = f1;
			ImGui::SliderFloat("FOV", &f1, 0.02f,0.05f);
		}
		if (ImGui::CollapsingHeader("ENGINE Settings"))
		{
			if (ImGui::Checkbox("60 FPS", &frameRate60Selected)) {
				frameRate60Selected = true;
				frameRate144Selected = false;
				frameRate240Selected = false;
				InitFrameRateLock();
			}
			if (ImGui::Checkbox("144 FPS", &frameRate144Selected)) {
				frameRate60Selected = false;
				frameRate144Selected = true;
				frameRate240Selected = false;
				InitFrameRateLock();
			}
			if (ImGui::Checkbox("240 FPS", &frameRate240Selected)){
				frameRate60Selected = false;
				frameRate144Selected = false;
				frameRate240Selected = true;
				InitFrameRateLock();
			}

		}

		if (ImGui::CollapsingHeader("INTERFACE Settings"))
		{
			ImGui::Checkbox("Show HUD", &displayHud);
			HUDModification();
		}


		if (ImGui::CollapsingHeader("MISC"))
		{
			ImGui::Text("Entitys");
			if (ImGui::BeginListBox("     "))
			{
				for (size_t i = 0; i < 200; i++)
				{
					if (ents[i] != NULL) {

						char final[999];
						float x = ents[i]->coords.x;
						float y = ents[i]->coords.y;
						float z = ents[i]->coords.z;

						char buffer[999];
						sprintf(buffer, "%.3f", x);

						strcpy(final, buffer); // copy string one into the result.
						sprintf(buffer, "%.3f", y);
						strcat(final, buffer); // copy string one into the result.
						sprintf(buffer, "%.3f", z);
						strcat(final, buffer); // copy string one into the result.

						uintptr_t currbaseaddr = (uintptr_t)ents[i];

						std::stringstream ss;
						ss << std::hex << currbaseaddr; // int decimal_value

						const std::string tmp = ss.str();
						const char* cstr = tmp.c_str();

						strcat(final, cstr);


						if (ImGui::Selectable(final, entIndex == i)) {
							entIndex = i;

						}
					}


				}
				ImGui::EndListBox();
			}
		
			ImGui::SameLine();
			if (ImGui::BeginTable("table2",3,0,ImVec2(500,500)))
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Button("TP to Entity")) {
					ents[entIndex]->coords.y += 0.5f;
					TeleportToEntity(ents[entIndex]->coords,true, 0);
					clearEntityList();
				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Button("TP Entity to me")) {
					ents[entIndex]->coords.y += 0.5f;
					TeleportToEntity(ents[0]->coords, false, entIndex);
					//clearEntityList();

				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Button("TP everyone to me")) {
					TeleportEveryoneToMe();
					//clearEntityList();
				}

				if (ImGui::Button("Clear List")) {
					clearEntityList();
				}
				if (ImGui::Button("Copy BaseAddr")) {

					uintptr_t currbaseaddr = (uintptr_t)ents[entIndex];

					std::stringstream ss;
					ss << std::hex << currbaseaddr; // int decimal_value

					const std::string tmp = ss.str();
					const char* cstr = tmp.c_str();

					const size_t len = strlen(cstr) + 1;

					HGLOBAL hgl = GlobalAlloc(GMEM_MOVEABLE, len);
					memcpy(GlobalLock(hgl), cstr, len);
					GlobalUnlock(hgl);

					OpenClipboard(NULL);
					EmptyClipboard();
					SetClipboardData(CF_TEXT, hgl);
					CloseClipboard();
				}
				ImGui::EndTable();
			}





		
		}



		ImGui::End();
	}


	if (snapline || healthesp || coordsesp || playerskeletesp || dirESP) {
		SetupHack();
		ESP();


	}
	
	ImGui::EndFrame();
	ImGui::Render();

	p_context->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		return p_present(p_swap_chain, sync_interval, flags);
}


DWORD __stdcall EjectThread(LPVOID lpParameter) {
	eject = true;
	frameRate144Selected = false;
	frameRate240Selected = false;
	InitFrameRateLock();
	Sleep(200);
	RestoreJumps();
	Sleep(100);
	FreeLibraryAndExitThread(dll_handle, 0);
	Sleep(100);
	return 0;
}





//"main" loop
int WINAPI main()
{

	if (!get_present_pointer()) 
	{
		return 1;
	}

	MH_STATUS status = MH_Initialize();
	if (status != MH_OK)
	{
		return 1;
	}

	if (MH_CreateHook(reinterpret_cast<void**>(p_present_target), &detour_present, reinterpret_cast<void**>(&p_present)) != MH_OK) {
		return 1;
	}

	if (MH_EnableHook(p_present_target) != MH_OK) {
		return 1;
	}

	while (true) {
		Sleep(50);
		

		if (GetAsyncKeyState(VK_F1) & 1) {
			ImGui::SetNextWindowSize(ImVec2(300, 300));
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			hackMenu = !hackMenu;

		}

		if (GetAsyncKeyState(VK_F2) & 1) {
			boneMenu = !boneMenu;

		}

		if (GetAsyncKeyState(VK_F12)) {
			break;
		}
	}

	//Cleanup
	if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
		return 1;
	}
	if (MH_Uninitialize() != MH_OK) {
		return 1;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
	if (p_context) { p_context->Release(); p_context = NULL; }
	if (p_device) { p_device->Release(); p_device = NULL; }
	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));

	CreateThread(0, 0, EjectThread, 0, 0, 0);

	return 0;
}



BOOL __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		dll_handle = hModule;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, NULL, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InitiateHooks, NULL, 0, NULL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{

	}
	return TRUE;
}
