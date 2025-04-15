struct vector_ui
{
    char Label[64] = {};
    simulation_vector* Vector;
};

struct vector_state_change
{
    bool ColorChanged;
    bool PositionChanged;
};

struct new_vector_ui
{
    char  Name[64]   = { "Vecteur sans nom." };
    vec_4 Color      = vec_4(1.0f, 1.0f, 1.0f, 1.0f);
    vec_3 Origin     = vec_3();
    vec_3 Direction  = vec_3(1.0f, 1.0f, 1.0f);
    vec_3 Rotation   = vec_3(0.0f, 0.0f, 0.0f);
};

struct external_vector_ui
{
    bool Initialized          = false;
    char Label[64]            = {};
    simulation_vector* Vector = nullptr;
};

struct vectors_storage_ui
{
    u32       Count                   = 0;
    vector_ui Vectors[MAX_VECTORS]    = {};
    new_vector_ui NewVector           = {};
    external_vector_ui ExternalVector = {};
};

static vector_state_change ShowVectorMenu(vec_4* OutColor, vec_3* OutOrigin, vec_3* OutDirection,
                                          vec_3* OutRotation, char* OutName, size_t OutNameSize, bool Sliders)
{
    vector_state_change StateChange = {};

    if (ImGui::BeginTable("VectorTable", 2, 0))
    {
        f32 LabelColumnWidth = 75.0f;
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        if (OutNameSize != 0)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Nom:");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputText("##Name", OutName, OutNameSize);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Couleur:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(200.0f);
        StateChange.ColorChanged = ImGui::ColorEdit4("##Color", OutColor->AsArray, ImGuiColorEditFlags_NoInputs);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Origine:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(200.0f);
        if (Sliders)
        {
            StateChange.PositionChanged |= ImGui::SliderFloat3("##Origin", OutOrigin->AsArray, -10.0f, 10.0f);
        }
        else
        {
            StateChange.PositionChanged |= ImGui::InputFloat3("##Origin", OutOrigin->AsArray);
        }
        

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Direction:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(200.0f);
        if (Sliders)
        {
            StateChange.PositionChanged |= ImGui::SliderFloat3("##Direction", OutDirection->AsArray, -10.0f, 10.0f);
        }
        else
        {
            StateChange.PositionChanged |= ImGui::InputFloat3("##Direction", OutDirection->AsArray);
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Rotation X:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(200.0f);
        StateChange.PositionChanged |= ImGui::SliderFloat("##RotationX", &OutRotation->x, 0.0f, 360.0f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Rotation Y:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(200.0f);
        StateChange.PositionChanged |= ImGui::SliderFloat("##RotationY", &OutRotation->y, 0.0f, 360.0f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Rotation Z:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(200.0f);
        StateChange.PositionChanged |= ImGui::SliderFloat("##RotationXZ", &OutRotation->z, 0.0f, 360.0f);

        ImGui::EndTable();
    }

    return StateChange;
}

static void RenderVectorStorageUI(vectors_storage_ui* VectorStorage)
{

    // Initialisation du UI
    if (VectorStorage->Count == 0)
    {
        vector_ui XAxis = {};
        XAxis.Vector    = CreateSimulationVector(vec_3(0.0f, 0.0f, 0.0f), vec_3(1.0f, 0.0f, 0.0f), vec_4(1.0f, 0.0f, 0.0f, 1.0f));;
        CreateVectorEntity(XAxis.Vector);
        memcpy(XAxis.Label, "Axe X", 6);

        vector_ui YAxis = {};
        YAxis.Vector    = CreateSimulationVector(vec_3(), vec_3(0.0f, 1.0f, 0.0f), vec_4(0.0f, 1.0f, 0.0f, 1.0f));
        CreateVectorEntity(YAxis.Vector);
        memcpy(YAxis.Label, "Axe Y", 6);

        vector_ui ZAxis = {};
        ZAxis.Vector    = CreateSimulationVector(vec_3(), vec_3(0.0f, 0.0f, 1.0f), vec_4(0.0f, 0.0f, 1.0f, 1.0f));
        CreateVectorEntity(ZAxis.Vector);
        memcpy(ZAxis.Label, "Axe Z", 6);

        VectorStorage->Vectors[0] = XAxis;
        VectorStorage->Vectors[1] = YAxis;
        VectorStorage->Vectors[2] = ZAxis;
        VectorStorage->Count      = 3;
    }

    // Creer un vecteur a partir d'un autre vecteur creer a l'exterieur de ce systeme.
    if (VectorStorage->ExternalVector.Initialized)
    {
        u32 Index = VectorStorage->Count;
        VectorStorage->Count++;

        vector_ui* VectorUI = &VectorStorage->Vectors[Index];
        VectorUI->Vector    = VectorStorage->ExternalVector.Vector;
        memcpy(VectorUI->Label, VectorStorage->ExternalVector.Label, sizeof(VectorUI->Label));

        VectorStorage->ExternalVector.Initialized = false;
    }

    ImGui::Text("Vecteurs");
    ImGui::BeginChild("StaticVectors", ImVec2(0, 0), true);

    // Logique pour ajouter des nouveaux vecteurs
    if (ImGui::Button("Ajouter Vecteur"))
    {
        ImGui::OpenPopup("Ajouter Vecteur");
    }

    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Once);
    if (ImGui::BeginPopupModal("Ajouter Vecteur", NULL))
    {
        new_vector_ui* NewVector = &VectorStorage->NewVector;
        ShowVectorMenu(&NewVector->Color, &NewVector->Origin, &NewVector->Direction, 
                        &NewVector->Rotation, NewVector->Name, sizeof(NewVector->Name),
                        false);

        if (ImGui::Button("Creer", ImVec2(137, 0)))
        {
            if (CanCreateVector())
            {
                vector_ui* V = &VectorStorage->Vectors[VectorStorage->Count];
                VectorStorage->Count++;

                V->Vector = CreateSimulationVector(VectorStorage->NewVector.Origin, VectorStorage->NewVector.Direction, VectorStorage->NewVector.Color);
                CreateVectorEntity(V->Vector);
                strncpy_s(V->Label, VectorStorage->NewVector.Name, sizeof(V->Label) - 1);
                V->Label[sizeof(V->Label) - 1] = '\0';
            }

            VectorStorage->NewVector = new_vector_ui();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Annuler", ImVec2(137, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Montre les vecteurs dans une liste
    for (u32 Index = 0; Index < VectorStorage->Count; Index++)
    {
        vector_ui* VecUI = &VectorStorage->Vectors[Index];

        char TreeLabel[128];
        snprintf(TreeLabel, sizeof(TreeLabel), "%s##%d", VecUI->Label, Index);

        bool NodeIsOpen = ImGui::TreeNodeEx(TreeLabel, ImGuiTreeNodeFlags_SpanAvailWidth);
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("VECTOR_UI", &VecUI, sizeof(vector_ui*));
            ImGui::TextUnformatted(VecUI->Label);
            ImGui::EndDragDropSource();
        }

        if (NodeIsOpen)
        {
            bool ColorChanged = false;
            bool PositionChanged = false;

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 2));

            simulation_vector* Vector       = VecUI->Vector;
            vector_state_change VectorState = ShowVectorMenu(&Vector->Color, &Vector->Origin, &Vector->Direction,
                                                             &Vector->Rotation, nullptr, 0, true);

            if (VectorState.PositionChanged)
            {
                UpdateVectorPosition(VecUI->Vector);
            }

            if (VectorState.ColorChanged)
            {
                UpdateVectorColor(VecUI->Vector);
            }

            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
}