#include "vectors_ui.cpp"
#include "physics_simulation_ui.cpp"
#include "vector_calculator_ui.cpp"

struct simulation_ui
{
	physics_simulation_ui Physics;
	vectors_storage_ui    VectorStorage;
    vector_calculator_ui  Calculator;
};

static void InitializeSimulationUI(HWND WindowHandle, ID3D11Device* Device, ID3D11DeviceContext* ImmediateContext)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(WindowHandle);
    ImGui_ImplDX11_Init(Device, ImmediateContext);
}

static void UIWindowResize(i32 Width, i32 Height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((f32)Width, (f32)Height);
}

static void RenderSimulationUI()
{
	static simulation_ui SimulationUI;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Simulation 3D");
    ImGui::Columns(2, "Main Columns");
    RenderVectorStorageUI(&SimulationUI.VectorStorage);

    ImGui::NextColumn();

    ImGui::Text("Simulation");
    ImGui::BeginChild("Simulation", ImVec2(0, 0), true);
    RenderCalculatorUI(&SimulationUI.Calculator, &SimulationUI.VectorStorage);
    RenderPhysicsSimulationUI(&SimulationUI.Physics);
    ImGui::EndChild();

    ImGui::End();

    ImGui::Render();
}