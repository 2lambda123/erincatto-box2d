/*
* Copyright (c) 2006-2016 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#define _CRT_SECURE_NO_WARNINGS

#include "glad.h"
#include "GLFW/glfw3.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "draw.h"
#include "settings.h"
#include "test.h"


#include <stdio.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

GLFWwindow* g_mainWindow = nullptr;
static int32 s_testSelection = 0;
static Test* s_test = nullptr;
static Settings s_settings;
static float s_uiScale = 1.0f;
static bool s_rightMouseDown = false;
static b2Vec2 s_clickPointWS = b2Vec2_zero;

void glfwErrorCallback(int error, const char* description)
{
	fprintf(stderr, "GLFW error occured. Code: %d. Description: %s\n", error, description);
}

static inline bool CompareTests(const TestEntry& a, const TestEntry& b)
{
	int result = strcmp(a.category, b.category);
	if (result == 0)
	{
		result = strcmp(a.name, b.name);
	}

	return result < 0;
}

static void SortTests()
{
	std::sort(g_testEntries, g_testEntries + g_testCount, CompareTests);
}

static void CreateUI(GLFWwindow* window)
{
	float xscale, yscale;
	glfwGetWindowContentScale(window, &xscale, &yscale);
	s_uiScale = xscale;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	bool success;
	success = ImGui_ImplGlfw_InitForOpenGL(window, false);
	if (success == false)
	{
		printf("ImGui_ImplGlfw_InitForOpenGL failed\n");
		assert(false);
	}

	success = ImGui_ImplOpenGL3_Init();
	if (success == false)
	{
		printf("ImGui_ImplOpenGL3_Init failed\n");
		assert(false);
	}

	const char* fontPath = "Data/DroidSans.ttf";
	ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath, s_uiScale * 14.0f);
}

static void ResizeWindowCallback(GLFWwindow*, int width, int height)
{
	g_camera.m_width = width;
	g_camera.m_height = height;
	s_settings.m_windowWidth = width;
	s_settings.m_windowHeight = height;
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureKeyboard)
	{
		return;
	}

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			// Quit
			glfwSetWindowShouldClose(g_mainWindow, GL_TRUE);
			break;

		case GLFW_KEY_LEFT:
			// Pan left
			if (mods == GLFW_MOD_CONTROL)
			{
				b2Vec2 newOrigin(2.0f, 0.0f);
				s_test->ShiftOrigin(newOrigin);
			}
			else
			{
				g_camera.m_center.x -= 0.5f;
			}
			break;

		case GLFW_KEY_RIGHT:
			// Pan right
			if (mods == GLFW_MOD_CONTROL)
			{
				b2Vec2 newOrigin(-2.0f, 0.0f);
				s_test->ShiftOrigin(newOrigin);
			}
			else
			{
				g_camera.m_center.x += 0.5f;
			}
			break;

		case GLFW_KEY_DOWN:
			// Pan down
			if (mods == GLFW_MOD_CONTROL)
			{
				b2Vec2 newOrigin(0.0f, 2.0f);
				s_test->ShiftOrigin(newOrigin);
			}
			else
			{
				g_camera.m_center.y -= 0.5f;
			}
			break;

		case GLFW_KEY_UP:
			// Pan up
			if (mods == GLFW_MOD_CONTROL)
			{
				b2Vec2 newOrigin(0.0f, -2.0f);
				s_test->ShiftOrigin(newOrigin);
			}
			else
			{
				g_camera.m_center.y += 0.5f;
			}
			break;

		case GLFW_KEY_HOME:
			// Reset view
			g_camera.m_zoom = 1.0f;
			g_camera.m_center.Set(0.0f, 20.0f);
			break;

		case GLFW_KEY_Z:
			// Zoom out
			g_camera.m_zoom = b2Min(1.1f * g_camera.m_zoom, 20.0f);
			break;

		case GLFW_KEY_X:
			// Zoom in
			g_camera.m_zoom = b2Max(0.9f * g_camera.m_zoom, 0.02f);
			break;

		case GLFW_KEY_R:
			// Reset test
			delete s_test;
			s_test = g_testEntries[s_settings.m_testIndex].createFcn();
			break;

		case GLFW_KEY_SPACE:
			// Launch a bomb.
			if (s_test)
			{
				s_test->LaunchBomb();
			}
			break;

		case GLFW_KEY_O:
			s_settings.m_singleStep = true;
			break;

		case GLFW_KEY_P:
			s_settings.m_pause = !s_settings.m_pause;
			break;

		case GLFW_KEY_LEFT_BRACKET:
			// Switch to previous test
			--s_testSelection;
			if (s_testSelection < 0)
			{
				s_testSelection = g_testCount - 1;
			}
			break;

		case GLFW_KEY_RIGHT_BRACKET:
			// Switch to next test
			++s_testSelection;
			if (s_testSelection == g_testCount)
			{
				s_testSelection = 0;
			}
			break;

		case GLFW_KEY_TAB:
			g_debugDraw.m_showUI = !g_debugDraw.m_showUI;

		default:
			if (s_test)
			{
				s_test->Keyboard(key);
			}
		}
	}
	else if (action == GLFW_RELEASE)
	{
		s_test->KeyboardUp(key);
	}
	// else GLFW_REPEAT
}

static void CharCallback(GLFWwindow* window, unsigned int c)
{
	ImGui_ImplGlfw_CharCallback(window, c);
}

static void MouseButtonCallback(GLFWwindow* window, int32 button, int32 action, int32 mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

	double xd, yd;
	glfwGetCursorPos(g_mainWindow, &xd, &yd);
	b2Vec2 ps((float)xd, (float)yd);

	// Use the mouse to move things around.
	if (button == GLFW_MOUSE_BUTTON_1)
	{
        //<##>
        //ps.Set(0, 0);
		b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
		if (action == GLFW_PRESS)
		{
			if (mods == GLFW_MOD_SHIFT)
			{
				s_test->ShiftMouseDown(pw);
			}
			else
			{
				s_test->MouseDown(pw);
			}
		}
		
		if (action == GLFW_RELEASE)
		{
			s_test->MouseUp(pw);
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_2)
	{
		if (action == GLFW_PRESS)
		{	
			s_clickPointWS = g_camera.ConvertScreenToWorld(ps);
			s_rightMouseDown = true;
		}

		if (action == GLFW_RELEASE)
		{
			s_rightMouseDown = false;
		}
	}
}

static void MouseMotionCallback(GLFWwindow*, double xd, double yd)
{
	b2Vec2 ps((float)xd, (float)yd);

	b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
	s_test->MouseMove(pw);
	
	if (s_rightMouseDown)
	{
		b2Vec2 diff = pw - s_clickPointWS;
		g_camera.m_center.x -= diff.x;
		g_camera.m_center.y -= diff.y;
		s_clickPointWS = g_camera.ConvertScreenToWorld(ps);
	}
}

static void ScrollCallback(GLFWwindow* window, double dx, double dy)
{
	ImGui_ImplGlfw_ScrollCallback(window, dx, dy);
	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}

	if (dy > 0)
	{
		g_camera.m_zoom /= 1.1f;
	}
	else
	{
		g_camera.m_zoom *= 1.1f;
	}
}

static void RestartTest()
{
	delete s_test;
	s_test = g_testEntries[s_settings.m_testIndex].createFcn();
}

static void UpdateUI()
{
	int menuWidth = 200;
	if (g_debugDraw.m_showUI)
	{
		ImGui::SetNextWindowPos(ImVec2((float)g_camera.m_width - menuWidth - 10, 10));
		ImGui::SetNextWindowSize(ImVec2((float)menuWidth, (float)g_camera.m_height - 20));

		ImGui::Begin("Tools", &g_debugDraw.m_showUI, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		if (ImGui::BeginTabBar("ControlTabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Controls"))
			{
				ImGui::SliderInt("Vel Iters", &s_settings.m_velocityIterations, 0, 50);
				ImGui::SliderInt("Pos Iters", &s_settings.m_positionIterations, 0, 50);
				ImGui::SliderFloat("Hertz", &s_settings.m_hertz, 5.0f, 120.0f, "%.0f hz");
				
				ImGui::Separator();

				ImGui::Checkbox("Sleep", &s_settings.m_enableSleep);
				ImGui::Checkbox("Warm Starting", &s_settings.m_enableWarmStarting);
				ImGui::Checkbox("Time of Impact", &s_settings.m_enableContinuous);
				ImGui::Checkbox("Sub-Stepping", &s_settings.m_enableSubStepping);

				ImGui::Separator();

				ImGui::Checkbox("Shapes", &s_settings.m_drawShapes);
				ImGui::Checkbox("Joints", &s_settings.m_drawJoints);
				ImGui::Checkbox("AABBs", &s_settings.m_drawAABBs);
				ImGui::Checkbox("Contact Points", &s_settings.m_drawContactPoints);
				ImGui::Checkbox("Contact Normals", &s_settings.m_drawContactNormals);
				ImGui::Checkbox("Contact Impulses", &s_settings.m_drawContactImpulse);
				ImGui::Checkbox("Friction Impulses", &s_settings.m_drawFrictionImpulse);
				ImGui::Checkbox("Center of Masses", &s_settings.m_drawCOMs);
				ImGui::Checkbox("Statistics", &s_settings.m_drawStats);
				ImGui::Checkbox("Profile", &s_settings.m_drawProfile);

				ImVec2 button_sz = ImVec2(-1, 0);
				if (ImGui::Button("Pause (P)", button_sz))
				{
					s_settings.m_pause = !s_settings.m_pause;
				}

				if (ImGui::Button("Single Step (O)", button_sz))
				{
					s_settings.m_singleStep = !s_settings.m_singleStep;
				}

				if (ImGui::Button("Restart (R)", button_sz))
				{
					RestartTest();
				}

				if (ImGui::Button("Quit", button_sz))
				{
					glfwSetWindowShouldClose(g_mainWindow, GL_TRUE);
				}

				ImGui::EndTabItem();
			}

			ImGuiTreeNodeFlags leafNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
			leafNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

			ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

			if (ImGui::BeginTabItem("Tests"))
			{
				int categoryIndex = 0;
				const char* category = g_testEntries[categoryIndex].category;
				int i = 0;
				while (i < g_testCount)
				{
					bool categorySelected = strcmp(category, g_testEntries[s_settings.m_testIndex].category) == 0;
					ImGuiTreeNodeFlags nodeSelectionFlags = categorySelected ? ImGuiTreeNodeFlags_Selected : 0;
					bool nodeOpen = ImGui::TreeNodeEx(category, nodeFlags | nodeSelectionFlags);

					if (nodeOpen)
					{
						while (i < g_testCount && strcmp(category, g_testEntries[i].category) == 0)
						{
							ImGuiTreeNodeFlags selectionFlags = 0;
							if (s_settings.m_testIndex == i)
							{
								selectionFlags = ImGuiTreeNodeFlags_Selected;
							}
							ImGui::TreeNodeEx((void*)(intptr_t)i, leafNodeFlags | selectionFlags, "%s", g_testEntries[i].name);
							if (ImGui::IsItemClicked())
							{
								delete s_test;
								s_settings.m_testIndex = i;
								s_test = g_testEntries[i].createFcn();
								s_test->SetUIScale(s_uiScale);
								s_testSelection = i;
							}
							++i;
						}
						ImGui::TreePop();
					}
					else
					{
						while (i < g_testCount && strcmp(category, g_testEntries[i].category) == 0)
						{
							++i;
						}
					}

					if (i < g_testCount)
					{
						category = g_testEntries[i].category;
						categoryIndex = i;
					}
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();

		s_test->UpdateUI();
	}
}

//
int main(int, char**)
{
#if defined(_WIN32)
	// Enable memory-leak reports
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

	char buffer[128];

	s_settings.Load();
	SortTests();

	glfwSetErrorCallback(glfwErrorCallback);

	g_camera.m_width = s_settings.m_windowWidth;
	g_camera.m_height = s_settings.m_windowHeight;

	if (glfwInit() == 0)
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	sprintf(buffer, "Box2D Samples Version %d.%d.%d", b2_version.major, b2_version.minor, b2_version.revision);
	g_mainWindow = glfwCreateWindow(g_camera.m_width, g_camera.m_height, buffer, NULL, NULL);
	if (g_mainWindow == NULL)
	{
		fprintf(stderr, "Failed to open GLFW g_mainWindow.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(g_mainWindow);

#if defined(__APPLE__) == FALSE
	// Load OpenGL functions using glad
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
	{
		printf("Failed to initialize OpenGL context\n");
		return -1;
	}
#endif

	printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	glfwSetScrollCallback(g_mainWindow, ScrollCallback);
	glfwSetWindowSizeCallback(g_mainWindow, ResizeWindowCallback);
	glfwSetKeyCallback(g_mainWindow, KeyCallback);
	glfwSetCharCallback(g_mainWindow, CharCallback);
	glfwSetMouseButtonCallback(g_mainWindow, MouseButtonCallback);
	glfwSetCursorPosCallback(g_mainWindow, MouseMotionCallback);
	glfwSetScrollCallback(g_mainWindow, ScrollCallback);

	g_debugDraw.Create();

	CreateUI(g_mainWindow);

	s_settings.m_testIndex = b2Clamp(s_settings.m_testIndex, 0, g_testCount - 1);
	s_testSelection = s_settings.m_testIndex;
	s_test = g_testEntries[s_settings.m_testIndex].createFcn();

	// Control the frame rate. One draw per monitor refresh.
	glfwSwapInterval(1);

	double time1 = glfwGetTime();
	double frameTime = 0.0;
   
	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	
	while (!glfwWindowShouldClose(g_mainWindow))
	{
		glfwGetWindowSize(g_mainWindow, &g_camera.m_width, &g_camera.m_height);
        
        int bufferWidth, bufferHeight;
        glfwGetFramebufferSize(g_mainWindow, &bufferWidth, &bufferHeight);
        glViewport(0, 0, bufferWidth, bufferHeight);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		if (g_debugDraw.m_showUI)
		{
			ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
			ImGui::SetNextWindowSize(ImVec2(float(g_camera.m_width), float(g_camera.m_height)));
			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::End();

			const TestEntry& entry = g_testEntries[s_settings.m_testIndex];
			sprintf_s(buffer, "%s : %s", entry.category, entry.name);
			s_test->DrawTitle(buffer);
		}

		s_test->Step(s_settings);

		if (s_testSelection != s_settings.m_testIndex)
		{
			s_settings.m_testIndex = s_testSelection;
			delete s_test;
			s_test = g_testEntries[s_settings.m_testIndex].createFcn();
			g_camera.m_zoom = 1.0f;
			g_camera.m_center.Set(0.0f, 20.0f);
		}

		UpdateUI();

		// Measure speed
		double time2 = glfwGetTime();
		double alpha = 0.9;
		frameTime = alpha * frameTime + (1.0 - alpha) * (time2 - time1);
		time1 = time2;

		if (g_debugDraw.m_showUI)
		{
			sprintf_s(buffer, "%.1f ms", 1000.0 * frameTime);
			g_debugDraw.DrawString(5, g_camera.m_height - 20, buffer);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(g_mainWindow);

		glfwPollEvents();
	}

	if (s_test)
	{
		delete s_test;
		s_test = NULL;
	}

	g_debugDraw.Destroy();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	glfwTerminate();

	s_settings.Save();

	return 0;
}
