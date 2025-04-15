struct physics_simulation_ui
{
    bool ApplyGravity     = false;
    simulation_cube* Cube = nullptr;
    vector_ui*        ForceVector;
};

static void RenderPhysicsSimulationUI(physics_simulation_ui* PhysicsSimulation)
{
    if (!PhysicsSimulation->Cube)
    {
        PhysicsSimulation->Cube = CreateSimulationCube(vec_3(1.0f, 0.5f, 1.0f));
    }

    ImGui::SeparatorText("Simulation physique");

    ImGui::BeginChild("ForceVectorTarget", ImVec2(140, 30), true);
    {
        if (PhysicsSimulation->ForceVector)
        {
            ImGui::Text("%s", PhysicsSimulation->ForceVector->Label);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Deposez un vecteur de force ici.");
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("VECTOR_UI"))
            {
                IM_ASSERT(Payload->DataSize == sizeof(vector_ui*));
                PhysicsSimulation->ForceVector = *(vector_ui**)Payload->Data;
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();

    if (ImGui::BeginTable("Simulation", 2, ImGuiTableFlags_SizingStretchSame))
    {
        f32 LabelColumnWidth = 100.0f;
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Puissance:");
        ImGui::TableSetColumnIndex(1);
        ImGui::InputFloat("##ForceMagnitude", &PhysicsSimulation->Cube->ForceMagnitude);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Gravite:");
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##ApplyGravity", &PhysicsSimulation->Cube->AffectedByGravity);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Simulation:");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::Button("Simuler le cube", ImVec2(150, 25)) && PhysicsSimulation->ForceVector)
        { 
            vec_3 ForceToApply = PhysicsSimulation->ForceVector->Vector->Direction;
            ApplyPushForce(PhysicsSimulation->Cube, ForceToApply);

            PhysicsSimulation->Cube->IsBeingSimulated = true;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Simulation:");
        ImGui::TableSetColumnIndex(1);
        if (ImGui::Button("Reinitialiser le cube", ImVec2(150, 25)) && PhysicsSimulation->Cube->IsBeingSimulated)
        {
            StopCubeSimulation(PhysicsSimulation->Cube);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Position:");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("(%f, %f, %f)", PhysicsSimulation->Cube->Position.x, PhysicsSimulation->Cube->Position.y, PhysicsSimulation->Cube->Position.z);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Velocite:");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("(%f, %f, %f)", PhysicsSimulation->Cube->Velocity.x, PhysicsSimulation->Cube->Velocity.y, PhysicsSimulation->Cube->Velocity.z);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Masse:");
        ImGui::TableSetColumnIndex(1);
        ImGui::InputFloat("##CubeMass", &PhysicsSimulation->Cube->Mass);

        ImGui::EndTable();
    }
}