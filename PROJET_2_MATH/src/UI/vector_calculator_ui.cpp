enum OPERATION_TYPE
{
    OPERATION_ADDITION,
    OPERATION_SUBTRACTION,
    OPERATION_VECTOR_PRODUCT,
    OPERATION_SCALING,
    OPERATION_VECTOR_PROJECTION,
    OPERATION_PLANE_PROJECTION,

    OPERATION_TYPE_COUNT
};

enum PLANES_TYPE
{
    XY_PLANE,
    XZ_PLANE,
    YZ_PLANE,
};

struct calculator_info_ui
{
    char  OutputLabel[64] = { "Vecteur sans nom." };
    vec_4 OutputColor     = vec_4(1.0f, 1.0f, 1.0f, 1.0f);
    vec_3 OutputOrigin    = vec_3();
    vec_3 OutputDirection = vec_3();
    char ButtonName[32]   = { "NON-DEFINI" };
    char Symbol[8]        = { "XXXXXXX" };
    OPERATION_TYPE OpType = OPERATION_ADDITION;
};

struct vector_calculator_ui
{
    bool       IsInitialized;

    vector_ui* LeftVector;
    vector_ui* RightVector;
    f32        Scalar;

    u32                CurrentCalculator;
    calculator_info_ui CalculatorInfos[OPERATION_TYPE_COUNT];

    PLANES_TYPE Plane;
    vec_3       PlaneNormal;
};

static calculator_info_ui BuildCalculatorInfoUI(OPERATION_TYPE Type)
{
    calculator_info_ui CalculatorInfo;
    CalculatorInfo.OpType = Type;

    switch (Type)
    {
    case OPERATION_ADDITION:
    {
        memcpy(CalculatorInfo.Symbol, "+", 2);
        memcpy(CalculatorInfo.ButtonName, "Addition", 9);

        break;
    }
    case OPERATION_SUBTRACTION:
    {
        memcpy(CalculatorInfo.Symbol, "-", 2);
        memcpy(CalculatorInfo.ButtonName, "Soustraction", 13);
        break;
    }
    case OPERATION_SCALING:
    {
        memcpy(CalculatorInfo.Symbol, "*", 2);
        memcpy(CalculatorInfo.ButtonName, "Mise a l'echelle", 17);
        break;
    }
    case OPERATION_VECTOR_PRODUCT:
    {
        memcpy(CalculatorInfo.Symbol, "X", 2);
        memcpy(CalculatorInfo.ButtonName, "Produit vectoriel", 18);
        break;
    }

    case OPERATION_VECTOR_PROJECTION:
        memcpy(CalculatorInfo.Symbol, "Proj", 5);
        memcpy(CalculatorInfo.ButtonName, "Projection", 11);
        break;

    case OPERATION_PLANE_PROJECTION:
        memcpy(CalculatorInfo.Symbol, "Proj", 5);
        memcpy(CalculatorInfo.ButtonName, "Projection", 11);
        break;

    }

    return CalculatorInfo;
}

static void RenderCalculatorUI(vector_calculator_ui* VectorCalculator, vectors_storage_ui* VectorStorage)
{
    if (!VectorCalculator->IsInitialized)
    {
        for (u32 TypeIndex = 0; TypeIndex < OPERATION_TYPE_COUNT; TypeIndex++)
        {
            OPERATION_TYPE OpType = (OPERATION_TYPE)TypeIndex;
            VectorCalculator->CalculatorInfos[TypeIndex] = BuildCalculatorInfoUI(OpType);
        }

        VectorCalculator->Plane         = XZ_PLANE;
        VectorCalculator->PlaneNormal   = vec_3(0.0f, 1.0f, 0.0f);
        VectorCalculator->IsInitialized = true;
    }

    u32 CurrentIndex                   = VectorCalculator->CurrentCalculator;
    calculator_info_ui* CalculatorInfo = &VectorCalculator->CalculatorInfos[CurrentIndex];

    ImGui::SeparatorText(CalculatorInfo->ButtonName);

    ImGui::BeginGroup();
    {
        char LeftDropTarget[64];
        snprintf(LeftDropTarget, sizeof(LeftDropTarget), "%s-%d", CalculatorInfo->ButtonName, 1);

        ImGui::BeginChild(LeftDropTarget, ImVec2(140, 30), true);
        {
            if (VectorCalculator->LeftVector)
            {
                ImGui::Text("%s", VectorCalculator->LeftVector->Label);
            }
            else
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Deposez ici");
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("VECTOR_UI"))
                {
                    IM_ASSERT(Payload->DataSize == sizeof(vector_ui*));
                    VectorCalculator->LeftVector = *(vector_ui**)Payload->Data;
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
    ImGui::Text(CalculatorInfo->Symbol);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine();

    ImGui::BeginGroup();
    {
        // printf("Op type:%d\n", CalculatorInfo->OpType);
        if (CalculatorInfo->OpType == OPERATION_PLANE_PROJECTION)
        {
            const char* Planes[] = { "Plan XY", "Plan XZ", "Plan YZ" };

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
            ImGui::SetNextItemWidth(100.0f);
            
            if (ImGui::Combo("##Planes", (i32*)&VectorCalculator->Plane, Planes, IM_ARRAYSIZE(Planes)))
            {
                PLANES_TYPE Plane = (PLANES_TYPE)VectorCalculator->Plane;
                if (Plane == XZ_PLANE)
                {
                    VectorCalculator->PlaneNormal = vec_3(0.0f, 1.0f, 0.0f);
                }
                else if (Plane == XY_PLANE)
                {
                    VectorCalculator->PlaneNormal = vec_3(0.0f, 0.0f, 1.0f);
                }
                else
                {
                    VectorCalculator->PlaneNormal = vec_3(1.0f, 0.0f, 0.0f);
                }
            }
        }
        else if(CalculatorInfo->OpType == OPERATION_SCALING)
        {
            ImGui::SetNextItemWidth(140.0f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
            ImGui::InputFloat("##ScalarInput", &VectorCalculator->Scalar);
        }
        else
        {
            char RightDropTarget[64];
            snprintf(RightDropTarget, sizeof(RightDropTarget), "%s-%d", CalculatorInfo->ButtonName, 2);

            ImGui::BeginChild(RightDropTarget, ImVec2(140, 30), true);
            {
                if (VectorCalculator->RightVector)
                {
                    ImGui::Text("%s", VectorCalculator->RightVector->Label);
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Deposez ici");
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("VECTOR_UI"))
                    {
                        IM_ASSERT(Payload->DataSize == sizeof(vector_ui*));
                        VectorCalculator->RightVector = *(vector_ui**)Payload->Data;
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            ImGui::EndChild();
        }

    }
    ImGui::EndGroup();

    ImGui::SameLine();

    bool ValidOutput       = false;
    bool ValidLeftVector   = VectorCalculator->LeftVector ? true : false;
    bool ValidRightVector  = VectorCalculator->RightVector ? true : false;
    bool ValidScalar       = VectorCalculator->Scalar != 0 ? true : false;
    bool IsPlaneProjection = CalculatorInfo->OpType == OPERATION_PLANE_PROJECTION
                                                       ? true : false;
    
    if (ValidLeftVector && ValidRightVector)
    {
        simulation_vector* LeftVector = VectorCalculator->LeftVector->Vector;
        simulation_vector* RightVector = VectorCalculator->RightVector->Vector;
        vec_3 Destination = vec_3();
        vec_3 Origin = vec_3();

        switch (CalculatorInfo->OpType)
        {
        case OPERATION_ADDITION:
        {
            Origin = LeftVector->Origin;

            vec_3 ResultVector = (LeftVector->Direction - LeftVector->Origin) + (RightVector->Direction - RightVector->Origin);
            Destination = Origin + ResultVector;
            break;
        }
            

        case OPERATION_SUBTRACTION:
        {
            Origin = RightVector->Direction;

            vec_3 ResultVector = (LeftVector->Direction - LeftVector->Origin) - (RightVector->Direction - RightVector->Origin);
            Destination = Origin + ResultVector;
            break;
        }

        case OPERATION_VECTOR_PRODUCT:
        {
            Origin = LeftVector->Origin;

            vec_3 LeftDir = LeftVector->Direction - LeftVector->Origin;
            vec_3 RightDir = RightVector->Direction - RightVector->Origin;
            Destination = Origin + VectorProduct(LeftDir, RightDir);
            break;
        }

        case OPERATION_VECTOR_PROJECTION:
        {
            Origin = LeftVector->Origin;
            vec_3 VectorToProject = RightVector->Direction - RightVector->Origin;
            vec_3 OntoVector = LeftVector->Direction - LeftVector->Origin;
            Destination = Origin + ProjectVectorOnVector(VectorToProject, OntoVector);
            break;
        }
        }

        CalculatorInfo->OutputOrigin    = Origin;
        CalculatorInfo->OutputDirection = Destination;

        ValidOutput = true;
    }

    if (ValidLeftVector && ValidScalar)
    {
        simulation_vector* Vector = VectorCalculator->LeftVector->Vector;

        vec_3 WorldVector = Vector->Direction - Vector->Origin;

        CalculatorInfo->OutputOrigin    = Vector->Origin;
        CalculatorInfo->OutputDirection = Vector->Origin + ScaleVector(WorldVector, VectorCalculator->Scalar);
    
        ValidOutput = true;
    }

    if (ValidLeftVector && IsPlaneProjection)
    {
        simulation_vector* Vector = VectorCalculator->LeftVector->Vector;

        vec_3 WorldVector = Vector->Direction - Vector->Origin;
        vec_3 Projected   = ProjectVectorOnPlane(WorldVector, VectorCalculator->PlaneNormal);

        CalculatorInfo->OutputOrigin = Vector->Origin;
        CalculatorInfo->OutputDirection = Vector->Origin + Projected;
    }
    

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
    if (ImGui::Button(CalculatorInfo->ButtonName, ImVec2(125, 25)))
    {
        if (CanCreateVector())
        {
            VectorStorage->ExternalVector.Vector = CreateSimulationVector(CalculatorInfo->OutputOrigin, CalculatorInfo->OutputDirection, CalculatorInfo->OutputColor);
            CreateVectorEntity(VectorStorage->ExternalVector.Vector);
            strncpy_s(VectorStorage->ExternalVector.Label, CalculatorInfo->OutputLabel, sizeof(VectorStorage->ExternalVector.Label));
            VectorStorage->ExternalVector.Initialized = true;
        }

        CalculatorInfo->OutputColor     = vec_4(1.0f, 1.0f, 1.0f, 1.0f);
        CalculatorInfo->OutputDirection = vec_3();
        CalculatorInfo->OutputOrigin    = vec_3();
        memcpy(CalculatorInfo->OutputLabel, "Vecteur sans nom.", 18);
        
        VectorCalculator->LeftVector  = nullptr;
        VectorCalculator->RightVector = nullptr;
    }

    ImGui::Text("=== Resultat ===");
    if (ImGui::BeginTable("ResultTable", 2, ImGuiTableFlags_SizingStretchSame))
    {
        f32 LabelColumnWidth = 75.0f;
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Nom:");
        ImGui::TableSetColumnIndex(1);
        ImGui::InputText("##ResultName", CalculatorInfo->OutputLabel, sizeof(CalculatorInfo->OutputLabel));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Couleur:");
        ImGui::TableSetColumnIndex(1);
        ImGui::ColorEdit4("##ResultColor", CalculatorInfo->OutputColor.AsArray);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Origine:");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("(%f,%f,%f)", CalculatorInfo->OutputOrigin.x, CalculatorInfo->OutputOrigin.y, CalculatorInfo->OutputOrigin.z);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Direciton:");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("(%f,%f,%f)", CalculatorInfo->OutputDirection.x, CalculatorInfo->OutputDirection.y, CalculatorInfo->OutputDirection.z);

        ImGui::EndTable();
    }

    const char* Operations[] = { "Addition", "Soustraction", "Produit Vectoriel", "Mise a l'echelle",
                                 "Projection sur vecteur", "Projection sur plan"};
    ImGui::SeparatorText("Options de calcul");
    ImGui::Text("Choisissez une operation:");
    ImGui::Combo("##OperationCombo", (i32*)&VectorCalculator->CurrentCalculator, Operations, IM_ARRAYSIZE(Operations));
}