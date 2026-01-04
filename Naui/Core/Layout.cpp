#include "Layout.h"
#include "Panel.h"
#include "Defer.h"
#include "FileSystem/File.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <string>
#include <imgui/imgui_internal.h>
#include <iostream>

namespace Naui {

using json = nlohmann::json;

struct LayoutData
{
	json root;
	bool isLoaded = false;
};

static LayoutData s_data;

// Helper function to serialize a dock node recursively
static json SerializeDockNode(ImGuiDockNode* node)
{
    if (!node)
        return json();
    
    json nodeData;
    nodeData["id"] = node->ID;
    nodeData["is_central"] = node->IsCentralNode();
    nodeData["is_split"] = node->IsSplitNode();
    nodeData["size_x"] = node->Size.x;
    nodeData["size_y"] = node->Size.y;
    
    if (node->IsSplitNode())
    {
        nodeData["split_axis"] = (int)node->SplitAxis;
        nodeData["child_0"] = SerializeDockNode(node->ChildNodes[0]);
        nodeData["child_1"] = SerializeDockNode(node->ChildNodes[1]);
    }
    else
    {
        // Leaf node - store which windows are docked here
        json windowsList = json::array();
        for (int i = 0; i < node->Windows.Size; i++)
        {
            ImGuiWindow* window = node->Windows[i];
            if (window)
            {
                windowsList.push_back(window->Name);
            }
        }
        nodeData["windows"] = windowsList;
        
        // Store which window is selected
        if (node->SelectedTabId != 0)
        {
            // Find the selected window by searching through the node's windows
            for (int i = 0; i < node->Windows.Size; i++)
            {
                ImGuiWindow* window = node->Windows[i];
                if (window && window->TabId == node->SelectedTabId)
                {
                    nodeData["selected_window"] = window->Name;
                    break;
                }
            }
        }
    }
    
    return nodeData;
}

bool Layout::Save(const std::string& filepath)
{
    json root;
    
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx)
        return false;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 viewportPos = viewport->Pos;
    
    // Store individual panel states
    std::unordered_map<std::string, std::vector<Panel*>> panelsByLayoutID;
    auto& allPanels = GetAllPanels();
    
    for (auto& [uid, panel] : allPanels)
    {
        if (!panel->GetLayoutID().empty())
        {
            panelsByLayoutID[panel->GetLayoutID()].push_back(panel);
        }
    }
    
    json layoutStates = json::object();
    
    for (auto& [layoutID, panels] : panelsByLayoutID)
    {
        if (panels.empty())
            continue;

        Panel* representativePanel = panels[0];
        const char* windowName = representativePanel->GetTitle().c_str();
        ImGuiWindow* window = ImGui::FindWindowByName(windowName);
        
        if (!window)
            continue;

        json layoutState;
        layoutState["collapsed"] = window->Collapsed;
        layoutState["pos_x"] = window->Pos.x - viewportPos.x;
        layoutState["pos_y"] = window->Pos.y - viewportPos.y;
        layoutState["size_x"] = window->Size.x;
        layoutState["size_y"] = window->Size.y;
        layoutState["window_name"] = windowName;
        
        layoutStates[layoutID] = layoutState;
    }
    
    root["layout_states"] = layoutStates;
    
    // Serialize the entire dock space hierarchy
    json dockSpaces = json::array();
    
    ImGuiDockContext* dockCtx = &ctx->DockContext;
    for (int i = 0; i < dockCtx->Nodes.Data.Size; i++)
    {
        if (dockCtx->Nodes.Data[i].val_p)
        {
            ImGuiDockNode* node = (ImGuiDockNode*)dockCtx->Nodes.Data[i].val_p;
            
            // Only save root nodes (nodes without parents)
            if (node->ParentNode == nullptr && node->IsRootNode())
            {
                json dockSpace;
                dockSpace["id"] = node->ID;
                dockSpace["pos_x"] = node->Pos.x - viewportPos.x;
                dockSpace["pos_y"] = node->Pos.y - viewportPos.y;
                dockSpace["size_x"] = node->Size.x;
                dockSpace["size_y"] = node->Size.y;
                dockSpace["hierarchy"] = SerializeDockNode(node);
                
                dockSpaces.push_back(dockSpace);
            }
        }
    }
    
    root["dock_spaces"] = dockSpaces;
    root["version"] = 2;
    
    try
    {
        std::ofstream file("Layouts\\" + filepath + ".json");
        if (!file.is_open())
            return false;
            
        file << root.dump(2);
        file.close();
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool Layout::Load(const std::string& filepath)
{
    try
    {
        std::ifstream file("Layouts\\" + filepath + ".json");
        if (!file.is_open())
            return false;
            
        json root = json::parse(file);
        file.close();
        
        if (!root.contains("version"))
            return false;

		s_data.root = root;
		s_data.isLoaded = true;
        
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

// Helper function to rebuild dock node hierarchy
static void RebuildDockNode(ImGuiID parentID, const json& nodeData, std::unordered_map<std::string, ImGuiID>& windowToDockID)
{
    if (nodeData.empty())
        return;
    
    if (nodeData["is_split"].get<bool>())
    {
        // Split node - recreate the split
        ImGuiAxis splitAxis = (ImGuiAxis)nodeData["split_axis"].get<int>();
        ImGuiDir splitDir = (splitAxis == ImGuiAxis_X) ? ImGuiDir_Left : ImGuiDir_Up;
        
        // Calculate split ratio from child sizes
        float size0 = splitAxis == ImGuiAxis_X ? 
            nodeData["child_0"]["size_x"].get<float>() : 
            nodeData["child_0"]["size_y"].get<float>();
        float size1 = splitAxis == ImGuiAxis_X ? 
            nodeData["child_1"]["size_x"].get<float>() : 
            nodeData["child_1"]["size_y"].get<float>();
        float ratio = size0 / (size0 + size1);
        
        ImGuiID child0ID = 0;
        ImGuiID child1ID = 0;
        
        // Split the parent node
        ImGui::DockBuilderSplitNode(parentID, splitDir, ratio, &child0ID, &child1ID);
        
        // Recursively build children
        RebuildDockNode(child0ID, nodeData["child_0"], windowToDockID);
        RebuildDockNode(child1ID, nodeData["child_1"], windowToDockID);
    }
    else
    {
        // Leaf node - map windows to this dock ID
        if (nodeData.contains("windows"))
        {
            for (const auto& windowName : nodeData["windows"])
            {
                windowToDockID[windowName.get<std::string>()] = parentID;
            }
        }
    }
}

void Layout::RenderLoad(void)
{
	if (!s_data.isLoaded)
		return;

	ImGuiContext* ctx = ImGui::GetCurrentContext();
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 viewportPos = viewport->Pos;
	
	auto& allPanels = GetAllPanels();
	
	// First, undock all windows
	for (auto& [uid, panel] : allPanels)
	{
		const char* windowName = panel->GetTitle().c_str();
		ImGuiWindow* window = ImGui::FindWindowByName(windowName);
		
		if (window && window->DockNode)
		{
			ImGui::DockContextProcessUndockWindow(ctx, window);
		}
	}
	
	// Map to store which dock ID each window should go to
	std::unordered_map<std::string, ImGuiID> windowToDockID;
	
	// Rebuild dock space hierarchy
	if (s_data.root.contains("dock_spaces"))
	{
		json dockSpaces = s_data.root["dock_spaces"];
		
		for (const auto& dockSpace : dockSpaces)
		{
			ImGuiID rootID = dockSpace["id"].get<ImGuiID>();
			float posX = dockSpace["pos_x"].get<float>();
			float posY = dockSpace["pos_y"].get<float>();
			float sizeX = dockSpace["size_x"].get<float>();
			float sizeY = dockSpace["size_y"].get<float>();
			
			// Remove existing node and create fresh
			ImGui::DockBuilderRemoveNode(rootID);
			ImGui::DockBuilderAddNode(rootID, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodePos(rootID, ImVec2(viewportPos.x + posX, viewportPos.y + posY));
			ImGui::DockBuilderSetNodeSize(rootID, ImVec2(sizeX, sizeY));
			
			// Rebuild the hierarchy and collect window-to-dock mappings
			if (dockSpace.contains("hierarchy"))
			{
				RebuildDockNode(rootID, dockSpace["hierarchy"], windowToDockID);
			}
			
			// Finish building this dock space
			ImGui::DockBuilderFinish(rootID);
		}
	}
	
	// Now dock all windows to their assigned nodes
	for (auto& [uid, panel] : allPanels)
	{
		if (panel->GetLayoutID().empty())
			continue;
		
		const char* windowName = panel->GetTitle().c_str();
		
		// Check if this window should be docked
		auto it = windowToDockID.find(windowName);
		if (it != windowToDockID.end())
		{
			ImGui::DockBuilderDockWindow(windowName, it->second);
		}
	}
	
	// Apply non-docking window states
	if (s_data.root.contains("layout_states"))
	{
		json layoutStates = s_data.root["layout_states"];
		
		for (auto& [uid, panel] : allPanels)
		{
			if (panel->GetLayoutID().empty())
				continue;
				
			if (!layoutStates.contains(panel->GetLayoutID()))
				continue;
				
			json layoutState = layoutStates[panel->GetLayoutID()];
			const char* windowName = panel->GetTitle().c_str();
			
			// Only set position/size for non-docked windows
			if (windowToDockID.find(windowName) == windowToDockID.end())
			{
				if (layoutState.contains("pos_x") && layoutState.contains("pos_y"))
				{
					float relX = layoutState["pos_x"].get<float>();
					float relY = layoutState["pos_y"].get<float>();
					
					ImGui::SetWindowPos(windowName, 
						ImVec2(viewportPos.x + relX, viewportPos.y + relY), 
						ImGuiCond_Always);
				}
				
				if (layoutState.contains("size_x") && layoutState.contains("size_y"))
				{
					ImGui::SetWindowSize(windowName,
						ImVec2(layoutState["size_x"], layoutState["size_y"]),
						ImGuiCond_Always);
				}
			}
		}
	}

	s_data.isLoaded = false;
}

std::vector<LayoutInfo> &Layout::Cache(void)
{
	namespace fs = std::filesystem;

    static std::vector<LayoutInfo> cache;
	cache.clear();

    static const fs::path layoutsDir = "Layouts";
    static const std::string immutableName = "Default.json";

    if (!fs::exists(layoutsDir) || !fs::is_directory(layoutsDir))
        return cache;

    for (const fs::directory_entry& entry : fs::directory_iterator(layoutsDir))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path& path = entry.path();

        if (path.extension() != ".json")
            continue;

        LayoutInfo info;
        info.filePath = path;
        info.name = path.stem().string();
        info.immutable = (path.filename() == immutableName);

        cache.emplace_back(std::move(info));
    }

    return cache;
}

}